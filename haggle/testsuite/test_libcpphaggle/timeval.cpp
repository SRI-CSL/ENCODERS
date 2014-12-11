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
#include <libcpphaggle/Timeval.h>
#include <haggleutils.h>
#include <libcpphaggle/Exception.h>

using namespace haggle;
/*
  This program tests the timeval implementation
*/

#if defined(OS_WINDOWS) 
int haggle_test_timeval(void)
#else 
int main(int argc, char *argv[])
#endif

{
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Timeval test: ");

	try {
#define NUMBER_OF_RANDOM_VALUES			10
		// If adding to this array of numbers, beware of floating-point errors.
		long randomValue[NUMBER_OF_RANDOM_VALUES][2] = {
			{0,0},
			{0,500000},
			{0,250000},
			{0,-250000},
			{0,-900000},
			{5,0},
			{2,0},
			{-2,0},
			{2,-250000},
			{2,-2500000}};
		bool success = true;
		bool tmp_succ;
		long i,j;
		double randomValueDouble[NUMBER_OF_RANDOM_VALUES];
		
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
			randomValueDouble[i] = ((double)randomValue[i][0]) + ((double)randomValue[i][1])/1000000;
		print_over_test_str(1, "Create plain: ");
		tmp_succ = true;
		try {
			Timeval t;
		} catch (...) { 
			tmp_succ = false;
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(1, "Create from longs: ");
		tmp_succ = true;
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
		{
			Timeval t(randomValue[i][0], randomValue[i][1]);
			
			if (t.getTimeAsSecondsDouble() != randomValueDouble[i])
				tmp_succ = false;
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(1, "Create from double: ");
		tmp_succ = true;
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
		{
			Timeval t(randomValueDouble[i]);
			
			if (t.getTimeAsSecondsDouble() != randomValueDouble[i])
				tmp_succ = false;
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(1, "Create from both: ");
		tmp_succ = true;
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
		{
			Timeval t1(randomValueDouble[i]), 
					t2(randomValue[i][0], randomValue[i][1]);
			
			if (t1 != t2)
				tmp_succ = false;
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		tmp_succ = true;
		print_over_test_str(1, "Add: ");
		
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
		{
			for(j = 0; j < NUMBER_OF_RANDOM_VALUES; j++)
			{
				Timeval t1(randomValue[i][0], randomValue[i][1]), 
						t2(randomValue[j][0], randomValue[j][1]),
						t3;
				
				t3 = t1 + t2;
				if(	t3.getTimeAsSecondsDouble() != 
					(randomValueDouble[i] + randomValueDouble[j]))
					tmp_succ = false;
			}
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		tmp_succ = true;
		print_over_test_str(1, "Subtract: ");
		
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
		{
			for(j = 0; j < NUMBER_OF_RANDOM_VALUES; j++)
			{
				Timeval t1(randomValue[i][0], randomValue[i][1]), 
						t2(randomValue[j][0], randomValue[j][1]),
						t3;
				
				t3 = t1 - t2;
				if(	t3.getTimeAsSecondsDouble() != 
					(randomValueDouble[i] - randomValueDouble[j]))
					tmp_succ = false;
			}
		}
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		tmp_succ = true;
		print_over_test_str(1, "Compare: ");
		
		for(i = 0; i < NUMBER_OF_RANDOM_VALUES; i++)
		{
			for(j = 0; j < NUMBER_OF_RANDOM_VALUES; j++)
			{
				Timeval t1(randomValue[i][0], randomValue[i][1]), 
						t2(randomValue[j][0], randomValue[j][1]);
				bool b;
				
				b = (t1 < t2);
				if(	b != (randomValueDouble[i] < randomValueDouble[j]))
				{
					printf("!(%f < %f, %s < %s) ", 
						randomValueDouble[i], 
						randomValueDouble[j],
						t1.getAsString().c_str(),
						t2.getAsString().c_str());
					tmp_succ = false;
				}
				b = (t1 <= t2);
				if(	b != (randomValueDouble[i] <= randomValueDouble[j]))
				{
					printf("!(%f <= %f) ", 
						randomValueDouble[i], 
						randomValueDouble[j]);
					tmp_succ = false;
				}
				b = (t1 == t2);
				if(	b != (randomValueDouble[i] == randomValueDouble[j]))
				{
					printf("!(%f == %f) ", 
						randomValueDouble[i], 
						randomValueDouble[j]);
					tmp_succ = false;
				}
				b = (t1 != t2);
				if(	b != (randomValueDouble[i] != randomValueDouble[j]))
				{
					printf("!(%f != %f) ", 
						randomValueDouble[i], 
						randomValueDouble[j]);
					tmp_succ = false;
				}
				b = (t1 >= t2);
				if(	b != (randomValueDouble[i] >= randomValueDouble[j]))
				{
					printf("!(%f >= %f) ", 
						randomValueDouble[i], 
						randomValueDouble[j]);
					tmp_succ = false;
				}
				b = (t1 > t2);
				if(	b != (randomValueDouble[i] > randomValueDouble[j]))
				{
					printf("!(%f > %f) ", 
						randomValueDouble[i], 
						randomValueDouble[j]);
					tmp_succ = false;
				}
			}
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
