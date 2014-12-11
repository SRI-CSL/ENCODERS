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

#if defined(OS_WINDOWS)
#define sleep(n) Sleep((n)*1000)
#endif

/*
	This program tests if it is possible to create and destroy condition 
	variables without crashing.
*/

#if defined(OS_WINDOWS)
int haggle_test_createcondition(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Create/destroy test: ");
	try{
	Condition *myCondition;
	int success;
	
	print_over_test_str(1, "Create: ");
	myCondition = new Condition();
	
	success = (myCondition != NULL);
	print_pass(success);
	if(success)
	{
		print_over_test_str(1, "Destroy: ");
		delete myCondition;
		
		print_passed();
	}
	print_over_test_str(1, "Total: ");
	return (success?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
