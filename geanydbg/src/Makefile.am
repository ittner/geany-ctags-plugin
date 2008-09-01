
lib_LTLIBRARIES = geanydbg.la
geanydbg_la_SOURCES = dbg.c
geanydbg_la_LDFLAGS = -module -avoid-version
geanydbg_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
