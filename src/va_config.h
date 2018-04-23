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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <va/va_backend.h>

#include "object_heap.h"

#define CONFIG(id)  ((struct object_config *)  object_heap_lookup(&driver_data->config_heap,  id))
#define CONFIG_ID_OFFSET		0x01000000

struct object_config {
	struct object_base base;

	VAProfile profile;
	VAEntrypoint entrypoint;
	VAConfigAttrib attributes[SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES];
	int attributes_count;
};

VAStatus SunxiCedrusQueryConfigProfiles(VADriverContextP ctx,
		VAProfile *profile_list, int *num_profiles);

VAStatus SunxiCedrusQueryConfigEntrypoints(VADriverContextP ctx,
		VAProfile profile, VAEntrypoint  *entrypoint_list,
		int *num_entrypoints);

VAStatus SunxiCedrusGetConfigAttributes(VADriverContextP ctx,
		VAProfile profile, VAEntrypoint entrypoint,
		VAConfigAttrib *attrib_list, int num_attribs);

VAStatus SunxiCedrusupdate_attribute(struct object_config *obj_config,
		VAConfigAttrib *attrib);

VAStatus SunxiCedrusCreateConfig(VADriverContextP ctx, VAProfile profile,
		VAEntrypoint entrypoint, VAConfigAttrib *attrib_list,
		int num_attribs, VAConfigID *config_id);

VAStatus SunxiCedrusDestroyConfig(VADriverContextP ctx, VAConfigID config_id);

VAStatus SunxiCedrusQueryConfigAttributes(VADriverContextP ctx,
		VAConfigID config_id, VAProfile *profile,
		VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list,
		int *num_attribs);

VAStatus SunxiCedrusQueryDisplayAttributes (VADriverContextP ctx,
		VADisplayAttribute *attr_list, int *num_attributes);

VAStatus SunxiCedrusGetDisplayAttributes (VADriverContextP ctx,
		VADisplayAttribute *attr_list, int num_attributes);

VAStatus SunxiCedrusSetDisplayAttributes (VADriverContextP ctx,
		VADisplayAttribute *attr_list, int num_attributes);

#endif /* _CONFIG_H_ */
