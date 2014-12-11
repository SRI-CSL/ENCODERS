#ifndef _ERRNO_H_
#define _ERRNO_H_

extern int errno;


#ifndef ENOENT
#define ENOENT	(2)
#endif

#ifndef EBADF
#define EBADF	(9)
#endif

#ifndef EAGAIN
#define EAGAIN	(11)
#endif

#ifndef ENOMEM
#define ENOMEM	(12)
#endif

#ifndef EINVAL
#define EINVAL	(22)
#endif

#ifndef EACCES
#define EACCES  (13)
#endif

// This should really be in stdlib.h
void perror(const char *prefix);
char *strerror(const int errno);

#endif /* _ERRNO_H_ */