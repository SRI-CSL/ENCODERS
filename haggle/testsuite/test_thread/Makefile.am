.PHONY: test testcreate testjoin testcancel teststop teststackmanagement testcancelthreadsocket

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I$(LIBCPPHAGGLE_DIR)include/ -I..
LDFLAGS = -lxml2
if OS_LINUX
LDFLAGS += -lpthread
endif

bin_PROGRAMS=createthread jointhread cancelthread stopthread stackmanagement cancelthreadsocket

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

createthread_SOURCES=createthread.cpp
createthread_DEPENDENCIES=$(STDDEPS)
jointhread_SOURCES=jointhread.cpp
jointhread_DEPENDENCIES=$(STDDEPS)
cancelthread_SOURCES=cancelthread.cpp
cancelthread_DEPENDENCIES=$(STDDEPS)
stopthread_SOURCES=stopthread.cpp
stopthread_DEPENDENCIES=$(STDDEPS)
stackmanagement_SOURCES=stackmanagement.cpp
stackmanagement_DEPENDENCIES=$(STDDEPS)
cancelthreadsocket_SOURCES=cancelthreadsocket.cpp
cancelthreadsocket_DEPENDENCIES=$(STDDEPS)

LDADD=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a

if OS_MACOSX
LDFLAGS += -framework CoreServices
endif

test: testcreate testjoin testcancel teststop teststackmanagement testcancelthreadsocket

testcreate: createthread
	@./createthread && echo "Passed!" || echo "Failed!"

testjoin: jointhread
	@./jointhread && echo "Passed!" || echo "Failed!"

testcancel: cancelthread
	@./cancelthread && echo "Passed!" || echo "Failed!"

teststop: stopthread
	@./stopthread && echo "Passed!" || echo "Failed!"

teststackmanagement: stackmanagement
	@./stackmanagement && echo "Passed!" || echo "Failed!"

testcancelthreadsocket: cancelthreadsocket
	@./cancelthreadsocket && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
