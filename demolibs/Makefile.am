AM_CFLAGS = -g  -O0 -fPIC -I. -I/usr/local/include/ulppk -DULPPK_DEBUG
AM_LDFLAGS = -ldl -lulppk 

lib_LTLIBRARIES=libdemolibs.la
libdemolibs_la_SOURCES = democonfig.c
 
libdemolibs_la_LDFLAGS = -release @PACKAGE_VERSION@ -version-info @LIBVERSION@

pkginclude_HEADERS = democonfig.h
