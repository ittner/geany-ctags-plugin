
lib_LTLIBRARIES = geanydebug.la
geanydebug_la_SOURCES = gdb-io-break.c \
    gdb-io-envir.c \
    gdb-io-frame.c \
    gdb-io-priv.h \
    gdb-io-read.c \
    gdb-io-run.c \
    gdb-io-stack.c \
    gdb-io.h \
    gdb-lex.c \
    gdb-lex.h \
    gdb-ui-break.c \
    gdb-ui-envir.c \
    gdb-ui-frame.c \
    gdb-ui-locn.c \
    gdb-ui-main.c \
    gdb-ui.h \
    geanydebug.c

geanydebug_la_LDFLAGS = -module -avoid-version
geanydebug_la_LIBADD  = @GEANY_LIBS@  $(INTLLIBS)

bin_PROGRAMS = geanydebug_ttyhelper
geanydebug_ttyhelper_SOURCES = ttyhelper.c

AM_CFLAGS = @GEANY_CFLAGS@ -DLOCALEDIR=\""$(localedir)"\" -DPREFIX=\""$(prefix)"\"

