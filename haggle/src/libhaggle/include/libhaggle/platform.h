/* Copyright 2008 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *     
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */ 
#ifndef _LIBHAGGLE_PLATFORM_H // To avoid conflicts with Platform.h (C++)
#define _LIBHAGGLE_PLATFORM_H

#include "exports.h"

/* Detect platform: */

/* Mac OS X platform forced? */
#if defined(OS_MACOSX)

/* Linux-type platform forced? */
#elif defined(OS_LINUX)

#elif defined(OS_ANDROID)
#define OS_LINUX

/* Windows mobile platform detected? */
#elif defined(WINCE)
/* Make sure it's known */
#define OS_WINDOWS_MOBILE
#define OS_WINDOWS

/* Windows XP/NT/Whatever platform detected? */
#elif defined(WIN32) || defined(_WIN32)
/* Need this to be able to include both winsock2.h and windows.h on Vista */
#define WIN32_LEAN_AND_MEAN
#define OS_WINDOWS
#include <SdkDdkver.h>

#if (_WIN32_WINNT <= 0x0500)
#define OS_WINDOWS_2000
#elif (_WIN32_WINNT <= 0x0501)
#define OS_WINDOWS_XP
#elif (_WIN32_WINNT <= 0x0502)
#define OS_WINDOWS_SERVER2003
#elif (_WIN32_WINNT <= 0x0600)
#define OS_WINDOWS_VISTA
#else
#error "Unsupported Windows version"
#endif

/* Define a convenience macro for all desktop versions. */
#if defined(OS_WINDOWS_VISTA) || defined(OS_WINDOWS_XP) || defined(OS_WINDOWS_2000) || defined(OS_WINDOWS_SERVER2003)
#define OS_WINDOWS_DESKTOP
#endif

/* Attempting to detect Mac OS X: (This is how apple does it) */
#elif defined(__GNUC__) && ( defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__) )
#include <Availability.h>
/* Set OS X: */

#define OS_MACOSX

#if defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
#define OS_MACOSX_IPHONE

#include <TargetConditionals.h>

#if defined(TARGET_IPHONE_SIMULATOR)
#define OS_MACOSX_IPHONE_SIMULATOR
#endif

#endif

#elif defined(__GNUC__) && defined(__linux__)
#define OS_LINUX
#else
/* Cause an error: */
#error "Unsupported platform: No platform either set or detected"
#endif

/* Detect platform groups */
#if defined(OS_MACOSX) || defined(OS_LINUX)
/* This is a unix-esque platform */
#define OS_UNIX
#include <sys/time.h>
#define libhaggle_gettimeofday(x,y) gettimeofday(x,y)
#endif

#if defined(OS_MACOSX_IPHONE)

#if defined(OS_MACOSX_IPHONE_SIMULATOR) 
#endif /* OS_MACOSX_IPHONE_SIMULATOR */

#endif /* OS_MACOSX_IPHONE */

#if defined(OS_UNIX)

typedef int SOCKET; /* This makes it easier to be compatible to the Windows API */
#define CLOSE_SOCKET(sock) close(sock)
#define CLOSE_FILE(fd) close(fd)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1 // For compatibility with Windows

// 'printf' length modifier for size_t on windows
#define SIZE_T_LMOD "z"

#endif /* OS_UNIX */

#if defined(OS_WINDOWS)
#pragma once

#include <windows.h>

/* This disables warnings against unsecure versions of strcpy, vsnprintf, etc. */
#pragma warning( push )
#pragma warning( disable : 4996 )

#ifdef OS_WINDOWS_MOBILE
#include <winerror.h>
#include <winsock2.h>
#define vsnprintf _vsnprintf
#endif /* OS_WINDOWS_MOBILE */

#define snprintf _snprintf

#pragma warning( pop )


/* Make sure we have a working inline keyword */
#ifndef inline
#define inline _inline
#endif

/* Stuff related to socket programming */
#define CLOSE_SOCKET(sock) closesocket(sock)
#ifdef OS_WINDOWS_MOBILE
#include <altcecrt.h>
#endif
// Winsock errors
#define ENOTSOCK WSAENOTSOCK
#define EOPNOTSUPP WSAEOPNOTSUPP
//#define EBADF WSAEBADF
//#define EINVAL WSAEINVAL
#if defined(OS_WINDOWS_VISTA)
#define EINTR WSAEINTR
#endif

/* Basic data types */
typedef	CHAR int8_t;
typedef	BYTE u_int8_t;
typedef SHORT int16_t;
typedef USHORT u_int16_t;
typedef DWORD32 u_int32_t;
typedef DWORD32 in_addr_t;
typedef INT64 int64_t;
typedef UINT64 u_int64_t;

/* Other data types */
typedef SSIZE_T ssize_t;
typedef int socklen_t;
typedef DWORD pid_t;

char *strtok_r(char *s, const char *delim, char **last);

HAGGLE_API int libhaggle_gettimeofday(struct timeval *tv, void *tz);

// 'printf' length modifier for size_t on windows
#define SIZE_T_LMOD "I"

#endif /* OS_WINDOWS */

#if defined(LIBHAGGLE_INTERNAL)

#define MAX_PATH_APPEND_LEN 20
#define MAX_PATH_DEFAULT_LEN 256

#if defined(OS_ANDROID)
#define PLATFORM_PATH_DELIMITER "/"
#define MAX_PATH_LEN MAX_PATH_DEFAULT_LEN
#define DEFAULT_STORAGE_PATH_PREFIX "/data/"
#define DEFAULT_STORAGE_PATH_POSTFIX "/haggle"
#elif defined(OS_LINUX)
#define PLATFORM_PATH_DELIMITER "/"
#define MAX_PATH_LEN MAX_PATH_DEFAULT_LEN
#define DEFAULT_STORAGE_PATH_PREFIX "/home/"
#define DEFAULT_STORAGE_PATH_POSTFIX "/.Haggle"
#elif defined(OS_MACOSX_IPHONE)
#define PLATFORM_PATH_DELIMITER "/"
#define MAX_PATH_LEN MAX_PATH_DEFAULT_LEN
#define DEFAULT_STORAGE_PATH "/Documents"
#elif defined(OS_MACOSX)
#define PLATFORM_PATH_DELIMITER "/"
#define MAX_PATH_LEN MAX_PATH_DEFAULT_LEN
#define DEFAULT_STORAGE_PATH_PREFIX "/Users/"
#define DEFAULT_STORAGE_PATH_POSTFIX "/Library/Application Support/Haggle"
#elif defined(OS_WINDOWS)
#include <shlobj.h>
#define MAX_PATH_LEN (MAX_PATH + MAX_PATH_APPEND_LEN)
#define PLATFORM_PATH_DELIMITER "\\"
#define DEFAULT_STORAGE_PATH_PREFIX ""
#define DEFAULT_STORAGE_PATH_POSTFIX "\\Haggle"
#else
#error "Unsupported Platform"
#endif

#endif /* LIBHAGGLE_INTERNAL */

typedef enum path_type {
        PLATFORM_PATH_HAGGLE_EXE, /* Location of Haggle executable */
	PLATFORM_PATH_HAGGLE_PRIVATE, /* Location where Haggle stores private data */
        PLATFORM_PATH_HAGGLE_DATA, /* Location where Haggle stores data (e.g., data objects). */
        PLATFORM_PATH_HAGGLE_TEMP, /* Location where Haggle stores temporary files */
	PLATFORM_PATH_APP_DATA, /* Application specific storage location */
} path_type_t;

/*
  Set and override the default paths defined above.
 */
int libhaggle_platform_set_path(path_type_t type, const char *path);
const char *libhaggle_platform_get_path(path_type_t type, const char *append);

/*
	Pseudo-random number generation library
*/

/*
	Initializes the PRNG library. Needs to be done at least once before the
	other functions are executed. There's no harm in calling this function more
	than once.
*/
HAGGLE_API void prng_init(void);

/*
	Returns an 8-bit random number.
*/
HAGGLE_API unsigned char prng_uint8(void);

/*
	Returns a 32-bit random number.
*/
HAGGLE_API unsigned long prng_uint32(void);

#endif /* _LIBHAGGLE_PLATFORM_H */
