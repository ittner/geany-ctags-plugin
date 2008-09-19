## Makefile.am -- Process this file with automake to produce Makefile.in
#SUBDIRS = tests

INCLUDES = -I..

if UNITTESTS
TESTS=unittests
noinst_PROGRAMS=unittests
unittests_SOURCES = unittests.c \
	../utils.c

AM_CFLAGS = @CHECK_CFLAGS@ @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\" -DUNITTESTS

unittests_LDADD  = @GEANY_LIBS@ $(INTLLIBS) @CHECK_LIBS@
endif
