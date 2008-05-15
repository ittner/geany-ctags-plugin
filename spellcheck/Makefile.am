
SUBDIRS = src po

EXTRA_DIST =				\
	autogen.sh				\
	makefile.win32			\
	po/LINGUAS

dist-bzip2: distdir
	BZIP2=$(BZIP2_ENV) $(AMTAR) --bzip2 -chof $(distdir).tar.bz2 $(distdir)
	-rm -rf $(distdir)

sign:
	if test -f $(PACKAGE)-$(VERSION).tar.gz; then \
		gpg --detach-sign --digest-algo SHA512 $(PACKAGE)-$(VERSION).tar.gz; fi
	if test -f $(PACKAGE)-$(VERSION).tar.bz2; then \
		gpg --detach-sign --digest-algo SHA512 $(PACKAGE)-$(VERSION).tar.bz2; fi

