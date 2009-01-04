## Makefile.am -- Process this file with automake to produce Makefile.in
lib_LTLIBRARIES = gms.la
gms_la_SOURCES  = gms.c  gms_gui.c
gms_la_LDFLAGS  = -module -avoid-version
gms_la_LIBADD   = @GEANY_LIBS@

AM_CPPFLAGS     = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
