noinst_LIBRARIES = libcpphaggle.a
libcpphaggle_a_SOURCES = Thread.cpp Timeval.cpp Watch.cpp Heap.cpp \
	Signal.cpp Condition.cpp Mutex.cpp String.cpp Reference.cpp
EXTRA_DIST = \
	Doxyfile.in \
	include/libcpphaggle/Condition.h \
	include/libcpphaggle/Exception.h \
	include/libcpphaggle/GenericQueue.h \
	include/libcpphaggle/HashMap.h \
	include/libcpphaggle/Heap.h \
	include/libcpphaggle/List.h \
	include/libcpphaggle/Map.h \
	include/libcpphaggle/Mutex.h \
	include/libcpphaggle/Pair.h \
	include/libcpphaggle/Platform.h \
	include/libcpphaggle/PlatformDetect.h \
	include/libcpphaggle/Reference.h \
	include/libcpphaggle/Signal.h \
	include/libcpphaggle/String.h \
	include/libcpphaggle/Thread.h \
	include/libcpphaggle/Timeval.h \
	include/libcpphaggle/Watch.h \
	Android.mk

if HAVE_EXCEPTION
CXXFLAGS +=-DHAVE_EXCEPTION=1
else
CXXFLAGS +=-DHAVE_EXCEPTION=0 -fno-exceptions -fno-non-call-exceptions
endif

libcpphaggle_a_LIBADD=$(top_srcdir)/src/utils/libhaggleutils.a

CPPFLAGS += -I$(top_srcdir)/src/utils -Iinclude

clean-local:
	rm -f *~
	rm -f include/libcpphaggle/*~
