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

#ifndef _V4L2_REQUEST_H_
#define _V4L2_REQUEST_H_

#include <stdbool.h>

#include "context.h"
#include "object_heap.h"
#include <va/va.h>

#include <linux/videodev2.h>

#define V4L2_REQUEST_STR_VENDOR			"Sunxi-Cedrus"

#define V4L2_REQUEST_MAX_PROFILES		11
#define V4L2_REQUEST_MAX_ENTRYPOINTS		5
#define V4L2_REQUEST_MAX_CONFIG_ATTRIBUTES	10
#define V4L2_REQUEST_MAX_IMAGE_FORMATS		10
#define V4L2_REQUEST_MAX_SUBPIC_FORMATS		4
#define V4L2_REQUEST_MAX_DISPLAY_ATTRIBUTES	4

struct cedrus_data {
	struct object_heap config_heap;
	struct object_heap context_heap;
	struct object_heap surface_heap;
	struct object_heap buffer_heap;
	struct object_heap image_heap;
	int video_fd;
	int media_fd;

	bool tiled_format;
};

VAStatus VA_DRIVER_INIT_FUNC(VADriverContextP context);
VAStatus RequestTerminate(VADriverContextP context);

#endif
