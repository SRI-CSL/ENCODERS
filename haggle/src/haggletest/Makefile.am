bin_PROGRAMS=haggletest
haggletest_SOURCES=haggletest.c

haggletest_CFLAGS =-std=gnu99
haggletest_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
haggletest_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2

EXTRA_DIST=Android.mk

all-local:

clean-local:
	rm -f *~
