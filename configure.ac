# intel-driver package version number
m4_define([sunxi_cedrus_major_version], [1])
m4_define([sunxi_cedrus_minor_version], [0])
m4_define([sunxi_cedrus_micro_version], [0])
m4_define([sunxi_cedrus_pre_version],   [0])
m4_define([sunxi_cedrus_version],
          [sunxi_cedrus_major_version.sunxi_cedrus_minor_version.sunxi_cedrus_micro_version])
m4_if(sunxi_cedrus_pre_version, [0], [], [
m4_append([sunxi_cedrus_version], sunxi_cedrus_pre_version, [.pre])
])

# libva minimum version requirement
m4_define([libva_package_version], [1.2.2])
m4_define([va_api_version], [0.34.0])

# libdrm minimum version requirement
m4_define([libdrm_version], [2.4.45])

AC_PREREQ([2.60])
AC_INIT([liva_wrapper], [sunxi_cedrus_version],
	[florent.revest@free-electrons.com], [sunxi_cedrus])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([1.9 tar-ustar])

AC_CONFIG_HEADERS([src/config.h])

SUNXI_CEDRUS_MAJOR_VERSION=sunxi_cedrus_major_version
SUNXI_CEDRUS_MINOR_VERSION=sunxi_cedrus_minor_version
SUNXI_CEDRUS_MICRO_VERSION=sunxi_cedrus_micro_version
AC_DEFINE([SUNXI_CEDRUS_MAJOR_VERSION], [sunxi_cedrus_major_version], [Major version of the driver])
AC_DEFINE([SUNXI_CEDRUS_MINOR_VERSION], [sunxi_cedrus_minor_version], [Minor version of the driver])
AC_DEFINE([SUNXI_CEDRUS_MICRO_VERSION], [sunxi_cedrus_micro_version], [Micro version of the driver])
AC_DEFINE([SUNXI_CEDRUS_PRE_VERSION], [sunxi_cedrus_pre_version], [Preversion of the driver])

SUNXI_CEDRUS_LT_LDFLAGS="-avoid-version"
AC_SUBST(SUNXI_CEDRUS_LT_LDFLAGS)

dnl Use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [
    AM_DEFAULT_VERBOSITY=1
    AC_SUBST(AM_DEFAULT_VERBOSITY)
])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AS

AC_C_BIGENDIAN
AC_HEADER_STDC
AC_SYS_LARGEFILE
AC_CHECK_LIB([m], [sin])

LIBVA_PACKAGE_VERSION=libva_package_version
AC_SUBST(LIBVA_PACKAGE_VERSION)

dnl Check for recent enough DRM
LIBDRM_VERSION=libdrm_version
PKG_CHECK_MODULES([DRM], [libdrm >= $LIBDRM_VERSION])
AC_SUBST(LIBDRM_VERSION)

dnl Check for VA-API
PKG_CHECK_MODULES(LIBVA_DEPS,     [libva >= va_api_version])
PKG_CHECK_MODULES(X11_DEPS, [x11])

dnl Check for VA/DRM API
PKG_CHECK_MODULES(LIBVA_DRM_DEPS, [libva-drm],
  [AC_DEFINE([HAVE_VA_DRM], [1], [Defined to 1 if VA/DRM API is enabled])],
  [USE_DRM="no"])

# Check for <drm_fourcc.h>
if test "$USE_DRM" = "yes"; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $DRM_CFLAGS"
    AC_CHECK_HEADERS([drm_fourcc.h], [:], [USE_DRM="no"])
    CPPFLAGS="$saved_CPPFLAGS"
fi

VA_VERSION=`$PKG_CONFIG --modversion libva`
VA_MAJOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f1`
VA_MINOR_VERSION=`echo "$VA_VERSION" | cut -d'.' -f2`
VA_MICRO_VERSION=`echo "$VA_VERSION" | cut -d'.' -f3`
VA_VERSION_STR="$VA_VERSION"

va_full_version_int=`expr ${VA_MAJOR_VERSION:-0} "*" 1000000 + \
                          ${VA_MINOR_VERSION:-0} "*" 10000   + \
                          ${VA_MICRO_VERSION:-0} "*" 100     + \
                          0`
VA_DRIVER_INIT_FUNC="__vaDriverInit_${VA_MAJOR_VERSION}_${VA_MINOR_VERSION}"
AC_DEFINE_UNQUOTED([VA_DRIVER_INIT_FUNC], [$VA_DRIVER_INIT_FUNC],
    [Define driver entry-point])

dnl Check for VA-API drivers path
AC_MSG_CHECKING([for VA drivers path])
LIBVA_DRIVERS_PATH=`$PKG_CONFIG libva --variable driverdir`
if test -z "$LIBVA_DRIVERS_PATH"; then
    LIBVA_DRIVERS_PATH="/usr/lib/dri/"
fi
AC_MSG_RESULT([$LIBVA_DRIVERS_PATH])
AC_SUBST(LIBVA_DRIVERS_PATH)

AC_OUTPUT([
    Makefile
    src/Makefile
])

echo
echo $PACKAGE configuration summary:
echo
echo VA-API version ................... : $VA_VERSION_STR
echo VA-API drivers path .............. : $LIBVA_DRIVERS_PATH
echo
