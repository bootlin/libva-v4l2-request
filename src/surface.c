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
#include <errno.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <X11/Xlib.h>

#include "v4l2.h"
#include "media.h"
#include "utils.h"

VAStatus SunxiCedrusCreateSurfaces(VADriverContextP context, int width,
	int height, int format, int surfaces_count, VASurfaceID *surfaces_ids)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	unsigned int length[2];
	unsigned int offset[2];
	void *destination_data[2];
	VASurfaceID id;
	unsigned int i, j;
	int rc;

	if (format != VA_RT_FORMAT_YUV420)
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

	rc = v4l2_set_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_PIX_FMT_MB32_NV12, width, height);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_create_buffers(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surfaces_count);
	if (rc < 0)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	for (i = 0; i < surfaces_count; i++) {
		id = object_heap_allocate(&driver_data->surface_heap);
		surface_object = SURFACE(id);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		rc = v4l2_request_buffer(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, i, length, offset);
		if (rc < 0)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		destination_data[0] = mmap(NULL, length[0], PROT_READ | PROT_WRITE, MAP_SHARED, driver_data->video_fd, offset[0]);
		if (destination_data[0] == MAP_FAILED)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		destination_data[1] = mmap(NULL, length[1], PROT_READ | PROT_WRITE, MAP_SHARED, driver_data->video_fd, offset[1]);
		if (destination_data[1] == MAP_FAILED)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		surface_object->status = VASurfaceReady;
		surface_object->width = width;
		surface_object->height = height;
		surface_object->source_index = 0;
		surface_object->source_data = NULL;
		surface_object->source_size = 0;
		surface_object->destination_index = i;

		for (j = 0; j < 2; j++) {
			surface_object->destination_data[j] = destination_data[j];
			surface_object->destination_size[j] = length[j];
		}

		memset(&surface_object->mpeg2_header, 0, sizeof(surface_object->mpeg2_header));
		surface_object->slices_size = 0;
		surface_object->request_fd = -1;

		surfaces_ids[i] = id;
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroySurfaces(VADriverContextP context,
	VASurfaceID *surfaces_ids, int surfaces_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	unsigned int i, j;

	for (i = 0; i < surfaces_count; i++) {
		surface_object = SURFACE(surfaces_ids[i]);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_INVALID_SURFACE;

		if (surface_object->source_data != NULL && surface_object->source_size > 0)
			munmap(surface_object->source_data, surface_object->source_size);

		for (j = 0; j < 2; j++)
			if (surface_object->destination_data[j] != NULL && surface_object->destination_size[j] > 0)
				munmap(surface_object->destination_data[j], surface_object->destination_size[j]);

		object_heap_free(&driver_data->surface_heap, (struct object_base *) surface_object);
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusSyncSurface(VADriverContextP context,
	VASurfaceID surface_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	int request_fd;
	int rc;

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	if (surface_object->status == VASurfaceSkipped)
		return VA_STATUS_ERROR_UNKNOWN;

	request_fd = surface_object->request_fd;
	if (request_fd < 0)
		return VA_STATUS_ERROR_UNKNOWN;

	rc = media_request_queue(request_fd);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = media_request_wait_completion(request_fd);
	if (rc < 0) {
		media_request_reinit(request_fd);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	rc = media_request_reinit(request_fd);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_dequeue_buffer(driver_data->video_fd, request_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, surface_object->source_index);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = v4l2_dequeue_buffer(driver_data->video_fd, request_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, surface_object->destination_index);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	surface_object->status = VASurfaceReady;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusQuerySurfaceStatus(VADriverContextP context,
	VASurfaceID surface_id, VASurfaceStatus *status)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;

	surface_object = SURFACE(surface_id);
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
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	GC gc;
	Display *display;
	const XID xid = (XID)(uintptr_t)draw;
	XColor xcolor;
	int screen;
	Colormap cm;
	int colorratio = 65535 / 255;
	int x, y;
	struct object_surface *surface_object;

	/* WARNING: This is for development purpose only!!! */

	surface_object = SURFACE(surface_id);

	display = XOpenDisplay(getenv("DISPLAY"));
	if (display == NULL) {
		sunxi_cedrus_log("Cannot connect to X server\n");
		exit(1);
	}

	sunxi_cedrus_log("warning: using vaPutSurface with sunxi-cedrus is not recommended\n");
	screen = DefaultScreen(display);
	gc =  XCreateGC(display, RootWindow(display, screen), 0, NULL);
	XSync(display, False);

	cm = DefaultColormap(display, screen);
	xcolor.flags = DoRed | DoGreen | DoBlue;

	for(x=dst_x; x < dst_x+dst_width; x++) {
		for(y=dst_y; y < dst_y+dst_height; y++) {
			char lum = ((char *) surface_object->destination_data[0])[x+src_width*y];
			xcolor.red = xcolor.green = xcolor.blue = lum*colorratio;
			XAllocColor(display, cm, &xcolor);
			XSetForeground(display, gc, xcolor.pixel);
			XDrawPoint(display, xid, gc, x, y);
		}
	}

	XFlush(display);
	XCloseDisplay(display);
	return VA_STATUS_SUCCESS;
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
