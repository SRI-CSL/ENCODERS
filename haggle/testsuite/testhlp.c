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

/*
	This file should be identical with testhlp.cpp
*/
#include "testhlp.h"

#include <stdio.h>
#include <string.h>

static const char *_test_str =
	"...................................................... ";
static const char *_empt_str =
	"                                                       ";

#define test_str(l) (&(_test_str[(l)*4]))
#define empty_str(l) (&(_empt_str[strlen(_empt_str)-(l)*4]))

void print_over_test_str_(
#if defined(OS_WINDOWS)
		unsigned long level, 
#else
		long level,
#endif
		char *str)
{
	unsigned long len; 

	if(level < 0)
		level = 0;
	if(level > strlen(_empt_str)/4)
		level = strlen(_empt_str)/4;
	
	len = strlen(str);
	if(len <= strlen(test_str(level)) - 4)
		printf("%s%s%s", empty_str(level), str, &(test_str(level)[len]));
	else
		printf("%s%s... ", empty_str(level), str);
}

void print_over_test_str_nl_(
#if defined(OS_WINDOWS)
		unsigned long level, 
#else
		long level,
#endif
		char *str)
{
	unsigned long len; 

	if(level < 0)
		level = 0;
	if(level > strlen(_empt_str)/4)
		level = strlen(_empt_str)/4;
	
	len = strlen(str);
	if(len <= strlen(test_str(level)) - 4)
		printf("%s%s%s\n", empty_str(level), str, &(test_str(level)[len]));
	else
		printf("%s%s... \n", empty_str(level), str);
}

void print_pass(int passed)
{
	printf(passed?"Passed!\n":"Failed! ***\n");
}
