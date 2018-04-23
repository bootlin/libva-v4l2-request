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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <va/va_backend.h>

#include "object_heap.h"
#include "sunxi_cedrus.h"

#define BUFFER(id)  ((struct object_buffer *)  object_heap_lookup(&driver_data->buffer_heap,  id))
#define BUFFER_ID_OFFSET		0x08000000

struct object_buffer {
	struct object_base base;

	VABufferType type;
	unsigned int initial_count;
	unsigned int count;

	void *data;
	unsigned int size;

	void *map;
	unsigned int map_size;
};

VAStatus SunxiCedrusCreateBuffer(VADriverContextP context,
	VAContextID context_id, VABufferType type, unsigned int size,
	unsigned int count, void *data, VABufferID *buffer_id);
VAStatus SunxiCedrusBufferSetNumElements(VADriverContextP context,
	VABufferID buffer_id, unsigned int count);
VAStatus SunxiCedrusMapBuffer(VADriverContextP context, VABufferID buffer_id,
	void **data_map);
VAStatus SunxiCedrusUnmapBuffer(VADriverContextP context, VABufferID buffer_id);
void sunxi_cedrus_destroy_buffer(struct sunxi_cedrus_driver_data *driver_data,
	struct object_buffer *obj_buffer);
VAStatus SunxiCedrusDestroyBuffer(VADriverContextP context,
	VABufferID buffer_id);
VAStatus SunxiCedrusBufferInfo(VADriverContextP context, VABufferID buffer_id,
	VABufferType *type, unsigned int *size, unsigned int *count);

#endif
