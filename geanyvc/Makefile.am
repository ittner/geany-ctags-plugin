## Makefile.am -- Process this file with automake to produce Makefile.in
SUBDIRS = po

EXTRA_DIST =				\
	autogen.sh				\
	makefile.win32			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	indent-all.sh           \
	THANKS                  \
	commit.glade

dist_noinst_SCRIPTS = indent-all.sh

noinst_HEADERS = geanyvc.h

lib_LTLIBRARIES = geanyvc.la
geanyvc_la_SOURCES = geanyvc.c externdiff.c vc_git.c vc_cvs.c vc_svn.c vc_svk.c vc_bzr.c vc_hg.c
geanyvc_la_LDFLAGS = -module -avoid-version
geanyvc_la_LIBADD  = @GEANY_LIBS@

AM_CPPFLAGS = @GEANY_CFLAGS@ -DGEANY_DISABLE_DEPRECATED -DLOCALEDIR=\""$(localedir)"\"


.PHONY: ChangeLog mrproper

ChangeLog:
	git log > ChangeLog

mrproper:
	git clean -d -x -f


