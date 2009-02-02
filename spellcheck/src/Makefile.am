
lib_LTLIBRARIES = spellcheck.la
spellcheck_la_SOURCES = speller.c speller.h scplugin.c scplugin.h gui.c gui.h
spellcheck_la_LDFLAGS = -module -avoid-version
spellcheck_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS) @ENCHANT_LIBS@

AM_CFLAGS = @GEANY_CFLAGS@ @ENCHANT_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\" -DDOCDIR=\""$(docdir)"\"
