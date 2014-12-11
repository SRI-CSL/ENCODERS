/* Copyright 2008-2009 Uppsala University
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
#ifndef _PLATFORMDETECT_H
#define _PLATFORMDETECT_H


/* Detect platform: */

/* Mac OS X platform forced? */
#if defined(OS_MACOSX)

/* Linux-type platform forced? */
#elif defined(OS_LINUX)

/* Windows mobile platform detected? */
#elif defined(WINCE)
/* Make sure it's known */
#define OS_WINDOWS_MOBILE
#define OS_WINDOWS

/* Windows XP/NT/Whatever platform detected? */
#elif defined(WIN32) || defined(_WIN32)
/* Make sure it's known */
#define OS_WINDOWS

// Default to Windows XP
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <sdkddkver.h>

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

#elif defined(OS_ANDROID)
#define OS_LINUX
#elif defined(__linux)
#define OS_LINUX
#else
/* Cause an error: */
#error "Unsupported platform: No platform either set or detected"
#endif


/* Detect platform groups */
#if defined(OS_MACOSX) || defined(OS_LINUX)
/* This is a unix-esque platform */
#define OS_UNIX
#endif

#if defined(OS_MACOSX_IPHONE)

#if defined(ENABLE_BLUETOOTH)
#undef ENABLE_BLUETOOTH
#endif

#endif /* OS_MACOSX_IPHONE */

#if defined(OS_WINDOWS_MOBILE)
#ifndef ENABLE_BLUETOOTH
#define ENABLE_BLUETOOTH
#endif

#ifndef ENABLE_ETHERNET
#define ENABLE_ETHERNET
#endif

#ifndef ENABLE_MEDIA
//#define ENABLE_MEDIA
#endif
#elif defined(OS_WINDOWS_DESKTOP)
#ifndef ENABLE_BLUETOOTH
//#define ENABLE_BLUETOOTH
#endif

#ifndef ENABLE_ETHERNET
#define ENABLE_ETHERNET
#endif

#ifndef ENABLE_MEDIA
//#define ENABLE_MEDIA
#endif
#endif

#endif
