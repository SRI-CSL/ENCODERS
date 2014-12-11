.PHONY: test testmetadata

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I$(LIBCPPHAGGLE_DIR)include/ -I.. $(XML_CPPFLAGS)
LDFLAGS = $(XML_LIBS)
if OS_LINUX
LDFLAGS += -lpthread
endif

bin_PROGRAMS=metadata

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

metadata_SOURCES=metadata.cpp
metadata_DEPENDENCIES=$(STDDEPS)

LDADD=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a

if OS_MACOSX
LDFLAGS += -framework IOKit -framework CoreFoundation -framework CoreServices
endif

test: testmetadata

testmetadata: metadata
	@./metadata && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
