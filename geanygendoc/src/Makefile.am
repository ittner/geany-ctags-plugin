
lib_LTLIBRARIES = geanygendoc.la
geanygendoc_la_SOURCES = ggd.c \
                         ggd-doc-setting.c \
                         ggd-doc-type.c \
                         ggd-file-type.c \
                         ggd-file-type-loader.c \
                         ggd-file-type-manager.c \
                         ggd-tag-utils.c \
                         ggd-options.c \
                         ggd-plugin.c \
                         ggd-utils.c
noinst_HEADERS         = ggd.h \
                         ggd-doc-setting.h \
                         ggd-doc-type.h \
                         ggd-file-type.h \
                         ggd-file-type-loader.h \
                         ggd-file-type-manager.h \
                         ggd-tag-utils.h \
                         ggd-macros.h \
                         ggd-options.h \
                         ggd-plugin.h \
                         ggd-utils.h
geanygendoc_la_LDFLAGS = -module -avoid-version
geanygendoc_la_LIBADD  = @GEANY_LIBS@ \
                         @GLIB_LIBS@ \
                         @GIO_LIBS@ \
                         @GTK_LIBS@ \
                         @CTPL_LIBS@ \
                         $(INTLLIBS)

AM_CFLAGS = @GEANY_CFLAGS@ \
            @GLIB_CFLAGS@ \
            @GIO_CFLAGS@ \
            @GTK_CFLAGS@ \
            @CTPL_CFLAGS@ \
            -DLOCALEDIR=\""$(localedir)"\"
