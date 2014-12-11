bin_PROGRAMS=hagglebbs
hagglebbs_SOURCES=hagglebbs.cpp httpd.cpp databuf.cpp mini_base64.c

CPPFLAGS +=-I$(top_builddir)/src/libhaggle/include 
CPPFLAGS +=-I$(top_builddir)/src/utils 
LDFLAGS +=-lhaggle -L$(top_builddir)/src/libhaggle/
LDFLAGS +=-lhaggleutils -L$(top_builddir)/src/utils/
LDFLAGS +=-lxml2

CFLAGS += -std=gnu99

EXTRA_DIST = databuf.h httpd.h mini_base64.h

all-local:

clean-local:
	rm -f *~
