
lib_LTLIBRARIES = tasks.la
tasks_la_SOURCES = tasks.c tasks.h
tasks_la_LDFLAGS = -module -avoid-version
tasks_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\" -DDOCDIR=\""$(docdir)"\"
