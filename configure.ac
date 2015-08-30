#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(ulppk-demo, 1.0.0, ulppk@garveybz.com, http://www.garveybz.com/swdev/ulppk-home.php)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE(ulppk-demo, 1.0.0)
AC_CONFIG_SRCDIR([src/demoserver.c])
AC_CONFIG_SRCDIR([demolibs/democonfig.c])
AC_CONFIG_HEADER([config.h])

# Options
AC_DISABLE_STATIC

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Variables
AC_SUBST(LIBVERSION, [1:0:0])

# Checks for header files.
AC_CHECK_HEADER([fcntl.h])

AC_SUBST(demolibs_inc)
demolibs_inc='-I$(top_srcdir)/demolibs'
AC_SUBST(demolibs_libflags)
demolibs_libflags='-L$(top_srcdir)/demolibs/.libs -Wl,--rpath -Wl,$(top_srcdir)/demolibs/.libs -Wl,-unresolved-symbols=ignore-in-shared-libs'

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL


AC_CONFIG_FILES([Makefile
		demolibs/Makefile
		src/Makefile])

AC_OUTPUT
