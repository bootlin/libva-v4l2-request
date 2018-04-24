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
#include "buffer.h"
#include "context.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

VAStatus SunxiCedrusCreateBuffer(VADriverContextP context,
	VAContextID context_id, VABufferType type, unsigned int size,
	unsigned int count, void *data, VABufferID *buffer_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	struct object_buffer *buffer_object;
	struct v4l2_plane plane[1];
	struct v4l2_buffer buf;
	void *buffer_data;
	void *map_data;
	unsigned int map_size;
	VABufferID id;
	int rc;

	switch (type) {
		case VAPictureParameterBufferType:
		case VAIQMatrixBufferType:
		case VASliceParameterBufferType:
		case VASliceDataBufferType:
		case VAImageBufferType:
			break;
		default:
			return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
	}

	id = object_heap_allocate(&driver_data->buffer_heap);
	buffer_object = BUFFER(buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	if(buffer_object->type == VASliceDataBufferType) {
		context_object = CONTEXT(context_id);
		if (context_object == NULL) {
			object_heap_free(&driver_data->buffer_heap, (struct object_base *) buffer_object);
			return VA_STATUS_ERROR_INVALID_CONTEXT;
		}

		memset(plane, 0, sizeof(plane));

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = context_object->num_rendered_surfaces % INPUT_BUFFERS_NB;
		buf.length = 1;
		buf.m.planes = plane;

		rc = ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYBUF, &buf);
		if (rc < 0) {
			object_heap_free(&driver_data->buffer_heap, (struct object_base *) buffer_object);
			return VA_STATUS_ERROR_ALLOCATION_FAILED;
		}

		map_size = driver_data->slice_offset[buf.index] + size * count;
		map_data = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			driver_data->mem2mem_fd, buf.m.planes[0].m.mem_offset);

		buffer_data = map_data + driver_data->slice_offset[buf.index];
		driver_data->slice_offset[buf.index] += size * count;
	} else {
		buffer_data = malloc(size * count);
		map_size = 0;
		map_data = NULL;
	}

	if (buffer_object->data == NULL || buffer_object->map == MAP_FAILED)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	if (data)
		memcpy(map_data, data, size * count);

	buffer_object->type = type;
	buffer_object->initial_count = count;
	buffer_object->count = count;

	buffer_object->data = buffer_data;
	buffer_object->map = map_data;
	buffer_object->size = size;
	buffer_object->map_size = map_size;

	*buffer_id = id;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroyBuffer(VADriverContextP context,
	VABufferID buffer_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	if (buffer_object->data != NULL) {
		if (buffer_object->type != VASliceDataBufferType)
			free(buffer_object->data);
		else if (buffer_object->map != NULL && buffer_object->map_size > 0)
			munmap(buffer_object->map, buffer_object->map_size);
	}

	object_heap_free(&driver_data->buffer_heap, (struct object_base *) buffer_object);

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusMapBuffer(VADriverContextP context, VABufferID buffer_id,
	void **data_map)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(buffer_id);
	if (buffer_object == NULL || buffer_object->data == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	/* Our buffers are always mapped. */
	*data_map = buffer_object->data;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusUnmapBuffer(VADriverContextP context, VABufferID buffer_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(buffer_id);
	if (buffer_object == NULL || buffer_object->data == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	/* Our buffers are always mapped. */

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusBufferSetNumElements(VADriverContextP context,
	VABufferID buffer_id, unsigned int count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	if (count > buffer_object->initial_count)
		return VA_STATUS_ERROR_INVALID_PARAMETER;

	buffer_object->count = count;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusBufferInfo(VADriverContextP context, VABufferID buffer_id,
	VABufferType *type, unsigned int *size, unsigned int *count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	*type = buffer_object->type;
	*size = buffer_object->size;
	*count = buffer_object->count;

	return VA_STATUS_SUCCESS;
}
