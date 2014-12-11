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
	This program tests to make sure that waiting with a time limit expires in 
	a timely fashion.
*/

static Mutex *counterMutex;
static Condition *myCondition;

static long counter;

class breakRunnable : public Runnable {
public:
	breakRunnable() {}
	~breakRunnable() {}
	
	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		Mutex		*myMutex;
		
		myMutex = new Mutex();
		
		// Lock the mutex (the condition variable takes a locked mutex)
		myMutex->lock();
		// Wait for the condition to be signalled, or time limit to expire:
		myCondition->timedWaitSeconds(myMutex, 1);
		
		// Decrease the counter:
		counterMutex->lock();
		counter--;
		counterMutex->unlock();
		
		delete myMutex;
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
int haggle_test_break(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Waiting test: ");
	try{

	counter = 0;
	
	counterMutex = new Mutex();
	myCondition = new Condition();
	
	counterMutex->lock();
	(new breakRunnable())->start(); counter++;
	counterMutex->unlock();
	
	// Wait for the thread to time out:
	milli_sleep(3000);
	
	print_over_test_str(1, "Released : ");
	print_pass(counter == 0);
	print_over_test_str(1, "Total: ");
	
	return ((counter == 0)?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
