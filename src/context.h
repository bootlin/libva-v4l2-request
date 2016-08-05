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

/* We can't dynamically call VIDIOC_REQBUFS for every MPEG slice we create.
 * Indeed, the queue might be busy processing a previous buffer, so we need to
 * pre-allocate a set of buffers with a max size */
#define INPUT_BUFFER_MAX_SIZE		131072
#define INPUT_BUFFERS_NB		4

#define CONTEXT(id) ((object_context_p) object_heap_lookup(&driver_data->context_heap, id))
#define CONTEXT_ID_OFFSET		0x02000000

struct object_context {
	struct object_base base;
	VAContextID context_id;
	VAConfigID config_id;
	VASurfaceID current_render_target;
	int picture_width;
	int picture_height;
	int num_render_targets;
	int flags;
	VASurfaceID *render_targets;
	uint32_t num_rendered_surfaces;
};

typedef struct object_context *object_context_p;

VAStatus sunxi_cedrus_CreateContext(VADriverContextP ctx, VAConfigID config_id,
		int picture_width, int picture_height, int flag,
		VASurfaceID *render_targets, int num_render_targets,
		VAContextID *context);

VAStatus sunxi_cedrus_DestroyContext(VADriverContextP ctx, VAContextID context);

#endif /* _CONTEXT_H_ */
