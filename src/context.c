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

#include "sunxi_cedrus_drv_video.h"
#include "context.h"
#include "va_config.h"
#include "surface.h"

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

/*
 * A Context is a global data structure used for rendering a video of a certain
 * format. When a context is created, input buffers are created and v4l's output
 * (which is the compressed data input queue, since capture is the real output)
 * format is set.
 */

VAStatus sunxi_cedrus_CreateContext(VADriverContextP ctx, VAConfigID config_id,
		int picture_width, int picture_height, int flag,
		VASurfaceID *render_targets, int num_render_targets,
		VAContextID *context)
{
	INIT_DRIVER_DATA
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	object_config_p obj_config;
	int i;
	struct v4l2_create_buffers create_bufs;
	struct v4l2_format fmt;
	enum v4l2_buf_type type;

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		return vaStatus;
	}

	int contextID = object_heap_allocate(&driver_data->context_heap);
	object_context_p obj_context = CONTEXT(contextID);
	if (NULL == obj_context)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		return vaStatus;
	}

	obj_context->context_id  = contextID;
	*context = contextID;
	obj_context->current_render_target = -1;
	obj_context->config_id = config_id;
	obj_context->picture_width = picture_width;
	obj_context->picture_height = picture_height;
	obj_context->num_render_targets = num_render_targets;
	obj_context->render_targets = (VASurfaceID *) malloc(num_render_targets * sizeof(VASurfaceID));
	obj_context->num_rendered_surfaces = 0;

	if (obj_context->render_targets == NULL)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		return vaStatus;
	}

	for(i = 0; i < num_render_targets; i++)
	{
		if (NULL == SURFACE(render_targets[i]))
		{
			vaStatus = VA_STATUS_ERROR_INVALID_SURFACE;
			break;
		}
		obj_context->render_targets[i] = render_targets[i];
	}
	obj_context->flags = flag;

	/* Error recovery */
	if (VA_STATUS_SUCCESS != vaStatus)
	{
		obj_context->context_id = -1;
		obj_context->config_id = -1;
		free(obj_context->render_targets);
		obj_context->render_targets = NULL;
		obj_context->num_render_targets = 0;
		obj_context->flags = 0;
		object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);
	}

	memset(&(fmt), 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	fmt.fmt.pix_mp.width = picture_width;
	fmt.fmt.pix_mp.height = picture_height;
	fmt.fmt.pix_mp.plane_fmt[0].sizeimage = INPUT_BUFFER_MAX_SIZE;
	switch(obj_config->profile) {
		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
			return vaStatus;
			break;
	}
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	fmt.fmt.pix_mp.num_planes = 1;
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_S_FMT, &fmt)==0);

	memset (&create_bufs, 0, sizeof (struct v4l2_create_buffers));
	create_bufs.count = INPUT_BUFFERS_NB;
	create_bufs.memory = V4L2_MEMORY_MMAP;
	create_bufs.format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_G_FMT, &create_bufs.format)==0);
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_CREATE_BUFS, &create_bufs)==0);

	type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_STREAMON, &type)==0);

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_STREAMON, &type)==0);

	return vaStatus;
}

VAStatus sunxi_cedrus_DestroyContext(VADriverContextP ctx, VAContextID context)
{
	INIT_DRIVER_DATA
	object_context_p obj_context = CONTEXT(context);
	assert(obj_context);

	obj_context->context_id = -1;
	obj_context->config_id = -1;
	obj_context->picture_width = 0;
	obj_context->picture_height = 0;
	if (obj_context->render_targets)
		free(obj_context->render_targets);
	obj_context->render_targets = NULL;
	obj_context->num_render_targets = 0;
	obj_context->flags = 0;

	obj_context->current_render_target = -1;

	object_heap_free(&driver_data->context_heap, (object_base_p) obj_context);

	return VA_STATUS_SUCCESS;
}

