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

AC_PREREQ([2.60])
AC_INIT([libva-v4l2-request], [1.0.0], [paul.kocialkowski@bootlin.com])
AM_INIT_AUTOMAKE([dist-bzip2 subdir-objects foreign])

AC_CONFIG_SRCDIR([src/request.c])
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_LANG_C
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AM_PROG_AS

PKG_CHECK_MODULES([LIBVA], [libva >= 1.1.0])
PKG_CHECK_MODULES([DRM], [libdrm >= 2.4.52])

#LIBS="$LIBS $DRM_LIBS"
#CFLAGS="$CFLAGS $DRM_CFLAGS $LIBVA_CFLAGS"

VA_VERSION=`$PKG_CONFIG --modversion libva`
VA_VERSION_MAJOR=`echo "$VA_VERSION" | cut -d'.' -f1`
VA_VERSION_MINOR=`echo "$VA_VERSION" | cut -d'.' -f2`

VA_DRIVER_INIT_FUNC="__vaDriverInit_${VA_VERSION_MAJOR}_${VA_VERSION_MINOR}"
AC_DEFINE_UNQUOTED([VA_DRIVER_INIT_FUNC], [$VA_DRIVER_INIT_FUNC],
		   [Driver init function])

AC_CONFIG_HEADERS([src/autoconfig.h])
AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
