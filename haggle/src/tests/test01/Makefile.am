bin_PROGRAMS=test1
test1_SOURCES=test1.cpp

CPPFLAGS += -I$(top_builddir)/src/libhaggle/include 
CPPFLAGS += -I$(top_builddir)/src/utils 
CPPFLAGS += -I$(top_builddir)/src/libcpphaggle
LDFLAGS += -lhaggle -L$(top_builddir)/src/libhaggle/
LDFLAGS += -lhaggleutils -L$(top_builddir)/src/utils/
LDFLAGS += -lcpphaggle -L$(top_builddir)/src/libcpphaggle
LDFLAGS +=-lxml2

all-local:

clean-local:
	rm -f *~
