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

# Explicit initialize pkg-config for conditional PKG_CHECK_MODULE
# see http://www.flameeyes.eu/autotools-mythbuster/autoconf/macros.html
# and http://www.flameeyes.eu/autotools-mythbuster/ \
#		pkgconfig/pkg_check_modules.html
PKG_PROG_PKG_CONFIG

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

AC_ARG_ENABLE([mbaf],
	[ --disable-mbaf	Turn off mbaf],
[case "${enableval}" in
  yes) mbaf=true ;;
  no) mbaf=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-mbaf]) ;;
esac],[mbaf=true])

AC_ARG_WITH([backend],
        [AS_HELP_STRING([--with-backend=FOO],[Use FOO as display backend (default is 'X')])],
        [case "${withval}" in
            ('X') backend='X'; default_graphic_engine="cairo" ;;
            ('dfb') backend='dfb'; default_graphic_engine="dfb" ;;
            ('no') backend='none'; default_graphic_engine="cairo" ;;
            (*) AC_MSG_ERROR([bad value ${withval} for --with-backend]) ;;
        esac],[backend='X'; default_graphic_engine="cairo"])

AC_ARG_WITH([graphic-engine],
	[AS_HELP_STRING([--with-graphic-engine=FOO],
			[Use FOO as graphic engine (default is '${default_graphic_engine}')])],
	[case "${withval}" in
	    ('cairo') graphic_engine="cairo" ;;
	    ('skia') graphic_engine="skia" ;;
	    ('openvg') graphic_engine="openvg" ;;
	    ('dfb') graphic_engine="dfb" ;;
	    (*) AC_MSG_ERROR([bad value ${withval} for --with-graphic-engine])
	    	;;
	esac], [graphic_engine="${default_graphic_engine}"])

AC_ARG_WITH([image-loader],
	[AS_HELP_STRING([--with-image-loader=FOO],
			[Use FOO as image loader (default is 'cairo')])],
	[case "${withval}" in
	    ('cairo') image_loader="cairo" ;;
	    ('imlib2') image_loader="imlib2" ;;
	    ('no') image_loader="none" ;;
	    (*) AC_MSG_ERROR([bad value ${withval} for --with-image-loader])
	    	;;
	esac], [image_loader="cairo"])

xshm=false
[if [ x"${backend}" = x"X" ]; then ]
AC_ARG_ENABLE([xshm],
	[AS_HELP_STRING([--disable-xshm],[Turn off XSHM supporting])],
	[case "${enableval}" in
	    (yes) xshm=true ;;
	    (no) xshm=false ;;
	    (*) AC_MSG_ERROR([bad value ${enableval} for --disable-xshm]) ;;
	esac], [xshm=true])
[fi]

# Validate options
[case "${backend}-${graphic_engine}-${image_loader}" in
    X-cairo-*|X-openvg-imlib2) ;;
    dfb-cairo-*) ;;
    none-*-*) ;;
    *)] AC_MSG_ERROR([The combination of --with-backend=${backend}, --with-graphic-engine=${graphic_engine} and --with-image-loader=${image_loader} is invalid]) [;;
esac]

[if [ x"${graphic_engine}" = x"skia" ]; then ]
    AC_DEFINE([SKIA_GRAPH_ENGINE])
[elif [ x"${graphic_engine}" = x"openvg" ]; then ]
    AC_DEFINE([OPENVG_GRAPH_ENGINE])
[elif [ x"${graphic_engine}" = x"dfb" ]; then ]
    AC_DEFINE([DFB_GRAPH_ENGINE])
[else]
    AC_DEFINE([CAIRO_GRAPH_ENGINE])
[fi]

AM_CONDITIONAL([SKIA_GRAPH_ENGINE], [test x"${graphic_engine}" = x"skia"])
AM_CONDITIONAL([OPENVG_GRAPH_ENGINE], [test x"${graphic_engine}" = x"openvg"])
AM_CONDITIONAL([DFB_GRAPH_ENGINE], [test x"${graphic_engine}" = x"dfb"])
AM_CONDITIONAL([CAIRO_GRAPH_ENGINE], [test x"${graphic_engine}" = x"cairo"])
AM_CONDITIONAL([MBAF], [test x"$mbaf" = xtrue])

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

AM_CONDITIONAL([SH_TEXT],
	[test x$sh_text = xtrue -a x"${graphic_engine}" = x"cairo"])

[if [ x"${sh_text}" = xtrue -a x"${graphic_engine}" = x"cairo" ]; then]
    AC_DEFINE([SH_TEXT])
[fi]

# Define AM and AC variable for display backend

AM_CONDITIONAL([X_BACKEND],
	[test x$backend = x'X'])

[if [ x"${backend}" = x'X' ]; then]
    AC_DEFINE([X_BACKEND])
[fi]

AM_CONDITIONAL([DFB_BACKEND],
	[test x$backend = x'dfb' -a x"${graphic_engine}" = x"cairo"])

[if [ x"${backend}" = x'dfb' -a x"${graphic_engine}" = x"cairo" ]; then]
    AC_DEFINE([DFB_BACKEND])
[fi]

AM_CONDITIONAL([CAIRO_IMG_LOADER],
	[test x"${image_loader}" = x"cairo"])

[if [ x"${image_loader}" = x"cairo" ]; then]
    AC_DEFINE([CAIRO_IMG_LOADER])
    [if [ x"${graphic_engine}" != x"cairo" ]; then]
        AC_MSG_ERROR([bad value cairo for --with-image-loader while value of --with-graphic-engine is not cairo])
    [fi]
[fi]

AM_CONDITIONAL([IMLIB2_IMG_LOADER],
	[test x"${image_loader}" = x"imlib2"])

[if [ x"${image_loader}" = x"imlib2" ]; then]
    AC_DEFINE([IMLIB2_IMG_LOADER])
[fi]

AM_CONDITIONAL([XSHM],
	[test x"${xshm}" = xtrue -a x"${graphic_engine}" = x"cairo" -a x$backend = x'X'])

[if [ x"${xshm}" = xtrue -a x"${graphic_engine}" = x"cairo" -a x$backend = x'X' ]; then]
    AC_DEFINE([XSHM])
[fi]

# Checks for libraries.
[if [ x"${graphic_engine}" = x"openvg" ]; then]
AC_CHECK_HEADERS([EGL/egl.h],, [AC_MSG_ERROR([can not find EGL/egl.h])])
[fi]

[if [ x"${image_loader}" = x"imlib2" ]; then]
PKG_CHECK_MODULES([imlib2], [imlib2 >= 1.4.1], , AC_MSG_ERROR([imlib2 >= 1.4.1 not found]))
[fi]

[if [ x"${graphic_engine}" = x"cairo" ]; then]
    PKG_CHECK_MODULES([cairo], [cairo >= 1.6], , AC_MSG_ERROR([cairo >= 1.6 not found]))
    #PKG_CHECK_MODULES([cairo2], [cairo2 >= 2.0], , AC_MSG_ERROR([cairo2 >= 2.0 not found]))
    [if [ x"${sh_text}" = xtrue ]; then]
        PKG_CHECK_MODULES([pangocairo], [pangocairo >= 1.0], , AC_MSG_ERROR([pangocairo >= 1.0 not found: use --disable-sh_text to configure without pangocairo.]))
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
AH_TEMPLATE([OPENVG_GRAPH_ENGINE], [Enable OpenVG Graphic Engine])
AH_TEMPLATE([CAIRO_GRAPH_ENGINE], [Enable Cairo Graphic Engine])
AH_TEMPLATE([DFB_GRAPH_ENGINE], [Enable DirectFB Graphic Engine])
AH_TEMPLATE([CAIRO_IMG_LOADER], [Enable Cairo Image Loader])
AH_TEMPLATE([IMLIB2_IMG_LOADER], [Enable Imlib2 Image Loader])
AH_TEMPLATE([X_BACKEND], [Enable X backend])
AH_TEMPLATE([DFB_BACKEND], [Enable DirectFB backend])
AH_TEMPLATE([XSHM], [Enable XSHM])

AC_OUTPUT
