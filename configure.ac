AC_INIT
AC_CONFIG_SRCDIR([Makefile.am])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_PREREQ(2.53)

AM_INIT_AUTOMAKE([modplugtools],[0.5.3])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER([config.h])

AC_USE_SYSTEM_EXTENSIONS

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$cflags_save"

AC_PROG_CXX
AC_PROG_LIBTOOL

dnl --------------------------------------------------
dnl System checks
dnl --------------------------------------------------

AC_SYS_LARGEFILE
AC_C_BIGENDIAN

HAVE_MODPLUG=no
dnl first check through pkg-config
dnl check for pkg-config itself so we don't try the m4 macro without pkg-config
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
 PKG_CHECK_MODULES(LIBMODPLUG, libmodplug >= 0.8.8, HAVE_MODPLUG=yes, HAVE_MODPLUG=no)
 AC_SUBST(LIBMODPLUG_CFLAGS)
 AC_SUBST(LIBMODPLUG_LIBS)
fi

if test "x$HAVE_MODPLUG" = "xno"
then
	AC_MSG_ERROR(libmodplug >= 0.8.8 required !)
fi

PKG_CHECK_MODULES(AO, ao >= 0.8.0,,build_modplug123=no; AC_MSG_WARN(libao too old; >= 0.8.0 required))
#ACX_PTHREAD(,build_modplug123=no; AC_MSG_WARN(POSIX threads missing))


AC_TYPE_INT8_T
AC_TYPE_UINT8_T
AC_TYPE_INT16_T
AC_TYPE_UINT16_T
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T

AC_CHECK_HEADERS([fcntl.h sys/ioctl.h sys/time.h ])


AC_CONFIG_FILES([Makefile
          mpplay/Makefile
          mp123/Makefile])
AC_OUTPUT


