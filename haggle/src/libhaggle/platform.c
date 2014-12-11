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

#define LIBHAGGLE_INTERNAL
#include <libhaggle/platform.h>
#include <libhaggle/debug.h>

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef OS_WINDOWS
/*
 * Copyright (c) 1998 Softweyr LLC.  All rights reserved.
 *
 * strtok_r, from Berkeley strtok
 * Oct 13, 1998 by Wes Peters <wes@softweyr.com>
 *
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *	This product includes software developed by Softweyr LLC, the
 *      University of California, Berkeley, and its contributors.
 *
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SOFTWEYR LLC, THE REGENTS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL SOFTWEYR LLC, THE
 * REGENTS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

char *
strtok_r(char *s, const char *delim, char **last)
{
    char *spanp;
    int c, sc;
    char *tok;

    if (s == NULL && (s = *last) == NULL)
    {
	return NULL;
    }

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0; )
    {
	if (c == sc)
	{
	    goto cont;
	}
    }

    if (c == 0)		/* no non-delimiter characters */
    {
	*last = NULL;
	return NULL;
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;)
    {
	c = *s++;
	spanp = (char *)delim;
	do
	{
	    if ((sc = *spanp++) == c)
	    {
		if (c == 0)
		{
		    s = NULL;
		}
		else
		{
		    char *w = s - 1;
		    *w = '\0';
		}
		*last = s;
		return tok;
	    }
	}
	while (sc != 0);
    }
    /* NOTREACHED */
}

wchar_t *strtowstr_alloc(const char *str)
{
	wchar_t *wstr;
	size_t str_len = strlen(str);
	
	wstr = (wchar_t *)malloc(sizeof(wchar_t) * (str_len + 1));
	
	if (!wstr)
		return NULL;
	
	if (MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, str_len + 1) == 0) {
		free(wstr);
		return NULL;
	}
	
	return wstr;
}

#endif /* OS_WINDOWS */

#include <time.h>

#if defined(OS_MACOSX) || defined(OS_LINUX)
#include <stdlib.h>
#elif defined(OS_WINDOWS_DESKTOP)
#include <winsock2.h>
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
	libhaggle_gettimeofday(&tv, NULL);
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


#if defined(OS_WINDOWS)
/* A wrapper for gettimeofday so that it can be used on Windows platforms. */
int libhaggle_gettimeofday(struct timeval *tv, void *tz)
{
#ifdef WINCE
	DWORD tickcount, tickcount_diff; // In milliseconds
	/* In Windows CE, GetSystemTime() returns a time accurate only to the second.
	For higher performance timers we need to use something better. */
	
	// This holds the base time, i.e. the estimated time of boot. This value plus
	// the value the high-performance timer/GetTickCount() provides gives us the
	// right time.
	static struct timeval base_time = { 0, 0};
	static DWORD base_tickcount = 0;

	if (!tv) 
		return (-1);

	/* The hardware does not implement a high performance timer.
	Note, the tick counter will wrap around after 49.7 days. 
	GetTickCount() returns number of milliseconds since device start. 
	*/
	tickcount = GetTickCount();
	
	// Should we determine the base time?
	if (base_tickcount == 0) {
		FILETIME  ft;
		SYSTEMTIME st;
		LARGE_INTEGER date, adjust;
		
		// Save tickcount
		base_tickcount = tickcount;

		// Find the system time:
		GetSystemTime(&st);
		// Convert it into "file time":
		SystemTimeToFileTime(&st, &ft);
		
		date.HighPart = ft.dwHighDateTime;
		date.LowPart = ft.dwLowDateTime;

		// 11644473600000 is the timestamp of January 1, 1601 (UTC), when
		// FILETIME started.
		// 100-nanoseconds = milliseconds * 10000
		adjust.QuadPart = 11644473600000 * 10000;

		// removes the diff between 1970 and 1601
		date.QuadPart -= adjust.QuadPart;

		// converts back from 100-nanoseconds to seconds and microseconds
		base_time.tv_sec =  (long)(date.QuadPart / 10000000);
		adjust.QuadPart = base_time.tv_sec;

		 // convert seconds to 100-nanoseconds
		adjust.QuadPart *= 10000000;

		// Remove the whole seconds
		date.QuadPart -= adjust.QuadPart;

		// Convert the remaining 100-nanoseconds to microseconds
		date.QuadPart /= 10;

		base_time.tv_usec = (long)date.QuadPart;
		
		printf("base_time: sec:%ld usec:%ld\n", base_time.tv_sec, base_time.tv_usec);
		printf("base_tickcount: %lu\n", tickcount);
	}

	tickcount_diff = tickcount - base_tickcount;
	tv->tv_sec = base_time.tv_sec;
	tv->tv_usec = base_time.tv_usec;
	
	// Add tickcount to seconds
	while (tickcount_diff >= 1000) {
		tv->tv_sec++;
		tickcount_diff -= 1000;
	}

	// Add remainding milliseconds to the microseconds part
	tv->tv_usec += (tickcount_diff * 1000);

	// If the milliseconds part is larger then 1 sec, adjust
	while (tv->tv_usec >= 1000000) {
		tv->tv_sec++;
		tv->tv_usec -= 1000000;
	}
#else
	FILETIME  ft;
	SYSTEMTIME st;
	LARGE_INTEGER date, adjust;

	if (!tv) 
		return (-1);

	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);

	date.HighPart = ft.dwHighDateTime;
	date.LowPart = ft.dwLowDateTime;

	// 11644473600000 is the timestamp of January 1, 1601 (UTC), when
	// FILETIME started.
	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;

	// removes the diff between 1970 and 1601
	date.QuadPart -= adjust.QuadPart;

	// converts back from 100-nanoseconds to seconds and microseconds
	tv->tv_sec =  (long)(date.QuadPart / 10000000);
	adjust.QuadPart = tv->tv_sec;

	// convert seconds to 100-nanoseconds
	adjust.QuadPart *= 10000000;

	// Remove the whole seconds
	date.QuadPart -= adjust.QuadPart;

	// Convert the remaining 100-nanoseconds to microseconds
	date.QuadPart /= 10;

	tv->tv_usec = (long)date.QuadPart;		
#endif
	return 0;
}
#endif

/*
	Define default path delimiters for each platform
*/

static char path[MAX_PATH_LEN + 1];

#if defined(OS_WINDOWS_MOBILE)
static int has_haggle_folder(LPCWSTR path)
{
	WCHAR my_path[MAX_PATH+1];
	long i, len;
	WIN32_FILE_ATTRIBUTE_DATA data;
	
	len = MAX_PATH;
	
	for (i = 0; i < MAX_PATH && len == MAX_PATH; i++) {
		my_path[i] = path[i];
		if (my_path[i] == 0)
			len = i;
	}
	
	if (len == MAX_PATH)
		return 0;
	
	i = -1;
	
	do {
		i++;
		my_path[len+i] = DEFAULT_STORAGE_PATH_POSTFIX[i];
	} while (DEFAULT_STORAGE_PATH_POSTFIX[i] != 0 && i < 15);
	
	if (GetFileAttributesEx(my_path, GetFileExInfoStandard, &data)) {
		return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	} 
		
	return 0;
}

#include <projects.h>
#pragma comment(lib,"note_prj.lib")
void fill_in_default_path()
{
	HANDLE	find_handle;
	WIN32_FIND_DATA	find_data;
	WCHAR best_path[MAX_PATH+1];
	int best_path_has_haggle_folder;
	ULARGE_INTEGER best_avail, best_size;
	long i;
	
	// Start with the application data folder, as a fallback:
	if (!SHGetSpecialFolderPath(NULL, best_path, CSIDL_APPDATA, FALSE)) {
		best_path[0] = 0;
		best_avail.QuadPart = 0;
		best_size.QuadPart = 0;
		best_path_has_haggle_folder = 0;
	} else {
		GetDiskFreeSpaceEx(best_path, &best_avail, &best_size, NULL);
		best_path_has_haggle_folder = has_haggle_folder(best_path);
	}

	LIBHAGGLE_DBG("Found data card path: \"%ls\" (size: %I64d/%I64d, haggle folder: %s)\n", 
		best_path, best_avail, best_size,
		best_path_has_haggle_folder ? "Yes" : "No");

	find_handle = FindFirstFlashCard(&find_data);
	
	if (find_handle != INVALID_HANDLE_VALUE) {
		do {
			// Ignore the root directory (this has been checked for above)
			if (find_data.cFileName[0] != 0) {
				ULARGE_INTEGER avail, size, free;
				int haggle_folder;
				
				GetDiskFreeSpaceEx(find_data.cFileName, &avail, &size, &free);
				haggle_folder = has_haggle_folder(find_data.cFileName);
				LIBHAGGLE_DBG("Found data card path: \"%ls\" (size: %I64d/%I64d, haggle folder: %s)\n", 
					find_data.cFileName, avail, size,
					haggle_folder ?" Yes" : "No");
				// is this a better choice than the previous one?
				// FIXME: should there be any case when a memory card is not used?
				if (1) {
					// Yes.
					
					// Save this as the path to use:
					for (i = 0; i < MAX_PATH; i++)
						best_path[i] = find_data.cFileName[i];
					best_avail = avail;
					best_size = size;
					best_path_has_haggle_folder = haggle_folder;
				}
			}
		} while(FindNextFlashCard(find_handle, &find_data));

		FindClose(find_handle);
	}
	// Convert the path to normal characters.
	for (i = 0; i < MAX_PATH; i++)
		path[i] = (char) best_path[i];
}

int libhaggle_platform_set_path(path_type_t type, const char *path)
{
	return -1;
}

const char *libhaggle_platform_get_path(path_type_t type, const char *append)
{
	long len = 0;
	
        wchar_t login1[MAX_PATH];
        int wintype = 0;
        
        switch (type) {
                case PLATFORM_PATH_HAGGLE_EXE:
                        wintype = CSIDL_PROGRAM_FILES;
                        break;
                case PLATFORM_PATH_HAGGLE_PRIVATE:
			wintype = CSIDL_APPDATA;
			break;
		case PLATFORM_PATH_HAGGLE_DATA:
                        wintype = CSIDL_APPDATA;
			fill_in_default_path();
			len = strlen(path);
			goto path_valid;
                        break;
                case PLATFORM_PATH_HAGGLE_TEMP:
                        wintype = CSIDL_APPDATA;
                        break;
                default:
                        return NULL;
        }
	
	if (!SHGetSpecialFolderPath(NULL, login1, wintype, FALSE)) {
		return NULL;
	}
	for (len = 0; login1[len] != 0; len++)
		path[len] = (char) login1[len];

path_valid:
	if (type == PLATFORM_PATH_HAGGLE_EXE || 
		type == PLATFORM_PATH_HAGGLE_PRIVATE || 
		type == PLATFORM_PATH_HAGGLE_DATA || 
		type == PLATFORM_PATH_HAGGLE_TEMP) {
			wchar_t *wpath;
			if (len + strlen(DEFAULT_STORAGE_PATH_POSTFIX) > MAX_PATH_LEN)
				return NULL;
			len += snprintf(path + len, MAX_PATH_LEN - len, "%s", DEFAULT_STORAGE_PATH_POSTFIX);

			/*
			  Make sure the path exists...
			*/
			wpath = strtowstr_alloc(path);

			if (wpath) {
				CreateDirectory(wpath, NULL);
				free(wpath);
			} 
	} 
	if (append) {
		if (len + strlen(append) > MAX_PATH_LEN)
			return NULL;
                strcpy(path + len, append);
        }
        return path;
}
#elif defined(OS_WINDOWS_VISTA)

int libhaggle_platform_set_path(path_type_t type, const char *path)
{
	return -1;
}

const char *libhaggle_platform_get_path(path_type_t type, const char *append)
{
#if 0 // Only on Windows Vista
	wchar_t *login1;
#else
	wchar_t login1[256];
#endif
	long len = 0;
#if 0 // Only on Windows Vista
        GUID wintype;
#else
	int folder;
#endif
        switch (type) {
                case PLATFORM_PATH_HAGGLE_EXE:
#if 0 // Only on Windows Vista
                        wintype = FOLDERID_ProgramFiles;
#else
			folder = CSIDL_PROGRAM_FILES;
#endif
			break;
                case PLATFORM_PATH_HAGGLE_PRIVATE:
                case PLATFORM_PATH_HAGGLE_DATA:
#if 0 // Only on Windows Vista
                        wintype = FOLDERID_LocalAppData;
#else
			folder = CSIDL_LOCAL_APPDATA;
#endif
                        break;
                case PLATFORM_PATH_HAGGLE_TEMP:
#if 0 // Only on Windows Vista
                        wintype = FOLDERID_LocalAppData;
#else
			folder = CSIDL_LOCAL_APPDATA;
#endif
                        break;
                default:
                        return NULL;
        }

#if 0 // Only on Windows Vista
	if (SHGetKnownFolderPath(&wintype, 0, NULL, &login1) != S_OK)
#else
	if (SHGetFolderPath(NULL, folder, NULL, SHGFP_TYPE_CURRENT, login1) != S_OK)
#endif
	{
		return NULL;
	}
	for (len = 0; login1[len] != 0; len++)
		path[len] = (char) login1[len];

	path[len] = '\0';
#if 0 // Only on Windows Vista
	CoTaskMemFree(login1);
#endif

        if (type == PLATFORM_PATH_HAGGLE_EXE || 
		type == PLATFORM_PATH_HAGGLE_PRIVATE || 
		type == PLATFORM_PATH_HAGGLE_DATA || 
		type == PLATFORM_PATH_HAGGLE_TEMP) {
                if (len + strlen(DEFAULT_STORAGE_PATH_POSTFIX) > MAX_PATH_LEN)
                        return NULL;
                len += snprintf(path + len, MAX_PATH_LEN - len, "%s", DEFAULT_STORAGE_PATH_POSTFIX);
	} 
        if (append) {
                if (len + strlen(append) > MAX_PATH_LEN)
                        return NULL;
                strcpy(path + len, append);
        }
        return path;
}

#elif defined(OS_ANDROID)
#include <sys/stat.h>

static char path_app_data[MAX_PATH_LEN + 1];
static int path_app_data_valid = 0;

int libhaggle_platform_set_path(path_type_t type, const char *path)
{
	if (!path || strlen(path) == 0) 
		return -1;

	if (type == PLATFORM_PATH_APP_DATA) {
		if (strlen(path) > MAX_PATH_LEN)
			return -1;
		
		strcpy(path_app_data, path);
		path_app_data_valid = 1;
		return 1;
	}
	return 0;
}

const char *libhaggle_platform_get_path(path_type_t type, const char *append)
{
	char *ret_path = path;
	struct stat sbuf;

        switch (type) {
	case PLATFORM_PATH_HAGGLE_EXE:
		sprintf(ret_path, "/system/bin");
		break;
	case PLATFORM_PATH_HAGGLE_PRIVATE:
		sprintf(ret_path, "/data/data/org.haggle.kernel/files");
		break;
	case PLATFORM_PATH_HAGGLE_DATA:
		if (stat("/sdcard/haggle", &sbuf) == 0 && 
		    (sbuf.st_mode & S_IFDIR) == S_IFDIR) {
			strcpy(path, "/sdcard/haggle");
		} else {
			sprintf(ret_path, "/data/data/org.haggle.kernel/files");
		}

		break;
	case PLATFORM_PATH_HAGGLE_TEMP:
		strcpy(ret_path, "/data/local/tmp");
		break;
	case PLATFORM_PATH_APP_DATA:
		if (path_app_data_valid == 1) {
			ret_path = path_app_data;				
		} else {
			return NULL;
		}
		break;
	default:
		return NULL;
        }
        
        if (append) {
                if (strlen(ret_path) + strlen(append) > MAX_PATH_LEN)
                        return NULL;
                strcpy(ret_path +  strlen(ret_path), append);
        }
        return ret_path;
}

#elif defined(OS_MACOSX_IPHONE)
//#include <CoreFoundation/CoreFoundation.h>
/*
  All iPhone applications are sandboxed, which means they cannot
  access the parts of the filesystem where Haggle lives. Therefore,
  an iPhone application cannot, for example, read the Haggle daemon's
  pid file, or launch the daemon if it is not running.

  The application can neither read any data objects passed as files if
  they are not in the application's sandbox.
 */

int libhaggle_platform_set_path(path_type_t type, const char *path)
{
	return -1;
}

const char *libhaggle_platform_get_path(path_type_t type, const char *append)
{
        /*
        CFStringRef homeDir = (CFStringRef)NSHomeDirectory();

        CFStringGetCString(homeDir, path, MAX_PATH_LEN, kCFStringEncodingUTF8);
        */
        switch (type) {
                case PLATFORM_PATH_HAGGLE_EXE:
                        // TODO: Set to something that makes sense, considering
                        // the iPhone apps are sandboxed.
                        strcpy(path, "/usr/bin");
                        break;
                case PLATFORM_PATH_HAGGLE_PRIVATE:
                case PLATFORM_PATH_HAGGLE_DATA:
                        strcpy(path, "/var/cache/haggle");
                        break;
                case PLATFORM_PATH_HAGGLE_TEMP:
                        strcpy(path, "/tmp");
                        break;
                default:
                        return NULL;
        }
        
        if (append) {
                if (strlen(path) + strlen(append) > MAX_PATH_LEN) {
                        return NULL;
                }
                strcpy(path +  strlen(path), append);
        }
        return path;
}
#elif defined(OS_UNIX)
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>


int libhaggle_platform_set_path(path_type_t type, const char *path)
{
	return -1;
}

const char *libhaggle_platform_get_path(path_type_t type, const char *append)
{
        struct passwd *pwd;
        char *login = NULL;
        long len = 0;
        path[0] = '\0';

        switch (type) {
                case PLATFORM_PATH_HAGGLE_EXE:
                        strcpy(path, "/usr/bin");
                        break;
                case PLATFORM_PATH_HAGGLE_PRIVATE:
                case PLATFORM_PATH_HAGGLE_DATA:
                        pwd = getpwuid(getuid());

                        if (pwd && pwd->pw_name) {
                                login = pwd->pw_name;
                        } else {
                                login = getlogin();

                                if (!login)
                                        return NULL;
                        }
                        if (strlen(DEFAULT_STORAGE_PATH_PREFIX) +
                            strlen(login) +
                            strlen(DEFAULT_STORAGE_PATH_POSTFIX) > MAX_PATH_LEN)
                                return NULL;
                        
                        len += snprintf(path, MAX_PATH_LEN, "%s%s%s",
                                        DEFAULT_STORAGE_PATH_PREFIX,
                                        login,
                                        DEFAULT_STORAGE_PATH_POSTFIX);
                        break;
                case PLATFORM_PATH_HAGGLE_TEMP:
                        strcpy(path, "/tmp");
                        break;
                default:
                        return NULL;
        }
        
        if (append) {
                if (strlen(path) + strlen(append) > MAX_PATH_LEN)
                        return NULL;
                strcpy(path +  strlen(path), append);
        }
        return path;
}
#else

int libhaggle_platform_set_path(path_type_t type, const char *path)
{
	return -1;
}

const char *libhaggle_platform_get_path(path_type_t type, const char *append)
{
        return NULL;
}

#endif
