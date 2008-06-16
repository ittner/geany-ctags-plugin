## Makefile.am -- Process this file with automake to produce Makefile.in
#SUBDIRS = tests

noinst_HEADERS = makefile.win32

lib_LTLIBRARIES = geanylatex.la
geanylatex_la_SOURCES = geanylatex.c letters.c latexencodings.c
geanylatex_la_LDFLAGS = -module -avoid-version
geanylatex_la_LIBADD  = @GEANY_LIBS@

AM_CPPFLAGS = @GEANY_CFLAGS@ -DGEANY_DISABLE_DEPRECATED

GEANY_DIR = $(shell geany --print-prefix | grep lib)

install: .libs/$(PACKAGE).so
	mkdir -p $(DESTDIR)$(GEANY_DIR)/geany
	cp $< $(DESTDIR)$(GEANY_DIR)/geany/
