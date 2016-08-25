AM_CPPFLAGS = \
	-DPTHREADS		\
	$(X11_DEPS_CFLAGS)\
	$(DRM_CFLAGS)		\
	$(LIBVA_DEPS_CFLAGS)

driver_cflags = \
	-Wall			\
	-fvisibility=hidden

driver_ldflags = \
	-module -avoid-version	\
	-no-undefined		\
	-Wl,--no-undefined

driver_libs = \
	-lpthread -ldl		\
	$(DRM_LIBS)		\
	$(X11_DEPS_LIBS)	\
	$(LIBVA_DEPS_LIBS)

source_c = sunxi_cedrus_drv_video.c object_heap.c buffer.c va_config.c \
	context.c image.c mpeg2.c picture.c subpicture.c surface.c

source_s = \
	tiled_yuv.S

source_h = sunxi_cedrus_drv_video.h object_heap.h buffer.h va_config.h \
	context.h image.h mpeg2.h picture.h subpicture.h surface.h \
	tiled_yuv.h

sunxi_cedrus_drv_video_la_LTLIBRARIES	= sunxi_cedrus_drv_video.la
sunxi_cedrus_drv_video_ladir		= $(LIBVA_DRIVERS_PATH)
sunxi_cedrus_drv_video_la_CFLAGS	= $(driver_cflags)
sunxi_cedrus_drv_video_la_LDFLAGS	= $(driver_ldflags)
sunxi_cedrus_drv_video_la_LIBADD	= $(driver_libs)
sunxi_cedrus_drv_video_la_SOURCES	= $(source_c) $(source_s)
noinst_HEADERS				= $(source_h)

MAINTAINERCLEANFILES = Makefile.in config.h.in
