AM_CPPFLAGS = -DPTHREADS $(DRM_CFLAGS) $(LIBVA_DEPS_CFLAGS)

backend_cflags = -Wall -fvisibility=hidden
backend_ldflags = -module -avoid-version -no-undefined -Wl,--no-undefined
backend_libs = -lpthread -ldl $(DRM_LIBS) $(LIBVA_DEPS_LIBS)

backend_c = request.c object_heap.c config.c surface.c context.c buffer.c \
	picture.c subpicture.c image.c v4l2.c video.c media.c utils.c

if WITH_MPEG2
backend_c += mpeg2.c
endif

if WITH_H264
backend_c += h264.c
endif

if WITH_H265
backend_c += h265.c
endif

backend_s = tiled_yuv.S

backend_h = request.h object_heap.h config.h surface.h context.h buffer.h \
	mpeg2.h picture.h subpicture.h image.h v4l2.h video.h media.h utils.h \
	tiled_yuv.h h264.h h265.h

v4l2_request_drv_video_la_LTLIBRARIES = v4l2_request_drv_video.la
v4l2_request_drv_video_ladir = $(LIBVA_DRIVERS_PATH)
v4l2_request_drv_video_la_CFLAGS = $(backend_cflags)
v4l2_request_drv_video_la_LDFLAGS = $(backend_ldflags)
v4l2_request_drv_video_la_LIBADD = $(backend_libs)
v4l2_request_drv_video_la_SOURCES = $(backend_c) $(backend_s)
noinst_HEADERS = $(backend_h)

MAINTAINERCLEANFILES = Makefile.in autoconfig.h.in
