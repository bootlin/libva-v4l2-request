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

#include "v4l2.h"

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
	unsigned int pixelformat;
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

	switch (config_object->profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			pixelformat = V4L2_PIX_FMT_MPEG2_FRAME;
			break;

		default:
			return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	rc = v4l2_set_format(driver_data->mem2mem_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, pixelformat, picture_width, picture_height);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_create_buffers(video_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, INPUT_BUFFERS_NB);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	rc = v4l2_set_stream(driver_data->mem2mem_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, true);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_set_stream(driver_data->mem2mem_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, true);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
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

	context_object = (struct object_context *) object_heap_lookup(&driver_data->context_heap, context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	object_heap_free(&driver_data->context_heap, (struct object_base *) context_object);

	rc = v4l2_set_stream(driver_data->mem2mem_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, false);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_stream(driver_data->mem2mem_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, false);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	return VA_STATUS_SUCCESS;
}
