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
#include "image.h"
#include "surface.h"
#include "buffer.h"

#include <assert.h>
#include <string.h>

#include "v4l2.h"
#include "tiled_yuv.h"
#include "utils.h"

VAStatus SunxiCedrusCreateImage(VADriverContextP context, VAImageFormat *format,
	int width, int height, VAImage *image)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	unsigned int destination_sizes[VIDEO_MAX_PLANES];
	unsigned int destination_bytesperlines[VIDEO_MAX_PLANES];
	unsigned int destination_planes_count;
	unsigned int size;
	struct object_image *image_object;
	VABufferID buffer_id;
	VAImageID id;
	VAStatus status;
	unsigned int i;
	int rc;

	rc = v4l2_get_format(driver_data->video_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, NULL, NULL, destination_bytesperlines, destination_sizes, &destination_planes_count);
	if (rc < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	size = 0;

	for (i = 0; i < destination_planes_count; i++)
		size += destination_sizes[i];

	id = object_heap_allocate(&driver_data->image_heap);
	image_object = IMAGE(id);
	if (image_object == NULL)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	status = SunxiCedrusCreateBuffer(context, 0, VAImageBufferType, size, 1, NULL, &buffer_id);
	if (status != VA_STATUS_SUCCESS) {
		object_heap_free(&driver_data->image_heap, (struct object_base *) image_object);
		return status;
	}

	image_object->buffer_id = buffer_id;

	memset(image, 0, sizeof(*image));

	image->format = *format;
	image->width = width;
	image->height = height;
	image->buf = buffer_id;
	image->image_id = id;

	image->num_planes = destination_planes_count;
	image->data_size = size;

	for (i = 0; i < image->num_planes; i++) {
		image->pitches[i] = destination_bytesperlines[i];
		image->offsets[i] = i > 0 ? destination_sizes[i-1] : 0;
	}

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroyImage(VADriverContextP context, VAImageID image_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_image *image_object;
	VAStatus status;

	image_object = IMAGE(image_id);
	if (image_object == NULL)
		return VA_STATUS_ERROR_INVALID_IMAGE;

	status = SunxiCedrusDestroyBuffer(context, image_object->buffer_id);
	if (status != VA_STATUS_SUCCESS)
		return status;

	object_heap_free(&driver_data->image_heap, (struct object_base *) image_object);

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDeriveImage(VADriverContextP context,
	VASurfaceID surface_id, VAImage *image)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *surface_object;
	struct object_buffer *buffer_object;
	unsigned int i;
	VAImageFormat format;
	VAStatus status;

	surface_object = SURFACE(driver_data, surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	if (surface_object->status == VASurfaceRendering) {
		status = SunxiCedrusSyncSurface(context, surface_id);
		if (status != VA_STATUS_SUCCESS)
			return status;
	} else if (surface_object->status == VASurfaceReady) {
		return VA_STATUS_SUCCESS;
	}

	format.fourcc = VA_FOURCC_NV12;

	status = SunxiCedrusCreateImage(context, &format, surface_object->width, surface_object->height, image);
	if (status != VA_STATUS_SUCCESS)
		return status;

	buffer_object = BUFFER(driver_data, image->buf);
	if (buffer_object == NULL)
		return VA_STATUS_ERROR_INVALID_BUFFER;

	for (i = 0; i < surface_object->destination_planes_count; i++) {
		if (driver_data->tiled_format)
			tiled_to_planar(surface_object->destination_data[i], buffer_object->data + image->offsets[i], image->pitches[i], image->width, i == 0 ? image->height : image->height / 2);
		else
			memcpy(buffer_object->data + image->offsets[i], surface_object->destination_data[i], surface_object->destination_sizes[i]);
	}

	surface_object->status = VASurfaceReady;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusQueryImageFormats(VADriverContextP context,
	VAImageFormat *formats, int *formats_count)
{
	formats[0].fourcc = VA_FOURCC_NV12;
	*formats_count = 1;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusSetImagePalette(VADriverContextP context,
	VAImageID image_id, unsigned char *palette)
{
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusGetImage(VADriverContextP context, VASurfaceID surface_id,
	int x, int y, unsigned int width, unsigned int height,
	VAImageID image_id)
{
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusPutImage(VADriverContextP context, VASurfaceID surface_id,
	VAImageID image, int src_x, int src_y, unsigned int src_width,
	unsigned int src_height, int dst_x, int dst_y, unsigned int dst_width,
	unsigned int dst_height)
{
	return VA_STATUS_SUCCESS;
}
