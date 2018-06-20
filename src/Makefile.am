AM_CPPFLAGS = -DPTHREADS $(DRM_CFLAGS) $(LIBVA_DEPS_CFLAGS)

backend_cflags = -Wall -fvisibility=hidden
backend_ldflags = -module -avoid-version -no-undefined -Wl,--no-undefined
backend_libs = -lpthread -ldl $(DRM_LIBS) $(LIBVA_DEPS_LIBS)

backend_c = sunxi_cedrus.c object_heap.c config.c surface.c context.c buffer.c \
	mpeg2.c picture.c subpicture.c image.c v4l2.c media.c utils.c

backend_s = tiled_yuv.S

backend_h = sunxi_cedrus.h object_heap.h config.h surface.h context.h buffer.h \
	mpeg2.h picture.h subpicture.h image.h v4l2.h media.h utils.h \
	tiled_yuv.h

sunxi_cedrus_drv_video_la_LTLIBRARIES = sunxi_cedrus_drv_video.la
sunxi_cedrus_drv_video_ladir = $(LIBVA_DRIVERS_PATH)
sunxi_cedrus_drv_video_la_CFLAGS = $(backend_cflags)
sunxi_cedrus_drv_video_la_LDFLAGS = $(backend_ldflags)
sunxi_cedrus_drv_video_la_LIBADD = $(backend_libs)
sunxi_cedrus_drv_video_la_SOURCES = $(backend_c) $(backend_s)
noinst_HEADERS = $(backend_h)

MAINTAINERCLEANFILES = Makefile.in autoconfig.h.in
