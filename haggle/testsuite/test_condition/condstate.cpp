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

static Mutex *myMutex;
static Condition *myCondition;

static bool success;

class condstateRunnable : public Runnable {
public:
	condstateRunnable() {}
	~condstateRunnable() {}

	Mutex *getMutex() { return &mutex; }

	bool run()
	{
		myCondition->wait(myMutex);

		success = false;

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
int haggle_test_condstate(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str(0, "Temporary state test: ");
	try{
		success = true;

		myMutex = new Mutex();
		myCondition = new Condition();

		myMutex->lock();
		myCondition->signal();

		(new condstateRunnable())->start();

		// Wait for the thread to be waiting on the condition:
		milli_sleep(1000);

		return (success?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
