.PHONY: test testtimeval testrefcount testnewmap testnewlist teststringimpl

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I.. -I$(LIBCPPHAGGLE_DIR)include/
LDFLAGS = -lxml2
if OS_LINUX
LDFLAGS += -lpthread
endif

bin_PROGRAMS=timeval refcount newmap newlist stringimpl

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
STDDEPS+=../libtesthlp.a

timeval_SOURCES=timeval.cpp
timeval_DEPENDENCIES=$(STDDEPS)

refcount_SOURCES=refcount.cpp
refcount_DEPENDENCIES=$(STDDEPS)

newmap_SOURCES=newmap.cpp
newmap_DEPENDENCIES=$(STDDEPS)

newlist_SOURCES=newlist.cpp
newlist_DEPENDENCIES=$(STDDEPS)

stringimpl_SOURCES=stringimpl.cpp
stringimpl_DEPENDENCIES=$(STDDEPS)

LDADD=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a
LDADD+= -lcrypto

if OS_MACOSX
LDFLAGS += -framework IOKit -framework CoreFoundation -framework CoreServices
endif

test: testtimeval testrefcount testnewmap testnewlist teststringimpl

testtimeval: timeval
	@./timeval && echo "Passed!" || echo "Failed!"

testrefcount: refcount
	@./refcount && echo "Passed!" || echo "Failed!"

testnewmap: newmap
	@./newmap && echo "Passed!" || echo "Failed!"

testnewlist: newlist
	@./newlist && echo "Passed!" || echo "Failed!"

teststringimpl: stringimpl
	@./stringimpl && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
