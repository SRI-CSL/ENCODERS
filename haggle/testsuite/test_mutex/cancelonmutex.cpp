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
	This program tests if threads can be cancelled while waiting on a mutex. 
	They should be.
*/
static bool success, ran_cleanup;

static Mutex *myMutex;

class cancelonmutexRunnable : public Runnable {
public:
	cancelonmutexRunnable() {}
	~cancelonmutexRunnable() {}
	
	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		myMutex->lock();
		success = false;

		return false;
	}
	void cleanup()
	{
		ran_cleanup = true;
#if defined(OS_WINDOWS)
		// Wait for the testsuite to finish before terminating this thread:
		// This removes garbage output in the middle of the testsuite results
		milli_sleep(100000);
#endif
	} 
};

#if defined(OS_WINDOWS)
int haggle_test_cancelonmutex(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Cancel on mutex test: ");
	try{
	
	success = true;
	ran_cleanup = false;
	
	// Create a locked mutx
	myMutex = new Mutex();
	myMutex->lock();
	
	// Start the thread:
	cancelonmutexRunnable *thr = new cancelonmutexRunnable();
	thr->start();
	
	// Wait for the thread to be waiting on the mutex:
	milli_sleep(1000);
	
	// Cancel the thread:
	thr->cancel();
	
	// Wait for the thread to notice:
	milli_sleep(1000);
	
	// Unlock the mutex:
	myMutex->unlock();
	
	// Wait for the thread to notice:
	milli_sleep(1000);
	
	print_over_test_str(1, "Did not return from lock(): ");
	print_pass(success);
	print_over_test_str(1, "Ran cleanup: ");
	print_pass(ran_cleanup);
	print_over_test_str(1, "Total: ");
	
	return (success&&ran_cleanup)?0:1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
