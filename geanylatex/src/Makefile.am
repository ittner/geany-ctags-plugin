lib_LTLIBRARIES = geanylatex.la
geanylatex_la_SOURCES = geanylatex.c datatypes.h latexencodings.c latexencodings.h letters.c letters.h
geanylatex_la_LDFLAGS = -module -avoid-version
geanylatex_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
