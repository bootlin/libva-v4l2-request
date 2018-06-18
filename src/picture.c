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
#include "picture.h"
#include "buffer.h"
#include "context.h"
#include "surface.h"
#include "config.h"

#include "mpeg2.h"

#include <assert.h>
#include <string.h>

#include <errno.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "v4l2.h"
#include "media.h"
#include "utils.h"

VAStatus SunxiCedrusBeginPicture(VADriverContextP context,
	VAContextID context_id, VASurfaceID surface_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	struct object_surface *surface_object;

	context_object = CONTEXT(context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	if (surface_object->status == VASurfaceRendering)
		SunxiCedrusSyncSurface(context, surface_id);

	surface_object->status = VASurfaceRendering;
	context_object->render_surface_id = surface_id;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusRenderPicture(VADriverContextP context,
	VAContextID context_id, VABufferID *buffers_ids, int buffers_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	struct object_config *config_object;
	struct object_surface *surface_object;
	struct object_buffer *buffer_object;
	VAPictureParameterBufferMPEG2 *mpeg2_parameters;
	void *data;
	unsigned int size;
	int rc;
	int i;

	context_object = CONTEXT(context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	config_object = CONFIG(context_object->config_id);
	if (config_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONFIG;

	surface_object = SURFACE(context_object->render_surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	for (i = 0; i < buffers_count; i++) {
		buffer_object = BUFFER(buffers_ids[i]);
		if (buffer_object == NULL)
			return VA_STATUS_ERROR_INVALID_BUFFER;

		switch (config_object->profile) {
			case VAProfileMPEG2Simple:
			case VAProfileMPEG2Main:
				if (buffer_object->type == VASliceDataBufferType) {
					data = buffer_object->data;
					size = buffer_object->size * buffer_object->count;

					rc = mpeg2_fill_slice_data(driver_data, context_object, surface_object, data, size);
					if (rc < 0)
						return VA_STATUS_ERROR_OPERATION_FAILED;
				} else if (buffer_object->type == VAPictureParameterBufferType) {
					mpeg2_parameters = (VAPictureParameterBufferMPEG2 *) buffer_object->data;

					rc = mpeg2_fill_picture_parameters(driver_data, context_object, surface_object, mpeg2_parameters);
					if (rc < 0)
						return VA_STATUS_ERROR_OPERATION_FAILED;
				}
				break;

			default:
				break;
		}
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusEndPicture(VADriverContextP context,
	VAContextID context_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	struct object_config *config_object;
	struct object_surface *surface_object;
	void *control_data;
	unsigned int control_size;
	unsigned int control_id;
	int request_fd;
	VAStatus status;
	int rc;

	context_object = CONTEXT(context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	config_object = CONFIG(context_object->config_id);
	if (config_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONFIG;

	surface_object = SURFACE(context_object->render_surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	request_fd = surface_object->request_fd;
	if (request_fd < 0) {
		request_fd = media_request_alloc(driver_data->media_fd);
		if (request_fd < 0)
			return VA_STATUS_ERROR_OPERATION_FAILED;

		surface_object->request_fd = request_fd;
	}

	switch (config_object->profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			surface_object->mpeg2_header.slice_pos = 0;
			surface_object->mpeg2_header.slice_len = surface_object->slices_size * 8;

			control_id = V4L2_CID_MPEG_VIDEO_MPEG2_SLICE_HEADER;
			control_data = &surface_object->mpeg2_header;
			control_size = sizeof(surface_object->mpeg2_header);
			break;

		default:
			return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
	}

	rc = v4l2_set_control(driver_data->video_fd, request_fd, control_id, control_data, control_size);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_queue_buffer(driver_data->video_fd, -1, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surface_object->destination_index, 0);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_queue_buffer(driver_data->video_fd, request_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, surface_object->source_index, surface_object->slices_size);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	surface_object->slices_size = 0;

	status = SunxiCedrusSyncSurface(context, context_object->render_surface_id);
	if (status != VA_STATUS_SUCCESS)
		return status;

	context_object->render_surface_id = VA_INVALID_ID;

	return VA_STATUS_SUCCESS;
}
