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

#ifndef _SURFACES_H_
#define _SURFACES_H_

#include <va/va_backend.h>

#include "object_heap.h"

#define SURFACE(id) ((struct object_surface *) object_heap_lookup(&driver_data->surface_heap, id))
#define SURFACE_ID_OFFSET		0x04000000

struct object_surface {
	struct object_base base;
	VASurfaceID surface_id;
	uint32_t request;
	uint32_t input_buf_index;
	uint32_t output_buf_index;
	int width;
	int height;
	VAStatus status;
};

VAStatus sunxi_cedrus_CreateSurfaces(VADriverContextP ctx, int width,
		int height, int format, int num_surfaces, VASurfaceID *surfaces);

VAStatus sunxi_cedrus_DestroySurfaces(VADriverContextP ctx,
		VASurfaceID *surface_list, int num_surfaces);

VAStatus sunxi_cedrus_SyncSurface(VADriverContextP ctx,
		VASurfaceID render_target);

VAStatus sunxi_cedrus_QuerySurfaceStatus(VADriverContextP ctx,
		VASurfaceID render_target, VASurfaceStatus *status);

VAStatus sunxi_cedrus_PutSurface(VADriverContextP ctx, VASurfaceID surface,
		void *draw, short srcx, short srcy, unsigned short srcw,
		unsigned short srch, short destx, short desty,
		unsigned short destw, unsigned short desth,
		VARectangle *cliprects, unsigned int number_cliprects,
		unsigned int flags);

VAStatus sunxi_cedrus_LockSurface(VADriverContextP ctx, VASurfaceID surface,
		unsigned int *fourcc, unsigned int *luma_stride,
		unsigned int *chroma_u_stride, unsigned int *chroma_v_stride,
		unsigned int *luma_offset, unsigned int *chroma_u_offset,
		unsigned int *chroma_v_offset, unsigned int *buffer_name,
		void **buffer);

VAStatus sunxi_cedrus_UnlockSurface(VADriverContextP ctx, VASurfaceID surface);

#endif /* _SURFACES_H_ */
