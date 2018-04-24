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
#include "utils.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

/* Set default visibility for the init function only. */
VAStatus __attribute__((visibility("default")))
	VA_DRIVER_INIT_FUNC(VADriverContextP context);

VAStatus VA_DRIVER_INIT_FUNC(VADriverContextP context)
{
	struct sunxi_cedrus_driver_data *driver_data;
	struct VADriverVTable *vtable = context->vtable;
	struct v4l2_capability capability;
	VAStatus status;
	unsigned int i;
	int video_fd = -1;
	int media_fd = -1;
	char *video_path;
	char *media_path;
	int rc;

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

	driver_data = (struct sunxi_cedrus_driver_data *) malloc(sizeof(*driver_data));
	memset(driver_data, 0, sizeof(*driver_data));

	context->pDriverData = (void *) driver_data;

	object_heap_init(&driver_data->config_heap, sizeof(struct object_config), CONFIG_ID_OFFSET);
	object_heap_init(&driver_data->context_heap, sizeof(struct object_context), CONTEXT_ID_OFFSET);
	object_heap_init(&driver_data->surface_heap, sizeof(struct object_surface), SURFACE_ID_OFFSET);
	object_heap_init(&driver_data->buffer_heap, sizeof(struct object_buffer), BUFFER_ID_OFFSET);
	object_heap_init(&driver_data->image_heap, sizeof(struct object_image), IMAGE_ID_OFFSET);

	video_path = getenv("LIBVA_CEDRUS_VIDEO_PATH");
	if (video_path == NULL)
		video_path = "/dev/video0";

	video_fd = open(video_path, O_RDWR | O_NONBLOCK);
	if (video_fd < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	rc = ioctl(driver_data->video_fd, VIDIOC_QUERYCAP, &capability);
	if (rc < 0 || !(capability.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE)) {
		sunxi_cedrus_log("Video device %s does not support m2m mplanes\n", video_path);
		return VA_STATUS_ERROR_OPERATION_FAILED;
	}

	media_path = getenv("LIBVA_CEDRUS_MEDIA_PATH");
	if (media_path == NULL)
		media_path = "/dev/media0";

	media_fd = open(video_path, O_RDWR | O_NONBLOCK);
	if (media_fd < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	driver_data->video_fd = video_fd;
	driver_data->media_fd = media_fd;

	for (i = 0; i < INPUT_BUFFERS_NB; i++) {
		driver_data->request_fds[i] = -1;
		driver_data->slice_offset[i] = 0;
	}

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	status = VA_STATUS_ERROR_OPERATION_FAILED;

	if (video_fd >= 0)
		close(video_fd);

	if (media_fd >= 0)
		close(media_fd);

complete:
	return status;
}

VAStatus SunxiCedrusTerminate(VADriverContextP context)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_buffer *buffer_object;
	struct object_image *image_object;
	struct object_surface *surface_object;
	struct object_context *context_object;
	struct object_config *config_object;
	object_heap_iterator iterator;
	unsigned int i;

	for (i = 0; i < INPUT_BUFFERS_NB; i++)
		if (driver_data->request_fds[i] >= 0)
			close(driver_data->request_fds[i]);

	close(driver_data->video_fd);
	close(driver_data->media_fd);

	/* Cleanup leftover buffers. */

	image_object = (struct object_image *) object_heap_first(&driver_data->image_heap, &iterator);
	while (image_object != NULL) {
		DumpDestroyImage(context, (VAImageID) image_object->base.id);
		image_object = (struct object_image *) object_heap_next(&driver_data->image_heap, &iterator);
	}

	object_heap_destroy(&driver_data->image_heap);

	buffer_object = (struct object_buffer *) object_heap_first(&driver_data->buffer_heap, &iterator);
	while (buffer_object != NULL) {
		DumpDestroyBuffer(context, (VABufferID) buffer_object->base.id);
		buffer_object = (struct object_buffer *) object_heap_next(&driver_data->buffer_heap, &iterator);
	}

	object_heap_destroy(&driver_data->buffer_heap);

	surface_object = (struct object_surface *) object_heap_first(&driver_data->surface_heap, &iterator);
	while (surface_object != NULL) {
		DumpDestroySurfaces(context, (VASurfaceID *) &surface_object->base.id, 1);
		surface_object = (struct object_surface *) object_heap_next(&driver_data->surface_heap, &iterator);
	}

	object_heap_destroy(&driver_data->surface_heap);

	context_object = (struct object_context *) object_heap_first(&driver_data->context_heap, &iterator);
	while (context_object != NULL) {
		DumpDestroyContext(context, (VAContextID) context_object->base.id);
		context_object = (struct object_context *) object_heap_next(&driver_data->context_heap, &iterator);
	}

	object_heap_destroy(&driver_data->context_heap);

	config_object = (struct object_config *) object_heap_first(&driver_data->config_heap, &iterator);
	while (config_object != NULL) {
		DumpDestroyConfig(context, (VAConfigID) config_object->base.id);
		config_object = (struct object_config *) object_heap_next(&driver_data->config_heap, &iterator);
	}

	object_heap_destroy(&driver_data->config_heap);

	free(context->pDriverData);
	context->pDriverData = NULL;

	return VA_STATUS_SUCCESS;
}
