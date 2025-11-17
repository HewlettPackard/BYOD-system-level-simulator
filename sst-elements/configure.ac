AC_INIT([byod], [1.0.0], [fabian.bohm@hpe.com])
AC_PREREQ([2.59])
AC_COPYRIGHT([@ Copyright Hewlett Packard Enterprise Development LP, 2025])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIRS([config])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADER([src_cpp/sst_byod_config.h])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_C_INLINE
AC_PROG_MAKE_SET

AC_PROG_CXX
AC_CACHE_SAVE

AC_PROG_CPP
AC_CANONICAL_HOST

AC_CACHE_SAVE
AC_HEADER_TIME
AM_INIT_AUTOMAKE([foreign subdir-objects])

LT_CONFIG_LTDL_DIR([src_cpp/libltdl])
LT_INIT([shared disable-static dlopen])
LTDL_INIT([recursive])


AC_HEADER_TIME
AC_CACHE_SAVE

SST_CORE_CHECK_INSTALL()

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
