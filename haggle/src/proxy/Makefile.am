bin_PROGRAMS=hagglepop hagglesmtp haggleproxy mmpd
hagglepop_SOURCES=hagglepop.cpp popserver.cpp popd.cpp databuf.cpp mini_base64.c
hagglesmtp_SOURCES=hagglesmtp.cpp smtpserver.cpp smtpd.cpp databuf.cpp mini_base64.c
haggleproxy_SOURCES=haggleproxy.cpp popserver.cpp popd.cpp smtpserver.cpp smtpd.cpp databuf.cpp mini_base64.c
mmpd_SOURCES=mmpd.cpp

CPPFLAGS +=-I$(top_builddir)/src/libhaggle/include 
CPPFLAGS +=-I$(top_builddir)/src/utils 
LDFLAGS +=-lhaggle -L$(top_builddir)/src/libhaggle/
LDFLAGS +=-lhaggleutils -L$(top_builddir)/src/utils/
LDFLAGS +=-lxml2

EXTRA_DIST = databuf.h mailattr.h mini_base64.h popd.h popserver.h smtpd.h smtpserver.h README

if OS_MACOSX
if OS_IPHONEOS
else
LDFLAGS +=-framework CoreServices
endif
endif
CFLAGS += -std=gnu99

all-local:

clean-local:
	rm -f *~
