/*
 * Copyright (C) 2007 Intel Corporation
 * Copyright (C) 2016 Florent Revest <florent.revest@free-electrons.com>
 * Copyright (C) 2018 Paul Kocialkowski <paul.kocialkowski@bootlin.com>
 * Copyright (C) 2018 Bootlin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <limits.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <h264-ctrls.h>

#include "request.h"
#include "surface.h"
#include "v4l2.h"

enum h264_slice_type {
	H264_SLICE_P    = 0,
	H264_SLICE_B    = 1,
};

static bool is_picture_null(VAPictureH264 *pic)
{
	return pic->picture_id == VA_INVALID_SURFACE;
}

static struct h264_dpb_entry *
dpb_find_invalid_entry(struct object_context *context)
{
	unsigned int i;

	for (i = 0; i < H264_DPB_SIZE; i++) {
		struct h264_dpb_entry *entry = &context->dpb.entries[i];

		if (!entry->valid && !entry->reserved)
			return entry;
	}

	return NULL;
}

static struct h264_dpb_entry *
dpb_find_oldest_unused_entry(struct object_context *context)
{
	unsigned int min_age = UINT_MAX;
	unsigned int i;
	struct h264_dpb_entry *match = NULL;

	for (i = 0; i < H264_DPB_SIZE; i++) {
		struct h264_dpb_entry *entry = &context->dpb.entries[i];

		if (!entry->used && (entry->age < min_age)) {
			min_age = entry->age;
			match = entry;
		}
	}

	return match;
}

static struct h264_dpb_entry *dpb_find_entry(struct object_context *context)
{
	struct h264_dpb_entry *entry;

	entry = dpb_find_invalid_entry(context);
	if (!entry)
		entry = dpb_find_oldest_unused_entry(context);

	return entry;
}

static struct h264_dpb_entry *dpb_lookup(struct object_context *context,
					 VAPictureH264 *pic, unsigned int *idx)
{
	unsigned int i;

	for (i = 0; i < H264_DPB_SIZE; i++) {
		struct h264_dpb_entry *entry = &context->dpb.entries[i];

		if (!entry->valid)
			continue;

		if (entry->pic.picture_id == pic->picture_id) {
			if (idx)
				*idx = i;

			return entry;
		}
	}

	return NULL;
}

static void dpb_clear_entry(struct h264_dpb_entry *entry, bool reserved)
{
	memset(entry, 0, sizeof(*entry));

	if (reserved)
		entry->reserved = true;
}

static void dpb_insert(struct object_context *context, VAPictureH264 *pic,
		       struct h264_dpb_entry *entry)
{
	if (is_picture_null(pic))
		return;

	if (dpb_lookup(context, pic, NULL))
		return;

	if (!entry)
		entry = dpb_find_entry(context);

	memcpy(&entry->pic, pic, sizeof(entry->pic));
	entry->age = context->dpb.age;
	entry->valid = true;
	entry->reserved = false;

	if (!(pic->flags & VA_PICTURE_H264_INVALID))
		entry->used = true;
}

static void dpb_update(struct object_context *context,
		       VAPictureParameterBufferH264 *parameters)
{
	unsigned int i;

	context->dpb.age++;

	for (i = 0; i < H264_DPB_SIZE; i++) {
		struct h264_dpb_entry *entry = &context->dpb.entries[i];

		entry->used = false;
	}

	for (i = 0; i < parameters->num_ref_frames; i++) {
		VAPictureH264 *pic = &parameters->ReferenceFrames[i];
		struct h264_dpb_entry *entry;

		if (is_picture_null(pic))
			continue;

		entry = dpb_lookup(context, pic, NULL);
		if (entry) {
			entry->age = context->dpb.age;
			entry->used = true;
		} else {
			dpb_insert(context, pic, NULL);
		}
	}
}

static void h264_fill_dpb(struct request_data *data,
			  struct object_context *context,
			  struct v4l2_ctrl_h264_decode_params *decode)
{
	int i;

	for (i = 0; i < H264_DPB_SIZE; i++) {
		struct v4l2_h264_dpb_entry *dpb = &decode->dpb[i];
		struct h264_dpb_entry *entry = &context->dpb.entries[i];
		struct object_surface *surface =
			SURFACE(data, entry->pic.picture_id);
		uint64_t timestamp;

		if (!entry->valid)
			continue;

		if (surface) {
			timestamp = v4l2_timeval_to_ns(&surface->timestamp);
			dpb->reference_ts = timestamp;
		}

		dpb->frame_num = entry->pic.frame_idx;
		dpb->pic_num = entry->pic.picture_id;
		dpb->top_field_order_cnt = entry->pic.TopFieldOrderCnt;
		dpb->bottom_field_order_cnt = entry->pic.BottomFieldOrderCnt;

		dpb->flags = V4L2_H264_DPB_ENTRY_FLAG_VALID;

		if (entry->used)
			dpb->flags |= V4L2_H264_DPB_ENTRY_FLAG_ACTIVE;

		if (entry->pic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)
			dpb->flags |= V4L2_H264_DPB_ENTRY_FLAG_LONG_TERM;
	}
}

static void h264_va_picture_to_v4l2(struct request_data *driver_data,
				    struct object_context *context,
				    struct object_surface *surface,
				    VAPictureParameterBufferH264 *VAPicture,
				    struct v4l2_ctrl_h264_decode_params *decode,
				    struct v4l2_ctrl_h264_pps *pps,
				    struct v4l2_ctrl_h264_sps *sps)
{
	unsigned char *b;
	unsigned char nal_ref_idc;
	unsigned char nal_unit_type;

	/* Extract missing nal_ref_idc and nal_unit_type */
	b = surface->source_data;
	if (context->h264_start_code)
		b += 3;
	nal_ref_idc = (b[0] >> 5) & 0x3;
	nal_unit_type = b[0] & 0x1f;

	h264_fill_dpb(driver_data, context, decode);

	decode->num_slices = surface->slices_count;
	decode->nal_ref_idc = nal_ref_idc;
	if (nal_unit_type == 5)
		decode->flags = V4L2_H264_DECODE_PARAM_FLAG_IDR_PIC;
	decode->top_field_order_cnt = VAPicture->CurrPic.TopFieldOrderCnt;
	decode->bottom_field_order_cnt = VAPicture->CurrPic.BottomFieldOrderCnt;

	pps->weighted_bipred_idc =
		VAPicture->pic_fields.bits.weighted_bipred_idc;
	pps->pic_init_qs_minus26 = VAPicture->pic_init_qs_minus26;
	pps->pic_init_qp_minus26 = VAPicture->pic_init_qp_minus26;
	pps->chroma_qp_index_offset = VAPicture->chroma_qp_index_offset;
	pps->second_chroma_qp_index_offset =
		VAPicture->second_chroma_qp_index_offset;
	pps->num_ref_idx_l0_default_active_minus1 =
		VAPicture->num_ref_idx_l0_default_active_minus1;
	pps->num_ref_idx_l1_default_active_minus1 =
		VAPicture->num_ref_idx_l1_default_active_minus1;

	if (VAPicture->pic_fields.bits.entropy_coding_mode_flag)
		pps->flags |= V4L2_H264_PPS_FLAG_ENTROPY_CODING_MODE;

	if (VAPicture->pic_fields.bits.weighted_pred_flag)
		pps->flags |= V4L2_H264_PPS_FLAG_WEIGHTED_PRED;

	if (VAPicture->pic_fields.bits.transform_8x8_mode_flag)
		pps->flags |= V4L2_H264_PPS_FLAG_TRANSFORM_8X8_MODE;

	if (VAPicture->pic_fields.bits.constrained_intra_pred_flag)
		pps->flags |= V4L2_H264_PPS_FLAG_CONSTRAINED_INTRA_PRED;

	if (VAPicture->pic_fields.bits.pic_order_present_flag)
		pps->flags |=
			V4L2_H264_PPS_FLAG_BOTTOM_FIELD_PIC_ORDER_IN_FRAME_PRESENT;

	if (VAPicture->pic_fields.bits.deblocking_filter_control_present_flag)
		pps->flags |=
			V4L2_H264_PPS_FLAG_DEBLOCKING_FILTER_CONTROL_PRESENT;

	if (VAPicture->pic_fields.bits.redundant_pic_cnt_present_flag)
		pps->flags |= V4L2_H264_PPS_FLAG_REDUNDANT_PIC_CNT_PRESENT;

	sps->max_num_ref_frames = VAPicture->num_ref_frames;
	sps->chroma_format_idc = VAPicture->seq_fields.bits.chroma_format_idc;
	sps->bit_depth_luma_minus8 = VAPicture->bit_depth_luma_minus8;
	sps->bit_depth_chroma_minus8 = VAPicture->bit_depth_chroma_minus8;
	sps->log2_max_frame_num_minus4 =
		VAPicture->seq_fields.bits.log2_max_frame_num_minus4;
	sps->log2_max_pic_order_cnt_lsb_minus4 =
		VAPicture->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4;
	sps->pic_order_cnt_type = VAPicture->seq_fields.bits.pic_order_cnt_type;
	sps->pic_width_in_mbs_minus1 = VAPicture->picture_width_in_mbs_minus1;
	sps->pic_height_in_map_units_minus1 =
		VAPicture->picture_height_in_mbs_minus1;

	if (VAPicture->seq_fields.bits.residual_colour_transform_flag)
		sps->flags |= V4L2_H264_SPS_FLAG_SEPARATE_COLOUR_PLANE;
	if (VAPicture->seq_fields.bits.gaps_in_frame_num_value_allowed_flag)
		sps->flags |=
			V4L2_H264_SPS_FLAG_GAPS_IN_FRAME_NUM_VALUE_ALLOWED;
	if (VAPicture->seq_fields.bits.frame_mbs_only_flag)
		sps->flags |= V4L2_H264_SPS_FLAG_FRAME_MBS_ONLY;
	if (VAPicture->seq_fields.bits.mb_adaptive_frame_field_flag)
		sps->flags |= V4L2_H264_SPS_FLAG_MB_ADAPTIVE_FRAME_FIELD;
	if (VAPicture->seq_fields.bits.direct_8x8_inference_flag)
		sps->flags |= V4L2_H264_SPS_FLAG_DIRECT_8X8_INFERENCE;
	if (VAPicture->seq_fields.bits.delta_pic_order_always_zero_flag)
		sps->flags |= V4L2_H264_SPS_FLAG_DELTA_PIC_ORDER_ALWAYS_ZERO;
}

static void h264_va_matrix_to_v4l2(struct request_data *driver_data,
				   struct object_context *context,
				   VAIQMatrixBufferH264 *VAMatrix,
				   struct v4l2_ctrl_h264_scaling_matrix *v4l2_matrix)
{
	memcpy(v4l2_matrix->scaling_list_4x4, &VAMatrix->ScalingList4x4,
	       sizeof(VAMatrix->ScalingList4x4));

	/*
	 * In YUV422, there's only two matrices involved, while YUV444
	 * needs 6. However, in the former case, the two matrices
	 * should be placed at the 0 and 3 offsets.
	 */
	memcpy(v4l2_matrix->scaling_list_8x8[0], &VAMatrix->ScalingList8x8[0],
	       sizeof(v4l2_matrix->scaling_list_8x8[0]));
	/* FIXME --> */
	memcpy(v4l2_matrix->scaling_list_8x8[1], &VAMatrix->ScalingList8x8[1],
	       sizeof(v4l2_matrix->scaling_list_8x8[1]));
	/* <-- FIXME */
	memcpy(v4l2_matrix->scaling_list_8x8[3], &VAMatrix->ScalingList8x8[1],
	       sizeof(v4l2_matrix->scaling_list_8x8[3]));
}

static void h264_copy_pred_table(struct v4l2_h264_weight_factors *factors,
				 unsigned int num_refs,
				 int16_t luma_weight[32],
				 int16_t luma_offset[32],
				 int16_t chroma_weight[32][2],
				 int16_t chroma_offset[32][2])
{
	unsigned int i;

	for (i = 0; i < num_refs; i++) {
		unsigned int j;

		factors->luma_weight[i] = luma_weight[i];
		factors->luma_offset[i] = luma_offset[i];

		for (j = 0; j < 2; j++) {
			factors->chroma_weight[i][j] = chroma_weight[i][j];
			factors->chroma_offset[i][j] = chroma_offset[i][j];
		}
	}
}

static void h264_va_slice_to_v4l2(struct request_data *driver_data,
				  struct object_context *context,
				  VASliceParameterBufferH264 *VASlice,
				  VAPictureParameterBufferH264 *VAPicture,
				  struct v4l2_ctrl_h264_slice_params *slice)
{
	slice->size = VASlice->slice_data_size;
	if (context->h264_start_code)
		slice->size += 3;
	slice->header_bit_size = VASlice->slice_data_bit_offset;
	slice->first_mb_in_slice = VASlice->first_mb_in_slice;
	slice->slice_type = VASlice->slice_type;
	slice->frame_num = VAPicture->frame_num;
	slice->idr_pic_id = VASlice->idr_pic_id;
	slice->dec_ref_pic_marking_bit_size = VASlice->dec_ref_pic_marking_bit_size;
	slice->pic_order_cnt_bit_size = VASlice->pic_order_cnt_bit_size;
	slice->cabac_init_idc = VASlice->cabac_init_idc;
	slice->slice_qp_delta = VASlice->slice_qp_delta;
	slice->disable_deblocking_filter_idc =
		VASlice->disable_deblocking_filter_idc;
	slice->slice_alpha_c0_offset_div2 = VASlice->slice_alpha_c0_offset_div2;
	slice->slice_beta_offset_div2 = VASlice->slice_beta_offset_div2;

	if (((VASlice->slice_type % 5) == H264_SLICE_P) ||
	    ((VASlice->slice_type % 5) == H264_SLICE_B)) {
		unsigned int i;

		slice->num_ref_idx_l0_active_minus1 =
			VASlice->num_ref_idx_l0_active_minus1;

		for (i = 0; i < VASlice->num_ref_idx_l0_active_minus1 + 1; i++) {
			VAPictureH264 *pic = &VASlice->RefPicList0[i];
			struct h264_dpb_entry *entry;
			unsigned int idx;

			entry = dpb_lookup(context, pic, &idx);
			if (!entry)
				continue;

			slice->ref_pic_list0[i] = idx;
		}
	}

	if ((VASlice->slice_type % 5) == H264_SLICE_B) {
		unsigned int i;

		slice->num_ref_idx_l1_active_minus1 =
			VASlice->num_ref_idx_l1_active_minus1;

		for (i = 0; i < VASlice->num_ref_idx_l1_active_minus1 + 1; i++) {
			VAPictureH264 *pic = &VASlice->RefPicList1[i];
			struct h264_dpb_entry *entry;
			unsigned int idx;

			entry = dpb_lookup(context, pic, &idx);
			if (!entry)
				continue;

			slice->ref_pic_list1[i] = idx;
		}
	}

	if (VASlice->direct_spatial_mv_pred_flag)
		slice->flags |= V4L2_H264_SLICE_FLAG_DIRECT_SPATIAL_MV_PRED;

	slice->pred_weight_table.chroma_log2_weight_denom =
		VASlice->chroma_log2_weight_denom;
	slice->pred_weight_table.luma_log2_weight_denom =
		VASlice->luma_log2_weight_denom;

	if (((VASlice->slice_type % 5) == H264_SLICE_P) ||
	    ((VASlice->slice_type % 5) == H264_SLICE_B))
		h264_copy_pred_table(&slice->pred_weight_table.weight_factors[0],
				     slice->num_ref_idx_l0_active_minus1 + 1,
				     VASlice->luma_weight_l0,
				     VASlice->luma_offset_l0,
				     VASlice->chroma_weight_l0,
				     VASlice->chroma_offset_l0);

	if ((VASlice->slice_type % 5) == H264_SLICE_B)
		h264_copy_pred_table(&slice->pred_weight_table.weight_factors[1],
				     slice->num_ref_idx_l1_active_minus1 + 1,
				     VASlice->luma_weight_l1,
				     VASlice->luma_offset_l1,
				     VASlice->chroma_weight_l1,
				     VASlice->chroma_offset_l1);
}

int h264_get_controls(struct request_data *driver_data,
		      struct object_context *context)
{
	struct v4l2_ext_control controls[2] = {
		{
			.id = V4L2_CID_MPEG_VIDEO_H264_DECODE_MODE,
		}, {
			.id = V4L2_CID_MPEG_VIDEO_H264_START_CODE,
		}
	};
	int rc;

	rc = v4l2_get_controls(context->video_fd, -1, controls, 2);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	switch (controls[0].value) {
	case V4L2_MPEG_VIDEO_H264_DECODE_MODE_SLICE_BASED:
		break;
	case V4L2_MPEG_VIDEO_H264_DECODE_MODE_FRAME_BASED:
		break;
	default:
		request_log("Unsupported decode mode\n");
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	switch (controls[1].value) {
	case V4L2_MPEG_VIDEO_H264_START_CODE_NONE:
		break;
	case V4L2_MPEG_VIDEO_H264_START_CODE_ANNEX_B:
		context->h264_start_code = true;
		break;
	default:
		request_log("Unsupported start code\n");
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	return VA_STATUS_SUCCESS;
}

static inline __u8 h264_profile_to_idc(VAProfile profile)
{
	switch (profile) {
	case VAProfileH264Main:
		return 77;
	case VAProfileH264High:
		return 100;
	case VAProfileH264ConstrainedBaseline:
		return 66;
	case VAProfileH264MultiviewHigh:
		return 118;
	case VAProfileH264StereoHigh:
		return 128;
	default:
		return 0;
	}
}

int h264_set_controls(struct request_data *driver_data,
		      struct object_context *context,
		      VAProfile profile,
		      struct object_surface *surface)
{
	struct v4l2_ctrl_h264_scaling_matrix matrix = { 0 };
	struct v4l2_ctrl_h264_decode_params decode = { 0 };
	struct v4l2_ctrl_h264_slice_params slice = { 0 };
	struct v4l2_ctrl_h264_pps pps = { 0 };
	struct v4l2_ctrl_h264_sps sps = { 0 };
	struct h264_dpb_entry *output;
	int rc;

	output = dpb_lookup(context, &surface->params.h264.picture.CurrPic,
			    NULL);
	if (!output)
		output = dpb_find_entry(context);

	dpb_clear_entry(output, true);

	dpb_update(context, &surface->params.h264.picture);

	h264_va_picture_to_v4l2(driver_data, context, surface,
				&surface->params.h264.picture,
				&decode, &pps, &sps);
	h264_va_matrix_to_v4l2(driver_data, context,
			       &surface->params.h264.matrix, &matrix);
	h264_va_slice_to_v4l2(driver_data, context,
			      &surface->params.h264.slice,
			      &surface->params.h264.picture, &slice);

	sps.profile_idc = h264_profile_to_idc(profile);

	struct v4l2_ext_control controls[5] = {
		{
			.id = V4L2_CID_MPEG_VIDEO_H264_SPS,
			.ptr = &sps,
			.size = sizeof(sps),
		}, {
			.id = V4L2_CID_MPEG_VIDEO_H264_PPS,
			.ptr = &pps,
			.size = sizeof(pps),
		}, {
			.id = V4L2_CID_MPEG_VIDEO_H264_SCALING_MATRIX,
			.ptr = &matrix,
			.size = sizeof(matrix),
		}, {
			.id = V4L2_CID_MPEG_VIDEO_H264_SLICE_PARAMS,
			.ptr = &slice,
			.size = sizeof(slice),
		}, {
			.id = V4L2_CID_MPEG_VIDEO_H264_DECODE_PARAMS,
			.ptr = &decode,
			.size = sizeof(decode),
		}
	};

	rc = v4l2_set_controls(context->video_fd, surface->request_fd,
			       controls, 5);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	dpb_insert(context, &surface->params.h264.picture.CurrPic, output);

	return VA_STATUS_SUCCESS;
}
