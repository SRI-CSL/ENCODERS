#include <process.h>

pid_t getpid()
{
	return GetCurrentProcessId();
}