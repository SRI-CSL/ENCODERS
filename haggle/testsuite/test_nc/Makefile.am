.PHONY: \
	test \
	testnc \
	testncmanager \
	testncmanagerconfig

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBHAGGLE_DIR=$(top_srcdir)/src/libhaggle/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
LIBXML2_INCLUDE_DIR=/usr/include/libxml2
CPPFLAGS = -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I.. -I$(LIBCPPHAGGLE_DIR)include/ -I$(LIBXML2_INCLUDE_DIR)  
CPPFLAGS += -I/usr/include -DDEBUG
CPPFLAGS += -Wall -pedantic -fstack-protector-all -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I$(LIBHAGGLE_DIR)include/ -I$(LIBCPPHAGGLE_DIR)include/ -I..
LDFLAGS = -lpthread -lxml2 -lcrypto -lsqlite3 
AM_LDFLAGS = -DDEBUG
AM_CPPFLAGS =




if OS_LINUX
AM_LDFLAGS += 
endif
if OS_MACOSX
AM_LDFLAGS += -framework IOKit -framework CoreFoundation -framework CoreServices
endif

bin_PROGRAMS= \
	nctest \
	ncmanagertest \
	ncmanagerconfigtest

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

nctest_SOURCES=nctest.cpp
nctest_DEPENDENCIES=$(STDDEPS)
ncmanagertest_SOURCES=ncmanagertest.cpp
ncmanagertest_DEPENDENCIES=$(STDDEPS)
ncmanagerconfigtest_SOURCES=ncmanagerconfigtest.cpp
ncmanagerconfigtest_DEPENDENCIES=$(STDDEPS)


LDADD=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a
LDADD+=../hagglemain.o
LDADD+= -lcrypto -lpthread -lxml2 -lsqlite3 


test: \
	testnc \
	testncmanager \
	testncmanagerconfig

testnc: nctest
	@./getputData && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
