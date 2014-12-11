bin_PROGRAMS=webserver
webserver_SOURCES=webserver.cpp

webserver_CPPFLAGS = -I$(top_builddir)/src/libhaggle/include
webserver_LDFLAGS = -lhaggle -lpthread -L$(top_builddir)/src/libhaggle/ -lxml2

EXTRA_DIST=index.html reload.js HaggleLogoBlue400.png

all-local:

clean-local:
	rm -f *~
