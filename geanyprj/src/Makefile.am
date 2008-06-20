
lib_LTLIBRARIES = geanyprj.la
geanyprj_la_SOURCES = geanyprj.c geanyprj.h menu.c project.c sidebar.c utils.c xproject.c
geanyprj_la_LDFLAGS = -module -avoid-version
geanyprj_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\"

