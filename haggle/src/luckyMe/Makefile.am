bin_PROGRAMS=luckyMe
luckyMe_SOURCES=luckyme.c
EXTRA_DIST=luckyme.h Android.mk

luckyMe_CFLAGS =-std=gnu99
luckyMe_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
luckyMe_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2 -lm

all-local:

clean-local:
	rm -f *~
