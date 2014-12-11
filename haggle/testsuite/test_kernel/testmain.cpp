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

#include "hagglemain.h"
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;

#include <libhaggle/haggle.h>

#include <stdio.h>
#include "testhlp.h"

/*
	This program tests to make sure that the hagglemain functions can properly
	set up and shut down a haggle kernel.
*/
static bool has_shut_down;

static Condition	*my_cond;
static Mutex		*mutex;

class hagglemainRunnable : public Runnable {
public:
	hagglemainRunnable() {}
	~hagglemainRunnable() {}

	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		// Wait for a tiny while so the main thread will be waiting on the 
		// condition variable
		milli_sleep(100);
		has_shut_down = hagglemain_stop_and_wait();
		my_cond->signal();
#if defined(OS_WINDOWS)
		// Wait for the testsuite to finish before terminating this thread:
		// This removes garbage output in the middle of the testsuite results
		milli_sleep(100000);
#endif
		return false;
	}
	void cleanup() {} 
};

#if defined(OS_WINDOWS)
int haggle_test_hagglemain(void)
#else
int main(int argc, char *argv[])
#endif
{
	bool success = true, tmp_succ;
		
	// Disable tracing
	trace_disable(true);

	set_trace_level(0);
	mutex = new Mutex();
	mutex->lock();
	my_cond = new Condition();
	
	has_shut_down = false;
	print_over_test_str_nl(0, "Basic function test: ");
	
	// Is the haggle kernel already running?
	print_over_test_str(1, "Is originally not running: ");
	tmp_succ = !hagglemain_is_running();
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Can we start a haggle kernel?
	print_over_test_str(1, "Can start: ");
	tmp_succ = 
		hagglemain_start(
			// All managers:
			hagglemain_have_all_managers 
			// Except the connectivity manager, since it most likely starts a 
			// Bluetooth scan:
			^ hagglemain_have_connectivity_manager,
			my_cond);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Sleep while haggle starts up:
	my_cond->timedWaitSeconds(mutex, 15);
	
	// Check that haggle has started:
	print_over_test_str(1, "Is running: ");
	tmp_succ = hagglemain_is_running();
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Stop haggle kernel:
	// Do this in another thread, in case it fails:
	(new hagglemainRunnable())->start();
	
	// Sleep while haggle shuts down:
	my_cond->timedWaitSeconds(mutex, 15);
	
	// Delay output so that winmobile can report thread shutdown without 
	// messing up the output:
	print_over_test_str(1, "Can shut down: ");
	success &= has_shut_down;
	print_pass(has_shut_down);
	
	// Check that haggle has shut down:
	print_over_test_str(1, "Is not running: ");
	tmp_succ = !hagglemain_is_running();
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Total: ");
	
	return (success?0:1);
}

