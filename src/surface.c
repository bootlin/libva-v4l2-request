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

VAStatus SunxiCedrusCreateSurfaces(VADriverContextP context, int width,
	int height, int format, int surfaces_count, VASurfaceID *surfaces)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	VASurfaceID id;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[2];
	struct v4l2_create_buffers create_bufs;
	struct v4l2_format fmt;
	int i;

	memset(planes, 0, 2 * sizeof(struct v4l2_plane));

	if (format != VA_RT_FORMAT_YUV420)
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SUNXI;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	fmt.fmt.pix_mp.num_planes = 2;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_S_FMT, &fmt);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	memset(&create_bufs, 0, sizeof(create_bufs));
	create_bufs.count = surfaces_count;
	create_bufs.memory = V4L2_MEMORY_MMAP;
	create_bufs.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;


	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_G_FMT, &create_bufs.format);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_CREATE_BUFS, &create_bufs);
	if (rc < 0)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	for (i = 0; i < surfaces_count; i++) {
		id = object_heap_allocate(&driver_data->surface_heap);
		surface_object = (struct object_surface *) object_heap_lookup(&driver_data->surface_heap, id);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		surfaces[i] = surfaceID;

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = create_bufs.index + i;
		buf.length = 2;
		buf.m.planes = planes;

		rc = ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYBUF, &buf);
		if (rc < 0)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		driver_data->luma_bufs[buf.index] = mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED,
			driver_data->mem2mem_fd, buf.m.planes[0].m.mem_offset);
		if (driver_data->luma_bufs[buf.index] == MAP_FAILED)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		driver_data->chroma_bufs[buf.index] = mmap(NULL, buf.m.planes[1].length, PROT_READ | PROT_WRITE, MAP_SHARED,
			driver_data->mem2mem_fd, buf.m.planes[1].m.mem_offset);
		if (driver_data->chroma_bufs[buf.index] == MAP_FAILED)
			return VA_STATUS_ERROR_ALLOCATION_FAILED;

		surface_object->status = VASurfaceReady;
		surface_object->width = width;
		surface_object->height = height;
		surface_object->input_buf_index = 0;
		surface_object->output_buf_index = create_bufs.index + i;

		surfaces_ids[i] = id;
	}

	driver_data->num_dst_bufs = create_bufs.count;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroySurfaces(VADriverContextP context,
	VASurfaceID *surfaces_ids, int surfaces_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	int i;

	for (i = 0; i < surfaces_count; i++) {
		surface_object = (struct object_surface *) object_heap_lookup(&driver_data->surface_heap, surfaces_ids[i]);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_INVALID_SURFACE;

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
	struct v4l2_buffer buf;
	struct v4l2_plane plane[1];
	struct v4l2_plane planes[2];
        fd_set read_fds;
	int request_fd;
	int rc;

	memset(plane, 0, sizeof(struct v4l2_plane));
	memset(planes, 0, 2 * sizeof(struct v4l2_plane));

	surface_object = SURFACE(surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	if (surface_object->status == VASurfaceSkipped)
		return VA_STATUS_ERROR_UNKNOWN;

	request_fd = driver_data->request_fds[surface_object->input_buf_index];
	if (request_fd < 0)
		return VA_STATUS_ERROR_UNKNOWN;

	rc = ioctl(request_fd, MEDIA_REQUEST_IOC_SUBMIT, NULL);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	FD_ZERO(&read_fds);
	FD_SET(request_fd, &read_fds);

	rc = select(request_fd + 1, &read_fds, NULL, NULL, NULL);
	if(rc <= 0)
		return VA_STATUS_ERROR_UNKNOWN;

	rc = ioctl(request_fd, MEDIA_REQUEST_IOC_REINIT, NULL);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = surface_object->input_buf_index;
	buf.length = 1;
	buf.m.planes = plane;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_DQBUF, &buf);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = surface_object->output_buf_index;
	buf.length = 2;
	buf.m.planes = planes;

	rc = ioctl(driver_data->mem2mem_fd, VIDIOC_DQBUF, &buf);
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

	surface_object = (struct object_surface *) object_heap_lookup(&driver_data->surface_heap, surface_id);
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

	surface_object = SURFACE(surface);

	display = XOpenDisplay(getenv("DISPLAY"));
	if (display == NULL) {
		sunxi_cedrus_msg("Cannot connect to X server\n");
		exit(1);
	}

	sunxi_cedrus_msg("warning: using vaPutSurface with sunxi-cedrus is not recommended\n");
	screen = DefaultScreen(display);
	gc =  XCreateGC(display, RootWindow(display, screen), 0, NULL);
	XSync(display, False);

	cm = DefaultColormap(display, screen);
	xcolor.flags = DoRed | DoGreen | DoBlue;

	for(x=dst_x; x < dst_x+dst_w; x++) {
		for(y=dst_y; y < dst_y+dst_h; y++) {
			char lum = driver_data->luma_bufs[surface_object->output_buf_index][x+srcw*y];
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
