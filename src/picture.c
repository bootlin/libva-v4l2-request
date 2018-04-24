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

VAStatus SunxiCedrusBeginPicture(VADriverContextP context,
	VAContextID context_id, VASurfaceID surface_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	struct object_surface *surface_object;
	VAStatus status;

	context_object = CONTEXT(context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	if (surface_object->status == VASurfaceRendering)
		SunxiCedrusSyncSurface(context, surface_id);

	surface_object->status = VASurfaceRendering;
	surface_object->input_buf_index = context_object->num_rendered_surfaces % INPUT_BUFFERS_NB;
	context_object->render_surface_id = surface_id;
	context_object->num_rendered_surfaces++;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusRenderPicture(VADriverContextP context,
	VAContextID context_id, VABufferID *buffers, int buffers_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_context *context_object;
	struct object_config *config_object;
	struct object_surface *surface_object;
	VAStatus status;
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
		buffer_object = BUFFER(buffers[i]);
		if (buffer_object == NULL)
			return VA_STATUS_ERROR_INVALID_BUFFER;

		switch (config_object->profile) {
			case VAProfileMPEG2Simple:
			case VAProfileMPEG2Main:
				if (buffer_object->type == VASliceDataBufferType) {
					status = sunxi_cedrus_render_mpeg2_slice_data(context, context_object, surface_object, buffer_object);
					if (status != VA_STATUS_SUCCESS)
						return status;
				} else if (buffer_object->type == VAPictureParameterBufferType) {
					status = sunxi_cedrus_render_mpeg2_picture_parameter(context, context_object, surface_object, buffer_object);
					if (status != VA_STATUS_SUCCESS)
						return status;
				} else {
					sunxi_cedrus_msg("Unsupported buffer type: %d\n", buffer_object->type);
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
	struct object_surface *surface_object;
	struct v4l2_buffer out_buf, cap_buf;
	struct v4l2_plane plane[1];
	struct v4l2_plane planes[2];
	struct v4l2_ext_control ctrl;
	struct v4l2_ext_controls ctrls;
	struct media_request_new media_request;
	int request_fd;
	int rc;

	context_object = CONTEXT(context_id);
	if (context_object == NULL)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	surface_object = SURFACE(context_object->render_surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	request_fd = driver_data->request_fds[surface_object->input_buf_index];
	if(request_fd < 0) {
		assert(ioctl(driver_data->mem2mem_fd, VIDIOC_NEW_REQUEST, &media_request)==0);
		driver_data->request_fds[surface_object->input_buf_index] = media_request.fd;
		request_fd = media_request.fd;
	}

	memset(plane, 0, sizeof(struct v4l2_plane));
	memset(planes, 0, 2 * sizeof(struct v4l2_plane));
	memset(&ctrl, 0, sizeof(struct v4l2_ext_control));
	memset(&ctrls, 0, sizeof(struct v4l2_ext_controls));

	memset(&(out_buf), 0, sizeof(out_buf));
	out_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	out_buf.memory = V4L2_MEMORY_MMAP;
	out_buf.index = surface_object->input_buf_index;
	out_buf.length = 1;
	out_buf.m.planes = plane;
	out_buf.request_fd = request_fd;

	switch (config_object->profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			context_object->mpeg2_frame_hdr.slice_pos = 0;
			context_object->mpeg2_frame_hdr.slice_len = driver_data->slice_offset[surface_object->input_buf_index] * 8;

			out_buf.m.planes[0].bytesused = driver_data->slice_offset[surface_object->input_buf_index];
			ctrl.id = V4L2_CID_MPEG_VIDEO_MPEG2_FRAME_HDR;
			ctrl.ptr = &context_object->mpeg2_frame_hdr;
			ctrl.size = sizeof(context_object->mpeg2_frame_hdr);
			break;

		default:
			out_buf.m.planes[0].bytesused = 0;
			ctrl.id = V4L2_CID_MPEG_VIDEO_MPEG2_FRAME_HDR;
			ctrl.ptr = NULL;
			ctrl.size = 0;
			break;
	}

	driver_data->slice_offset[surface_object->input_buf_index] = 0;

	memset(&cap_buf, 0, sizeof(cap_buf));
	cap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	cap_buf.memory = V4L2_MEMORY_MMAP;
	cap_buf.index = surface_object->output_buf_index;
	cap_buf.length = 2;
	cap_buf.m.planes = planes;

	ctrls.controls = &ctrl;
	ctrls.count = 1;
	ctrls.request_fd = request_fd;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_S_EXT_CTRLS, &ctrls);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_QBUF, &cap_buf);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_QBUF, &out_buf);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	context_object->render_surface_id = VA_INVALID_ID;

	return VA_STATUS_SUCCESS;
}
