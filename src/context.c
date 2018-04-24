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
#include "context.h"
#include "config.h"
#include "surface.h"

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

VAStatus SunxiCedrusCreateContext(VADriverContextP context,
	VAConfigID config_id, int picture_width, int picture_height, int flag,
	VASurfaceID *surfaces_ids, int surfaces_count,
	VAContextID *context_id)
{
	struct dump_driver_data *driver_data = (struct dump_driver_data *) context->pDriverData;
	struct object_config *config_object;
	struct object_surface *surface_object;
	struct object_context *context_object = NULL;
	VASurfaceID *ids = NULL;
	VAContextID id;
	VAStatus status;
	struct v4l2_create_buffers create_buffers;
	struct v4l2_format format;
	enum v4l2_buf_type type;
	unsigned int i;

	config_object = CONFIG(config_id);
	if (config_object == NULL) {
		status = VA_STATUS_ERROR_INVALID_CONFIG;
		goto error;
	}

	id = object_heap_allocate(&driver_data->context_heap);
	context_object = CONTEXT(id);
	if (context_object == NULL) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	ids = malloc(surfaces_count * sizeof(VASurfaceID));
	if (ids == NULL) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	for (i = 0; i < surfaces_count; i++) {
		surface_object = SURFACE(surfaces_ids[i]);
		if (surface_object == NULL) {
			status = VA_STATUS_ERROR_INVALID_SURFACE;
			goto error;
		}

		ids[i] = surfaces_ids[i];
	}

	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	format.fmt.pix_mp.width = picture_width;
	format.fmt.pix_mp.height = picture_height;
	format.fmt.pix_mp.plane_fmt[0].sizeimage = INPUT_BUFFER_MAX_SIZE * INPUT_BUFFERS_NB;
	format.fmt.pix_mp.field = V4L2_FIELD_ANY;
	format.fmt.pix_mp.num_planes = 1;

	switch (config_object->profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_MPEG2_FRAME;
			break;
		default:
			status = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
			goto error;
	}

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_S_FMT, &format);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	memset(&create_buffers, 0, sizeof(create_buffers));
	create_buffers.count = INPUT_BUFFERS_NB;
	create_buffers.memory = V4L2_MEMORY_MMAP;
	create_buffers.format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_G_FMT, &create_buffers.format);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_CREATE_BUFS, &create_buffers);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_STREAMON, &type);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_STREAMON, &type);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	context_object->config_id = config_id;
	context_object->surfaces_ids = ids;

	*context_id = id;

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	if (ids != NULL)
		free(ids);

	if (context_object != NULL)
		object_heap_free(&driver_data->context_heap, (struct object_base *) context_object);

complete:
	return status;
}

VAStatus SunxiCedrusDestroyContext(VADriverContextP context,
	VAContextID context_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	enum v4l2_buf_type type;

	context_object = (struct object_context *) object_heap_lookup(&driver_data->context_heap, context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	object_heap_free(&driver_data->context_heap, (struct object_base *) context_object);

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_STREAMOFF, &type);
	if (rc < 0)
		return VA_STATUS_ERROR_UNKNOWN;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_STREAMOFF, &type);
	if (rc < 0)
		return VA_STATUS_ERROR_UNKNOWN;

	return VA_STATUS_SUCCESS;
}
