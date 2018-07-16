/*
 * Copyright (C) 2018 Paul Kocialkowski <paul.kocialkowski@bootlin.com>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "utils.h"
#include "v4l2.h"

bool v4l2_find_format(int video_fd, unsigned int type, unsigned int pixelformat)
{
	struct v4l2_fmtdesc fmtdesc;
	int rc;

	memset(&fmtdesc, 0, sizeof(fmtdesc));
	fmtdesc.type = type;
	fmtdesc.index = 0;

	do {
		rc = ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (rc < 0)
			break;

		if (fmtdesc.pixelformat == pixelformat)
			return true;

		fmtdesc.index++;
	} while (rc >= 0);

	return false;
}

int v4l2_set_format(int video_fd, unsigned int type, unsigned int pixelformat,
		    unsigned int width, unsigned int height)
{
	struct v4l2_format format;
	int rc;

	memset(&format, 0, sizeof(format));
	format.type = type;
	format.fmt.pix_mp.width = width;
	format.fmt.pix_mp.height = height;
	format.fmt.pix_mp.plane_fmt[0].sizeimage =
		type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ? SOURCE_SIZE_MAX : 0;
	format.fmt.pix_mp.pixelformat = pixelformat;

	rc = ioctl(video_fd, VIDIOC_S_FMT, &format);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to set format for type %d: %s\n", type,
				 strerror(errno));
		return -1;
	}

	return 0;
}

int v4l2_get_format(int video_fd, unsigned int type, unsigned int *width,
		    unsigned int *height, unsigned int *bytesperline,
		    unsigned int *sizes, unsigned int *planes_count)
{
	struct v4l2_format format;
	unsigned int count;
	unsigned int i;
	int rc;

	memset(&format, 0, sizeof(format));
	format.type = type;

	rc = ioctl(video_fd, VIDIOC_G_FMT, &format);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to get format for type %d: %s\n", type,
				 strerror(errno));
		return -1;
	}

	count = format.fmt.pix_mp.num_planes;

	if (width != NULL)
		*width = format.fmt.pix_mp.width;

	if (height != NULL)
		*height = format.fmt.pix_mp.height;

	if (planes_count != NULL)
		if (*planes_count > 0 && *planes_count < count)
			count = *planes_count;

	if (bytesperline != NULL)
		for (i = 0; i < count; i++)
			bytesperline[i] =
				format.fmt.pix_mp.plane_fmt[i].bytesperline;

	if (sizes != NULL)
		for (i = 0; i < count; i++)
			sizes[i] = format.fmt.pix_mp.plane_fmt[i].sizeimage;

	if (planes_count != NULL)
		*planes_count = count;

	return 0;
}

int v4l2_create_buffers(int video_fd, unsigned int type,
			unsigned int buffers_count)
{
	struct v4l2_create_buffers buffers;
	int rc;

	memset(&buffers, 0, sizeof(buffers));
	buffers.format.type = type;
	buffers.memory = V4L2_MEMORY_MMAP;
	buffers.count = buffers_count;

	rc = ioctl(video_fd, VIDIOC_G_FMT, &buffers.format);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to get format for type %d: %s\n", type,
				 strerror(errno));
		return -1;
	}

	rc = ioctl(video_fd, VIDIOC_CREATE_BUFS, &buffers);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to create buffer for type %d: %s\n",
				 type, strerror(errno));
		return -1;
	}

	return 0;
}

int v4l2_request_buffer(int video_fd, unsigned int type, unsigned int index,
			unsigned int *lengths, unsigned int *offsets,
			unsigned int buffers_count)
{
	struct v4l2_plane planes[buffers_count];
	struct v4l2_buffer buffer;
	unsigned int i;
	int rc;

	memset(planes, 0, sizeof(planes));
	memset(&buffer, 0, sizeof(buffer));

	buffer.type = type;
	buffer.memory = V4L2_MEMORY_MMAP;
	buffer.index = index;
	buffer.length = buffers_count;
	buffer.m.planes = planes;

	rc = ioctl(video_fd, VIDIOC_QUERYBUF, &buffer);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to query buffer: %s\n",
				 strerror(errno));
		return -1;
	}

	if (lengths != NULL)
		for (i = 0; i < buffer.length; i++)
			lengths[i] = buffer.m.planes[i].length;

	if (offsets != NULL)
		for (i = 0; i < buffer.length; i++)
			offsets[i] = buffer.m.planes[i].m.mem_offset;

	return 0;
}

int v4l2_queue_buffer(int video_fd, int request_fd, unsigned int type,
		      unsigned int index, unsigned int size,
		      unsigned int buffers_count)
{
	struct v4l2_plane planes[buffers_count];
	struct v4l2_buffer buffer;
	unsigned int i;
	int rc;

	memset(planes, 0, sizeof(planes));
	memset(&buffer, 0, sizeof(buffer));

	buffer.type = type;
	buffer.memory = V4L2_MEMORY_MMAP;
	buffer.index = index;
	buffer.length = buffers_count;
	buffer.m.planes = planes;

	for (i = 0; i < buffers_count; i++)
		buffer.m.planes[i].bytesused = size;

	if (request_fd >= 0) {
		buffer.flags = V4L2_BUF_FLAG_REQUEST_FD;
		buffer.request_fd = request_fd;
	}

	rc = ioctl(video_fd, VIDIOC_QBUF, &buffer);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to queue buffer: %s\n",
				 strerror(errno));
		return -1;
	}

	return 0;
}

int v4l2_dequeue_buffer(int video_fd, int request_fd, unsigned int type,
			unsigned int index, unsigned int buffers_count)
{
	struct v4l2_plane planes[buffers_count];
	struct v4l2_buffer buffer;
	int rc;

	memset(planes, 0, sizeof(planes));
	memset(&buffer, 0, sizeof(buffer));

	buffer.type = type;
	buffer.memory = V4L2_MEMORY_MMAP;
	buffer.index = index;
	buffer.length = buffers_count;
	buffer.m.planes = planes;

	if (request_fd >= 0) {
		buffer.flags = V4L2_BUF_FLAG_REQUEST_FD;
		buffer.request_fd = request_fd;
	}

	rc = ioctl(video_fd, VIDIOC_DQBUF, &buffer);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to dequeue buffer: %s\n",
				 strerror(errno));
		return -1;
	}

	return 0;
}

int v4l2_set_control(int video_fd, int request_fd, unsigned int id, void *data,
		     unsigned int size)
{
	struct v4l2_ext_control control;
	struct v4l2_ext_controls controls;
	int rc;

	memset(&control, 0, sizeof(control));
	memset(&controls, 0, sizeof(controls));

	control.id = id;
	control.ptr = data;
	control.size = size;

	controls.controls = &control;
	controls.count = 1;

	if (request_fd >= 0) {
		controls.which = V4L2_CTRL_WHICH_REQUEST_VAL;
		controls.request_fd = request_fd;
	}

	rc = ioctl(video_fd, VIDIOC_S_EXT_CTRLS, &controls);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to set control: %s\n",
				 strerror(errno));
		return -1;
	}

	return 0;
}

int v4l2_set_stream(int video_fd, unsigned int type, bool enable)
{
	enum v4l2_buf_type buf_type = type;
	int rc;

	rc = ioctl(video_fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF,
		   &buf_type);
	if (rc < 0) {
		sunxi_cedrus_log("Unable to %sable stream: %s\n",
				 enable ? "en" : "dis", strerror(errno));
		return -1;
	}

	return 0;
}
