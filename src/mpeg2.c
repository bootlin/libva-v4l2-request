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

#include "sunxi_cedrus.h"
#include "mpeg2.h"

#include <assert.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

int mpeg2_fill_picture_parameters(struct sunxi_cedrus_driver_data *driver_data,
	struct object_context *context_object,
	struct object_surface *surface_object,
	VAPictureParameterBufferMPEG2 *parameters)
{
	struct v4l2_ctrl_mpeg2_frame_hdr *header = &surface_object->mpeg2_header;
	struct object_surface *forward_reference_surface;
	struct object_surface *backward_reference_surface;

	header->type = MPEG2;

	header->width = parameters->horizontal_size;
	header->height = parameters->vertical_size;

	header->picture_coding_type = parameters->picture_coding_type;
	header->f_code[0][0] = (parameters->f_code >> 12) & 0x0f;
	header->f_code[0][1] = (parameters->f_code >> 8) & 0x0f;
	header->f_code[1][0] = (parameters->f_code >> 4) & 0x0f;
	header->f_code[1][1] = (parameters->f_code >> 0) & 0x0f;

	header->intra_dc_precision = parameters->picture_coding_extension.bits.intra_dc_precision;
	header->picture_structure = parameters->picture_coding_extension.bits.picture_structure;
	header->top_field_first = parameters->picture_coding_extension.bits.top_field_first;
	header->frame_pred_frame_dct = parameters->picture_coding_extension.bits.frame_pred_frame_dct;
	header->concealment_motion_vectors = parameters->picture_coding_extension.bits.concealment_motion_vectors;
	header->q_scale_type = parameters->picture_coding_extension.bits.q_scale_type;
	header->intra_vlc_format = parameters->picture_coding_extension.bits.intra_vlc_format;
	header->alternate_scan = parameters->picture_coding_extension.bits.alternate_scan;

	forward_reference_surface = SURFACE(parameters->forward_reference_picture);
	if (forward_reference_surface != NULL)
		header->forward_ref_index = forward_reference_surface->destination_index;
	else
		header->forward_ref_index = surface_object->destination_index;

	backward_reference_surface = SURFACE(parameters->backward_reference_picture);
	if (backward_reference_surface != NULL)
		header->backward_ref_index = backward_reference_surface->destination_index;
	else
		header->backward_ref_index = surface_object->destination_index;

	header->width = context_object->picture_width;
	header->height = context_object->picture_height;

	return 0;
}

int mpeg2_fill_slice_data(struct sunxi_cedrus_driver_data *driver_data,
	struct object_context *context_object,
	struct object_surface *surface_object, void *data, unsigned int size)
{
	unsigned char *p = (unsigned char *) surface_object->source_data +
		surface_object->slices_size;

	/*
	 * Since there is no guarantee that the allocation order is the same as
	 * the submission order (via RenderPicture), we can't use a V4L2 buffer
	 * directly and have to copy from a regular buffer.
	 * */
	memcpy(p, data, size);

	surface_object->slices_size += size;

	return 0;
}
