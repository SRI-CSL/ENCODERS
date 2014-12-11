.PHONY: \
	test \
	testcreate \
	testtrycondition \
	testunlockedmutex \
	testcondstate \
	testbroadcast \
	testsignal \
	testunique \
	testbreak \
	testcancelthreadoncondition

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I.. -I$(LIBCPPHAGGLE_DIR)include/
LDFLAGS = -lxml2
if OS_LINUX
LDFLAGS += -lpthread
endif

bin_PROGRAMS= \
	createcondition \
	trycondition \
	unlockedmutex \
	condstate \
	broadcast \
	signal \
	unique \
	break \
	cancelthreadoncondition

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

createcondition_SOURCES=createcondition.cpp
createcondition_DEPENDENCIES=$(STDDEPS)
trycondition_SOURCES=trycondition.cpp
trycondition_DEPENDENCIES=$(STDDEPS)
unlockedmutex_SOURCES=unlockedmutex.cpp
unlockedmutex_DEPENDENCIES=$(STDDEPS)
condstate_SOURCES=condstate.cpp
condstate_DEPENDENCIES=$(STDDEPS)
broadcast_SOURCES=broadcast.cpp
broadcast_DEPENDENCIES=$(STDDEPS)
signal_SOURCES=signal.cpp
signal_DEPENDENCIES=$(STDDEPS)
unique_SOURCES=unique.cpp
unique_DEPENDENCIES=$(STDDEPS)
break_SOURCES=break.cpp
break_DEPENDENCIES=$(STDDEPS)
cancelthreadoncondition_SOURCES=cancelthreadoncondition.cpp
cancelthreadoncondition_DEPENDENCIES=$(STDDEPS)

LDADD=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a

if OS_MACOSX
LDFLAGS += -framework CoreServices
endif

test: \
	testcreate \
	testtrycondition \
	testunlockedmutex \
	testcondstate \
	testbroadcast \
	testsignal \
	testunique \
	testbreak \
	testcancelthreadoncondition

testcreate: createcondition
	@./createcondition && echo "Passed!" || echo "Failed!"

testtrycondition: trycondition
	@./trycondition && echo "Passed!" || echo "Failed!"

testunlockedmutex: unlockedmutex
	@./unlockedmutex && echo "Passed!" || echo "Failed!"

testcondstate: condstate
	@./condstate && echo "Passed!" || echo "Failed!"

testbroadcast: broadcast
	@./broadcast && echo "Passed!" || echo "Failed!"

testsignal: signal
	@./signal && echo "Passed!" || echo "Failed!"

testunique: unique
	@./unique && echo "Passed!" || echo "Failed!"

testbreak: break
	@./break && echo "Passed!" || echo "Failed!"

testcancelthreadoncondition: cancelthreadoncondition
	@./cancelthreadoncondition && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
