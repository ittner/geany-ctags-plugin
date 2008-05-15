
lib_LTLIBRARIES = geanydoc.la
geanydoc_la_SOURCES = config.c geanydoc.c geanydoc.h
geanydoc_la_LDFLAGS = -module -avoid-version
geanydoc_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
