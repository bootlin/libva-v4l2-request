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

#ifndef _SUNXI_CEDRUS_H_
#define _SUNXI_CEDRUS_H_

#include <va/va.h>
#include "object_heap.h"
#include "context.h"

#include <linux/videodev2.h>

#define SUNXI_CEDRUS_STR_VENDOR			"Sunxi Cedrus Driver 1.0"

#define SUNXI_CEDRUS_MAX_PROFILES		11
#define SUNXI_CEDRUS_MAX_ENTRYPOINTS		5
#define SUNXI_CEDRUS_MAX_CONFIG_ATTRIBUTES	10
#define SUNXI_CEDRUS_MAX_IMAGE_FORMATS		10
#define SUNXI_CEDRUS_MAX_SUBPIC_FORMATS		4
#define SUNXI_CEDRUS_MAX_DISPLAY_ATTRIBUTES	4

void sunxi_cedrus_msg(const char *msg, ...);

struct sunxi_cedrus_driver_data {
	struct object_heap	config_heap;
	struct object_heap	context_heap;
	struct object_heap	surface_heap;
	struct object_heap	buffer_heap;
	struct object_heap	image_heap;
	char                   *luma_bufs[VIDEO_MAX_FRAME];
	char                   *chroma_bufs[VIDEO_MAX_FRAME];
	unsigned int		num_dst_bufs;
	int			mem2mem_fd;
	int			request_fds[INPUT_BUFFERS_NB];
	int			slice_offset[INPUT_BUFFERS_NB];
};

#endif /* _SUNXI_CEDRUS_H_ */
