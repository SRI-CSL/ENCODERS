.PHONY: \
	test \
	testcreate \
	testblocking \
	testnonblock \
	testtimeout \
	testcancelonqueue \
	testwaitforsocket

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
AM_CPPFLAGS = -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I.. -I$(LIBCPPHAGGLE_DIR)include/ -I$(LIBXML2_INCLUDE_DIR)
AM_LDFLAGS = -lxml2 -lcrypto
if OS_LINUX
AM_LDFLAGS += -lpthread
endif
if OS_MACOSX
AM_LDFLAGS += -framework CoreServices -framework IOKit
endif

bin_PROGRAMS= \
	createtest \
	blockingtest \
	nonblockingtest \
	timeouttest \
	cancelonqueue \
	waitforsocket

LDADD=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

createtest_SOURCES=createtest.cpp
createtest_DEPENDENCIES=$(STDDEPS)

blockingtest_SOURCES=blockingtest.cpp
blockingtest_DEPENDENCIES=$(STDDEPS)

nonblockingtest_SOURCES=nonblockingtest.cpp
nonblockingtest_DEPENDENCIES=$(STDDEPS)

timeouttest_SOURCES=timeouttest.cpp
timeouttest_DEPENDENCIES=$(STDDEPS)

waitforsocket_SOURCES=waitforsocket.cpp
waitforsocket_DEPENDENCIES=$(STDDEPS)

cancelonqueue_SOURCES=cancelonqueue.cpp
cancelonqueue_DEPENDENCIES=$(STDDEPS)

test: \
	testcreate \
	testblocking \
	testnonblock \
	testtimeout \
	testwaitforsocket \
	testcancelonqueue

testcreate: createtest
	@./createtest && echo "Passed!" || echo "Failed!"

testblocking: blockingtest
	@./blockingtest && echo "Passed!" || echo "Failed!"

testnonblock: nonblockingtest
	@./nonblockingtest && echo "Passed!" || echo "Failed!"

testtimeout: timeouttest
	@./timeouttest && echo "Passed!" || echo "Failed!"

testwaitforsocket: waitforsocket
	@./waitforsocket && echo "Passed!" || echo "Failed!"

testcancelonqueue: cancelonqueue
	@./cancelonqueue && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
