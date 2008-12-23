
lib_LTLIBRARIES = externdbg.la
externdbg_la_SOURCES = dbg.c
externdbg_la_LDFLAGS = -module -avoid-version
externdbg_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
