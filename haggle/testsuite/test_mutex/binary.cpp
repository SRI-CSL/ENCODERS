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
This program tests if it is possible to unlock a mutex more than once.
*/
static long counter;

static Mutex *myMutex;

class binaryRunnable : public Runnable {
public:
	binaryRunnable() {}
	~binaryRunnable() {}

	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		myMutex->lock(); counter++;
		myMutex->lock(); counter++;
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
int haggle_test_binary(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str(0, "Is binary test: ");
	try {
		binaryRunnable	*thr;

		counter = 0;

		myMutex = new Mutex();

		thr = new binaryRunnable();

		myMutex->unlock();
		myMutex->unlock();
		myMutex->unlock();
		thr->start();
		milli_sleep(1000);

		return (counter == 1)?0:1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
