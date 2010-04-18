
SUBDIRS = src data po

ACLOCAL_AMFLAGS = -I build/m4

EXTRA_DIST = autogen.sh \
             README \
             po/LINGUAS

sign:
	if test -f $(PACKAGE)-$(VERSION).tar.gz; then \
		gpg --detach-sign --digest-algo SHA512 $(PACKAGE)-$(VERSION).tar.gz; fi
	if test -f $(PACKAGE)-$(VERSION).tar.bz2; then \
		gpg --detach-sign --digest-algo SHA512 $(PACKAGE)-$(VERSION).tar.bz2; fi
