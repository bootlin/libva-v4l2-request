/*
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

#include "sunxi_cedrus_drv_video.h"
#include "mpeg4.h"

#include <assert.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

/*
 * This file takes care of filling v4l2's frame API MPEG4 headers extended
 * controls from VA's data structures.
 */

VAStatus sunxi_cedrus_render_mpeg4_slice_data(VADriverContextP ctx,
		object_context_p obj_context, object_surface_p obj_surface,
		struct object_buffer *obj_buffer)
{
	INIT_DRIVER_DATA
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	struct v4l2_buffer buf;
	struct v4l2_plane plane[1];

	memset(plane, 0, sizeof(struct v4l2_plane));

	/* Query */
	memset(&(buf), 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = obj_surface->input_buf_index;
	buf.length = 1;
	buf.m.planes = plane;

	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYBUF, &buf)==0);

	/* Populate frame */
	char *src_buf = mmap(NULL, obj_buffer->size,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			driver_data->mem2mem_fd, buf.m.planes[0].m.mem_offset);
	assert(src_buf != MAP_FAILED);
	memcpy(src_buf, obj_buffer->buffer_data, obj_buffer->size);

	return vaStatus;
}

VAStatus sunxi_cedrus_render_mpeg4_picture_parameter(VADriverContextP ctx,
		object_context_p obj_context, object_surface_p obj_surface,
		struct object_buffer *obj_buffer)
{
	INIT_DRIVER_DATA
	VAStatus vaStatus = VA_STATUS_SUCCESS;

	VAPictureParameterBufferMPEG4 *pic_param = (VAPictureParameterBufferMPEG4 *)obj_buffer->buffer_data;

	obj_context->mpeg4_frame_hdr.width = pic_param->vop_width;
	obj_context->mpeg4_frame_hdr.height = pic_param->vop_height;

	obj_context->mpeg4_frame_hdr.vol_fields.short_video_header = pic_param->vol_fields.bits.short_video_header;
	obj_context->mpeg4_frame_hdr.vol_fields.chroma_format = pic_param->vol_fields.bits.chroma_format;
	obj_context->mpeg4_frame_hdr.vol_fields.interlaced = pic_param->vol_fields.bits.interlaced;
	obj_context->mpeg4_frame_hdr.vol_fields.obmc_disable = pic_param->vol_fields.bits.obmc_disable;
	obj_context->mpeg4_frame_hdr.vol_fields.sprite_enable = pic_param->vol_fields.bits.sprite_enable;
	obj_context->mpeg4_frame_hdr.vol_fields.sprite_warping_accuracy = pic_param->vol_fields.bits.sprite_warping_accuracy;
	obj_context->mpeg4_frame_hdr.vol_fields.quant_type = pic_param->vol_fields.bits.quant_type;
	obj_context->mpeg4_frame_hdr.vol_fields.quarter_sample = pic_param->vol_fields.bits.quarter_sample;
	obj_context->mpeg4_frame_hdr.vol_fields.data_partitioned = pic_param->vol_fields.bits.data_partitioned;
	obj_context->mpeg4_frame_hdr.vol_fields.reversible_vlc = pic_param->vol_fields.bits.reversible_vlc;
	obj_context->mpeg4_frame_hdr.vol_fields.resync_marker_disable = pic_param->vol_fields.bits.resync_marker_disable;

	obj_context->mpeg4_frame_hdr.vop_fields.vop_coding_type = pic_param->vop_fields.bits.vop_coding_type;
	obj_context->mpeg4_frame_hdr.vop_fields.backward_reference_vop_coding_type = pic_param->vop_fields.bits.backward_reference_vop_coding_type;
	obj_context->mpeg4_frame_hdr.vop_fields.vop_rounding_type = pic_param->vop_fields.bits.vop_rounding_type;
	obj_context->mpeg4_frame_hdr.vop_fields.intra_dc_vlc_thr = pic_param->vop_fields.bits.intra_dc_vlc_thr;
	obj_context->mpeg4_frame_hdr.vop_fields.top_field_first = pic_param->vop_fields.bits.top_field_first;
	obj_context->mpeg4_frame_hdr.vop_fields.alternate_vertical_scan_flag = pic_param->vop_fields.bits.alternate_vertical_scan_flag;

	obj_context->mpeg4_frame_hdr.vop_fcode_forward = pic_param->vop_fcode_forward;
	obj_context->mpeg4_frame_hdr.vop_fcode_backward = pic_param->vop_fcode_backward;

	obj_context->mpeg4_frame_hdr.trb = pic_param->TRB;
	obj_context->mpeg4_frame_hdr.trd = pic_param->TRD;

	object_surface_p fwd_surface = SURFACE(pic_param->forward_reference_picture);
	if(fwd_surface)
		obj_context->mpeg4_frame_hdr.forward_index = fwd_surface->output_buf_index;
	else
		obj_context->mpeg4_frame_hdr.forward_index = obj_surface->output_buf_index;
	object_surface_p bwd_surface = SURFACE(pic_param->backward_reference_picture);
	if(bwd_surface)
		obj_context->mpeg4_frame_hdr.backward_index = bwd_surface->output_buf_index;
	else
		obj_context->mpeg4_frame_hdr.backward_index = obj_surface->output_buf_index;

	return vaStatus;
}

VAStatus sunxi_cedrus_render_mpeg4_slice_parameter(VADriverContextP ctx,
		object_context_p obj_context, object_surface_p obj_surface,
		struct object_buffer *obj_buffer)
{
	VASliceParameterBufferMPEG4 *slice_param = (VASliceParameterBufferMPEG4 *)obj_buffer->buffer_data;

	obj_context->mpeg4_frame_hdr.slice_pos = slice_param->slice_data_offset*8+slice_param->macroblock_offset;
	obj_context->mpeg4_frame_hdr.slice_len = slice_param->slice_data_size*8;
	obj_context->mpeg4_frame_hdr.quant_scale = slice_param->quant_scale;

	return VA_STATUS_SUCCESS;
}

