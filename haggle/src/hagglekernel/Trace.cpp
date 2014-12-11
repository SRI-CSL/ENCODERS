/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 *   Minyoung Kim (MK)
 */

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
#include "Trace.h"
#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Timeval.h>

#include <stdio.h>
#include <stdarg.h>

#include <sys/syscall.h>

int gettid(void)
{
#ifdef OS_ANDROID
  return syscall(__NR_gettid);  // MK
#else
  return syscall(SYS_gettid);
#endif
}

#if defined(OS_ANDROID)
#include <android/log.h>
#define HAGGLE_LOG_TAG "Haggle"
#endif

Trace Trace::trace;
bool Trace::stdout_enabled = false; // MOS - debugging to stdout now disabled by default
bool Trace::traceFile_enabled = false;

Trace::Trace(TraceType_t _type, bool _enabled) :
        type(_type), traceFile(NULL), startTime(Timeval::now()), enabled(_enabled), flush(false)
{
        set_trace_timestamp_base(startTime.getTimevalStruct());
}

Trace::~Trace()
{
    if (traceFile) {
        fclose(traceFile);
        traceFile = NULL;
    }
}

void Trace::setTraceType(const TraceType_t _type)
{
  type = _type;
}

#define TRACE_BUFLEN (4096)

int Trace::write(const TraceType_t _type, const char *func, const char *fmt, ...)
{
        if(_type < type) return 0; // MOS

        Mutex::AutoLocker l(m);
	char buf[TRACE_BUFLEN] = { 0 };
	char thread_id[20] = { 0 };
	va_list args;
	int len;
	Timeval t = Timeval::now() - startTime;
	FILE *stream = NULL;
	unsigned long thrNum = 0;

	if (!enabled)
		return 0;

	memset(thread_id, '\0', 20);
	memset(buf, '\0', TRACE_BUFLEN);
	memset(&args, 0, sizeof(va_list));

	switch (_type) {
		case TRACE_TYPE_ERROR: 
			stream = stderr;
			break;
		case TRACE_TYPE_LOG: 
			stream = NULL;
		case TRACE_TYPE_STAT: 
		case TRACE_TYPE_DEBUG: 
		case TRACE_TYPE_DEBUG1: 
		case TRACE_TYPE_DEBUG2: 
		default:
			stream = stdout;
			break;
	}

	/* MOS - no special treatment of LOG
	   important info should go into trace file as well

	if (_type == TRACE_TYPE_LOG) {
		va_start(args, fmt);
		len = vfprintf(stream, fmt, args);
		va_end(args);
		return len;
	}
	*/
	
	va_start(args, fmt);

	if (Thread::selfGetNum(&thrNum)) {
		snprintf(thread_id, 20, "%lu", thrNum);
	} else {
		snprintf(thread_id, 20, "-");
	}

    // MOS
    char system_thread_id[20] = { 0 };
    // SW: adding (unsigned long) typecast to avoid compiler warning
    snprintf(system_thread_id, 20, "%lu", (unsigned long)gettid());

#ifdef WINCE
    len = _vsnprintf(buf, TRACE_BUFLEN, fmt, args);
#else
    len = vsnprintf(buf, TRACE_BUFLEN, fmt, args);
#endif
    va_end(args);

// MOS - BEGIN - extract function name if possible
#define MAX_FUNC_NAME_SIZE 100
    char func_name[MAX_FUNC_NAME_SIZE+1];
    int i = 0;
    while(func[i]) {
      if(func[i] == ':' && func[i+1] == ':') break;
      if(func[i] == '(') break;
      i++;
    }

    if(func[i] == ':' && func[i+1] == ':' || func[i] == '(') {
      int j = i;
      while(j > 0 && func[j-1] != ' ') j--;
      int k = 0;
      while(k < MAX_FUNC_NAME_SIZE && func[j] && func[j] != '(') {
	func_name[k++] = func[j++];
      }
      func_name[k] = 0;
      func = func_name;
    }
// MOS - END

    if (stdout_enabled && stream) {
        // JJOY added ERROR tag to log if trace type error
        switch (_type) {
            case TRACE_TYPE_ERROR:
                fprintf(stream, "%.3lf:[%s-%s]{%s}: ERROR: %s",
                        t.getTimeAsSecondsDouble(), system_thread_id, thread_id, func, buf);
                break;
            default:
                fprintf(stream, "%.3lf:[%s-%s]{%s}: %s",
                            t.getTimeAsSecondsDouble(), system_thread_id, thread_id, func, buf);
                break;
        }
    }


    if (traceFile_enabled && traceFile) { // MOS
        // JJOY added ERROR tag to log if trace type error
        switch (_type) {
            case TRACE_TYPE_ERROR:
                    fprintf(traceFile, "%.3lf:[%s-%s]{%s}: ERROR: %s",
                            t.getTimeAsSecondsDouble(), system_thread_id, thread_id, func, buf);
                    break;
            default:
                    fprintf(traceFile, "%.3lf:[%s-%s]{%s}: %s",
                            t.getTimeAsSecondsDouble(), system_thread_id, thread_id, func, buf);
                    break;
        }
        if (flush) fflush(traceFile);
    }

    return len;
}

void Trace::enableFileTraceFlushing()
{
  flush = true;
  if(traceFile) fflush(traceFile);
}

void Trace::disableFileTraceFlushing()
{
  flush = false;
}

int Trace::writeWithoutTimestamp(const char *fmt, ...)
{  
        Mutex::AutoLocker l(m);
    va_list args;
    int len;

    if (!enabled)
        return 0;

    va_start(args, fmt);
    len = vfprintf(stdout, fmt, args);
    va_end(args);

    return len;
}

bool Trace::enableFileTrace(const string path)
{
    if (traceFile)
        return false;

    if (!create_path(path.c_str())) {
        HAGGLE_ERR("Could not create directory path \'%s\'\n", path.c_str());
        return false;
    }

    string fpath = path + PLATFORM_PATH_DELIMITER + "haggle.log";

    traceFile = fopen(fpath.c_str(), "a");

    if (!traceFile) {
        traceFile = NULL;
        HAGGLE_ERR("Could not open haggle log file \'%s\' : %s\n",
            fpath.c_str(), STRERROR(ERRNO));
        return false;
	} else {
	  traceFile_enabled = true; // MOS
    }
    return true;
}

bool Trace::disableFileTrace()
{
        traceFile_enabled = false; // MOS

	/* MOS - avoiding race condition in HAGGLE_DBG

    if (!traceFile)
        return false;

    fclose(traceFile);

    traceFile = NULL;
	*/

    return true;
}

LogTrace LogTrace::ltrace;

/*
  NOTE: there is some weird behavior in the statically allocated
  LogTrace object on Android. Due to a bug in Bionic, the constructor is
  called twice, although there is only one statically allocated
  LogTrace object in the class itself.
  
  On regular Linux, with glibc instead of bionic, the constructor is
  only called once.
 
  See android/README for details.
 */
LogTrace::LogTrace(void)
{
}

LogTrace::~LogTrace(void)
{
    close();
}

bool LogTrace::init(void)
{
    return ltrace.open("trace.log");
}

void LogTrace::fini(void)
{
    return ltrace.close();
}

bool LogTrace::open(const string name)
{
        if (traceFile)
        return false;

    if (!create_path(DEFAULT_LOG_STORAGE_PATH)) {
        fprintf(stderr, "Could not create storage path %s\n",
            DEFAULT_LOG_STORAGE_PATH);
        return false;
    }

    string filename = string(DEFAULT_LOG_STORAGE_PATH) +
        PLATFORM_PATH_DELIMITER +
        name;

    traceFile = fopen(filename.c_str(), "a");

    if (traceFile) {
        addToLog("\n\n%s: Log started, fd=%ld\n",
             Timeval::now().getAsString().c_str(),
             fileno(traceFile));
    } else {
        fprintf(stderr,"Unable to open log file!\n");
        return false;
    }

    return true;
}

void LogTrace::close(void)
{
    if (traceFile) {
        addToLog("%s: Log stopped\n", Timeval::now().getAsString().c_str());
        fclose(traceFile);
        traceFile = NULL;
    }
}

void LogTrace::addToLog(const char *fmt, ...)
{  
        Mutex::AutoLocker l(m);
    va_list args;

    if (traceFile) {
        va_start(args, fmt);
        vfprintf(traceFile, fmt, args);
        va_end(args);
    } else {
      // HAGGLE_ERR("Attempted to write to log file, but no log file open!\n"); // MOS
    }
}

BenchmarkTrace BenchmarkTrace::trace;

BenchmarkLogEntry_t *BenchmarkTrace::bench_log = NULL;

BenchmarkTrace::BenchmarkTrace() : 
    Trace(TRACE_TYPE_LOG, false), bench_log_length(0), bench_log_entries(0)
{

}

BenchmarkTrace::~BenchmarkTrace()
{

}

int BenchmarkTrace::write(const BenchmarkTraceType_t type, const Timeval &t, unsigned int res)
{
    //printf("logging\n");
    // Are we out of space?
    if (bench_log_length >= bench_log_entries) {
        BenchmarkLogEntry_t *tmp;

        // Allocate another 1000 entries:
        tmp = (BenchmarkLogEntry_t *)realloc(bench_log, (bench_log_entries + 1000) * sizeof(BenchmarkLogEntry_t));
        // Check, just in case:
        if(tmp == NULL)
            return -1;

        bench_log = tmp;
        bench_log_entries += 1000;
    }

    // Insert another log entry:
    bench_log[bench_log_length].timestamp = t;
    bench_log[bench_log_length].type = type;
    bench_log[bench_log_length].result = res;
    bench_log_length++;

    return 0;
}

void BenchmarkTrace::dump(unsigned int DataObjects_Attr,
         unsigned int Nodes_Attr,
         unsigned int Attr_Num,
         unsigned int DataObjects_Num)
{
    FILE *fp;
#define MAX_FILEPATH_LEN 256
    char str[MAX_FILEPATH_LEN];
    size_t i;
    size_t strlen = 0;
    char typeChar[10] = { 'I', 'S', 'E', 'R', 'F', 'D', 'Z', 'Y', 'X', 'W' };   // init, sqlStart, sqlEnd, result, finish, delay, node insert delay, node delete delay, dobj insert delay, dobj delete delay

    // Dummy check: empty logs aren't written to file
    if (bench_log_length == 0)
        return;

    strlen += snprintf(str + strlen, MAX_FILEPATH_LEN - strlen, HAGGLE_DEFAULT_STORAGE_PATH);
    strlen += snprintf(str + strlen, MAX_FILEPATH_LEN - strlen, PLATFORM_PATH_DELIMITER);

    // Open file with timestamp of first log entry:
    strlen += snprintf(str + strlen, MAX_FILEPATH_LEN - strlen,
        "benchmark-"
#if defined(OS_MACOSX)
        "macosx-"
#elif  defined(OS_LINUX)
        "linux-"
#elif  defined(WINCE)
        "winmobile-"
#elif  defined(WIN32)
        "windows-"
#else
        "unknown-"
#endif
        "%u-%u-%u-%u-%ld.log",
        DataObjects_Attr, Nodes_Attr, Attr_Num, DataObjects_Num,
        bench_log[0].timestamp.getSeconds());

    fp = fopen(str, "w");

    if (fp) {
                fprintf(fp, 
                        "# DataObjects_Attr=%u Nodes_attr=%u Attr_num=%u DataObjects_num=%u Platform="
#if defined(OS_MACOSX)
                        "Mac OS X"
#elif  defined(OS_LINUX)
                        "Linux"
#elif  defined(WINCE)
                        "Windows mobile"
#elif  defined(WIN32)
                        "Windows"
#else
                        "Unknown"
#endif
                        "\n", 
                        DataObjects_Attr, Nodes_Attr, Attr_Num, DataObjects_Num);
                // Outpout each log entry:
                for (i = 0; i < bench_log_length; i++) {
                        
                        if (bench_log[i].type == BENCH_TYPE_RESULT) {
                                fprintf(fp, "%s %c %u\n", 
                    bench_log[i].timestamp.getAsString().c_str(),
                    typeChar[bench_log[i].type],
                    bench_log[i].result);
                        } else {
                                fprintf(fp, "%s %c\n", 
                    bench_log[i].timestamp.getAsString().c_str(),
                    typeChar[bench_log[i].type]);
                        }
                        
                }
                // Close file:
                fclose(fp);
                
    } else {
        HAGGLE_ERR("Attempted to write to log file, but no log file open!\n");
    }
}
