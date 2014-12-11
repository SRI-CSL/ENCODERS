.PHONY: \
	test \
	testgetputData

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
AM_CPPFLAGS = -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I.. -I$(LIBCPPHAGGLE_DIR)include/ -I$(LIBXML2_INCLUDE_DIR)
AM_LDFLAGS = -lxml2 -lcrypto

if OS_LINUX
AM_LDFLAGS += -lpthread
endif
if OS_MACOSX
AM_LDFLAGS += -framework IOKit -framework CoreFoundation -framework CoreServices
endif

bin_PROGRAMS= \
	getputData

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

getputData_SOURCES=getputData.cpp
getputData_DEPENDENCIES=$(STDDEPS)

LDADD=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a

test: \
	testgetputData

testgetputData: getputData
	@./getputData && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
