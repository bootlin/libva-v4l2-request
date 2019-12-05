# Copyright (C) 2019 Bootlin
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AM_CFLAGS = $(CFLAGS)

v4l2_request_drv_video_la_SOURCES = request.c \
	request.h \
	object_heap.c \
	object_heap.h \
	config.c \
	config.h \
	surface.c \
	surface.h \
	context.c \
	context.h \
	buffer.c \
	buffer.h \
	picture.c \
	picture.h \
	subpicture.c \
	subpicture.h \
	image.c \
	image.h \
	utils.c \
	utils.h \
	tiled_yuv.S \
	tiled_yuv.h \
	video.c \
	video.h \
	media.c \
	media.h \
	v4l2.c \
	v4l2.h \
	mpeg2.c \
	mpeg2.h \
	h264.c \
	h264.h \
	h265.c \
	h265.h

v4l2_request_drv_video_la_CFLAGS = -I../include $(DRM_CFLAGS) $(LIBVA_CFLAGS)
v4l2_request_drv_video_la_LDFLAGS = -module -avoid-version -no-undefined \
				    -Wl,--no-undefined
v4l2_request_drv_video_la_LIBADD = $(DRM_LIBS) $(LIBVA_LIBS)
v4l2_request_drv_video_la_LTLIBRARIES = v4l2_request_drv_video.la
v4l2_request_drv_video_ladir = $(exec_prefix)$(LIBVA_DRIVER_PATH)

MAINTAINERCLEANFILES = Makefile.in autoconfig.h.in
