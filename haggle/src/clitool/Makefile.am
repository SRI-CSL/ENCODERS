bin_PROGRAMS=clitool
clitool_SOURCES=clitool.cpp

clitool_CFLAGS =-std=gnu99
clitool_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
clitool_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2

EXTRA_DIST=Android.mk

all-local:

clean-local:
	rm -f *~
