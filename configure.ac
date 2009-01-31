#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([MadButterfly],[0.0],[http://www.assembla.com/spaces/MadButterfly])
AC_CONFIG_SRCDIR([README.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC
LT_INIT
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([cairo], [cairo >= 0.22], , AC_MSG_ERROR([cairo >= 0.22 not found]))
PKG_CHECK_MODULES([pangocairo], [pangocairo >= 0.22], , AC_MSG_ERROR([pangocairo >= 0.22 not found]))

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

AC_CONFIG_FILES([Makefile
                 libmbfly.pc
                 examples/Makefile
                 examples/calculator/Makefile
                 examples/svg2code_ex/Makefile
                 examples/tank/Makefile
                 examples/drag/Makefile
                 examples/dynamic/Makefile
                 src/Makefile
                 include/Makefile
                 inkscape/Makefile
                 tools/Makefile])
AC_OUTPUT
