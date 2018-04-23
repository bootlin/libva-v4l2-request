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
#include "config.h"

#include <assert.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

VAStatus SunxiCedrusQueryConfigProfiles(VADriverContextP context,
	VAProfile *profiles, int *profiles_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	int i = 0;
	struct v4l2_fmtdesc vid_fmtdesc;
	memset(&vid_fmtdesc, 0, sizeof(vid_fmtdesc));
	vid_fmtdesc.index = 0;
	vid_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

	while(ioctl(driver_data->mem2mem_fd, VIDIOC_ENUM_FMT, &vid_fmtdesc) == 0)
	{
		switch(vid_fmtdesc.pixelformat) {
		case V4L2_PIX_FMT_MPEG2_FRAME:
			profiles[i++] = VAProfileMPEG2Simple;
			profiles[i++] = VAProfileMPEG2Main;
			break;
		}
		vid_fmtdesc.index++;
	}

	assert(i <= SUNXI_CEDRUS_MAX_PROFILES);
	*profiles_count = i;

	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusQueryConfigEntrypoints(VADriverContextP context,
	VAProfile profile, VAEntrypoint *entrypoints, int *entrypoints_count)
{
	switch (profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			*entrypoints_count = 2;
			entrypoints[0] = VAEntrypointVLD;
			entrypoints[1] = VAEntrypointMoComp;
			break;

		default:
			*entrypoints_count = 0;
			break;
	}

	assert(*entrypoints_count <= SUNXI_CEDRUS_MAX_ENTRYPOINTS);
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusGetConfigAttributes(VADriverContextP context,
	VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attributes,
	int attributes_count)
{
	int i;

	for (i = 0; i < attributes_count; i++)
	{
		switch (attributes[i].type)
		{
			case VAConfigAttribRTFormat:
				attributes[i].value = VA_RT_FORMAT_YUV420;
				break;

			default:
				/* Do nothing */
				attributes[i].value = VA_ATTRIB_NOT_SUPPORTED;
				break;
		}
	}

	return VA_STATUS_SUCCESS;
}

VAStatus sunxi_cedrus_update_attribute(struct object_config *obj_config,
	VAConfigAttrib *attrib)
{
	int i;
	/* Check existing attributes */
	for(i = 0; obj_config->attributes_count < i; i++)
	{
		if (obj_config->attributes[i].type == attrib->type)
		{
			/* Update existing attribute */
			obj_config->attributes[i].value = attrib->value;
			return VA_STATUS_SUCCESS;
		}
	}
	if (obj_config->attributes_count < SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES)
	{
		i = obj_config->attributes_count;
		obj_config->attributes[i].type = attrib->type;
		obj_config->attributes[i].value = attrib->value;
		obj_config->attributes_count++;
		return VA_STATUS_SUCCESS;
	}
	return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
}

VAStatus SunxiCedrusCreateConfig(VADriverContextP context, VAProfile profile,
	VAEntrypoint entrypoint, VAConfigAttrib *attributes,
	int attributes_count, VAConfigID *config_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VAStatus vaStatus;
	int configID;
	struct object_config *obj_config;
	int i;

	/* Validate profile & entrypoint */
	switch (profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			if ((VAEntrypointVLD == entrypoint) ||
					(VAEntrypointMoComp == entrypoint))
				vaStatus = VA_STATUS_SUCCESS;
			else
				vaStatus = VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;
			break;

		default:
			vaStatus = VA_STATUS_ERROR_UNSUPPORTED_PROFILE;
			break;
	}

	if (VA_STATUS_SUCCESS != vaStatus)
		return vaStatus;

	configID = object_heap_allocate(&driver_data->config_heap);
	obj_config = CONFIG(configID);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
		return vaStatus;
	}

	obj_config->profile = profile;
	obj_config->entrypoint = entrypoint;
	obj_config->attributes[0].type = VAConfigAttribRTFormat;
	obj_config->attributes[0].value = VA_RT_FORMAT_YUV420;
	obj_config->attributes_count = 1;

	for(i = 0; i < attributes_count; i++)
	{
		vaStatus = sunxi_cedrus_update_attribute(obj_config, &(attributes[i]));
		if (VA_STATUS_SUCCESS != vaStatus)
			break;
	}

	/* Error recovery */
	if (VA_STATUS_SUCCESS != vaStatus)
		object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
	else
		*config_id = configID;

	return vaStatus;
}

VAStatus SunxiCedrusDestroyConfig(VADriverContextP context,
	VAConfigID config_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VAStatus vaStatus;
	struct object_config *obj_config;

	obj_config = CONFIG(config_id);
	if (NULL == obj_config)
	{
		vaStatus = VA_STATUS_ERROR_INVALID_CONFIG;
		return vaStatus;
	}

	object_heap_free(&driver_data->config_heap, (object_base_p) obj_config);
	return VA_STATUS_SUCCESS;
}

VAStatus SunxiCedrusQueryConfigAttributes(VADriverContextP context,
	VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint,
	VAConfigAttrib *attributes, int *attributes_count)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) context->pDriverData;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	struct object_config *obj_config;
	int i;

	obj_config = CONFIG(config_id);
	assert(obj_config);

	*profile = obj_config->profile;
	*entrypoint = obj_config->entrypoint;
	*attributes_count =  obj_config->attributes_count;
	for(i = 0; i < obj_config->attributes_count; i++)
		attributes[i] = obj_config->attributes[i];

	return vaStatus;
}

VAStatus SunxiCedrusQueryDisplayAttributes(VADriverContextP context,
	VADisplayAttribute *attributes, int *attributes_count)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusGetDisplayAttributes(VADriverContextP context,
	VADisplayAttribute *attributes, int *attributes_count)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus SunxiCedrusSetDisplayAttributes(VADriverContextP context,
	VADisplayAttribute *attributes, int *attributes_count)
{
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
