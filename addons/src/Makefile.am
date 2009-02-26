
lib_LTLIBRARIES = addons.la
addons_la_SOURCES = addons.c addons.h ao_doclist.c ao_doclist.h ao_openuri.c ao_openuri.h
addons_la_LDFLAGS = -module -avoid-version
addons_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\" -DDOCDIR=\""$(docdir)"\"
