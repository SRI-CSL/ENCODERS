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
This program tests if the trylock function returns the correct value.
*/
#if defined(OS_WINDOWS)
int haggle_test_trylock(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Trylock test: ");
	try{
		Mutex *myMutex;
		int success = (1==1), tmp_succ;

		myMutex = new Mutex();

		print_over_test_str(1, "Unlocked mutex: ");
		// The mutex is unlocked to begin with, so trylock should return true.
		tmp_succ = myMutex->trylock();
		success &= tmp_succ;
		print_pass(tmp_succ);

		print_over_test_str(1, "Locked mutex: ");
		// The mutex is now locked, so trylock() should return false.
		tmp_succ = !myMutex->trylock();
		success &= tmp_succ;
		print_pass(tmp_succ);
		print_over_test_str(1, "Total: ");

		return (success?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
