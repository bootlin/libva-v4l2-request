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

#include "buffer.h"
#include "context.h"
#include "image.h"
#include "picture.h"
#include "subpicture.h"
#include "surface.h"
#include "config.h"

#include "autoconfig.h"

#include <va/va_backend.h>

#include "sunxi_cedrus.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

void sunxi_cedrus_msg(const char *msg, ...)
{
	va_list args;

	/* We need to use stderr if we want to be heard */
	fprintf(stderr, "sunxi_cedrus_drv_video: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

/* Set default visibility for the init function only. */
VAStatus __attribute__((visibility("default")))
	VA_DRIVER_INIT_FUNC(VADriverContextP context);

VAStatus VA_DRIVER_INIT_FUNC(VADriverContextP context)
{
	struct VADriverVTable * const vtable = context->vtable;
	struct sunxi_cedrus_driver_data *driver_data;
	struct v4l2_capability cap;
	char *path;

	context->version_major = VA_MAJOR_VERSION;
	context->version_minor = VA_MINOR_VERSION;
	context->max_profiles = SUNXI_CEDRUS_MAX_PROFILES;
	context->max_entrypoints = SUNXI_CEDRUS_MAX_ENTRYPOINTS;
	context->max_attributes = SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES;
	context->max_image_formats = SUNXI_CEDRUS_MAX_IMAGE_FORMATS;
	context->max_subpic_formats = SUNXI_CEDRUS_MAX_SUBPIC_FORMATS;
	context->max_display_attributes = SUNXI_CEDRUS_MAX_DISPLAY_ATTRIBUTES;
	context->str_vendor = SUNXI_CEDRUS_STR_VENDOR;

	vtable->vaTerminate = SunxiCedrusTerminate;
	vtable->vaQueryConfigEntrypoints = SunxiCedrusQueryConfigEntrypoints;
	vtable->vaQueryConfigProfiles = SunxiCedrusQueryConfigProfiles;
	vtable->vaQueryConfigEntrypoints = SunxiCedrusQueryConfigEntrypoints;
	vtable->vaQueryConfigAttributes = SunxiCedrusQueryConfigAttributes;
	vtable->vaCreateConfig = SunxiCedrusCreateConfig;
	vtable->vaDestroyConfig = SunxiCedrusDestroyConfig;
	vtable->vaGetConfigAttributes = SunxiCedrusGetConfigAttributes;
	vtable->vaCreateSurfaces = SunxiCedrusCreateSurfaces;
	vtable->vaDestroySurfaces = SunxiCedrusDestroySurfaces;
	vtable->vaCreateContext = SunxiCedrusCreateContext;
	vtable->vaDestroyContext = SunxiCedrusDestroyContext;
	vtable->vaCreateBuffer = SunxiCedrusCreateBuffer;
	vtable->vaBufferSetNumElements = SunxiCedrusBufferSetNumElements;
	vtable->vaMapBuffer = SunxiCedrusMapBuffer;
	vtable->vaUnmapBuffer = SunxiCedrusUnmapBuffer;
	vtable->vaDestroyBuffer = SunxiCedrusDestroyBuffer;
	vtable->vaBeginPicture = SunxiCedrusBeginPicture;
	vtable->vaRenderPicture = SunxiCedrusRenderPicture;
	vtable->vaEndPicture = SunxiCedrusEndPicture;
	vtable->vaSyncSurface = SunxiCedrusSyncSurface;
	vtable->vaQuerySurfaceStatus = SunxiCedrusQuerySurfaceStatus;
	vtable->vaPutSurface = SunxiCedrusPutSurface;
	vtable->vaQueryImageFormats = SunxiCedrusQueryImageFormats;
	vtable->vaCreateImage = SunxiCedrusCreateImage;
	vtable->vaDeriveImage = SunxiCedrusDeriveImage;
	vtable->vaDestroyImage = SunxiCedrusDestroyImage;
	vtable->vaSetImagePalette = SunxiCedrusSetImagePalette;
	vtable->vaGetImage = SunxiCedrusGetImage;
	vtable->vaPutImage = SunxiCedrusPutImage;
	vtable->vaQuerySubpictureFormats = SunxiCedrusQuerySubpictureFormats;
	vtable->vaCreateSubpicture = SunxiCedrusCreateSubpicture;
	vtable->vaDestroySubpicture = SunxiCedrusDestroySubpicture;
	vtable->vaSetSubpictureImage = SunxiCedrusSetSubpictureImage;
	vtable->vaSetSubpictureChromakey = SunxiCedrusSetSubpictureChromakey;
	vtable->vaSetSubpictureGlobalAlpha = SunxiCedrusSetSubpictureGlobalAlpha;
	vtable->vaAssociateSubpicture = SunxiCedrusAssociateSubpicture;
	vtable->vaDeassociateSubpicture = SunxiCedrusDeassociateSubpicture;
	vtable->vaQueryDisplayAttributes = SunxiCedrusQueryDisplayAttributes;
	vtable->vaGetDisplayAttributes = SunxiCedrusGetDisplayAttributes;
	vtable->vaSetDisplayAttributes = SunxiCedrusSetDisplayAttributes;
	vtable->vaLockSurface = SunxiCedrusLockSurface;
	vtable->vaUnlockSurface = SunxiCedrusUnlockSurface;
	vtable->vaBufferInfo = SunxiCedrusBufferInfo;

	driver_data =
		(struct sunxi_cedrus_driver_data *) malloc(sizeof(*driver_data));
	context->pDriverData = (void *) driver_data;

	assert(object_heap_init(&driver_data->config_heap,
			sizeof(struct object_config), CONFIG_ID_OFFSET)==0);
	assert(object_heap_init(&driver_data->context_heap,
			sizeof(struct object_context), CONTEXT_ID_OFFSET)==0);
	assert(object_heap_init(&driver_data->surface_heap,
			sizeof(struct object_surface), SURFACE_ID_OFFSET)==0);
	assert(object_heap_init(&driver_data->buffer_heap,
			sizeof(struct object_buffer), BUFFER_ID_OFFSET)==0);
	assert(object_heap_init(&driver_data->image_heap,
			sizeof(struct object_image), IMAGE_ID_OFFSET)==0);

	path = getenv("LIBVA_CEDRUS_DEV");
	if (!path)
		path = "/dev/video0";

	driver_data->mem2mem_fd = open(path, O_RDWR | O_NONBLOCK, 0);
	assert(driver_data->mem2mem_fd >= 0);

	for (int i = 0; i < INPUT_BUFFERS_NB; i++) {
		driver_data->request_fds[i] = -1;
		driver_data->slice_offset[i] = 0;
	}

	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYCAP, &cap)==0);
	if (!(cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE))
	{
		sunxi_cedrus_msg("%s does not support m2m_mplane\n", path);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusTerminate(VADriverContextP context)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *obj_buffer;
	struct object_config *obj_config;
	object_heap_iterator iter;

	/* Free memory and close v4l device */

	for (int i = 0; i < INPUT_BUFFERS_NB; i++)
		if (driver_data->request_fds[i] >= 0)
			close(driver_data->request_fds[i]);

	close(driver_data->mem2mem_fd);

	/* Clean up left over buffers */
	obj_buffer = (struct object_buffer *) object_heap_first(&driver_data->buffer_heap, &iter);
	while (obj_buffer)
	{
		sunxi_cedrus_msg("vaTerminate: bufferID %08x still allocated, destroying\n", obj_buffer->base.id);
		sunxi_cedrus_destroy_buffer(driver_data, obj_buffer);
		obj_buffer = (struct object_buffer *) object_heap_next(&driver_data->buffer_heap, &iter);
	}

	object_heap_destroy(&driver_data->buffer_heap);
	object_heap_destroy(&driver_data->surface_heap);
	object_heap_destroy(&driver_data->context_heap);

	/* Clean up configIDs */
	obj_config = (struct object_config *) object_heap_first(&driver_data->config_heap, &iter);
	while (obj_config)
	{
		object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
		obj_config = (struct object_config *) object_heap_next(&driver_data->config_heap, &iter);
	}
	object_heap_destroy(&driver_data->config_heap);

	free(context->pDriverData);
	context->pDriverData = NULL;

	return VA_STATUS_SUCCESS;
}
