
SUBDIRS = src po

DOCDIR = $(DESTDIR)$(docdir)

EXTRA_DIST = autogen.sh po/LINGUAS

dist-bzip2: distdir
	BZIP2=$(BZIP2_ENV) $(AMTAR) --bzip2 -chof $(distdir).tar.bz2 $(distdir)
	-rm -rf $(distdir)

sign:
	if test -f $(PACKAGE)-$(VERSION).tar.gz; then \
		gpg --detach-sign --digest-algo SHA512 $(PACKAGE)-$(VERSION).tar.gz; fi
	if test -f $(PACKAGE)-$(VERSION).tar.bz2; then \
		gpg --detach-sign --digest-algo SHA512 $(PACKAGE)-$(VERSION).tar.bz2; fi

html:
	rst2html README index.html

install-data-local:
	$(mkinstalldirs) $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/README $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/ChangeLog $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/NEWS $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/COPYING $(DOCDIR)
	$(INSTALL_DATA) $(top_srcdir)/AUTHORS $(DOCDIR)

uninstall-local:
	rm -rf $(DOCDIR)
