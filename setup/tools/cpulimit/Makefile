VERSION?=2.1
PREFIX?=/usr
CFLAGS?=-Wall -O2 -DVERSION=$(VERSION)
CC?=gcc

all: cpulimit

osx:
	$(CC) -o cpulimit cpulimit.c -D__APPLE__ $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

minix:
	$(CC) -o cpulimit cpulimit.c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

freebsd:
	$(CC) -o cpulimit cpulimit.c -lrt -DFREEBSD $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

cpulimit: cpulimit.c
	$(CC) -o cpulimit cpulimit.c -lrt -DLINUX $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

tests:
	$(MAKE) -C test

install: cpulimit
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/share/man/man1
	cp cpulimit ${PREFIX}/bin/limitcpu
	cp cpulimit.1.gz ${PREFIX}/share/man/man1/limitcpu.1.gz

deinstall:
	rm -f ${PREFIX}/bin/limitcpu
	rm -f ${PREFIX}/share/man/man1/limitcpu.1.gz

clean:
	rm -f *~ cpulimit
	$(MAKE) -C test clean

tarball: clean
	cd .. && tar czf limitcpu-$(VERSION).tar.gz limitcpu-$(VERSION)
	
