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

#if defined(WINCE)
#define OS_WINDOWS_MOBILE
#define OS_WINDOWS
#endif

#if defined(WIN32)
#define OS_WINDOWS
#include <stdlib.h>
#endif

#include <time.h>
#include "utils.h"

#if defined(OS_MACOSX) || defined(OS_LINUX)
#include <stdlib.h>
#endif

void prng_init(void)
{
#if defined(OS_MACOSX)
	// No need for initialization
#elif defined(OS_LINUX)
	srandom(time(NULL));
#elif defined(OS_WINDOWS_MOBILE)
	srand(GetTickCount());
#elif defined(OS_WINDOWS)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_sec + tv.tv_usec);
#endif
}

unsigned char prng_uint8(void)
{
	return
#if defined(OS_MACOSX)
		arc4random() & 0xFF;
#elif defined(OS_LINUX)
		random() & 0xFF;
#elif defined(OS_WINDOWS)
		rand() & 0xFF;
#endif
}

unsigned long prng_uint32(void)
{
	return
#if defined(OS_MACOSX)
		// arc4random() returns a 32-bit random number:
		arc4random();
#elif defined(OS_LINUX)
		// random() returns a 31-bit random number:
		(((unsigned long)(random() & 0xFFFF)) << 16) |
		(((unsigned long)(random() & 0xFFFF)) << 0);
#elif defined(OS_WINDOWS_MOBILE)
		Random();
#elif defined(OS_WINDOWS)
		// rand() returns a 15-bit random number:
		(((unsigned long) (rand() & 0xFF)) << 24) |
		(((unsigned long) (rand() & 0xFFF)) << 12) |
		(((unsigned long) (rand() & 0xFFF)) << 0);
#endif
}

