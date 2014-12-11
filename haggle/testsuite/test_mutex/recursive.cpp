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
	This program tests if mutexes are recursive (in both variants).
*/

static Mutex			*myMutex;
static RecursiveMutex	*myRecMutex;
static bool				mutex_returned_from_lock_1,
						mutex_returned_from_lock_2,
						recursive_mutex_returned_from_lock_1,
						recursive_mutex_returned_from_lock_2;

class recursiveRunnable1 : public Runnable {
public:
	recursiveRunnable1() {}
	~recursiveRunnable1() {}
	
	bool run()
	{
		// The mutex is unlocked to begin with, so locking should be ok.
		myMutex->lock();
		mutex_returned_from_lock_1 = true;
		
		// The mutex is locked, so lock should not return.
		myMutex->lock();
		mutex_returned_from_lock_2 = true;
		
#if defined(OS_WINDOWS)
		// Wait for the testsuite to finish before terminating this thread:
		// This removes garbage output in the middle of the testsuite results
		milli_sleep(100000);
#endif
		return false;
	}
	void cleanup() { } 
};

class recursiveRunnable2 : public Runnable {
public:
public:
	recursiveRunnable2() {}
	~recursiveRunnable2() {}
	
	bool run()
	{
		// The mutex is unlocked to begin with, so locking should be ok.
		myRecMutex->lock();
		recursive_mutex_returned_from_lock_1 = true;
		
		// The mutex is locked but recursive, so locking should be ok.
		myRecMutex->lock();
		recursive_mutex_returned_from_lock_2 = true;
		
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
int haggle_test_recursive(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Recursiveness test: ");
	try{
	
	myMutex = new Mutex();
	myRecMutex = new RecursiveMutex();
	
	mutex_returned_from_lock_1 = false;
	mutex_returned_from_lock_2 = false;
	recursive_mutex_returned_from_lock_1 = false;
	recursive_mutex_returned_from_lock_2 = false;
	
	(new recursiveRunnable1())->start();
	(new recursiveRunnable2())->start();
	
	// Wait for the threads to finish:
	milli_sleep(1000);
	
	print_over_test_str(1, "Mutex returned from lock 1: ");
	print_pass(mutex_returned_from_lock_1);
	print_over_test_str(1, "Mutex did not return from lock 2: ");
	print_pass(!mutex_returned_from_lock_2);
	print_over_test_str(1, "Recursive mutex returned from lock 1: ");
	print_pass(recursive_mutex_returned_from_lock_1);
	print_over_test_str(1, "Recursive mutex returned from lock 2: ");
	print_pass(recursive_mutex_returned_from_lock_2);
	print_over_test_str(1, "Total: ");
	
	return (
		mutex_returned_from_lock_1 && !mutex_returned_from_lock_2 &&
		recursive_mutex_returned_from_lock_1 && 
		recursive_mutex_returned_from_lock_2)?0:1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
