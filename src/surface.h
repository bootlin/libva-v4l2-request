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

#ifndef _SURFACE_H_
#define _SURFACE_H_

#include <va/va_backend.h>

#include "object_heap.h"

#define SURFACE(id) ((struct object_surface *) object_heap_lookup(&driver_data->surface_heap, id))
#define SURFACE_ID_OFFSET		0x04000000

struct object_surface {
	struct object_base base;

	VASurfaceID surface_id;
	VAStatus status;
	int width;
	int height;

	uint32_t request;
	uint32_t input_buf_index;
	uint32_t output_buf_index;
};

VAStatus SunxiCedrusCreateSurfaces(VADriverContextP context, int width,
	int height, int format, int surfaces_count, VASurfaceID *surfaces_ids);
VAStatus SunxiCedrusDestroySurfaces(VADriverContextP context,
	VASurfaceID *surfaces_ids, int surfaces_count);
VAStatus SunxiCedrusSyncSurface(VADriverContextP context,
	VASurfaceID surface_id);
VAStatus SunxiCedrusQuerySurfaceStatus(VADriverContextP context,
	VASurfaceID surface_id, VASurfaceStatus *status);
VAStatus SunxiCedrusPutSurface(VADriverContextP context, VASurfaceID surface_id,
	void *draw, short src_x, short src_y, unsigned short src_width,
	unsigned short src_height, short dst_x, short dst_y,
	unsigned short dst_width, unsigned short dst_height,
	VARectangle *cliprects, unsigned int cliprects_count,
	unsigned int flags);
VAStatus SunxiCedrusLockSurface(VADriverContextP context,
	VASurfaceID surface_id, unsigned int *fourcc, unsigned int *luma_stride,
	unsigned int *chroma_u_stride, unsigned int *chroma_v_stride,
	unsigned int *luma_offset, unsigned int *chroma_u_offset,
	unsigned int *chroma_v_offset, unsigned int *buffer_name,
	void **buffer);
VAStatus SunxiCedrusUnlockSurface(VADriverContextP context,
	VASurfaceID surface_id);

#endif
