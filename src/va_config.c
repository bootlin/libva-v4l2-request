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
#include "va_config.h"

#include <assert.h>
#include <string.h>

#include <sys/ioctl.h>

#include <linux/videodev2.h>

/*
 * This file provides generalities to VA's user regarding the file formats
 * supported by the v4l driver. It uses VIDIOC_ENUM_FMT to make the
 * correspondence between v4l and VA video formats.
 */

VAStatus sunxi_cedrus_QueryConfigProfiles(VADriverContextP ctx,
		VAProfile *profile_list, int *num_profiles)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) ctx->pDriverData;
	int i = 0;
	struct v4l2_fmtdesc vid_fmtdesc;
	memset(&vid_fmtdesc, 0, sizeof(vid_fmtdesc));
	vid_fmtdesc.index = 0;
	vid_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

	while(ioctl(driver_data->mem2mem_fd, VIDIOC_ENUM_FMT, &vid_fmtdesc) == 0)
	{
		switch(vid_fmtdesc.pixelformat) {
		case V4L2_PIX_FMT_MPEG2_FRAME:
			profile_list[i++] = VAProfileMPEG2Simple;
			profile_list[i++] = VAProfileMPEG2Main;
			break;
		case V4L2_PIX_FMT_MPEG4_FRAME:
			profile_list[i++] = VAProfileMPEG4Simple;
			profile_list[i++] = VAProfileMPEG4AdvancedSimple;
			profile_list[i++] = VAProfileMPEG4Main;
			break;
		}
		vid_fmtdesc.index++;
	}

	assert(i <= SUNXI_CEDRUS_MAX_PROFILES);
	*num_profiles = i;

	return VA_STATUS_SUCCESS;
}

VAStatus sunxi_cedrus_QueryConfigEntrypoints(VADriverContextP ctx,
		VAProfile profile, VAEntrypoint  *entrypoint_list,
		int *num_entrypoints)
{
	switch (profile) {
		case VAProfileMPEG2Simple:
		case VAProfileMPEG2Main:
			*num_entrypoints = 2;
			entrypoint_list[0] = VAEntrypointVLD;
			entrypoint_list[1] = VAEntrypointMoComp;
			break;

		case VAProfileMPEG4Simple:
		case VAProfileMPEG4AdvancedSimple:
		case VAProfileMPEG4Main:
			*num_entrypoints = 1;
			entrypoint_list[0] = VAEntrypointVLD;
			break;

		default:
			*num_entrypoints = 0;
			break;
	}

	assert(*num_entrypoints <= SUNXI_CEDRUS_MAX_ENTRYPOINTS);
	return VA_STATUS_SUCCESS;
}

VAStatus sunxi_cedrus_GetConfigAttributes(VADriverContextP ctx,
		VAProfile profile, VAEntrypoint entrypoint,
		VAConfigAttrib *attrib_list, int num_attribs)
{
	int i;

	for (i = 0; i < num_attribs; i++)
	{
		switch (attrib_list[i].type)
		{
			case VAConfigAttribRTFormat:
				attrib_list[i].value = VA_RT_FORMAT_YUV420;
				break;

			default:
				/* Do nothing */
				attrib_list[i].value = VA_ATTRIB_NOT_SUPPORTED;
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
	for(i = 0; obj_config->attrib_count < i; i++)
	{
		if (obj_config->attrib_list[i].type == attrib->type)
		{
			/* Update existing attribute */
			obj_config->attrib_list[i].value = attrib->value;
			return VA_STATUS_SUCCESS;
		}
	}
	if (obj_config->attrib_count < SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES)
	{
		i = obj_config->attrib_count;
		obj_config->attrib_list[i].type = attrib->type;
		obj_config->attrib_list[i].value = attrib->value;
		obj_config->attrib_count++;
		return VA_STATUS_SUCCESS;
	}
	return VA_STATUS_ERROR_MAX_NUM_EXCEEDED;
}

VAStatus sunxi_cedrus_CreateConfig(VADriverContextP ctx, VAProfile profile,
		VAEntrypoint entrypoint, VAConfigAttrib *attrib_list,
		int num_attribs, VAConfigID *config_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) ctx->pDriverData;
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

		case VAProfileMPEG4Simple:
		case VAProfileMPEG4AdvancedSimple:
		case VAProfileMPEG4Main:
			if (VAEntrypointVLD == entrypoint)
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
	obj_config->attrib_list[0].type = VAConfigAttribRTFormat;
	obj_config->attrib_list[0].value = VA_RT_FORMAT_YUV420;
	obj_config->attrib_count = 1;

	for(i = 0; i < num_attribs; i++)
	{
		vaStatus = sunxi_cedrus_update_attribute(obj_config, &(attrib_list[i]));
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

VAStatus sunxi_cedrus_DestroyConfig(VADriverContextP ctx, VAConfigID config_id)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) ctx->pDriverData;
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

VAStatus sunxi_cedrus_QueryConfigAttributes(VADriverContextP ctx,
		VAConfigID config_id, VAProfile *profile,
		VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list,
		int *num_attribs)
{
	struct sunxi_cedrus_driver_data *driver_data =
		(struct sunxi_cedrus_driver_data *) ctx->pDriverData;
	VAStatus vaStatus = VA_STATUS_SUCCESS;
	struct object_config *obj_config;
	int i;

	obj_config = CONFIG(config_id);
	assert(obj_config);

	*profile = obj_config->profile;
	*entrypoint = obj_config->entrypoint;
	*num_attribs =  obj_config->attrib_count;
	for(i = 0; i < obj_config->attrib_count; i++)
		attrib_list[i] = obj_config->attrib_list[i];

	return vaStatus;
}

/* sunxi-cedrus doesn't support display attributes */
VAStatus sunxi_cedrus_QueryDisplayAttributes (VADriverContextP ctx,
		VADisplayAttribute *attr_list, int *num_attributes)
{ return VA_STATUS_ERROR_UNKNOWN; }

VAStatus sunxi_cedrus_GetDisplayAttributes (VADriverContextP ctx,
		VADisplayAttribute *attr_list, int num_attributes)
{ return VA_STATUS_ERROR_UNKNOWN; }

VAStatus sunxi_cedrus_SetDisplayAttributes (VADriverContextP ctx,
		VADisplayAttribute *attr_list, int num_attributes)
{ return VA_STATUS_ERROR_UNKNOWN; }
