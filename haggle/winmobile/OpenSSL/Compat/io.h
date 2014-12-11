#ifndef _IO_H_
#define _IO_H_

int __cdecl _open(const char* filename, int flags, int mode);
int __cdecl _close(int fd);
long __cdecl _lseek(int fd, long offset, int whence);
int __cdecl _read(int fd, void *buffer, unsigned int count);
int __cdecl _write(int fd, const void *buffer, unsigned int count);

#define open _open
#define close _close
#define lseek _lseek
#define read _read
#define write _write

#endif /* _IO_H_ */