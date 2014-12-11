#include "errno.h"
#include <stdlib.h>
#include <string.h>

int errno;

void perror(const char *prefix)
{
	if (prefix == NULL || *prefix == 0)
		fprintf(stderr, "errno=%d\n", errno);
	else
		fprintf(stderr, "%s: errno=%d\n", prefix, errno);
}

char *strerror(const int errno)
{
	static const char *msg = "strerror not implemented";
	return (char *)msg;
}