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

#ifndef testhlp_h
#define testhlp_h

#include <libcpphaggle/PlatformDetect.h>
#include "utils.h"

#undef printf

#ifdef __cplusplus
extern "C" {
#endif

#define print_over_test_str(a,b) print_over_test_str_(a,(char *)b)
void print_over_test_str_(
#if defined(OS_WINDOWS)
		unsigned long level, 
#else
		long level,
#endif
		char *str);
#define print_over_test_str_nl(a,b) print_over_test_str_nl_(a,(char *)b)
void print_over_test_str_nl_(
#if defined(OS_WINDOWS)
		unsigned long level, 
#else
		long level,
#endif
		char *str);
// Pass a boolean value to this:
void print_pass(int passed);
#define print_passed() print_pass(1==1)
#define print_failed() print_pass(1==0)

#ifdef __cplusplus
}
#endif

#endif
