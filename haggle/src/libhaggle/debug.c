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

#include <stdio.h>
#include <stdarg.h>

#define LIBHAGGLE_INTERNAL
#include <libhaggle/haggle.h>
#include <libhaggle/platform.h>

#if defined(OS_ANDROID)
#include <android/log.h>
#endif

#if defined(WINCE)
#define TRACE_TO_FILE 1
#endif

#if defined(TRACE_TO_FILE)
FILE *tr_out = NULL;
FILE *tr_err = NULL;
#else
#define tr_out stdout
#define tr_err stderr
#endif

static int trace_level = 2;

void set_trace_level(int level)
{
	trace_level = level;
}

void libhaggle_debug_fini(void)
{
#ifdef TRACE_TO_FILE
	if (tr_out)
		fclose(tr_out);
#endif
}

#if defined(DEBUG) && (defined(__GNUC__) || defined(__BIONIC__))
__attribute__((constructor))
#endif
void libhaggle_debug_init(void)
{
#ifdef TRACE_TO_FILE
	const char *path = libhaggle_platform_get_path(PLATFORM_PATH_HAGGLE_TEMP, "/libhaggle.txt");
	
	if (!path || tr_out || tr_err)
		return;

	tr_out = tr_err = fopen(path, "w");

	if (!tr_out) {
		fprintf(stderr, "Could not open file %s\n", path);
		return;
	}
#if defined(OS_UNIX)
	atexit(libhaggle_debug_fini);
#endif
#endif
}

int libhaggle_trace(int err, const char *func, const char *fmt, ...)
{
	static char buf[1024];
	struct timeval now;
	va_list args;
	int len;

	if (!tr_out)
		return -1;

	if (trace_level == 0 || (trace_level == 1 && err == 0))
		return 0;

	libhaggle_gettimeofday(&now, NULL);

	va_start(args, fmt);
#ifdef WINCE
	len = _vsnprintf(buf, 1024, fmt, args);
#else
	len = vsnprintf(buf, 1024, fmt, args);
#endif
	va_end(args);
	
	if (len == -1)
		return -1;

#if defined(OS_ANDROID)
	len = __android_log_print(ANDROID_LOG_DEBUG, "LIBHAGGLE", 
				  "%ld.%06ld %s: %s", (long)now.tv_sec, 
				  (long)now.tv_usec, func, buf);
#else
	fprintf((err ? tr_err : tr_out), "%ld.%06ld %s: %s", 
		(long)now.tv_sec, (long)now.tv_usec, func, buf);
    fflush(tr_err);
	fflush(tr_out);
#endif
	return 0;
}
