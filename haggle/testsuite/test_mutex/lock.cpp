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
	This program tests if the lock function returns the correct value.
*/

static Mutex	*myMutex;
static bool		returned_from_lock_1, retval_1_correct,
				waited_on_lock_2,
				returned_from_lock_2, retval_2_correct,
				returned_from_lock_3;

class lockRunnable : public Runnable {
public:
	lockRunnable() {}
	~lockRunnable() {}
	
	bool run()
	{
		// The mutex is unlocked to begin with, so lock should return true.
		retval_1_correct = myMutex->lock();
		returned_from_lock_1 = true;
		
		// The mutex is unlocked, so lock should return true.
		retval_2_correct = myMutex->lock();
		returned_from_lock_2 = true;
		
		// The mutex will be destroyed here, so lock should return false.
		myMutex->lock();
		returned_from_lock_3 = true;
		
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
int haggle_test_lock(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Lock test: ");
	try{
	
	myMutex = new Mutex();
	
	returned_from_lock_1 = false;
	retval_1_correct = false;
	waited_on_lock_2 = false;
	returned_from_lock_2 = false;
	retval_2_correct = false;
	returned_from_lock_3 = false;
	
	(new lockRunnable())->start();
	
	// Wait for the thread to hang on the second lock():
	milli_sleep(1000);
	
	// Check that the thread actually waited:
	waited_on_lock_2 = !returned_from_lock_2;
	
	// Unlock the mutex:
	myMutex->unlock();
	
	// Wait for the thread to hang on the third lock():
	milli_sleep(1000);
	
	// Delete the mutex:
	delete myMutex;
	
	// Wait for the thread to (possibly) notice:
	milli_sleep(1000);

	print_over_test_str(1, "Returned from lock 1: ");
	print_pass(returned_from_lock_1);
	print_over_test_str(1, "Returned from lock 2: ");
	print_pass(returned_from_lock_2);
	print_over_test_str(1, "Returned from lock 3: ");
	print_pass(!returned_from_lock_3);
	print_over_test_str(1, "Correct return value from lock 1: ");
	print_pass(retval_1_correct);
	print_over_test_str(1, "Correct return value from lock 2: ");
	print_pass(retval_2_correct);
	print_over_test_str(1, "Waited on lock 2: ");
	print_pass(waited_on_lock_2);
	print_over_test_str(1, "Total: ");
	
	return (
		returned_from_lock_1 && retval_1_correct &&
		returned_from_lock_2 && retval_2_correct &&
		!returned_from_lock_3 &&
		waited_on_lock_2)?0:1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
