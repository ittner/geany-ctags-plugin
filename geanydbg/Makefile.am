SUBDIRS = src po tests

EXTRA_DIST =				\
	autogen.sh				\
	makefile.win32			\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	indent-all.sh

DISTCLEANFILES =			\
	intltool-extract		\
	intltool-merge			\
	intltool-update

.PHONY: ChangeLog mrproper

ChangeLog:
	git log . > ChangeLog

mrproper:
	git clean -d -x -f
