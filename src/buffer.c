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
	int bufferID;
	struct v4l2_plane plane[1];
	struct object_buffer *obj_buffer;

	memset(plane, 0, sizeof(struct v4l2_plane));

	/*
	 * A Buffer is a memory zone used to handle all kind of data, for example an IQ
	 * matrix or image buffer (which are allocated using realloc) or slice data
	 * (which are mmapped from v4l's kernel space)
	 */

	/* Validate type */
	switch (type) {
		case VAPictureParameterBufferType:
		case VAIQMatrixBufferType: /* Ignored */
		case VASliceParameterBufferType:
		case VASliceDataBufferType:
		case VAImageBufferType:
			/* Ok */
			break;
		default:
			return VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
	}

	bufferID = object_heap_allocate(&driver_data->buffer_heap);
	obj_buffer = BUFFER(bufferID);
	if (obj_buffer == NULL)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	obj_buffer->type = type;
	obj_buffer->initial_count = count;
	obj_buffer->count = count;

	obj_buffer->data = NULL;
	obj_buffer->map = NULL;
	obj_buffer->size = size;
	obj_buffer->map_size = 0;

	if(obj_buffer->type == VASliceDataBufferType) {
		struct object_context *obj_context;

		obj_context = CONTEXT(context_id);
		assert(obj_context);

		struct v4l2_buffer buf;
		memset(&(buf), 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = obj_context->num_rendered_surfaces%INPUT_BUFFERS_NB;
		buf.length = 1;
		buf.m.planes = plane;

		assert(ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYBUF, &buf)==0);

		obj_buffer->map_size = driver_data->slice_offset[buf.index] + size * count;
		obj_buffer->map = mmap(NULL, obj_buffer->map_size,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				driver_data->mem2mem_fd, buf.m.planes[0].m.mem_offset);

		obj_buffer->data = obj_buffer->map + driver_data->slice_offset[buf.index];
		driver_data->slice_offset[buf.index] += size * count;
	} else {
		obj_buffer->map = NULL;
		obj_buffer->data = malloc(size * count);
	}

	if (obj_buffer->data == NULL || obj_buffer->map == MAP_FAILED)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	if (data)
		memcpy(obj_buffer->data, data, size * count);

	*buffer_id = bufferID;

	return VA_STATUS_SUCCESS;
}

void sunxi_cedrus_destroy_buffer(struct sunxi_cedrus_driver_data *driver_data,
	struct object_buffer *obj_buffer)
{
	if (obj_buffer->data != NULL) {
		if (obj_buffer->type != VASliceDataBufferType)
			free(obj_buffer->data);
		else if (obj_buffer->map != NULL && obj_buffer->map_size > 0)
			munmap(obj_buffer->map, obj_buffer->map_size);

		obj_buffer->map = NULL;
		obj_buffer->data = NULL;
	}

	object_heap_free(&driver_data->buffer_heap, obj_buffer);
}

VAStatus SunxiCedrusDestroyBuffer(VADriverContextP context,
	VABufferID buffer_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *obj_buffer = BUFFER(buffer_id);
	assert(obj_buffer);

	sunxi_cedrus_destroy_buffer(driver_data, obj_buffer);

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusMapBuffer(VADriverContextP context, VABufferID buffer_id,
	void **data_map)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VAStatus vaStatus = VA_STATUS_ERROR_UNKNOWN;
	struct object_buffer *obj_buffer = BUFFER(buffer_id);
	assert(obj_buffer);

	if (NULL == obj_buffer)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_BUFFER;
		return vaStatus;
	}

	if (NULL != obj_buffer->data)
	{
		*data_map = obj_buffer->data;
		vaStatus = VA_STATUS_SUCCESS;
	}
	return vaStatus;
}

VAStatus SunxiCedrusUnmapBuffer(VADriverContextP context, VABufferID buffer_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *obj_buffer = BUFFER(buffer_id);

	if (obj_buffer == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	/* Do nothing */
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusBufferSetNumElements(VADriverContextP context,
	VABufferID buffer_id, unsigned int count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	struct object_buffer *obj_buffer = BUFFER(buffer_id);
	assert(obj_buffer);

	if ((count < 0) || (count > obj_buffer->initial_count))
		vaStatus = VA_STATUS_ERROR_UNKNOWN;
	if (VA_STATUS_SUCCESS == vaStatus)
		obj_buffer->count = count;

	return vaStatus;
}

VAStatus SunxiCedrusBufferInfo(VADriverContextP context, VABufferID buffer_id,
	VABufferType *type, unsigned int *size, unsigned int *count)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
