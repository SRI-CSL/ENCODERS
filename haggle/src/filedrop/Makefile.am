if ENABLE_FILEDROP
if OS_IPHONEOS
else
if OS_LINUX
bin_PROGRAMS = filedrop
filedrop_SOURCES = filedrop.c
endif

filedrop_LDFLAGS =-lhaggle -L$(top_builddir)/src/libhaggle/ -lxml2
filedrop_CPPFLAGS =-I$(top_builddir)/src/libhaggle/include

if OS_MACOSX
bin_PROGRAMS = filedrop
filedrop_SOURCES = filedropMacOSX.c
filedrop_LDFLAGS += -framework CoreServices -framework ApplicationServices
endif

EXTRA_DIST = filedropMacOSX.c filedrop.c

all-local:

clean-local:
	rm -f *~

endif 
endif
