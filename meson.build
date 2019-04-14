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
	default_library : 'shared',
	meson_version: '>= 0.43.0')

add_project_arguments('-DMESON_BUILD', language : 'c')

cc = meson.get_compiler('c')

libva_dep = dependency('libva',
	method: 'pkg-config',
	version: '>= 1.1.0',
	required: true)

libdrm_dep = dependency('libdrm',
	method: 'pkg-config',
	version : '>= 2.4.52',
	required: true)

libudev_dep = dependency('libudev',
	method: 'pkg-config',
	required: true)

va_api_version_array = libva_dep.version().split('.')
va_api_major_version = va_api_version_array[0]
va_api_minor_version = va_api_version_array[1]

va_driver_init_func = '__vaDriverInit_@0@_@1@'.format(va_api_major_version,
						      va_api_minor_version)

subdir('src')
