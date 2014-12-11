#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <windows.h>

typedef DWORD pid_t;

pid_t getpid();

#define _getpid getpid

#endif /* _PROCESS_H_ */