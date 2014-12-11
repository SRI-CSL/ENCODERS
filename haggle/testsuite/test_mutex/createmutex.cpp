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
  This program tests if it is possible to create and destroy mutexes without 
  crashing.
*/

#if defined(OS_WINDOWS)
int haggle_test_createmutex(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Create/destroy test: ");
	try {
		Mutex *myMutex;
		int success = (1==1);
	
		try {
			print_over_test_str(1, "Create: ");
			myMutex = new Mutex();
		
			success &= (myMutex != NULL);
			print_pass((myMutex != NULL));
		
			print_over_test_str(1, "Destroy: ");
			if(myMutex)
			{
				delete myMutex;
				print_passed();
			}else{
				print_failed();
				success = false;
			}
		
			print_over_test_str(1, "Total: ");
		} catch(Exception &) {
			return 2;
		}
		return 0;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
