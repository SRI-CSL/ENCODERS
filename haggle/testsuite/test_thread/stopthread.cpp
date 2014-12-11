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
	This program tests if it is possible to stop a thread when it is waiting on
	the cancelableSleep() function.
	
	It first creates the thread, then starts it, waits for a second to allow it
	to start executing, then stops it, then waits another 3 seconds. The 
	thread is supposed to wait three seconds and then set the success flag to 
	false, so after one second it is still in the call to sleep. If the stop 
	function is successful, success is never set to 0.
*/

static volatile bool success, ran_cleanup;

class stopthreadRunnable : public Runnable {
public:
	stopthreadRunnable() {}
	~stopthreadRunnable() {}
	
	bool run()
	{
		success = true;
		cancelableSleep(3000);
		success = false;

		return false;
	}
	void cleanup() { ran_cleanup = true; } 
};

#if defined(OS_WINDOWS)
int haggle_test_stopthread(void)
#else
int main(int argc, char *argv[])
#endif
{
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Stop test: ");
	try{
	stopthreadRunnable	*thr;
	success = false;
	ran_cleanup = false;
	
	thr = new stopthreadRunnable();
	thr->start();
	// Wait for the thread to start: 
	// (Can't use a mutex - they haven't been tested yet.)
	do{
		milli_sleep(1*1000);
	}while(!success);
	// The thread should now be sleep()ing, stop it:
	thr->stop();
	// Wait for longer than it would take the thread to end normally:
	milli_sleep(5*1000);
	print_over_test_str(1, "Thread stopped: ");
	print_pass(success);
	print_over_test_str(1, "Thread ran cleanup: ");
	print_pass(ran_cleanup);
	print_over_test_str(1, "Total: ");
	
	return ((success&&ran_cleanup)?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
