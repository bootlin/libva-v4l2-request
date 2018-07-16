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

#include "buffer.h"
#include "context.h"
#include "sunxi_cedrus.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include "utils.h"
#include "v4l2.h"

VAStatus SunxiCedrusCreateBuffer(VADriverContextP context,
				 VAContextID context_id, VABufferType type,
				 unsigned int size, unsigned int count,
				 void *data, VABufferID *buffer_id)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *)context->pDriverData;
	struct object_buffer *buffer_object = NULL;
	void *buffer_data;
	VAStatus status;
	VABufferID id;

	switch (type) {
	case VAPictureParameterBufferType:
	case VAIQMatrixBufferType:
	case VASliceParameterBufferType:
	case VASliceDataBufferType:
	case VAImageBufferType:
		break;

	default:
		status = VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE;
		goto error;
	}

	id = object_heap_allocate(&driver_data->buffer_heap);
	buffer_object = BUFFER(driver_data, id);
	if (buffer_object == NULL) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	buffer_data = malloc(size * count);
	if (buffer_data == NULL) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	if (data != NULL)
		memcpy(buffer_data, data, size * count);

	buffer_object->type = type;
	buffer_object->initial_count = count;
	buffer_object->count = count;
	buffer_object->data = buffer_data;
	buffer_object->size = size;

	*buffer_id = id;

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	if (buffer_object != NULL)
		object_heap_free(&driver_data->buffer_heap,
				 (struct object_base *)buffer_object);

complete:
	return status;
}

VAStatus SunxiCedrusDestroyBuffer(VADriverContextP context,
				  VABufferID buffer_id)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *)context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(driver_data, buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	if (buffer_object->data != NULL)
		free(buffer_object->data);

	object_heap_free(&driver_data->buffer_heap,
			 (struct object_base *)buffer_object);

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusMapBuffer(VADriverContextP context, VABufferID buffer_id,
			      void **data_map)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *)context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(driver_data, buffer_id);
	if (buffer_object == NULL || buffer_object->data == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	/* Our buffers are always mapped. */
	*data_map = buffer_object->data;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusUnmapBuffer(VADriverContextP context, VABufferID buffer_id)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *)context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(driver_data, buffer_id);
	if (buffer_object == NULL || buffer_object->data == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	/* Our buffers are always mapped. */

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusBufferSetNumElements(VADriverContextP context,
					 VABufferID buffer_id,
					 unsigned int count)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *)context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(driver_data, buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	if (count > buffer_object->initial_count)
		return VA_STATUS_ERROR_INVALID_PARAMETER;

	buffer_object->count = count;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusBufferInfo(VADriverContextP context, VABufferID buffer_id,
			       VABufferType *type, unsigned int *size,
			       unsigned int *count)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *)context->pDriverData;
	struct object_buffer *buffer_object;

	buffer_object = BUFFER(driver_data, buffer_id);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	*type = buffer_object->type;
	*size = buffer_object->size;
	*count = buffer_object->count;

	return VA_STATUS_SUCCESS;
}
