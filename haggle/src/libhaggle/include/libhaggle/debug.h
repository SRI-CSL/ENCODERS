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

#ifndef _LIBHAGGLE_DEBUG_H
#define _LIBHAGGLE_DEBUG_H

#include "exports.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
	This functions makes it possible to disable output from haggle based on the
	kind of output.
	
	The level is initially 2.
	
	Level:		What is displayed:
	2			Debugging output (If compiled with debugging on) and errors
	1			Only errors
	0			Nothing
*/
HAGGLE_API void set_trace_level(int level);

HAGGLE_API void libhaggle_debug_init(void);
HAGGLE_API void libhaggle_debug_fini(void);
HAGGLE_API int libhaggle_trace(int err, const char *func, const char *fmt, ...);

#ifdef DEBUG
#define LIBHAGGLE_DBG(f, ...) libhaggle_trace((1==0), __FUNCTION__, f, ## __VA_ARGS__)
#define LIBHAGGLE_ERR(f, ...) libhaggle_trace((1==1), __FUNCTION__, f, ## __VA_ARGS__)
#else 
#define LIBHAGGLE_DBG(f, ...)
#define LIBHAGGLE_ERR(f, ...)
#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* _LIBHAGGLE_DEBUG_H */
