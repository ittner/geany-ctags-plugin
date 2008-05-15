
lib_LTLIBRARIES = spellcheck.la
spellcheck_la_SOURCES = spellcheck.c
spellcheck_la_LDFLAGS = -module -avoid-version
spellcheck_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS) @SPELLER_LIB@

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
