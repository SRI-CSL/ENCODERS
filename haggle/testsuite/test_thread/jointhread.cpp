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
#include <haggleutils.h>

using namespace haggle;
/*
	This program tests if it is possible to join on a thread.
	
	It first creates and starts a thread, then tries to join it. The thread
	waits for 1 second, then sets the success flag. If the main thread managed
	to join, the success flag will be set when the join function returns.
*/

static bool success, ran_cleanup;

class jointhreadRunnable : public Runnable {
public:
	jointhreadRunnable() {}
	~jointhreadRunnable() {}
	
	bool run()
	{
		milli_sleep(1000);
		success = true;
		milli_sleep(500);

		return false;
	}
	void cleanup()
	{
		ran_cleanup = true;
		milli_sleep(500);
	} 
};

#if defined(OS_WINDOWS)
int haggle_test_jointhread(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Join test: ");
	try{
	jointhreadRunnable	*thr;
	success = false;
	ran_cleanup = false;

	thr = new jointhreadRunnable();
	thr->start();
	// We need to let the thread start up
	milli_sleep(500);
	thr->join();
	print_over_test_str(1, "Joined: ");
	print_pass(success);
	print_over_test_str(1, "Ran cleanup: ");
	print_pass(ran_cleanup);
	print_over_test_str(1, "Total: ");
	return ((success&&ran_cleanup)?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
