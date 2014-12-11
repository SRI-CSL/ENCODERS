CC?=gcc
CFLAGS?=-Wall -O2
TARGETS=busy
LIBS?=-lpthread

all::	$(TARGETS)

busy:	busy.c $(LIBS)
	$(CC) -o busy busy.c $(LIBS) $(CFLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

clean:
	rm -f *~ *.o $(TARGETS)

