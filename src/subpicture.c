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

#include "request.h"
#include "subpicture.h"

VAStatus RequestCreateSubpicture(VADriverContextP context, VAImageID image_id,
				 VASubpictureID *subpicture_id)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestDestroySubpicture(VADriverContextP context,
				  VASubpictureID subpicture_id)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestQuerySubpictureFormats(VADriverContextP context,
				       VAImageFormat *formats,
				       unsigned int *flags,
				       unsigned int *formats_count)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestSetSubpictureImage(VADriverContextP context,
				   VASubpictureID subpicture_id,
				   VAImageID image_id)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestSetSubpicturePalette(VADriverContextP context,
				     VASubpictureID subpicture_id,
				     unsigned char *palette)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestSetSubpictureChromakey(VADriverContextP context,
				       VASubpictureID subpicture_id,
				       unsigned int chromakey_min,
				       unsigned int chromakey_max,
				       unsigned int chromakey_mask)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestSetSubpictureGlobalAlpha(VADriverContextP context,
					 VASubpictureID subpicture_id,
					 float global_alpha)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestAssociateSubpicture(
	VADriverContextP context, VASubpictureID subpicture_id,
	VASurfaceID *surfaces_ids, int surfaces_count, short src_x, short src_y,
	unsigned short src_width, unsigned short src_height, short dst_x,
	short dst_y, unsigned short dst_width, unsigned short dst_height,
	unsigned int flags)
{
	return VA_STATUS_SUCCESS;
}

VAStatus RequestDeassociateSubpicture(VADriverContextP context,
				      VASubpictureID subpicture_id,
				      VASurfaceID *surfaces_ids,
				      int surfaces_count)
{
	return VA_STATUS_SUCCESS;
}
