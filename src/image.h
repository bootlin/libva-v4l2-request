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

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <va/va_backend.h>

#include "object_heap.h"

#define IMAGE(id)   ((struct object_image *)   object_heap_lookup(&driver_data->image_heap,   id))
#define IMAGE_ID_OFFSET			0x10000000

struct object_image {
	struct object_base base;
	VABufferID buffer_id;
};

VAStatus SunxiCedrusQueryImageFormats(VADriverContextP context,
	VAImageFormat *formats, int *formats_count);
VAStatus SunxiCedrusCreateImage(VADriverContextP context, VAImageFormat *format,
	int width, int height, VAImage *image);
VAStatus SunxiCedrusDeriveImage(VADriverContextP context,
	VASurfaceID surface_id, VAImage *image);
VAStatus SunxiCedrusDestroyImage(VADriverContextP context, VAImageID image_id);
VAStatus SunxiCedrusSetImagePalette(VADriverContextP context,
	VAImageID image_id, unsigned char *palette);
VAStatus SunxiCedrusGetImage(VADriverContextP context, VASurfaceID surface_id,
	int x, int y, unsigned int width, unsigned int height,
	VAImageID image_id);
VAStatus SunxiCedrusPutImage(VADriverContextP context, VASurfaceID surface_id,
	VAImageID image, int src_x, int src_y, unsigned int src_width,
	unsigned int src_height, int dst_x, int dst_y, unsigned int dst_width,
	unsigned int dst_height);

#endif /* _IMAGE_H_ */
