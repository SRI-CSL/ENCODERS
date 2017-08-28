/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

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

#ifndef __UTILS_H_
#define __UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "prng.h"

// If compiling for Android, also define OS_LINUX
#if defined(OS_ANDROID)
#ifndef OS_LINUX
#define OS_LINUX
#endif
#endif

#if defined(WIN32) || defined(WINCE)
/* Need this to be able to include both winsock2.h and windows.h on Vista */
#define WIN32_LEAN_AND_MEAN 
#pragma once
#include <windows.h>
#include <winsock2.h>
typedef LONGLONG long64_t;
typedef DWORD32 in_addr_t;

#define ERRNO WSAGetLastError()
#define STRERROR(err) StrError(err)

#ifndef inline
#define inline __inline
#endif

#pragma warning( disable : 4996 ) /* This disables warnings against
				   * unsecure versions of strcpy,
				   * vsnprintf, etc. */
int gettimeofday(struct timeval *tv, void *tz);
char *strsep(char **stringp, const char *delim);

/*
	Given a WSAGetLastError() return value, this returns a string with the
	symbolic error code, like WSAEACCES, and a string explaining what the
	error is. 
	
	Similar to strerror() for errno.
*/
char *StrError(long err);
#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_MACOSX_IPHONE)
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
typedef long long long64_t;

#define ERRNO errno
#define STRERROR(err) strerror(err)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#else
#error "Unkown operating system!"
#endif
	
#include <stdio.h>

typedef unsigned long usecs_t;

double absolute_time_double(double _offset);
char *ip_to_str(struct in_addr addr);
char *eth_to_str(unsigned char *addr);
void swap_6bytes(void* to, const void *from);
void buf2str(const char* buf, char* str, int len);
void str2buf(const char* str, char* buf, int len);
unsigned short in_cksum(const unsigned short *addr, register int len, unsigned short csum);

#define RANDOM_INT(max) ((unsigned int)(max*(((double)prng_uint32()) / ((double)0xFFFFFFFF))))

/**
	This function makes the current thread sleep for at least the given number
	of microseconds.
	
	This function will not return before the time has elapsed, even if a signal
	is sent.
*/
void milli_sleep(unsigned long milliseconds);

// trace related (HAGGLE_DBG, HAGGLE_TRACE, logfile, etc)

typedef enum {
	TRACE_DEBUG,
	TRACE_ERR,
} trace_type_t;

#if defined(OS_UNIX) 
#define __TRACE_FUNC__ __PRETTY_FUNCTION__ 
#else
#define __TRACE_FUNC__ __FUNCTION__ 
#endif

// This function enables trace output to be disabled:
void trace_disable(int disable);

#define TRACE(_type, f, ...) trace(_type, __TRACE_FUNC__, f, ## __VA_ARGS__)
void trace(const trace_type_t _type, const char *func, const char *fmt, ...);

void set_trace_timestamp_base(const struct timeval *tv);

#define TRACE_ERR(f, ...) TRACE(TRACE_ERR, f, ## __VA_ARGS__)
#ifdef DEBUG
#define TRACE_DBG(f, ...) TRACE(TRACE_DEBUG, f, ## __VA_ARGS__)
#else
#define TRACE_DBG(f, ...)
#endif /* DEBUG */

/*
	Utility function which given an IP address of a peer and the
	local interface name to reach the peer, retreives the mac
	address of the peer.  Returns greater than zero on success,
	zero if there is no corresponding interface, or less than zero
	on error.

        'ifname' may be NULL.
*/
int get_peer_mac_address(const struct sockaddr *saddr, const char *ifname, unsigned char *mac, size_t maclen);

// SW: START added has_peer_mac_address helper:
int has_peer_mac_address(const struct sockaddr *saddr, const char *ifname);
// SW: END added has_peer_mac_address helper.

int send_file(const char* filename, int fd);
	

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s

enum {
// There is no version information available for linux yet.
	platform_linux = 0x00000000,
	
	platform_macosx = 0x01000000,
// makes any Mac OS X version, like so: 
// platform_macosx_(10,4,0) <- 10.4.0 (tiger)
// platform_macosx_(10,5,6) <- 10.5.6 (leopard, update 6)
// platform_macosx_(10,6,0) <- 10.6.0 (snow leopard)
// etc.
#define platform_macosx_(x,y,z) (platform_macosx | ((x) << 16) | ((y) << 8) | (z))
	platform_macosx_iphone = 0x02000000,
// makes any Mac OS X (iphone) version, like so: 
// platform_macosx_iphone_(1,0,0) <- 1.0.0
// platform_macosx_iphone_(2,2,1) <- 2.2.1
// platform_macosx_iphone_(3,0,0) <- 3.0.0
// etc.
#define platform_macosx_iphone_(x,y,z) (platform_macosx_iphone | ((x) << 16) | ((y) << 8) | (z))
	
	platform_windows = 0x03000000,
	platform_windows_xp = platform_windows | 5,
	platform_windows_vista = platform_windows | 6,
	platform_windows_7 = platform_windows | 7,
	
	platform_windows_mobile_standard = 0x04000000,
	platform_windows_mobile_professional = 0x05000000,
// makes any windows mobile version, like so: 
// platform_windows_mobile_professional(6) <- Windows mobile 6 professional
// platform_windows_mobile_standard(6) <- Windows mobile 6 standard
// platform_windows_mobile_standard(5) <- Windows mobile 6
#define platform_windows_mobile_professional_(x) (platform_windows_mobile_professional | ((x) - 1))
#define platform_windows_mobile_standard_(x) (platform_windows_mobile_standard | ((x) - 1))
	
// Returns only the platform type, not version:
#define platform_type(x) ((x) & 0xFF000000)
	platform_none = -1
};

/*
	This function tries to programmatically figure out which platform the 
	program is running on.

	This function is only (potentially) expensive to call the first time. 
	All subsequent return values are cached.
*/
int current_platform(void);

/*
	This function tries to figure out what hardware the program is running on. 
	The returned string must not be deallocated in any way.
	
	This function is only (potentially) expensive to call the first time. 
	All subsequent return values are cached.
	
	FIXME: only really works on Windows Mobile. All other platforms return a 
	default string.
*/
char *get_hardware_name(void);

#if defined(WIN32) || defined(OS_MACOSX) || defined(OS_ANDROID)
#if defined(WIN32)
struct ether_addr {
        BYTE ether_addr_octet[6];
};
#elif defined(OS_ANDROID)
#include <net/if_ether.h>
#elif defined(OS_MACOSX)
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#endif
struct ether_addr *ether_aton_r(const char *asc, struct ether_addr *addr);
#else
#include <netinet/ether.h>
#endif

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif /* __UTILS_H_ */


