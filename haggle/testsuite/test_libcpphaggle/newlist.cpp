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
#include <libcpphaggle/List.h>
#include <haggleutils.h>
#include <libcpphaggle/Exception.h>

using namespace haggle;
/*
  This program tests the map implementation
*/

#if defined(OS_WINDOWS) 
int haggle_test_list(void)
#else 
int main(int argc, char *argv[])
#endif

{
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "List test: ");

	try {
		bool success = true;
		bool tmp_succ;
			
		print_over_test_str(1, "Create: ");
		tmp_succ = true;
		try {
			List<long> test1_map;
		} catch (...) { 
			tmp_succ = false;
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
				
		try {
			List<long> test2_list;
			long should_be;
			
			print_over_test_str(1, "Iteration: ");
			for(should_be = 0; should_be < 15; should_be++)
				test2_list.push_back(should_be);
			
			tmp_succ = true;
			should_be = 0;
			for(List<long>::iterator it = test2_list.begin();
							it != test2_list.end();
							it++)
			{
				if(*it != should_be)
					tmp_succ = false;
				should_be++;
			}
		} catch (...) { 
			tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		// This was taken out because haggle lists don't support reverse 
		// iteration
#if 0
		try {
			List<long> test2_list;
			long should_be;
			
			print_over_test_str(1, "Reverse iteration: ");
			for(should_be = 14; should_be >= 0; should_be--)
				test2_list.push_back(should_be);
			
			tmp_succ = true;
			should_be = 0;
			for(List<long>::reverse_iterator it = test2_list.rbegin();
							it != test2_list.rend();
							it++)
			{
				if(*it != should_be)
					tmp_succ = false;
				should_be++;
			}
		} catch (...) { 
			tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
#endif

		print_over_test_str(1, "Total: ");
		
		return success ? 0 : 1;
	} catch(...) {
			printf("**CRASH** ");
			return 1;
	}
}
