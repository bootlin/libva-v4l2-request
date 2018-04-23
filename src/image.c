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

#include "tiled_yuv.h"

VAStatus SunxiCedrusQueryImageFormats(VADriverContextP context,
	VAImageFormat *formats, int *formats_count)
{
	formats[0].fourcc = VA_FOURCC_NV12;
	*formats_count = 1;
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusCreateImage(VADriverContextP context, VAImageFormat *format,
	int width, int height, VAImage *image)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	int sizeY, sizeUV;
	struct object_image *obj_img;

	memset(image, 0, sizeof(VAImage));

	image->format = *format;
	image->buf = VA_INVALID_ID;
	image->width = width;
	image->height = height;

	sizeY    = image->width * image->height;
	sizeUV   = ((image->width+1) * (image->height+1)/2);

	image->num_planes = 2;
	image->pitches[0] = (image->width+31)&~31;
	image->pitches[1] = (image->width+31)&~31;
	image->offsets[0] = 0;
	image->offsets[1] = sizeY;
	image->data_size  = sizeY + sizeUV;

	image->image_id = object_heap_allocate(&driver_data->image_heap);
	if (image->image_id == VA_INVALID_ID)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	obj_img = IMAGE(image->image_id);

	if (SunxiCedrusCreateBuffer(context, 0, VAImageBufferType, image->data_size,
	    1, NULL, &image->buf) != VA_STATUS_SUCCESS) {
		// TODO: free image object
		return VA_STATUS_ERROR_ALLOCATION_FAILED;
	}
	obj_img->buf = image->buf;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDeriveImage(VADriverContextP context,
	VASurfaceID surface_id, VAImage *image)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_surface *obj_surface;
	VAImageFormat fmt;
	struct object_buffer *obj_buffer;
	VAStatus ret;

	obj_surface = SURFACE(surface);
	fmt.fourcc = VA_FOURCC_NV12;

	if (obj_surface->status == VASurfaceRendering)
		SunxiCedrusSyncSurface(context, surface);

	ret = SunxiCedrusCreateImage(context, &fmt, obj_surface->width, obj_surface->height, image);
	if(ret != VA_STATUS_SUCCESS)
		return ret;

	obj_buffer = BUFFER(image->buf);
	if (NULL == obj_buffer)
		return VA_STATUS_ERROR_ALLOCATION_FAILED;

	/* TODO: Use an appropriate DRM plane instead */
	tiled_to_planar(driver_data->luma_bufs[obj_surface->output_buf_index], obj_buffer->buffer_data, image->pitches[0], image->width, image->height);
	tiled_to_planar(driver_data->chroma_bufs[obj_surface->output_buf_index], obj_buffer->buffer_data + image->width*image->height, image->pitches[1], image->width, image->height/2);

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusDestroyImage(VADriverContextP context, VAImageID image_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	struct object_image *obj_img;

	obj_img = IMAGE(image);
	assert(obj_img);

	SunxiCedrusDestroyBuffer(context, obj_img->buf);
	object_heap_free(&driver_data->image_heap, &obj_img->base);

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusSetImagePalette(VADriverContextP context,
	VAImageID image_id, unsigned char *palette)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusGetImage(VADriverContextP context, VASurfaceID surface_id,
	int x, int y, unsigned int width, unsigned int height,
	VAImageID image_id)
{ return VA_STATUS_SUCCESS; }

VAStatus SunxiCedrusPutImage(VADriverContextP context, VASurfaceID surface_id,
	VAImageID image, int src_x, int src_y, unsigned int src_width,
	unsigned int src_height, int dst_x, int dst_y, unsigned int dst_width,
	unsigned int dst_height)
{ return VA_STATUS_SUCCESS; }
