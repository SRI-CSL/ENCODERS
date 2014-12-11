#ifndef _STAT_H_
#define _STAT_H_

#include <stdlib.h>	


#define _S_IFMT         0170000   
#define _S_IFDIR        0040000   
#define _S_IFCHR        0020000
#define _S_IFIFO        0010000
#define _S_IFREG        0100000
#define _S_IREAD        0000400 
#define _S_IWRITE       0000200
#define _S_IEXEC        0000100

#define S_IFMT			_S_IFMT
#define S_IFDIR			_S_IFDIR
#define S_IFCHR			_S_IFCHR
#define S_IFIFO			_S_IFIFO
#define S_IFREG			_S_IFREG
#define S_IREAD			_S_IREAD
#define S_IWRITE		_S_IWRITE
#define S_IEXEC			_S_IEXEC


#ifndef _DEV_T_DEFINED
typedef unsigned int _dev_t;
#define _DEV_T_DEFINED
#endif

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;
#define _INO_T_DEFINED
#endif

#ifndef __OFF_T_DEFINED
typedef long _off_t;
#define __OFF_T_DEFINED
#endif

struct stat
{
	_dev_t			st_dev;
	_ino_t			st_ino;
	unsigned short	st_mode;
	short			st_nlink;
	short			st_uid;
	short			st_gid;
	_dev_t			st_rdev;
	_off_t			st_size;
	time_t			st_atime;
	time_t			st_mtime;
	time_t			st_ctime;
};

#define _stat stat

int __cdecl stat(const char *filename, struct stat *buf);



#endif /* _STAT_H_ */