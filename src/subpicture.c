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
#include "subpicture.h"

/*
 * Subpictures aren't supported yet
 */

VAStatus SunxiCedrusQuerySubpictureFormats(VADriverContextP ctx,
		VAImageFormat *format_list, unsigned int *flags,
		unsigned int *num_formats)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusCreateSubpicture(VADriverContextP ctx, VAImageID image,
		VASubpictureID *subpicture)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusDestroySubpicture(VADriverContextP ctx,
		VASubpictureID subpicture)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusSetSubpictureImage(VADriverContextP ctx,
		VASubpictureID subpicture, VAImageID image)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusSetSubpicturePalette(VADriverContextP ctx,
		VASubpictureID subpicture, unsigned char *palette)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusSetSubpictureChromakey(VADriverContextP ctx,
		VASubpictureID subpicture, unsigned int chromakey_min,
		unsigned int chromakey_max, unsigned int chromakey_mask)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusSetSubpictureGlobalAlpha(VADriverContextP ctx,
		VASubpictureID subpicture, float global_alpha)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusAssociateSubpicture(VADriverContextP ctx,
		VASubpictureID subpicture, VASurfaceID *target_surfaces,
		int num_surfaces, short src_x, short src_y,
		unsigned short src_width, unsigned short src_height,
		short dest_x, short dest_y, unsigned short dest_width,
		unsigned short dest_height, unsigned int flags)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusDeassociateSubpicture(VADriverContextP ctx,
		VASubpictureID subpicture, VASurfaceID *target_surfaces,
		int num_surfaces)
{ return VA_STATUS_SUCCESS; }

