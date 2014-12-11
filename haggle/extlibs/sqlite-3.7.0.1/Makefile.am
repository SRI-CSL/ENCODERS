AM_CFLAGS = -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_RTREE

lib_LTLIBRARIES = libsqlite3.la
libsqlite3_la_SOURCES = sqlite3.c
libsqlite3_la_LDFLAGS = -no-undefined -version-info 8:6:8
libsqlite3_la_CFLAGS = -Wno-uninitialized -Wno-unused-value -Wno-unused-variable

bin_PROGRAMS = sqlite3
sqlite3_SOURCES = shell.c sqlite3.h
sqlite3_LDADD = libsqlite3.la
sqlite3_DEPENDENCIES = libsqlite3.la
sqlite3_CFLAGS = -Wno-uninitialized -Wno-unused-value -Wno-unused-variable

if OS_LINUX
sqlite3_LDFLAGS =-lpthread -ldl
endif

if DEBUG
else
# No debugging symbols
libsqlite3_la_CFLAGS +=-Os
endif

include_HEADERS = sqlite3.h sqlite3ext.h

EXTRA_DIST = sqlite3.pc sqlite3.1
pkgconfigdir = ${libdir}/pkgconfig
pkgconfig_DATA = sqlite3.pc

man_MANS = sqlite3.1
