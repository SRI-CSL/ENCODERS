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
This program tests if inserting/deleting elements into a hagglequeue work 
as it should.

Only receive with timeout is used in this version.
*/

static GenericQueue<long> myQueue;
static Mutex myMutex;
static long number_sent, number_received, number_timed_out;

class timeouttestRunnable : public Runnable {
public:
	timeouttestRunnable() {}
	~timeouttestRunnable() {}

	bool run()
	{
		long elem;
		long i;

		// Try to retrieve 9 elements (only 5 inserted, which means 5 retrieved,
		// 4 timeouts)
		for (i = 0; i < 9; i++)
		{
			Timeval t(1);
			QueueEvent_t qev = myQueue.retrieve(&elem, &t);

			if (qev == QUEUE_ELEMENT)
			{
				number_received++;
			} else if (qev == QUEUE_TIMEOUT) {
				number_timed_out++;
				if (number_timed_out == 1)
					myMutex.unlock();
			} else {
				fprintf(stderr, "Q retrieve event %d\n", qev);
			}
		}

		// Show that we are done:
		myMutex.unlock();

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
int haggle_test_timeouttest(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str(0, "Timeout test: ");
	try{
		long	i;
		number_received = 0;
		number_sent = 0;
		number_timed_out = 0;

		// Send 3:
		for(i = 0; i < 3; i++)
		{
			myQueue.insert(0); number_sent++;
		}

		myMutex.lock();

		// Start thread:
		timeouttestRunnable *thr = new timeouttestRunnable();
		thr->start();

		// Wait for the thread to have 2 timeouts:
		myMutex.lock();

		// Send another 2 items:
		for(i = 0; i < 2; i++)
		{
			myQueue.insert(0); number_sent++;
		}

		// Wait for the thread to finish:
		myMutex.lock();

		if(	number_sent == number_received && 
			number_sent == 5 &&
			number_timed_out == 4)
			return 0;

		return 1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
