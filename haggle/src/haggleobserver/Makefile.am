bin_PROGRAMS=haggleobserver
haggleobserver_SOURCES=haggleobserver.c

haggleobserver_CFLAGS =-std=gnu99
haggleobserver_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
haggleobserver_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2

EXTRA_DIST=Android.mk

all-local:

clean-local:
	rm -f *~
