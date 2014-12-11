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

#include "testhlp.h"
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;
/*
	This program tests if mutexes are created unlocked. They should be.
*/
static bool success;

static Mutex *myMutex;

class isunlockedRunnable : public Runnable {
public:
	isunlockedRunnable() {}
	~isunlockedRunnable() {}
	
	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		myMutex->lock();
		success = true;

#if defined(OS_WINDOWS)
		// Wait for the testsuite to finish before terminating this thread:
		// This removes garbage output in the middle of the testsuite results
		milli_sleep(100000);
#endif
		return false;
	}
	void cleanup() { } 
};

#if defined(OS_WINDOWS)
int haggle_test_isunlocked(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str(0, "Created unlocked test: ");
	try {
		isunlockedRunnable	*thr;

		success = false;

		myMutex = new Mutex();

		thr = new isunlockedRunnable();

		thr->start();
		milli_sleep(1000);

		return (success)?0:1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
