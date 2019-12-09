/*
 * Copyright (C) 2007 Intel Corporation
 * Copyright (C) 2016 Florent Revest <florent.revest@free-electrons.com>
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

#include "request.h"
#include "surface.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <va/va_drmcommon.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>

#include <h264-ctrls.h>

#include "media.h"
#include "utils.h"
#include "v4l2.h"
#include "video.h"

VAStatus RequestCreateSurfaces2(VADriverContextP context, unsigned int format,
				unsigned int width, unsigned int height,
				VASurfaceID *surfaces_ids,
				unsigned int surfaces_count,
				VASurfaceAttrib *attributes,
				unsigned int attributes_count)
{
	struct request_data *driver_data = context->pDriverData;
	struct object_surface *surface_object;
	struct video_format *video_format = NULL;
	unsigned int destination_sizes[VIDEO_MAX_PLANES];
	unsigned int destination_bytesperlines[VIDEO_MAX_PLANES];
	unsigned int destination_planes_count;
	unsigned int format_width, format_height;
	unsigned int capture_type;
	unsigned int output_type;
	unsigned int index_base;
	unsigned int index;
	unsigned int dmabuf_index_base;
	unsigned int dmabuf_index;
	unsigned int i, j;
	VAStatus status;
	VASurfaceID id;
	int video_fd;
	bool found;
	int rc;

	if (format != VA_RT_FORMAT_YUV420)
		return VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT;

	capture_type = v4l2_type_video_capture(driver_data->mplane);
	output_type = v4l2_type_video_output(driver_data->mplane);

	video_fd = open(driver_data->video_path, O_RDWR | O_NONBLOCK);
	if (video_fd < 0)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	if (!driver_data->video_format) {
		found = v4l2_find_format(driver_data->video_fd, capture_type,
					 V4L2_PIX_FMT_SUNXI_TILED_NV12);
		if (found)
			video_format = video_format_find(V4L2_PIX_FMT_SUNXI_TILED_NV12);

		found = v4l2_find_format(driver_data->video_fd, capture_type,
					 V4L2_PIX_FMT_NV12);
		if (found)
			video_format = video_format_find(V4L2_PIX_FMT_NV12);

		if (video_format == NULL) {
			status = VA_STATUS_ERROR_OPERATION_FAILED;
			goto error;
		}

		driver_data->video_format = video_format;

		/* Set output format in case driver limits capture format to
		 * output format dimensions.
		*/

		unsigned int format = V4L2_PIX_FMT_H264_SLICE;
		rc = v4l2_set_format(driver_data->video_fd, output_type,
				     format, width, height);
		if (rc < 0) {
			status = VA_STATUS_ERROR_OPERATION_FAILED;
			goto error;
		}

		rc = v4l2_set_format(driver_data->video_fd, capture_type,
				     video_format->v4l2_format, width, height);
		if (rc < 0) {
			status = VA_STATUS_ERROR_OPERATION_FAILED;
			goto error;
		}
        } else {
		video_format = driver_data->video_format;
	}

	/* Set output format in case driver limits capture format to output
	 * format dimensions.
	 */
	rc = v4l2_set_format(video_fd, output_type, V4L2_PIX_FMT_H264_SLICE,
			     width, height);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_set_format(video_fd, capture_type, video_format->v4l2_format,
			     width, height);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_get_format(video_fd, capture_type, &format_width,
			     &format_height, destination_bytesperlines,
			     destination_sizes, NULL);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	if (format_width < width || format_height < height) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	destination_planes_count = video_format->planes_count;

	rc = v4l2_create_buffers(video_fd, capture_type,
				 V4L2_MEMORY_MMAP, surfaces_count,
				 &index_base);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	rc = v4l2_create_buffers(driver_data->video_fd, capture_type,
				 V4L2_MEMORY_DMABUF, surfaces_count,
				 &dmabuf_index_base);
	if (rc < 0) {
		status = VA_STATUS_ERROR_ALLOCATION_FAILED;
		goto error;
	}

	for (i = 0; i < surfaces_count; i++) {
		index = index_base + i;
		dmabuf_index = dmabuf_index_base + i;

		id = object_heap_allocate(&driver_data->surface_heap);
		surface_object = SURFACE(driver_data, id);
		if (surface_object == NULL) {
			status = VA_STATUS_ERROR_ALLOCATION_FAILED;
			goto error;
		}

		for (j = 0; j < VIDEO_MAX_PLANES; j++)
			surface_object->destination_dmabuf_fds[j] = -1;

		rc = v4l2_query_buffer(video_fd, capture_type, index,
				       surface_object->destination_map_lengths,
				       surface_object->destination_map_offsets,
				       video_format->v4l2_buffers_count);
		if (rc < 0) {
			status = VA_STATUS_ERROR_ALLOCATION_FAILED;
			goto error;
		}

		rc = v4l2_export_buffer(video_fd, capture_type, index,
					O_RDONLY,
					surface_object->destination_dmabuf_fds,
					video_format->v4l2_buffers_count);
		if (rc < 0) {
			status = VA_STATUS_ERROR_ALLOCATION_FAILED;
			goto error;
		}

		for (j = 0; j < video_format->v4l2_buffers_count; j++) {
			surface_object->destination_map[j] =
				mmap(NULL,
				     surface_object->destination_map_lengths[j],
				     PROT_READ | PROT_WRITE, MAP_SHARED,
				     video_fd,
				     surface_object->destination_map_offsets[j]);

			if (surface_object->destination_map[j] == MAP_FAILED) {
				status = VA_STATUS_ERROR_ALLOCATION_FAILED;
				goto error;
			}
		}

		/*
		 * FIXME: Handle this per-pixelformat, trying to generalize it
		 * is not a reasonable approach. The final description should be
		 * in terms of (logical) planes.
		 */

		if (video_format->v4l2_buffers_count == 1) {
			destination_sizes[0] = destination_bytesperlines[0] *
					       format_height;

			for (j = 1; j < destination_planes_count; j++)
				destination_sizes[j] = destination_sizes[0] / 2;

			for (j = 0; j < destination_planes_count; j++) {
				surface_object->destination_offsets[j] =
					j > 0 ? destination_sizes[j - 1] : 0;
				surface_object->destination_data[j] =
					((unsigned char *)surface_object->destination_map[0] +
					 surface_object->destination_offsets[j]);
				surface_object->destination_sizes[j] =
					destination_sizes[j];
				surface_object->destination_bytesperlines[j] =
					destination_bytesperlines[0];
			}
		} else if (video_format->v4l2_buffers_count == destination_planes_count) {
			for (j = 0; j < destination_planes_count; j++) {
				surface_object->destination_offsets[j] = 0;
				surface_object->destination_data[j] =
					surface_object->destination_map[j];
				surface_object->destination_sizes[j] =
					destination_sizes[j];
				surface_object->destination_bytesperlines[j] =
					destination_bytesperlines[j];
			}
		} else {
			status = VA_STATUS_ERROR_ALLOCATION_FAILED;
			goto error;
		}

		surface_object->status = VASurfaceReady;
		surface_object->width = width;
		surface_object->height = height;

		surface_object->source_index = 0;
		surface_object->source_data = NULL;
		surface_object->source_size = 0;

		surface_object->destination_index = dmabuf_index;

		surface_object->destination_planes_count =
			destination_planes_count;
		surface_object->destination_buffers_count =
			video_format->v4l2_buffers_count;

		memset(&surface_object->params, 0,
		       sizeof(surface_object->params));
		surface_object->slices_count = 0;
		surface_object->slices_size = 0;

		surface_object->request_fd = -1;

		surfaces_ids[i] = id;
	}

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	/* TODO */

complete:
	close(video_fd);
	return status;
}

VAStatus RequestCreateSurfaces(VADriverContextP context, int width, int height,
			       int format, int surfaces_count,
			       VASurfaceID *surfaces_ids)
{
	return RequestCreateSurfaces2(context, format, width, height,
				      surfaces_ids, surfaces_count, NULL, 0);
}

VAStatus RequestDestroySurfaces(VADriverContextP context,
				VASurfaceID *surfaces_ids, int surfaces_count)
{
	struct request_data *driver_data = context->pDriverData;
	struct object_surface *surface_object;
	unsigned int i, j;

	for (i = 0; i < surfaces_count; i++) {
		surface_object = SURFACE(driver_data, surfaces_ids[i]);
		if (surface_object == NULL)
			return VA_STATUS_ERROR_INVALID_SURFACE;

		if (surface_object->source_data != NULL &&
		    surface_object->source_size > 0)
			munmap(surface_object->source_data,
			       surface_object->source_size);

		for (j = 0; j < surface_object->destination_buffers_count; j++)
			if (surface_object->destination_map[j] != NULL &&
			    surface_object->destination_map_lengths[j] > 0) {
				munmap(surface_object->destination_map[j],
				       surface_object->destination_map_lengths[j]);
			if (surface_object->destination_dmabuf_fds[j] != -1)
				close(surface_object->destination_dmabuf_fds[j]);
		}

		if (surface_object->request_fd > 0)
			close(surface_object->request_fd);

		object_heap_free(&driver_data->surface_heap,
				 (struct object_base *)surface_object);
	}

	return VA_STATUS_SUCCESS;
}

VAStatus RequestSyncSurface(VADriverContextP context, VASurfaceID surface_id)
{
	struct request_data *driver_data = context->pDriverData;
	struct object_surface *surface_object;
	VAStatus status;
	struct video_format *video_format;
	unsigned int output_type, capture_type;
	int request_fd = -1;
	int rc;

	video_format = driver_data->video_format;
	if (video_format == NULL) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	output_type = v4l2_type_video_output(driver_data->mplane);
	capture_type = v4l2_type_video_capture(driver_data->mplane);

	surface_object = SURFACE(driver_data, surface_id);
	if (surface_object == NULL) {
		status = VA_STATUS_ERROR_INVALID_SURFACE;
		goto error;
	}

	if (surface_object->status != VASurfaceRendering) {
		status = VA_STATUS_SUCCESS;
		goto complete;
	}

	request_fd = surface_object->request_fd;
	if (request_fd < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_queue(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_wait_completion(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = media_request_reinit(request_fd);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_dequeue_buffer(driver_data->video_fd, -1, output_type,
				 surface_object->source_index, 1);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	rc = v4l2_dequeue_dmabuf(driver_data->video_fd, -1, capture_type,
				 surface_object->destination_index,
				 surface_object->destination_buffers_count);
	if (rc < 0) {
		status = VA_STATUS_ERROR_OPERATION_FAILED;
		goto error;
	}

	surface_object->status = VASurfaceDisplaying;

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	if (request_fd >= 0) {
		close(request_fd);
		surface_object->request_fd = -1;
	}

complete:
	return status;
}

VAStatus RequestQuerySurfaceAttributes(VADriverContextP context,
				       VAConfigID config,
				       VASurfaceAttrib *attributes,
				       unsigned int *attributes_count)
{
	struct request_data *driver_data = context->pDriverData;
	VASurfaceAttrib *attributes_list;
	unsigned int attributes_list_size = V4L2_REQUEST_MAX_CONFIG_ATTRIBUTES *
					    sizeof(*attributes);
	int memory_types;
	unsigned int i = 0;

	attributes_list = malloc(attributes_list_size);
	memset(attributes_list, 0, attributes_list_size);

	attributes_list[i].type = VASurfaceAttribPixelFormat;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = VA_FOURCC_NV12;
	i++;

	attributes_list[i].type = VASurfaceAttribMinWidth;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 32;
	i++;

	attributes_list[i].type = VASurfaceAttribMaxWidth;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 2048;
	i++;

	attributes_list[i].type = VASurfaceAttribMinHeight;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 32;
	i++;

	attributes_list[i].type = VASurfaceAttribMaxHeight;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;
	attributes_list[i].value.value.i = 2048;
	i++;

	attributes_list[i].type = VASurfaceAttribMemoryType;
	attributes_list[i].flags = VA_SURFACE_ATTRIB_GETTABLE |
				   VA_SURFACE_ATTRIB_SETTABLE;
	attributes_list[i].value.type = VAGenericValueTypeInteger;

	memory_types = VA_SURFACE_ATTRIB_MEM_TYPE_VA |
		VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2;

	/*
	 * First version of DRM prime export does not handle modifiers,
	 * that are required for supporting the tiled output format.
	 */

	if (video_format_is_linear(driver_data->video_format))
		memory_types |= VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;

	attributes_list[i].value.value.i = memory_types;
	i++;

	attributes_list_size = i * sizeof(*attributes);

	if (attributes != NULL)
		memcpy(attributes, attributes_list, attributes_list_size);

	free(attributes_list);

	*attributes_count = i;

	return VA_STATUS_SUCCESS;
}

VAStatus RequestQuerySurfaceStatus(VADriverContextP context,
				   VASurfaceID surface_id,
				   VASurfaceStatus *status)
{
	struct request_data *driver_data = context->pDriverData;
	struct object_surface *surface_object;

	surface_object = SURFACE(driver_data, surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	*status = surface_object->status;

	return VA_STATUS_SUCCESS;
}

VAStatus RequestPutSurface(VADriverContextP context, VASurfaceID surface_id,
			   void *draw, short src_x, short src_y,
			   unsigned short src_width, unsigned short src_height,
			   short dst_x, short dst_y, unsigned short dst_width,
			   unsigned short dst_height, VARectangle *cliprects,
			   unsigned int cliprects_count, unsigned int flags)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus RequestLockSurface(VADriverContextP context, VASurfaceID surface_id,
			    unsigned int *fourcc, unsigned int *luma_stride,
			    unsigned int *chroma_u_stride,
			    unsigned int *chroma_v_stride,
			    unsigned int *luma_offset,
			    unsigned int *chroma_u_offset,
			    unsigned int *chroma_v_offset,
			    unsigned int *buffer_name, void **buffer)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus RequestUnlockSurface(VADriverContextP context, VASurfaceID surface_id)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus RequestExportSurfaceHandle(VADriverContextP context,
				    VASurfaceID surface_id, uint32_t mem_type,
				    uint32_t flags, void *descriptor)
{
	struct request_data *driver_data = context->pDriverData;
	VADRMPRIMESurfaceDescriptor *surface_descriptor = descriptor;
	struct object_surface *surface_object;
	struct video_format *video_format;
	int *export_fds = NULL;
	unsigned int export_fds_count;
	unsigned int planes_count;
	unsigned int size;
	unsigned int i;
	VAStatus status;

	video_format = driver_data->video_format;
	if (video_format == NULL)
		return VA_STATUS_ERROR_OPERATION_FAILED;

	if (mem_type != VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2)
		return VA_STATUS_ERROR_UNSUPPORTED_MEMORY_TYPE;

	surface_object = SURFACE(driver_data, surface_id);
	if (surface_object == NULL)
		return VA_STATUS_ERROR_INVALID_SURFACE;

	export_fds_count = surface_object->destination_buffers_count;
	export_fds = malloc(export_fds_count * sizeof(*export_fds));

	for (i = 0; i < export_fds_count; i++) {
		if (surface_object->destination_dmabuf_fds[i] == -1) {
			for (i = 0; i < export_fds_count; i++)
				export_fds[i] = -1;
			status = VA_STATUS_ERROR_OPERATION_FAILED;
			goto error;
		}
	}
	for (i = 0; i < export_fds_count; i++) {
		export_fds[i] = dup(surface_object->destination_dmabuf_fds[i]);
		if (export_fds[i] == -1) {
			while (++i < export_fds_count)
				export_fds[i] = -1;
			status = VA_STATUS_ERROR_OPERATION_FAILED;
			goto error;
		}
	}

	planes_count = surface_object->destination_planes_count;

	surface_descriptor->fourcc = VA_FOURCC_NV12;
	surface_descriptor->width = surface_object->width;
	surface_descriptor->height = surface_object->height;
	surface_descriptor->num_objects = export_fds_count;

	size = 0;

	if (export_fds_count == 1)
		for (i = 0; i < planes_count; i++)
			size += surface_object->destination_sizes[i];

	for (i = 0; i < export_fds_count; i++) {
		surface_descriptor->objects[i].drm_format_modifier =
			video_format->drm_modifier;
		surface_descriptor->objects[i].fd = export_fds[i];
		surface_descriptor->objects[i].size = export_fds_count == 1 ?
						      size :
						      surface_object->destination_sizes[i];
	}

	surface_descriptor->num_layers = 1;

	surface_descriptor->layers[0].drm_format = video_format->drm_format;
	surface_descriptor->layers[0].num_planes = planes_count;

	for (i = 0; i < planes_count; i++) {
		surface_descriptor->layers[0].object_index[i] =
			export_fds_count == 1 ? 0 : i;
		surface_descriptor->layers[0].offset[i] =
			surface_object->destination_offsets[i];
		surface_descriptor->layers[0].pitch[i] =
			surface_object->destination_bytesperlines[i];
	}

	status = VA_STATUS_SUCCESS;
	goto complete;

error:
	for (i = 0; i < export_fds_count; i++)
		if (export_fds[i] >= 0)
			close(export_fds[i]);

complete:
	if (export_fds != NULL)
		free(export_fds);

	return status;
}
