.PHONY: test testtest64 testbloom testbloom_count testshatest

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I.. -I$(LIBCPPHAGGLE_DIR)include/
LDFLAGS =

if OS_LINUX
LDFLAGS += -lpthread
endif

bin_PROGRAMS=test64 bloom shatest bloom_count

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a

test64_SOURCES=test64.cpp
test64_DEPENDENCIES=$(STDDEPS)
bloom_SOURCES=bloom.cpp
bloom_DEPENDENCIES=$(STDDEPS)
bloom_count_SOURCES=bloom_count.cpp
bloom_count_DEPENDENCIES=$(STDDEPS)
shatest_SOURCES=shatest.cpp
shatest_DEPENDENCIES=$(STDDEPS)

LDADD=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a
LDADD+= -lcrypto
 
test: testtest64 testbloom testbloom_count testshatest

testtest64: test64
	@./test64 && echo "Passed!" || echo "Failed!"

testbloom: bloom
	@./bloom && echo "Passed!" || echo "Failed!"

testbloom_count: bloom_count
	@./bloom_count && echo "Passed!" || echo "Failed!"

testshatest: shatest
	@./shatest && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
