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
#include <libcpphaggle/String.h>
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

	print_over_test_str_nl(0, "String test: ");

	try {
		bool success = true;
		bool tmp_succ;
			
		print_over_test_str(1, "Create: ");
		
		tmp_succ = true;
		try {
		string str;
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str_nl(1, "String-String tests: ");
		
		print_over_test_str(2, "Equality: ");
		
		tmp_succ = false;
		try {
		string	str1 = String("content");
		string	str2 = String("content");
		tmp_succ = (str1 == str2);
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(2, "Non-equality: ");
		
		tmp_succ = false;
		try {
		string	str1 = String("foo");
		string	str2 = String("bar");
		tmp_succ = (str1 != str2);
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(2, "Prefix test: ");
		
		tmp_succ = false;
		try {
		string	str1 = String("foobar");
		string	str2 = String("foo");
		tmp_succ = !((str1 == str2) || (str2 == str1));
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str_nl(1, "String-char * tests: ");
		
		print_over_test_str(2, "Equality: ");
		
		tmp_succ = false;
		try {
		string	str1 = String("content");
		tmp_succ = (str1 == ((char *)"content"));
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(2, "Non-equality: ");
		
		tmp_succ = false;
		try {
		string	str1 = String("foo");
		tmp_succ = (str1 != ((char *)"bar"));
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(2, "Prefix test: ");
		
		tmp_succ = false;
		try {
		string	str1 = String("foobar");
		tmp_succ = !((str1 == ((char *)"foo")) || (((char *)"foo") == str1));
		}catch(...)
		{
		tmp_succ = false;
		}
		
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(1, "Total: ");
		
		return success ? 0 : 1;
	} catch(...) {
			printf("**CRASH** ");
			return 1;
	}
}
