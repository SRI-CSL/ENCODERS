.PHONY: \
	test \
	testhagglemain \
	testsingleapp \
	testmultipleapp \
	testlargedo

HAGGLE_KERNEL_DIR=$(top_srcdir)/src/hagglekernel/
UTILS_DIR=$(top_srcdir)/src/utils/
LIBHAGGLE_DIR=$(top_srcdir)/src/libhaggle/
LIBCPPHAGGLE_DIR=$(top_srcdir)/src/libcpphaggle/
CPPFLAGS += -I$(HAGGLE_KERNEL_DIR) -I$(UTILS_DIR) -I$(LIBHAGGLE_DIR)include/ -I$(LIBCPPHAGGLE_DIR)include/ -I..
LDFLAGS = -lxml2
LDADD = 

if BUNDLED_SQLITE
LDADD += $(top_srcdir)/extlibs/sqlite-3.5.6/libsqlite.a
else
LDFLAGS += -lsqlite3 
endif
LDADD += -lcrypto

if OS_LINUX

LDFLAGS += -lpthread

if HAVE_DBUS
LDFLAGS += `pkg-config --cflags --libs dbus-1`
endif

endif # OS_LINUX

if ENABLE_BLUETOOTH
if OS_LINUX
LDFLAGS += -lbluetooth
endif

if OS_MACOSX
LDFLAGS += -framework IOBluetooth
endif
endif

if OS_MACOSX
LDFLAGS += -framework CoreFoundation -framework CoreServices -framework IOKit
endif

bin_PROGRAMS= \
	hagglemain \
	singleapp \
	multipleapp \
	largedo

LDADD+=$(HAGGLE_KERNEL_DIR)libhagglekernel.a 
LDADD+=$(UTILS_DIR)libhaggleutils.a
LDADD+=$(LIBCPPHAGGLE_DIR)libcpphaggle.a
LDADD+=../libtesthlp.a
LDADD+=../hagglemain.o
LDADD+=-lhaggle -L$(top_builddir)/src/libhaggle/

if OS_MACOSX

if ENABLE_BLUETOOTH
LDADD+=$(HAGGLE_KERNEL_DIR)libmacosxobjc.a
LDFLAGS += -framework IOBluetooth -framework CoreFoundation -framework Foundation
endif

endif

STDDEPS=$(HAGGLE_KERNEL_DIR)libhagglekernel.a
STDDEPS+=$(UTILS_DIR)libhaggleutils.a
STDDEPS+=../libtesthlp.a
STDDEPS+=../hagglemain.o

hagglemain_SOURCES=testmain.cpp
hagglemain_DEPENDENCIES=$(STDDEPS)

singleapp_SOURCES=singleapp.cpp
singleapp_DEPENDENCIES=$(STDDEPS)

multipleapp_SOURCES=multipleapp.cpp
multipleapp_DEPENDENCIES=$(STDDEPS)

largedo_SOURCES=largedo.cpp
largedo_DEPENDENCIES=$(STDDEPS)

test: \
	testhagglemain \
	testsingleapp \
	testmultipleapp \
	testlargedo

testhagglemain: hagglemain
	@./hagglemain && echo "Passed!" || echo "Failed!"

testsingleapp: singleapp
	@./singleapp && echo "Passed!" || echo "Failed!"

testmultipleapp: multipleapp
	@./multipleapp && echo "Passed!" || echo "Failed!"

testlargedo: largedo
	@./largedo && echo "Passed!" || echo "Failed!"

all-local:

clean-local:
	rm -f *~ *.o
