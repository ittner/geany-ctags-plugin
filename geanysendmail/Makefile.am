## Makefile.am -- Process this file with automake to produce Makefile.in
#SUBDIRS = tests

lib_LTLIBRARIES = geanysendmail.la
geanysendmail_la_SOURCES = geanysendmail.c
geanysendmail_la_LDFLAGS = -module -avoid-version
geanysendmail_la_LIBADD  = @GEANY_LIBS@

AM_CPPFLAGS = @GEANY_CFLAGS@

GEANY_DIR = $(shell geany --print-prefix | grep lib)

install: .libs/$(PACKAGE).so
	mkdir -p $(DESTDIR)$(GEANY_DIR)/geany
	cp $< $(DESTDIR)$(GEANY_DIR)/geany/
