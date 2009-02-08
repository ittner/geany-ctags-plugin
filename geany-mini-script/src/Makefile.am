## Makefile.am -- Process this file with automake to produce Makefile.in
lib_LTLIBRARIES = geany-mini-script.la
geany_mini_script_la_SOURCES  = gms.c  gms_gui.c
geany_mini_script_la_LDFLAGS  = -module -avoid-version
geany_mini_script_la_LIBADD   = @GEANY_LIBS@

AM_CPPFLAGS     = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
