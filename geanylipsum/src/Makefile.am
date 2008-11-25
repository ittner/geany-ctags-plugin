
lib_LTLIBRARIES = geanylipsum.la
geanylipsum_la_SOURCES = geanylipsum.c
geanylipsum_la_LDFLAGS = -module -avoid-version
geanylipsum_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
