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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <va/va_backend.h>

#include "object_heap.h"
#include "request.h"

#define CONFIG(data, id)                                                       \
	((struct object_config *)object_heap_lookup(&(data)->config_heap, id))
#define CONFIG_ID_OFFSET		0x01000000

struct object_config {
	struct object_base base;

	VAProfile profile;
	VAEntrypoint entrypoint;
	VAConfigAttrib attributes[V4L2_REQUEST_MAX_CONFIG_ATTRIBUTES];
	int attributes_count;
};

VAStatus RequestCreateConfig(VADriverContextP context, VAProfile profile,
			     VAEntrypoint entrypoint,
			     VAConfigAttrib *attributes, int attributes_count,
			     VAConfigID *config_id);
VAStatus RequestDestroyConfig(VADriverContextP context, VAConfigID config_id);
VAStatus RequestQueryConfigProfiles(VADriverContextP context,
				    VAProfile *profiles, int *profiles_count);
VAStatus RequestQueryConfigEntrypoints(VADriverContextP context,
				       VAProfile profile,
				       VAEntrypoint *entrypoints,
				       int *entrypoints_count);
VAStatus RequestQueryConfigAttributes(VADriverContextP context,
				      VAConfigID config_id, VAProfile *profile,
				      VAEntrypoint *entrypoint,
				      VAConfigAttrib *attributes,
				      int *attributes_count);
VAStatus RequestGetConfigAttributes(VADriverContextP context, VAProfile profile,
				    VAEntrypoint entrypoint,
				    VAConfigAttrib *attributes,
				    int attributes_count);
VAStatus RequestQueryDisplayAttributes(VADriverContextP context,
				       VADisplayAttribute *attributes,
				       int *attributes_count);
VAStatus RequestGetDisplayAttributes(VADriverContextP context,
				     VADisplayAttribute *attributes,
				     int attributes_count);
VAStatus RequestSetDisplayAttributes(VADriverContextP context,
				     VADisplayAttribute *attributes,
				     int attributes_count);

#endif
