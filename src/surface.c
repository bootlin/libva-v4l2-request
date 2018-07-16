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
#include "surface.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "v4l2.h"
#include "video.h"
#include "media.h"
#include "utils.h"

VAStatus SunxiCedrusCreateSurfaces(VADriverContextP context, int width,
	int height, int format, int surfaces_count, VASurfaceID *surfaces_ids)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *) context->pDriverData;
	struct object_surface *surface_object;
	struct video_format *video_format;
	unsigned int destination_sizes[VIDEO_MAX_PLANES];
	unsigned int destination_bytesperlines[VIDEO_MAX_PLANES];
	unsigned int destination_planes_count;
	unsigned int i, j;
	VASurfaceID id;
	bool found;
	int rc;

	if (format != VA_RT_FORMAT_YUV420)
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

	driver_data->tiled_format = true;

	found = v4l2_find_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_PIX_FMT_NV12);
	if (found)
		driver_data->tiled_format = false;

	video_format = video_format_find(driver_data->tiled_format);
	if (video_format == NULL)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_set_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, video_format->v4l2_format, width, height);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_get_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, NULL, NULL, destination_bytesperlines, destination_sizes, &destination_planes_count);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_create_buffers(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surfaces_count);
	if (rc < 0)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	for (i = 0; i < surfaces_count; i++) {
		id = object_heap_allocate(&driver_data->surface_heap);
		surface_object = SURFACE(driver_data, id);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		rc = v4l2_request_buffer(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, i, surface_object->destination_map_lengths, surface_object->destination_map_offsets, video_format->v4l2_buffers_count);
		if (rc < 0)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		for (j = 0; j < video_format->v4l2_buffers_count; j++) {
			surface_object->destination_map[j] = mmap(NULL, surface_object->destination_map_lengths[j], PROT_READ | PROT_WRITE, MAP_SHARED, driver_data->video_fd, surface_object->destination_map_offsets[j]);
			if (surface_object->destination_map[j] == MAP_FAILED)
				return VA_STATUS_ERROR_ALLOCATION_FAILED;
		}

		if (video_format->v4l2_buffers_count == 1) {
			for (j = 0; j < destination_planes_count; j++) {
				surface_object->destination_offsets[j] = j > 0 ? destination_sizes[j-1] : 0;
				surface_object->destination_data[j] = (void *) ((unsigned char *) surface_object->destination_map[0] + surface_object->destination_offsets[j]);
				surface_object->destination_sizes[j] = destination_sizes[j];
				surface_object->destination_bytesperlines[j] = destination_bytesperlines[j];
			}
		} else if (video_format->v4l2_buffers_count == destination_planes_count) {
			for (j = 0; j < destination_planes_count; j++) {
				surface_object->destination_offsets[j] = 0;
				surface_object->destination_data[j] = surface_object->destination_map[j];
				surface_object->destination_sizes[j] = destination_sizes[j];
				surface_object->destination_bytesperlines[j] = destination_bytesperlines[j];
			}
		} else {
			return VA_STATUS_ERROR_ALLOCATION_FAILED;
		}

		surface_object->status = VASurfaceReady;
		surface_object->width = width;
		surface_object->height = height;

		surface_object->source_index = 0;
		surface_object->source_data = NULL;
		surface_object->source_size = 0;

		surface_object->destination_index = 0;

		surface_object->destination_planes_count = destination_planes_count;
		surface_object->destination_buffers_count = video_format->v4l2_buffers_count;

		memset(&surface_object->params, 0, sizeof(surface_object->params));
		surface_object->slices_size = 0;

		surface_object->request_fd = -1;

		surfaces_ids[i] = id;
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroySurfaces(VADriverContextP context,
	VASurfaceID *surfaces_ids, int surfaces_count)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *) context->pDriverData;
	struct object_surface *surface_object;
	unsigned int i, j;

	for (i = 0; i < surfaces_count; i++) {
		surface_object = SURFACE(driver_data, surfaces_ids[i]);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_INVALID_SURFACE;

		if (surface_object->source_data != NULL && surface_object->source_size > 0)
			munmap(surface_object->source_data, surface_object->source_size);

		if (surface_object->request_fd >= 0)
			close(surface_object->request_fd);

		for (j = 0; j < surface_object->destination_buffers_count; j++)
			if (surface_object->destination_map[j] != NULL && surface_object->destination_map_lengths[j] > 0)
				munmap(surface_object->destination_map[j], surface_object->destination_map_lengths[j]);

		if (surface_object->request_fd > 0)
			close(surface_object->request_fd);

		object_heap_free(&driver_data->surface_heap, (struct object_base *) surface_object);
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusSyncSurface(VADriverContextP context,
	VASurfaceID surface_id)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *) context->pDriverData;
	struct object_surface *surface_object;
	VAStatus status;
	int request_fd = -1;
	int rc;

	surface_object = SURFACE(driver_data, surface_id);
	if (surface_object == NULL) {
		status = VA_STATUS_ERROR_INVALID_SURFACE;
		goto error;
	}

	if (surface_object->status != VASurfaceRendering) {
		status = VA_STATUS_SUCCESS;
		goto complete;
	}

	request_fd = surface_object->request_fd;
	if (request_fd < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_queue(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_wait_completion(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_reinit(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_dequeue_buffer(driver_data->video_fd, -1, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, surface_object->source_index, 1);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_dequeue_buffer(driver_data->video_fd, -1, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surface_object->destination_index, surface_object->destination_buffers_count);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	surface_object->status = VASurfaceDisplaying;

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	if (request_fd >= 0) {
		close(request_fd);
		surface_object->request_fd = -1;
	}

complete:
	return status;
}

VAStatus SunxiCedrusQuerySurfaceStatus(VADriverContextP context,
	VASurfaceID surface_id, VASurfaceStatus *status)
{
	struct cedrus_data *driver_data =
		(struct cedrus_data *) context->pDriverData;
	struct object_surface *surface_object;

	surface_object = SURFACE(driver_data, surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	*status = surface_object->status;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusPutSurface(VADriverContextP context, VASurfaceID surface_id,
	void *draw, short src_x, short src_y, unsigned short src_width,
	unsigned short src_height, short dst_x, short dst_y,
	unsigned short dst_width, unsigned short dst_height,
	VARectangle *cliprects, unsigned int cliprects_count,
	unsigned int flags)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusLockSurface(VADriverContextP context,
	VASurfaceID surface_id, unsigned int *fourcc, unsigned int *luma_stride,
	unsigned int *chroma_u_stride, unsigned int *chroma_v_stride,
	unsigned int *luma_offset, unsigned int *chroma_u_offset,
	unsigned int *chroma_v_offset, unsigned int *buffer_name, void **buffer)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusUnlockSurface(VADriverContextP context,
	VASurfaceID surface_id)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
