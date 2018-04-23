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
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	int i;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[2];
	struct v4l2_create_buffers create_bufs;
	struct v4l2_format fmt;

	memset(planes, 0, 2 * sizeof(struct v4l2_plane));

	/* We only support one format */
	if (VA_RT_FORMAT_YUV420 != format)
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

	/* Set format for capture */
	memset(&(fmt), 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SUNXI;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	fmt.fmt.pix_mp.num_planes = 2;
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_S_FMT, &fmt)==0);

	memset (&create_bufs, 0, sizeof (struct v4l2_create_buffers));
	create_bufs.count = surfaces_count;
	create_bufs.memory = V4L2_MEMORY_MMAP;
	create_bufs.format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_G_FMT, &create_bufs.format)==0);
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_CREATE_BUFS, &create_bufs)==0);
	driver_data->num_dst_bufs = create_bufs.count;

	for (i = 0; i < create_bufs.count; i++)
	{
		VASurfaceID surfaceID = object_heap_allocate(&driver_data->surface_heap);
		struct object_surface *obj_surface = SURFACE(surfaceID);
		if (NULL == obj_surface)
		{
			vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
			break;
		}
		obj_surface->surface_id = surfaceID;
		surfaces[i] = surfaceID;

		memset(&(buf), 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = create_bufs.index + i; // FIXME that's just i isn't it?
		buf.length = 2;
		buf.m.planes = planes;

		assert(ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYBUF, &buf)==0);

		driver_data->luma_bufs[buf.index] = mmap(NULL, buf.m.planes[0].length,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				driver_data->mem2mem_fd, buf.m.planes[0].m.mem_offset);
		assert(driver_data->luma_bufs[buf.index] != MAP_FAILED);

		driver_data->chroma_bufs[buf.index] = mmap(NULL, buf.m.planes[1].length,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				driver_data->mem2mem_fd, buf.m.planes[1].m.mem_offset);
		assert(driver_data->chroma_bufs[buf.index] != MAP_FAILED);

		obj_surface->input_buf_index = 0;
		obj_surface->output_buf_index = create_bufs.index + i; // FIXME that's just i isn't it?

		obj_surface->width = width;
		obj_surface->height = height;
		obj_surface->status = VASurfaceReady;
	}

	/* Error recovery */
	if (VA_STATUS_SUCCESS != vaStatus)
	{
		/* surfaces[i-1] was the last successful allocation */
		for(; i--;)
		{
			struct object_surface *obj_surface = SURFACE(surfaces[i]);
			surfaces[i] = VA_INVALID_SURFACE;
			assert(obj_surface);
			object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
		}
	}
	return vaStatus;
}

VAStatus SunxiCedrusDestroySurfaces(VADriverContextP context,
	VASurfaceID *surfaces_ids, int surfaces_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	int i;
	for(i = surfaces_count; i--;)
	{
		struct object_surface *obj_surface = SURFACE(surfaces_ids[i]);
		assert(obj_surface);
		object_heap_free(&driver_data->surface_heap, (object_base_p) obj_surface);
	}
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusSyncSurface(VADriverContextP context,
	VASurfaceID surface_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *obj_surface;
	struct v4l2_buffer buf;
	struct v4l2_plane plane[1];
	struct v4l2_plane planes[2];
        fd_set read_fds;
	int request_fd;
	int rc;

	memset(plane, 0, sizeof(struct v4l2_plane));
	memset(planes, 0, 2 * sizeof(struct v4l2_plane));

	obj_surface = SURFACE(surface_id);
	assert(obj_surface);

	if(obj_surface->status == VASurfaceSkipped)
		return VA_STATUS_ERROR_UNKNOWN;

	request_fd = driver_data->request_fds[obj_surface->input_buf_index];
	if(request_fd < 0)
		return  VA_STATUS_ERROR_UNKNOWN;

	assert(ioctl(request_fd, MEDIA_REQUEST_IOC_SUBMIT, NULL)==0);

	FD_ZERO(&read_fds);
	FD_SET(request_fd, &read_fds);

	rc = select(request_fd + 1, &read_fds, NULL, NULL, NULL);
	if(rc <= 0)
		// FIXME: Properly dispose of the buffers here, also reinit request when it fails, also set surface status
		return VA_STATUS_ERROR_UNKNOWN;

	assert(ioctl(request_fd, MEDIA_REQUEST_IOC_REINIT, NULL)==0);

	memset(&(buf), 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = obj_surface->input_buf_index;
	buf.length = 1;
	buf.m.planes = plane;

	if(ioctl(driver_data->mem2mem_fd, VIDIOC_DQBUF, &buf)) {
		sunxi_cedrus_msg("Error when dequeuing input: %s\n", strerror(errno));
		return VA_STATUS_ERROR_UNKNOWN;
	}

	memset(&(buf), 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = obj_surface->output_buf_index;
	buf.length = 2;
	buf.m.planes = planes;


	if(ioctl(driver_data->mem2mem_fd, VIDIOC_DQBUF, &buf)) {
		sunxi_cedrus_msg("Error when dequeuing output: %s\n", strerror(errno));
		return VA_STATUS_ERROR_UNKNOWN;
	}

	obj_surface->status = VASurfaceReady;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusQuerySurfaceStatus(VADriverContextP context,
	VASurfaceID surface_id, VASurfaceStatus *status)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	struct object_surface *obj_surface;

	obj_surface = SURFACE(surface_id);
	assert(obj_surface);

	*status = obj_surface->status;

	return vaStatus;
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
	struct object_surface *obj_surface;

	/* WARNING: This is for development purpose only!!! */

	obj_surface = SURFACE(surface);
	assert(obj_surface);

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
			char lum = driver_data->luma_bufs[obj_surface->output_buf_index][x+srcw*y];
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
{ return VA_STATUS_ERROR_UNIMPLEMENTED; }

VAStatus SunxiCedrusUnlockSurface(VADriverContextP context,
	VASurfaceID surface_id)
{ return VA_STATUS_ERROR_UNIMPLEMENTED; }
