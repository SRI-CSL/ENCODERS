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
#include <libcpphaggle/PlatformDetect.h>

#include <libcpphaggle/Exception.h>
#include <libcpphaggle/Thread.h>

#include <stdio.h>

#include <windows.h>
#include <Trace.h>

using namespace haggle;

#include "testhlp.h"

#if defined(OS_WINDOWS)

#define ADD_SEPA(txt) \
	printf(txt);

#define ADD_TEST(tst) \
	int tst(void); \
	try { \
		ret = tst(); \
	}catch(Exception &){ \
		ret = 2; \
	} \
	if(ret == 0) \
		printf("Passed!\n"); \
	else if(ret == 1) \
		printf("Failed!\n"); \
	else \
		printf("Crashed!\n");
/*
extern "C" {
	int haggle_test_test64(void);
	int haggle_test_bloom(void);
	int haggle_test_bloom_count(void);
	int haggle_test_sha(void);
}
*/
#define ADD_TSTC(tst) \
	try { \
		ret = tst(); \
	}catch(Exception &){ \
		ret = 2; \
	} \
	if(ret == 0) \
		printf("Passed!\n"); \
	else if(ret == 1) \
		printf("Failed!\n"); \
	else \
		printf("Crashed!\n");

#if defined(OS_WINDOWS_MOBILE)
int wmain(void)
#else
int main(void)
#endif
{
	int ret;
#if defined(OS_WINDOWS)
	WSADATA wsaData;
	int iResult;
#endif
	// Disable tracing
	Trace::trace.disable();

#if defined(OS_WINDOWS)
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed!\n");
		goto fail;
	}
#endif
/*
	This testsuite is written so it will mimic the exact output from the mac os x/linux
	testsuite (make test).
*/
	ADD_SEPA("------ Thread test suite             ------\n");
	ADD_TEST(haggle_test_createthread);
	ADD_TEST(haggle_test_jointhread);
	ADD_TEST(haggle_test_cancelthread);
	ADD_TEST(haggle_test_stopthread);
	ADD_TEST(haggle_test_stackmanagement);
	ADD_TEST(haggle_test_cancelthreadsocket);
	
	ADD_SEPA("------ Mutex test suite              ------\n");
	ADD_TEST(haggle_test_createmutex);
	ADD_TEST(haggle_test_binary);
	ADD_TEST(haggle_test_isunlocked);
	ADD_TEST(haggle_test_lock);
	ADD_TEST(haggle_test_recursive);
	ADD_TEST(haggle_test_trylock);
	ADD_TEST(haggle_test_cancelonmutex);

	ADD_SEPA("------ Condition variable test suite ------\n");
	ADD_TEST(haggle_test_createcondition);
	ADD_TEST(haggle_test_trycondition);
	ADD_TEST(haggle_test_unlockedmutex);
	ADD_TEST(haggle_test_condstate);
	ADD_TEST(haggle_test_broadcast);
	ADD_TEST(haggle_test_signal);
	ADD_TEST(haggle_test_unique);
	ADD_TEST(haggle_test_cancelthreadoncond);

	ADD_SEPA("------ Metadata test suite ----------------\n");
	ADD_TEST(haggle_test_metadata);
	
	ADD_SEPA("------ Libcpphaggle -----------------------\n");
	ADD_TEST(haggle_test_timeval);
	//ADD_TEST(haggle_test_refcount);
	ADD_TEST(haggle_test_map);
	ADD_TEST(haggle_test_list);

	ADD_SEPA("------ HaggleQueue test suite -------------\n");
	ADD_TEST(haggle_test_createtest);
	ADD_TEST(haggle_test_blockingtest);
	ADD_TEST(haggle_test_nonblockingtest);
	ADD_TEST(haggle_test_timeouttest);
	ADD_TEST(haggle_test_waitforsocket);
	ADD_TEST(haggle_test_cancelonqueue);
	
	ADD_SEPA("------ Utilities test suite          ------\n");
	ADD_TEST(haggle_test_test64);
	ADD_TEST(haggle_test_bloom);
	ADD_TEST(haggle_test_bloom_count);
	ADD_TEST(haggle_test_sha);
	
	ADD_SEPA("------ Data object test suite        ------\n");
	ADD_TEST(haggle_test_getputData);
/*
	ADD_SEPA("------ Haggle kernel test suite      ------\n");
	ADD_TEST(haggle_test_hagglemain);
	ADD_TEST(haggle_test_singleapp);
	ADD_TEST(haggle_test_multipleapp);
	ADD_TEST(haggle_test_largedo);
*/
#ifdef OS_WINDOWS
	// Cleanup winsock
	WSACleanup();
#endif
fail:

#if defined(OS_WINDOWS_XP)
	milli_sleep(100*1000);
#endif
	
	return 0;
}
#endif
