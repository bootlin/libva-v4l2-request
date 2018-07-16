/*
 * Copyright (c) 2018 Bootlin
 * Copyright (c) 2016 Florent Revest, <florent.revest@free-electrons.com>
 *               2007 Intel Corporation. All Rights Reserved.
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
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "sunxi_cedrus.h"
#include "surface.h"
#include "v4l2.h"

enum h264_slice_type {
	H264_SLICE_P    = 0,
	H264_SLICE_B    = 1,
};

static int h264_lookup_ref_pic(VAPictureParameterBufferH264 *VAPicture,
			       unsigned int frame_num)
{
	int i;

	for (i = 0; i < VAPicture->num_ref_frames; i++) {
		VAPictureH264 *pic = &VAPicture->ReferenceFrames[i];

		if (frame_num == pic->frame_idx)
			return i;
	}

	return 0;
}

static void h264_va_picture_to_v4l2(struct cedrus_data *driver_data,
				    VAPictureParameterBufferH264 *VAPicture,
				    struct v4l2_ctrl_h264_decode_param *decode,
				    struct v4l2_ctrl_h264_pps *pps,
				    struct v4l2_ctrl_h264_sps *sps)
{
	int i;

	decode->num_slices = VAPicture->num_ref_frames;
	decode->top_field_order_cnt = VAPicture->CurrPic.TopFieldOrderCnt;
	decode->bottom_field_order_cnt = VAPicture->CurrPic.BottomFieldOrderCnt;

	for (i = 0; i < VAPicture->num_ref_frames; i++) {
		struct v4l2_h264_dpb_entry *dpb = &decode->dpb[i];
		VAPictureH264 *pic = &VAPicture->ReferenceFrames[i];
		struct object_surface *surface_object =
			SURFACE(driver_data, pic->picture_id);

		if (surface_object)
			dpb->buf_index = surface_object->destination_index;

		dpb->frame_num = pic->frame_idx;
		dpb->top_field_order_cnt = pic->TopFieldOrderCnt;
		dpb->bottom_field_order_cnt = pic->BottomFieldOrderCnt;

		if (pic->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)
			dpb->flags |= V4L2_H264_DPB_ENTRY_FLAG_LONG_TERM;

		if (!(pic->flags & VA_PICTURE_H264_INVALID))
			dpb->flags |= V4L2_H264_DPB_ENTRY_FLAG_ACTIVE;
	}

	pps->weighted_bipred_idc =
		VAPicture->pic_fields.bits.weighted_bipred_idc;
	pps->pic_init_qs_minus26 = VAPicture->pic_init_qs_minus26;
	pps->chroma_qp_index_offset = VAPicture->chroma_qp_index_offset;
	pps->second_chroma_qp_index_offset =
		VAPicture->second_chroma_qp_index_offset;

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

static void h264_va_matrix_to_v4l2(struct cedrus_data *driver_data,
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
	memcpy(v4l2_matrix->scaling_list_8x8[3], &VAMatrix->ScalingList8x8[1],
	       sizeof(v4l2_matrix->scaling_list_8x8[3]));
}

static void h264_va_slice_to_v4l2(struct cedrus_data *driver_data,
				  VASliceParameterBufferH264 *VASlice,
				  VAPictureParameterBufferH264 *VAPicture,
				  struct v4l2_ctrl_h264_slice_param *slice)
{
	struct v4l2_h264_weight_factors *factors;
	int i;

	slice->size = VASlice->slice_data_size;
	slice->header_bit_size = VASlice->slice_data_bit_offset;
	slice->first_mb_in_slice = VASlice->first_mb_in_slice;
	slice->slice_type = VASlice->slice_type;
	slice->cabac_init_idc = VASlice->cabac_init_idc;
	slice->slice_qp_delta = VASlice->slice_qp_delta;
	slice->disable_deblocking_filter_idc =
		VASlice->disable_deblocking_filter_idc;
	slice->slice_alpha_c0_offset_div2 = VASlice->slice_alpha_c0_offset_div2;
	slice->slice_beta_offset_div2 = VASlice->slice_beta_offset_div2;

	if (((VASlice->slice_type % 5) == H264_SLICE_P) ||
	    ((VASlice->slice_type % 5) == H264_SLICE_B)) {
		slice->num_ref_idx_l0_active_minus1 =
			VASlice->num_ref_idx_l0_active_minus1;

		for (i = 0; i < VASlice->num_ref_idx_l0_active_minus1 + 1; i++)
			slice->ref_pic_list0[i] = h264_lookup_ref_pic(
				VAPicture, VASlice->RefPicList0[i].frame_idx);
	}

	if ((VASlice->slice_type % 5) == H264_SLICE_B) {
		slice->num_ref_idx_l1_active_minus1 =
			VASlice->num_ref_idx_l1_active_minus1;

		for (i = 0; i < VASlice->num_ref_idx_l1_active_minus1 + 1; i++)
			slice->ref_pic_list1[i] = h264_lookup_ref_pic(
				VAPicture, VASlice->RefPicList1[i].frame_idx);
	}

	if (VASlice->direct_spatial_mv_pred_flag)
		slice->flags |= V4L2_H264_SLICE_FLAG_DIRECT_SPATIAL_MV_PRED;

	slice->pred_weight_table.chroma_log2_weight_denom =
		VASlice->chroma_log2_weight_denom;
	slice->pred_weight_table.luma_log2_weight_denom =
		VASlice->luma_log2_weight_denom;

	factors = &slice->pred_weight_table.weight_factors[0];
	memcpy(&factors->chroma_weight, &VASlice->chroma_weight_l0,
	       sizeof(factors->chroma_weight));
	memcpy(&factors->chroma_offset, &VASlice->chroma_offset_l0,
	       sizeof(factors->chroma_offset));
	memcpy(&factors->luma_weight, &VASlice->luma_weight_l0,
	       sizeof(factors->luma_weight));
	memcpy(&factors->luma_offset, &VASlice->luma_offset_l0,
	       sizeof(factors->luma_offset));

	factors = &slice->pred_weight_table.weight_factors[1];
	memcpy(&factors->chroma_weight, &VASlice->chroma_weight_l1,
	       sizeof(factors->chroma_weight));
	memcpy(&factors->chroma_offset, &VASlice->chroma_offset_l1,
	       sizeof(factors->chroma_offset));
	memcpy(&factors->luma_weight, &VASlice->luma_weight_l1,
	       sizeof(factors->luma_weight));
	memcpy(&factors->luma_offset, &VASlice->luma_offset_l1,
	       sizeof(factors->luma_offset));
}

int h264_set_controls(struct cedrus_data *driver_data,
		      struct object_context *context,
		      struct object_surface *surface)
{
	struct v4l2_ctrl_h264_scaling_matrix matrix = { 0 };
	struct v4l2_ctrl_h264_decode_param decode = { 0 };
	struct v4l2_ctrl_h264_slice_param slice = { 0 };
	struct v4l2_ctrl_h264_pps pps = { 0 };
	struct v4l2_ctrl_h264_sps sps = { 0 };
	int rc;

	h264_va_picture_to_v4l2(driver_data, &surface->params.h264.picture,
				&decode, &pps, &sps);
	h264_va_matrix_to_v4l2(driver_data, &surface->params.h264.matrix,
			       &matrix);
	h264_va_slice_to_v4l2(driver_data, &surface->params.h264.slice,
			      &surface->params.h264.picture, &slice);

	rc = v4l2_set_control(driver_data->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_DECODE_PARAMS, &decode,
			      sizeof(decode));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver_data->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_SLICE_PARAMS, &slice,
			      sizeof(slice));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver_data->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_PPS, &pps, sizeof(pps));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver_data->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_SPS, &sps, sizeof(sps));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_control(driver_data->video_fd, surface->request_fd,
			      V4L2_CID_MPEG_VIDEO_H264_SCALING_MATRIX, &matrix,
			      sizeof(matrix));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	return VA_STATUS_SUCCESS;
}
