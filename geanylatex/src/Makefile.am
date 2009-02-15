lib_LTLIBRARIES = geanylatex.la
geanylatex_la_SOURCES = geanylatex.c geanylatex.h datatypes.h latexencodings.c latexencodings.h letters.c letters.h bibtex.h bibtex.c bibtexlabels.c reftex.c reftex.h latexutils.c latexutils.h formatutils.c formatutils.h formatpatterns.c

geanylatex_la_LDFLAGS = -module -avoid-version
geanylatex_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"
