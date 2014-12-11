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
#include "DataObject.h"
#include <haggleutils.h>

using namespace haggle;
/*
This program tests if inserting/deleting elements into a hagglequeue work 
as it should.

Only non-blocking receive is used in this version.
*/

static GenericQueue<long> *myQueue;

static long counter;

class nonblockingtestRunnable : public Runnable {
public:
	nonblockingtestRunnable() {}
	~nonblockingtestRunnable() {}

	bool run()
	{
		long elem;

		// Insert 3, retrieve 3:
		myQueue->insert(0); counter++;
		myQueue->insert(0); counter++;
		myQueue->insert(0); counter++;

		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		// Insert, then retrieve, 3:
		myQueue->insert(0); counter++;
		
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		myQueue->insert(0); counter++;
		
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		myQueue->insert(0); counter++;
		
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		// Retrieve 3 (the main thread put these in)
		
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}
		
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

		// The queue should now be empty
		if (myQueue->retrieveTry(&elem) == QUEUE_ELEMENT)
		{
			counter--;
		}

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
int haggle_test_nonblockingtest(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str(0, "Non-blocking test: ");
	try {
		myQueue = new GenericQueue<long>();

		counter = 0;

		// Insert 3
		myQueue->insert(0); counter++;
		myQueue->insert(0); counter++;
		myQueue->insert(0); counter++;

		nonblockingtestRunnable *thr = new nonblockingtestRunnable();
		thr->start();

		// Wait for the thread to finish: (hopefully it does!)
		milli_sleep(1000);

		return counter;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
