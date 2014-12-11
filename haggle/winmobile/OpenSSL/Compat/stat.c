#include <sys/stat.h>
#include <stdio.h>


int stat(const char *filename, struct stat *buf)
{
	fprintf(stderr, "stat: not implemented\n");
	return -1;
}