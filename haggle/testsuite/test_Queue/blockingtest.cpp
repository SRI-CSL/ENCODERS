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

/*
This program tests if inserting/deleting elements into a hagglequeue work 
as it should.

Only blocking receive is used in this version.
*/

using namespace haggle;

static GenericQueue<long> *myQueue;

static bool pass_1, pass_2, pass_3, pass_4, pass_5;

class blockingtestRunnable : public Runnable {
public:
	blockingtestRunnable() {}
	~blockingtestRunnable() {}

	bool run()
	{
		long elem;

		// Insert 3, retrieve 3:
		myQueue->insert(0);
		myQueue->insert(1);
		myQueue->insert(2);

		myQueue->retrieve(&elem);
		myQueue->retrieve(&elem);
		myQueue->retrieve(&elem);
		pass_1 = true;

		// Insert, then retrieve, 3:
		myQueue->insert(0);
		
		myQueue->retrieve(&elem);

		myQueue->insert(0);
	
		myQueue->retrieve(&elem);

		myQueue->insert(0);
		
		myQueue->retrieve(&elem);
		pass_2 = true;

		// Retrieve 3 (the main thread put these in)
		myQueue->retrieve(&elem);
		myQueue->retrieve(&elem);
		myQueue->retrieve(&elem);
		pass_3 = true;
		
		// Retrieve 3 (the main thread will put these in)
		myQueue->retrieve(&elem);
		myQueue->retrieve(&elem);
		myQueue->retrieve(&elem);
		pass_4 = true;

		// The queue should now be empty
		pass_5 = true;
		// This should block indefinately, and not count down the counter:
		myQueue->retrieve(&elem);
		pass_5 = false;
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
int haggle_test_blockingtest(void)
#else
int main(int argc, char *argv[])
#endif
{
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Blocking test: ");

	try{
		myQueue = new GenericQueue<long>();

		pass_1 = false;
		pass_2 = false;
		pass_3 = false;
		pass_4 = false;
		pass_5 = false;

		// Insert 3
		myQueue->insert(0);
		myQueue->insert(0);
		myQueue->insert(0);

		blockingtestRunnable *thr = new blockingtestRunnable();
		thr->start();

		// Wait for the thread to finish:
		milli_sleep(2000);
		
		// Insert 3
		myQueue->insert(0);
		myQueue->insert(0);
		myQueue->insert(0);
		
		// Wait for the thread to finish:
		milli_sleep(2000);
		
		print_over_test_str(1,"Subtest 1: ");
		print_pass(pass_1);
		print_over_test_str(1,"Subtest 2: ");
		print_pass(pass_2);
		print_over_test_str(1,"Subtest 3: ");
		print_pass(pass_3);
		print_over_test_str(1,"Subtest 4: ");
		print_pass(pass_4);
		print_over_test_str(1,"Subtest 5: ");
		print_pass(pass_5);
		print_over_test_str(1,"Total: ");

		return (pass_1&&pass_2&&pass_3&&pass_4&&pass_5?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
