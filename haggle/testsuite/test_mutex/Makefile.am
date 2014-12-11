.PHONY: test testcreate testbinary testlock testrecursive testtrylock testcancelonmutex

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I$(LIBCPPHAGGLE_DIR)include/ -I..
LDFLAGS = -lxml2
if OS_LINUX
LDFLAGS += -lpthread
endif

bin_PROGRAMS=createmutex binary isunlocked lock recursive trylock cancelonmutex

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

createmutex_SOURCES=createmutex.cpp
createmutex_DEPENDENCIES=$(STDDEPS)
binary_SOURCES=binary.cpp
binary_DEPENDENCIES=$(STDDEPS)
isunlocked_SOURCES=isunlocked.cpp
isunlocked_DEPENDENCIES=$(STDDEPS)
lock_SOURCES=lock.cpp
lock_DEPENDENCIES=$(STDDEPS)
recursive_SOURCES=recursive.cpp
recursive_DEPENDENCIES=$(STDDEPS)
trylock_SOURCES=trylock.cpp
trylock_DEPENDENCIES=$(STDDEPS)
cancelonmutex_SOURCES=cancelonmutex.cpp
cancelonmutex_DEPENDENCIES=$(STDDEPS)

LDADD=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a

if OS_MACOSX
LDFLAGS += -framework CoreServices
endif

test: testcreate testbinary testisunlocked testlock testrecursive testtrylock testcancelonmutex

testcreate: createmutex
	@./createmutex && echo "Passed!" || echo "Failed!"

testbinary: binary
	@./binary && echo "Passed!" || echo "Failed!"

testisunlocked: isunlocked
	@./isunlocked && echo "Passed!" || echo "Failed!"

testlock: lock
	@./lock && echo "Passed!" || echo "Failed!"

testrecursive: recursive
	@./recursive && echo "Passed!" || echo "Failed!"

testtrylock: trylock
	@./trylock && echo "Passed!" || echo "Failed!"

testcancelonmutex: cancelonmutex
	@./cancelonmutex && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
