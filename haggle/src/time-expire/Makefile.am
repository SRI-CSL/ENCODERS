bin_PROGRAMS=time_expire_data

time_expire_data_SOURCES=time_expire_data.c

time_expire_data_CFLAGS =-std=gnu99
time_expire_data_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
time_expire_data_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2

EXTRA_DIST=Android.mk

all-local:

clean-local:
	rm -f *~
