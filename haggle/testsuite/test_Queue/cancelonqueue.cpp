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
#include <libcpphaggle/GenericQueue.h>
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;
/*
	This program tests if a thread can be cancelled while it's waiting on a 
	hagglequeue. It should be.
*/

static GenericQueue<long> *myQueue;

static bool success, ran_cleanup;

class cancelonqueueRunnable : public Runnable {
public:
	cancelonqueueRunnable() {}
	~cancelonqueueRunnable() {}

	bool run()
	{
		long elem;

		myQueue->retrieve(&elem);
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
int haggle_test_cancelonqueue(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Cancel on queue test: ");
	try{
		myQueue = new GenericQueue<long>();

		success = true;
		ran_cleanup = false;
		
		// Start thread:
		cancelonqueueRunnable *thr = new cancelonqueueRunnable();
		thr->start();

		// Wait for the thread to be waiting on the queue:
		milli_sleep(1000);
		
		// Cancel the thread:
		thr->cancel();
		
		// Wait for the thread to finish:
		milli_sleep(1000);
		
		// Insert a fake element into the queue:
		myQueue->insert(0);
		
		// Wait for the thread to (hopefully not) notice:
		milli_sleep(1000);
		
		print_over_test_str(1,"Did not return from retrieve(): ");
		print_pass(success);
		print_over_test_str(1,"Ran cleanup: ");
		print_pass(ran_cleanup);
		print_over_test_str(1,"Total: ");

		return (success&&ran_cleanup?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
