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

project('libva-v4l2-request', 'c',
	version: '1.0.0',
	meson_version: '>= 0.43.0')

cc = meson.get_compiler('c')

# Let user specify kernel sources for newer UAPI headers
kernel_headers_inc = include_directories()
kernel_headers_dep = dependency('', required : false)
kernel_headers = get_option('kernel_headers')
if kernel_headers != ''
    kernel_headers_inc = include_directories(join_paths(kernel_headers, 'include'))
    kernel_headers_dep = declare_dependency(include_directories : kernel_headers_inc)
endif


# has_header_symbol() does not support the 'required' keyword
# until Meson 0.50. Meanwhile Debian Buster is still at 0.49.
# This is why if statements are used here.

# Check for new Request API macros
if not cc.has_header_symbol('linux/media.h', 'MEDIA_IOC_REQUEST_ALLOC',
			    include_directories : kernel_headers_inc)
    error('Missing request API from Linux Kernel headers')
endif
if not cc.has_header_symbol('linux/videodev2.h', 'V4L2_CTRL_WHICH_REQUEST_VAL',
			    include_directories : kernel_headers_inc)
    error('Missing V4L2_CTRL_WHICH_REQUEST_VAL from Linux Kernel headers')
endif

# Check for format macros
if not cc.has_header_symbol('linux/videodev2.h', 'V4L2_PIX_FMT_MPEG2_SLICE',
			    include_directories : kernel_headers_inc)
    error('Missing macro V4L2_PIX_FMT_MPEG2_SLICE from Linux Kernel headers')
endif
if not cc.has_header_symbol('linux/videodev2.h', 'V4L2_PIX_FMT_SUNXI_TILED_NV12',
			    include_directories : kernel_headers_inc)
    error('Missing macro V4L2_PIX_FMT_SUNXI_TILED_NV12 from Linux Kernel headers')
endif

libva_dep = dependency('libva', version : '>= 1.1.0')
libdrm_dep = dependency('libdrm', version : '>= 2.4.52')

va_api_version_array = libva_dep.version().split('.')
va_api_major_version = va_api_version_array[0]
va_api_minor_version = va_api_version_array[1]

va_driver_init_func = '__vaDriverInit_@0@_@1@'.format(va_api_major_version,
						      va_api_minor_version)

subdir('src')
