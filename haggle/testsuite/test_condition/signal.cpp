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
	This program tests to make sure that conditions are temporary.
*/

static Mutex		*counterMutex;
static Condition	*myCondition;

static long counter;

class signalRunnable : public Runnable {
public:
	signalRunnable() {}
	~signalRunnable() {}
	
	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		Mutex		*myMutex;
		
		myMutex = new Mutex();
		
		// Lock the mutex (the condition variable takes a locked mutex)
		myMutex->lock();
		// Wait for the condition to be signalled:
		myCondition->wait(myMutex);
		
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
int haggle_test_signal(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Signal test: ");
	try{
	
	counter = 0;
	
	counterMutex = new Mutex();
	myCondition = new Condition();
	
	counterMutex->lock();
	(new signalRunnable())->start(); counter++;
	(new signalRunnable())->start(); counter++;
	(new signalRunnable())->start(); counter++;
	(new signalRunnable())->start(); counter++;
	counterMutex->unlock();
	
	// Wait for the threads to be waiting on the condition:
	milli_sleep(1000);
	// Tell all threads to release:
	myCondition->signal();
	// Wait for the threads to be done:
	milli_sleep(1000);
	
	print_over_test_str(1, "Released one: ");
	print_pass(counter < 4);
	print_over_test_str(1, "Released only one: ");
	print_pass(counter == 3);
	print_over_test_str(1, "Total: ");
	
	return (counter==3?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
