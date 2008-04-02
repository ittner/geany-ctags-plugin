
lib_LTLIBRARIES = instantsave.la
instantsave_la_SOURCES = instantsave.c
instantsave_la_LDFLAGS = -module -avoid-version
instantsave_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
