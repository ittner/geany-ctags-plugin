include $(top_srcdir)/build/vars.build.mk

if ENABLE_CTAGS
geanyplugins_LTLIBRARIES = ctags.la
else
EXTRA_LTLIBRARIES = ctags.la
endif

ctags_la_SOURCES = ctags.c readtags.c readtags.h
ctags_la_LIBADD = $(COMMONLIBS)

include $(top_srcdir)/build/cppcheck.mk
