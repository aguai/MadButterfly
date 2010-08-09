#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([MadButterfly],[0.0],[http://www.assembla.com/spaces/MadButterfly])
AC_CONFIG_SRCDIR([README.h])
AC_CONFIG_HEADER([config.h include/mb_config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
LT_INIT
AC_PROG_LIBTOOL

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS([stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([gettimeofday memset select sqrt strdup])

AC_ARG_ENABLE([testcase],
        [AS_HELP_STRING([--enable-testcase],[Enable testcase])],
        [case "${enableval}" in
            (yes) testcase=true ;;
            (no) testcase=false ;;
            (*) AC_MSG_ERROR([bad value ${enableval} for --enable-testcase]) ;;
        esac],[testcase=false])

AM_CONDITIONAL([TESTCASE], [test x$testcase = xtrue])

AC_ARG_ENABLE([sh_text],
        [AS_HELP_STRING([--disable-sh_text],[Turn off sh_text])],
        [case "${enableval}" in
            (yes) sh_text=true ;;
            (no) sh_text=false ;;
            (*) AC_MSG_ERROR([bad value ${enableval} for --disable-sh_text]) ;;
        esac],[sh_text=true])

AC_ARG_ENABLE([sh_stext],
        [AS_HELP_STRING([--disable-sh_stext],[Turn off sh_stext])],
        [case "${enableval}" in
            (yes) sh_stext=true ;;
            (no) sh_stext=false ;;
            (*) AC_MSG_ERROR([bad value ${enableval} for --disable-sh_stext]) ;;
        esac],[sh_stext=true])

AM_CONDITIONAL([SH_STEXT], [test x$sh_stext = xtrue])

[if [ x"${sh_stext}" = xtrue ]; then]
    AC_DEFINE([SH_STEXT])
[fi]

AC_ARG_WITH([backend],
        [AS_HELP_STRING([--with-backend=FOO],[Use FOO as display backend (default is 'X')])],
        [case "${withval}" in
            ('X') backend='X' ;;
            ('dfb') backend='dfb' ;;
            ('no') backend='none' ;;
            (*) AC_MSG_ERROR([bad value ${withval} for --with-backend]) ;;
        esac],[backend='X'])

AC_ARG_ENABLE([skia],
        [AS_HELP_STRING([--enable-skia],[Turn on Skia instead of Cairo])],
        [case "${enableval}" in
            (yes) skia=true ;;
            (no) skia=false ;;
            (*) AC_MSG_ERROR([bad value ${enableval} for --enable-skia]) ;;
        esac],[skia=false])

AM_CONDITIONAL([SKIA_GRAPH_ENGINE], [test x$skia = xtrue])
AM_CONDITIONAL([CAIRO_GRAPH_ENGINE], [test x$skia != xtrue])

[if [ x"${skia}" = xtrue ]; then ]
    AC_DEFINE([SKIA_GRAPH_ENGINE])
    cairo=false
[else]
    AC_DEFINE([CAIRO_GRAPH_ENGINE])
    cairo=true
[fi]

AC_ARG_ENABLE([nodejs],
        [AS_HELP_STRING([--enable-nodejs],[Turn on nodejs support])],
        [case "${enableval}" in
            (yes) nodejs=true ;;
            (no) nodejs=false ;;
            (*) AC_MSG_ERROR([bad value ${enableval} for --enable-nodejs]) ;;
        esac],[nodejs=false])

[if [ x"${nodejs}" = xtrue ]; then ]
    AC_PATH_PROG([NODE_WAF], [node-waf])
    AC_CHECK_PROG([has_node], [node], [true], [false])
[fi]

AM_CONDITIONAL([NODEJS], [test x"${nodejs}-${has_node}" = xtrue-true])

# Define AM and AC variable for sh_text

AM_CONDITIONAL([SH_TEXT], [test x$sh_text = xtrue -a x$cairo = xtrue])

[if [ x"${sh_text}" = xtrue -a x$cairo = xtrue ]; then]
    AC_DEFINE([SH_TEXT])
[fi]

# Define AM and AC variable for display backend

AM_CONDITIONAL([X_BACKEND], [test x$backend = x'X' -a x$cairo = xtrue])

[if [ x"${backend}" = x'X' -a x"${cairo}" = xtrue ]; then]
    AC_DEFINE([X_BACKEND])
[fi]

AM_CONDITIONAL([DFB_BACKEND], [test x$backend = x'dfb' -a x$cairo = xtrue])

[if [ x"${backend}" = x'dfb' -a x"${cairo}" = xtrue ]; then]
    AC_DEFINE([DFB_BACKEND])
[fi]

# Checks for libraries.

[if [ x"${cairo}" = xtrue ]; then]
    PKG_CHECK_MODULES([cairo], [cairo >= 1.6], , AC_MSG_ERROR([cairo >= 1.6 not found]))
    [if [ x"${sh_text}" = xtrue ]; then]
        PKG_CHECK_MODULES([pangocairo], [pangocairo >= 1.0], , AC_MSG_ERROR([pangocairo >= 1.0 not found]))
    [fi]
[fi]

AC_CONFIG_FILES([
        Makefile
        libmbfly.pc
        libmbfly-uninstalled.pc
        examples/Makefile
        examples/calculator/Makefile
        examples/svg2code_ex/Makefile
        examples/tank/Makefile
        examples/drag/Makefile
        examples/dynamic/Makefile
        examples/menu/Makefile
        src/Makefile
        include/Makefile
        inkscape/Makefile
        nodejs/Makefile
        tools/Makefile
        ])

AH_TEMPLATE([SH_TEXT],[Enable sh_text object])
AH_TEMPLATE([SH_STEXT],[Enable sh_stext object])
AH_TEMPLATE([SKIA_GRAPH_ENGINE], [Enable Skia Graphic Engine])
AH_TEMPLATE([CAIRO_GRAPH_ENGINE], [Enable Cairo Graphic Engine])
AH_TEMPLATE([X_BACKEND], [Enable X backend])
AH_TEMPLATE([DFB_BACKEND], [Enable DirectFB backend])

AC_OUTPUT
