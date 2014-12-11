#include "io.h"
#include <stdio.h>

int __cdecl _open(const char* filename, int flags, int mode)
{
	fprintf(stderr, "open not implemented\n");
	return -1;
}

int __cdecl _close(int fd)
{
	fprintf(stderr, "close not implemented\n");
	return -1;
}

long _lseek(int fd, long offset, int whence)
{
	fprintf(stderr, "lseek not implemented\n");
	return -1;
}

int _read(int fd, void *buffer, unsigned int count)
{
	fprintf(stderr, "read not implemented\n");
	return -1;
}

int _write(int fd, const void *buffer, unsigned int count)
{
	fprintf(stderr, "write not implemented\n");
	return -1;
}
