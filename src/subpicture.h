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

#ifndef _SUBPICTURE_H_
#define _SUBPICTURE_H_

#include <va/va_backend.h>

VAStatus sunxi_cedrus_QuerySubpictureFormats(VADriverContextP ctx,
		VAImageFormat *format_list, unsigned int *flags,
		unsigned int *num_formats);

VAStatus sunxi_cedrus_CreateSubpicture(VADriverContextP ctx, VAImageID image,
		VASubpictureID *subpicture);

VAStatus sunxi_cedrus_DestroySubpicture(VADriverContextP ctx,
		VASubpictureID subpicture);

VAStatus sunxi_cedrus_SetSubpictureImage(VADriverContextP ctx,
		VASubpictureID subpicture, VAImageID image);

VAStatus sunxi_cedrus_SetSubpicturePalette(VADriverContextP ctx,
		VASubpictureID subpicture, unsigned char *palette);

VAStatus sunxi_cedrus_SetSubpictureChromakey(VADriverContextP ctx,
		VASubpictureID subpicture, unsigned int chromakey_min,
		unsigned int chromakey_max, unsigned int chromakey_mask);

VAStatus sunxi_cedrus_SetSubpictureGlobalAlpha(VADriverContextP ctx,
		VASubpictureID subpicture, float global_alpha);

VAStatus sunxi_cedrus_AssociateSubpicture(VADriverContextP ctx,
		VASubpictureID subpicture, VASurfaceID *target_surfaces,
		int num_surfaces, short src_x, short src_y,
		unsigned short src_width, unsigned short src_height,
		short dest_x, short dest_y, unsigned short dest_width,
		unsigned short dest_height, unsigned int flags);

VAStatus sunxi_cedrus_DeassociateSubpicture(VADriverContextP ctx,
		VASubpictureID subpicture, VASurfaceID *target_surfaces,
		int num_surfaces);

#endif /* _SUBPICTURE_H_ */
