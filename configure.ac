#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([VERSION_H],m4_esyscmd([cat version.h]))
changequote({{,}})dnl
m4_define({{XX_VERSION}},m4_bregexp(VERSION_H,{{^const.*"\(.+\)";}},{{\1}}))
changequote([,])dnl

AC_INIT([aacelem], [XX_VERSION], [honeycomb77@gmail.com])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h memory.h stddef.h stdint.h stdlib.h string.h strings.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_CHECK_FUNC(getopt)
AM_CONDITIONAL([XX_NO_GETOPT],[test "$ac_cv_func_getopt" != "yes"])

AC_CANONICAL_HOST

X_PLATFORM=posix
case ${host} in
*-*-mingw*)
    X_PLATFORM=win32
    ;;
esac

AM_CONDITIONAL([XX_PLATFORM_POSIX],[test "$X_PLATFORM" = "posix"])
AM_CONDITIONAL([XX_PLATFORM_WIN32],[test "$X_PLATFORM" = "win32"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
