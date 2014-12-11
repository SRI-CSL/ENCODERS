
CC	= gcc
CPP	= c++
CCOPT	= -g

DEFINE	= 
INCLUDES = \

INSTALL	= /usr/bin/install -c
MKDEP	= makedepend
RANLIB	= ranlib


BLANK	= # make a blank space.  DO NOT add anything to this line
AR	= ar rc $(BLANK)
LINK	= $(CPP)
LDFLAGS	=
LDOUT	= -o $(BLANK)
PERL	= perl
RM	= rm -f

BFLAGS	= $(INCLUDES) $(DEFINE)
CFLAGS	= $(INCLUDES) $(DEFINE) $(CCOPT) -Wall

# Explicitly define compilation rules since SunOS 4's make doesn't like gcc.
# Also, gcc does not remove the .o before forking 'as', which can be a
# problem if you don't own the file but can write to the directory.
.SUFFIXES: .cc	# $(.SUFFIXES)

.cc.o:
	@rm -f $@
	$(CPP) -c $(CFLAGS) -o $@ $*.cc

.c.o:
	@rm -f $@
	$(CC) -c $(CFLAGS) -o $@ $*.c

what-time: what-time.o
	$(CC) -o what-time $(LDFLAGS) what-time.o