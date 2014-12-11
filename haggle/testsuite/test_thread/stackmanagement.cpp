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

#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Reference.h>
#include "utils.h"
#include <haggleutils.h>
#include "testhlp.h"

using namespace haggle;

/*
	This program tests how things on the stack are handled when shutting down
	a thread in various ways.
*/

typedef enum {
	thread_cancel_self,
	thread_wait_to_be_cancelled,
	thread_exit_self
} thread_termination_style;

typedef Reference<Mutex> MutexRef;
MutexRef	dummy_mutex[4];

class stackmanagementRunnable : public Runnable {
public:
	thread_termination_style ts;
	long which_mutex;
	
	stackmanagementRunnable(thread_termination_style _ts, long wm) :
		ts(_ts), which_mutex(wm)
	{}
	~stackmanagementRunnable() {}
	
	bool run()
	{
		MutexRef	myRef = dummy_mutex[which_mutex];
		
		switch(ts)
		{
			case thread_cancel_self:
				cancel();
			break;
			
			case thread_wait_to_be_cancelled:
				cancelableSleep(100000);
			break;
			
			case thread_exit_self:
			break;
		}
		return false;
	}
	void cleanup()
	{
	} 
};

#if defined(OS_WINDOWS)
int haggle_test_stackmanagement(void)
#else
int main(int argc, char *argv[])
#endif
{
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Stack management test: ");
	try{
	stackmanagementRunnable	*thr1, *thr2, *thr3, *thr4;
	
	dummy_mutex[0] = new Mutex();
	dummy_mutex[1] = new Mutex();
	dummy_mutex[2] = new Mutex();
	dummy_mutex[3] = new Mutex();
	
	thr1 = new stackmanagementRunnable(thread_cancel_self, 0);
	thr2 = new stackmanagementRunnable(thread_wait_to_be_cancelled, 1);
	thr3 = new stackmanagementRunnable(thread_wait_to_be_cancelled, 2);
	thr4 = new stackmanagementRunnable(thread_exit_self, 3);
	
	if(!thr1->start())
		return 1;
	if(!thr2->start())
		return 1;
	if(!thr3->start())
		return 1;
	if(!thr4->start())
		return 1;
	
	// Wait for the threads to do their thing...
	milli_sleep(1000);
	
	// Thread 1 cancels itself - no need to deal with it:
	
	// Cancel thread 2:
	thr2->cancel();
	// Stop thread 3:
	thr3->stop();
	// Thread 4 terminates properly - no need to deal with it:
	
	
	// Wait for the threads to be finished:
	milli_sleep(1000);
	
	print_over_test_str(1, "Self-cancelled thread's stack unrolls: ");
	printf("%s\n", (dummy_mutex[0].refcount() == 1)?"Yes":"No");
	print_over_test_str(1, "Cancelled thread's stack unrolls: ");
	printf("%s\n", (dummy_mutex[1].refcount() == 1)?"Yes":"No");
	print_over_test_str(1, "Joined thread's stack unrolls: ");
	printf("%s\n", (dummy_mutex[2].refcount() == 1)?"Yes":"No");
	print_over_test_str(1, "Self-terminating thread's stack unrolls: ");
	printf("%s\n", (dummy_mutex[3].refcount() == 1)?"Yes":"No");
	
	dummy_mutex[0] = NULL;
	dummy_mutex[1] = NULL;
	dummy_mutex[2] = NULL;
	dummy_mutex[3] = NULL;
	
	print_over_test_str(1, "Total: ");

	return 0;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
