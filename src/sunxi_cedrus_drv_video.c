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
#include "va_config.h"

#include "config.h"

#include <va/va_backend.h>

#include "sunxi_cedrus_drv_video.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

/* We need to use stderr if we want to be heard */
void sunxi_cedrus_msg(const char *msg, ...)
{
	va_list args;

	fprintf(stderr, "sunxi_cedrus_drv_video: ");
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
}

/* Free memory and close v4l device */
VAStatus sunxi_cedrus_Terminate(VADriverContextP ctx)
{
	INIT_DRIVER_DATA
	object_buffer_p obj_buffer;
	object_config_p obj_config;
	object_heap_iterator iter;

	for (int i = 0; i < INPUT_BUFFERS_NB; i++)
		if (driver_data->request_fds[i] >= 0)
			close(driver_data->request_fds[i]);

	close(driver_data->mem2mem_fd);

	/* Clean up left over buffers */
	obj_buffer = (object_buffer_p) object_heap_first(&driver_data->buffer_heap, &iter);
	while (obj_buffer)
	{
		sunxi_cedrus_msg("vaTerminate: bufferID %08x still allocated, destroying\n", obj_buffer->base.id);
		sunxi_cedrus_destroy_buffer(driver_data, obj_buffer);
		obj_buffer = (object_buffer_p) object_heap_next(&driver_data->buffer_heap, &iter);
	}

	object_heap_destroy(&driver_data->buffer_heap);
	object_heap_destroy(&driver_data->surface_heap);
	object_heap_destroy(&driver_data->context_heap);

	/* Clean up configIDs */
	obj_config = (object_config_p) object_heap_first(&driver_data->config_heap, &iter);
	while (obj_config)
	{
		object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
		obj_config = (object_config_p) object_heap_next(&driver_data->config_heap, &iter);
	}
	object_heap_destroy(&driver_data->config_heap);

	free(ctx->pDriverData);
	ctx->pDriverData = NULL;

	return VA_STATUS_SUCCESS;
}

/* Only expose the init function */
VAStatus __attribute__((visibility("default")))
VA_DRIVER_INIT_FUNC(VADriverContextP ctx);

/* Setup a bunch of function pointers for VA */
VAStatus VA_DRIVER_INIT_FUNC(VADriverContextP ctx)
{
	struct VADriverVTable * const vtable = ctx->vtable;
	struct sunxi_cedrus_driver_data *driver_data;
	struct v4l2_capability cap;

	ctx->version_major = VA_MAJOR_VERSION;
	ctx->version_minor = VA_MINOR_VERSION;
	ctx->max_profiles = SUNXI_CEDRUS_MAX_PROFILES;
	ctx->max_entrypoints = SUNXI_CEDRUS_MAX_ENTRYPOINTS;
	ctx->max_attributes = SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES;
	ctx->max_image_formats = SUNXI_CEDRUS_MAX_IMAGE_FORMATS;
	ctx->max_subpic_formats = SUNXI_CEDRUS_MAX_SUBPIC_FORMATS;
	ctx->max_display_attributes = SUNXI_CEDRUS_MAX_DISPLAY_ATTRIBUTES;
	ctx->str_vendor = SUNXI_CEDRUS_STR_VENDOR;

	vtable->vaTerminate = sunxi_cedrus_Terminate;
	vtable->vaQueryConfigEntrypoints = sunxi_cedrus_QueryConfigEntrypoints;
	vtable->vaQueryConfigProfiles = sunxi_cedrus_QueryConfigProfiles;
	vtable->vaQueryConfigEntrypoints = sunxi_cedrus_QueryConfigEntrypoints;
	vtable->vaQueryConfigAttributes = sunxi_cedrus_QueryConfigAttributes;
	vtable->vaCreateConfig = sunxi_cedrus_CreateConfig;
	vtable->vaDestroyConfig = sunxi_cedrus_DestroyConfig;
	vtable->vaGetConfigAttributes = sunxi_cedrus_GetConfigAttributes;
	vtable->vaCreateSurfaces = sunxi_cedrus_CreateSurfaces;
	vtable->vaDestroySurfaces = sunxi_cedrus_DestroySurfaces;
	vtable->vaCreateContext = sunxi_cedrus_CreateContext;
	vtable->vaDestroyContext = sunxi_cedrus_DestroyContext;
	vtable->vaCreateBuffer = sunxi_cedrus_CreateBuffer;
	vtable->vaBufferSetNumElements = sunxi_cedrus_BufferSetNumElements;
	vtable->vaMapBuffer = sunxi_cedrus_MapBuffer;
	vtable->vaUnmapBuffer = sunxi_cedrus_UnmapBuffer;
	vtable->vaDestroyBuffer = sunxi_cedrus_DestroyBuffer;
	vtable->vaBeginPicture = sunxi_cedrus_BeginPicture;
	vtable->vaRenderPicture = sunxi_cedrus_RenderPicture;
	vtable->vaEndPicture = sunxi_cedrus_EndPicture;
	vtable->vaSyncSurface = sunxi_cedrus_SyncSurface;
	vtable->vaQuerySurfaceStatus = sunxi_cedrus_QuerySurfaceStatus;
	vtable->vaPutSurface = sunxi_cedrus_PutSurface;
	vtable->vaQueryImageFormats = sunxi_cedrus_QueryImageFormats;
	vtable->vaCreateImage = sunxi_cedrus_CreateImage;
	vtable->vaDeriveImage = sunxi_cedrus_DeriveImage;
	vtable->vaDestroyImage = sunxi_cedrus_DestroyImage;
	vtable->vaSetImagePalette = sunxi_cedrus_SetImagePalette;
	vtable->vaGetImage = sunxi_cedrus_GetImage;
	vtable->vaPutImage = sunxi_cedrus_PutImage;
	vtable->vaQuerySubpictureFormats = sunxi_cedrus_QuerySubpictureFormats;
	vtable->vaCreateSubpicture = sunxi_cedrus_CreateSubpicture;
	vtable->vaDestroySubpicture = sunxi_cedrus_DestroySubpicture;
	vtable->vaSetSubpictureImage = sunxi_cedrus_SetSubpictureImage;
	vtable->vaSetSubpictureChromakey = sunxi_cedrus_SetSubpictureChromakey;
	vtable->vaSetSubpictureGlobalAlpha = sunxi_cedrus_SetSubpictureGlobalAlpha;
	vtable->vaAssociateSubpicture = sunxi_cedrus_AssociateSubpicture;
	vtable->vaDeassociateSubpicture = sunxi_cedrus_DeassociateSubpicture;
	vtable->vaQueryDisplayAttributes = sunxi_cedrus_QueryDisplayAttributes;
	vtable->vaGetDisplayAttributes = sunxi_cedrus_GetDisplayAttributes;
	vtable->vaSetDisplayAttributes = sunxi_cedrus_SetDisplayAttributes;
	vtable->vaLockSurface = sunxi_cedrus_LockSurface;
	vtable->vaUnlockSurface = sunxi_cedrus_UnlockSurface;
	vtable->vaBufferInfo = sunxi_cedrus_BufferInfo;

	driver_data = (struct sunxi_cedrus_driver_data *) malloc(sizeof(*driver_data));
	ctx->pDriverData = (void *) driver_data;

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

	driver_data->mem2mem_fd = open("/dev/video0", O_RDWR | O_NONBLOCK, 0);
	assert(driver_data->mem2mem_fd >= 0);

	for (int i = 0; i < INPUT_BUFFERS_NB; i++)
		driver_data->request_fds[i] = -1;

	assert(ioctl(driver_data->mem2mem_fd, VIDIOC_QUERYCAP, &cap)==0);
	if (!(cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE))
	{
		sunxi_cedrus_msg("/dev/video0 does not support m2m_mplane\n");
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	return VA_STATUS_SUCCESS;
}

