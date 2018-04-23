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
	void *buffer_data;
	void *buffer_map;
	int max_num_elements;
	int num_elements;
	VABufferType type;
	unsigned int size;
	unsigned int map_size;
};

VAStatus SunxiCedrusCreateBuffer(VADriverContextP ctx, VAContextID context,
		VABufferType type, unsigned int size, unsigned int num_elements,
		void *data, VABufferID *buf_id);

VAStatus SunxiCedrusBufferSetNumElements(VADriverContextP ctx,
		VABufferID buf_id, unsigned int num_elements);

VAStatus SunxiCedrusMapBuffer(VADriverContextP ctx, VABufferID buf_id,
		void **pbuf);

VAStatus SunxiCedrusUnmapBuffer(VADriverContextP ctx, VABufferID buf_id);

void sunxi_cedrus_destroy_buffer(struct sunxi_cedrus_driver_data *driver_data,
		struct object_buffer *obj_buffer);

VAStatus SunxiCedrusDestroyBuffer(VADriverContextP ctx, VABufferID buffer_id);

VAStatus SunxiCedrusBufferInfo(VADriverContextP ctx, VABufferID buf_id,
		VABufferType *type, unsigned int *size,
		unsigned int *num_elements);

#endif /* _BUFFER_H_ */
