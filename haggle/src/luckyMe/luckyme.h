#ifndef _LUCKYME_H
#define _LUCKYME_H

#include <libhaggle/platform.h>

#if defined(OS_WINDOWS_MOBILE)
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LUCKYME_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LUCKYME_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#if defined(CONSOLE)
#define LUCKYME_API 
#else
#ifdef LUCKYME_EXPORTS
#define LUCKYME_API __declspec(dllexport)
#else
#define LUCKYME_API __declspec(dllimport)
#endif
#endif

LUCKYME_API int luckyme_test_start(void);
LUCKYME_API int luckyme_test_stop(void);

LUCKYME_API int luckyme_start(void);
LUCKYME_API int luckyme_stop(int stop_haggle);

LUCKYME_API int luckyme_haggle_start(daemon_spawn_callback_t callback);
LUCKYME_API int luckyme_haggle_stop(void);

LUCKYME_API int luckyme_is_haggle_shuttingdown();
LUCKYME_API int luckyme_is_test_running();

/*
 Returns the number of data objects received since luckyme started last.
 0 if luckyme is not running.
 */
LUCKYME_API unsigned long luckyme_get_num_dataobjects_received(void);

/*
 Returns the number of data objects created on this node since luckyme started last.
 0 if luckyme is not running.
 */
LUCKYME_API unsigned long luckyme_get_num_dataobjects_created(void);

/*
 Returns the number of neighbors currently. 0 if luckyme is not running.
 */
LUCKYME_API unsigned long luckyme_get_num_neighbors(void);

/*
 Returns the name of the given neighbor. NULL if luckyme is not running,
 the index is out of range etc. 
 The returned string must be freed after usage.
 */
LUCKYME_API char *luckyme_get_neighbor_unlocked(unsigned int n);
LUCKYME_API char *luckyme_get_neighbor_locked(unsigned int n);
LUCKYME_API void luckyme_free_neighbor(char *neigh);

LUCKYME_API int luckyme_neighborlist_lock();
LUCKYME_API void luckyme_neighborlist_unlock();
/*
 Returns 1 if luckyMe is running
 Returns 0 if luckyMe is not running
 */
LUCKYME_API int luckyme_is_running(void);

enum {
	EVENT_TYPE_ERROR = -1,
	EVENT_TYPE_SHUTDOWN = 0,
	EVENT_TYPE_NEIGHBOR_UPDATE,
	EVENT_TYPE_NEW_DATAOBJECT,
	EVENT_TYPE_DATA_OBJECT_GENERATED,
	EVENT_TYPE_STATUS_UPDATE
};

typedef void (__stdcall *callback_function_t) (int);

LUCKYME_API int set_callback(callback_function_t callback);

#endif /* OS_WINDOWS_MOBILE */

#endif /* _LUCKYME_H */
