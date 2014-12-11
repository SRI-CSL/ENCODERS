bin_PROGRAMS=gps_test gps_person now

gps_test_SOURCES=gps_test.c

gps_test_CFLAGS =-std=gnu99
gps_test_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
gps_test_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2

gps_person_SOURCES=gps_person.c

gps_person_CFLAGS =-std=gnu99
gps_person_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include -I$(top_builddir)/src/utils 
gps_person_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lhaggleutils -L$(top_builddir)/src/utils/ -lxml2

now_SOURCES=now.c

now_CFLAGS =-std=gnu99
now_CPPFLAGS =-I$(top_builddir)/src/utils 
now_LDFLAGS =-lhaggleutils -L$(top_builddir)/src/utils/ 

EXTRA_DIST=Android.mk

all-local:

clean-local:
	rm -f *~
