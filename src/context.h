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

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include <va/va_backend.h>

#include "object_heap.h"

#define CONTEXT(data, id) ((struct object_context *) object_heap_lookup(&(data)->context_heap, id))
#define CONTEXT_ID_OFFSET		0x02000000

struct object_context {
	struct object_base base;

	VAConfigID config_id;
	VASurfaceID render_surface_id;
	VASurfaceID *surfaces_ids;
	int surfaces_count;

	int picture_width;
	int picture_height;
	int flags;
};

VAStatus SunxiCedrusCreateContext(VADriverContextP context,
	VAConfigID config_id, int picture_width, int picture_height, int flags,
	VASurfaceID *surfaces_ids, int surfaces_count,
	VAContextID *context_id);
VAStatus SunxiCedrusDestroyContext(VADriverContextP context,
	VAContextID context_id);

#endif
