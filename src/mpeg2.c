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

#include "mpeg2.h"
#include "context.h"
#include "request.h"
#include "surface.h"

#include <assert.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "v4l2.h"

int mpeg2_set_controls(struct request_data *driver_data,
		       struct object_context *context_object,
		       struct object_surface *surface_object)
{
	VAPictureParameterBufferMPEG2 *parameters =
		&surface_object->params.mpeg2.picture;
	VAIQMatrixBufferMPEG2 *iqmatrix =
		&surface_object->params.mpeg2.iqmatrix;
	bool iqmatrix_set = surface_object->params.mpeg2.iqmatrix_set;
	struct v4l2_ctrl_mpeg2_slice_params slice_params;
	struct v4l2_ctrl_mpeg2_quantization quantization;
	struct object_surface *forward_reference_surface;
	struct object_surface *backward_reference_surface;
	unsigned int i;
	int rc;

	memset(&slice_params, 0, sizeof(slice_params));

	slice_params.width = surface_object->width;
	slice_params.height = surface_object->height;

	slice_params.slice_pos = 0;
	slice_params.slice_len = surface_object->slices_size * 8;

	slice_params.slice_type = parameters->picture_coding_type;
	slice_params.f_code[0][0] = (parameters->f_code >> 12) & 0x0f;
	slice_params.f_code[0][1] = (parameters->f_code >> 8) & 0x0f;
	slice_params.f_code[1][0] = (parameters->f_code >> 4) & 0x0f;
	slice_params.f_code[1][1] = (parameters->f_code >> 0) & 0x0f;

	slice_params.intra_dc_precision =
		parameters->picture_coding_extension.bits.intra_dc_precision;
	slice_params.picture_structure =
		parameters->picture_coding_extension.bits.picture_structure;
	slice_params.top_field_first =
		parameters->picture_coding_extension.bits.top_field_first;
	slice_params.frame_pred_frame_dct =
		parameters->picture_coding_extension.bits.frame_pred_frame_dct;
	slice_params.concealment_motion_vectors =
		parameters->picture_coding_extension.bits
			.concealment_motion_vectors;
	slice_params.q_scale_type =
		parameters->picture_coding_extension.bits.q_scale_type;
	slice_params.intra_vlc_format =
		parameters->picture_coding_extension.bits.intra_vlc_format;
	slice_params.alternate_scan =
		parameters->picture_coding_extension.bits.alternate_scan;

	forward_reference_surface =
		SURFACE(driver_data, parameters->forward_reference_picture);
	if (forward_reference_surface != NULL)
		slice_params.forward_ref_index =
			forward_reference_surface->destination_index;
	else
		slice_params.forward_ref_index =
			surface_object->destination_index;

	backward_reference_surface =
		SURFACE(driver_data, parameters->backward_reference_picture);
	if (backward_reference_surface != NULL)
		slice_params.backward_ref_index =
			backward_reference_surface->destination_index;
	else
		slice_params.backward_ref_index =
			surface_object->destination_index;

	rc = v4l2_set_control(driver_data->video_fd, surface_object->request_fd,
			      V4L2_CID_MPEG_VIDEO_MPEG2_SLICE_PARAMS,
			      &slice_params, sizeof(slice_params));
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	if (iqmatrix_set) {
		quantization.load_intra_quantiser_matrix =
			iqmatrix->load_intra_quantiser_matrix;
		quantization.load_non_intra_quantiser_matrix =
			iqmatrix->load_non_intra_quantiser_matrix;
		quantization.load_chroma_intra_quantiser_matrix =
			iqmatrix->load_chroma_intra_quantiser_matrix;
		quantization.load_chroma_non_intra_quantiser_matrix =
			iqmatrix->load_chroma_non_intra_quantiser_matrix;

		for (i = 0; i < 64; i++) {
			quantization.intra_quantiser_matrix[i] =
				iqmatrix->intra_quantiser_matrix[i];
			quantization.non_intra_quantiser_matrix[i] =
				iqmatrix->non_intra_quantiser_matrix[i];
			quantization.chroma_intra_quantiser_matrix[i] =
				iqmatrix->chroma_intra_quantiser_matrix[i];
			quantization.chroma_non_intra_quantiser_matrix[i] =
				iqmatrix->chroma_non_intra_quantiser_matrix[i];
		}

		rc = v4l2_set_control(driver_data->video_fd,
				      surface_object->request_fd,
				      V4L2_CID_MPEG_VIDEO_MPEG2_QUANTIZATION,
				      &quantization, sizeof(quantization));
	}

	return 0;
}
