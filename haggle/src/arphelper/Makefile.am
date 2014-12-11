
bin_PROGRAMS = arphelper

arphelper_SOURCES = arphelper.c

arphelper_CFLAGS =-std=gnu99

EXTRA_DIST=Android.mk

install-exec-local:
	chown root "$(DESTDIR)$(bindir)/arphelper"
	chmod u+s "$(DESTDIR)$(bindir)/arphelper"
