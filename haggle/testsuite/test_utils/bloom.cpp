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

#include <utils.h>
#include <bloomfilter.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(OS_MACOSX)
#include <stdlib.h>
#elif defined(OS_LINUX)
#include <time.h>
#include <stdlib.h>
#elif defined(OS_WINDOWS)
#include <time.h>
#endif

#define BLOOMFILTER_SIZE 1000
// Birthday principe: this should be _much_ lower than BLOOMFILTER_SIZE to 
// reasonably avoid accidental collisions:
#define NUMBER_OF_DATA_OBJECTS 100
// This must be less than NUMBER_OF_DATA_OBJECTS for the tests to work:
#define NUMBER_OF_DATA_OBJECTS_1	75
#ifdef COUNTING_BLOOMFILTER
#define NUMBER_OF_DATA_OBJECTS_2	25
#endif
#define DATA_OBJECT_BYTES				1024
char data_object[NUMBER_OF_DATA_OBJECTS][DATA_OBJECT_BYTES];
long data_object_len[NUMBER_OF_DATA_OBJECTS];

// Returns boolean true/false actually:
int check_for_data_objects_1(struct bloomfilter *bf)
{
	long i;
	
	// Check for presence of inserted objects:
	for (i = 0; i < NUMBER_OF_DATA_OBJECTS_1; i++)
		if (bloomfilter_check(bf, data_object[i], data_object_len[i]) == 0)
			return 0;
	
	// Check for non-presence of non-inserted objects:
	for (i = NUMBER_OF_DATA_OBJECTS_1; i < NUMBER_OF_DATA_OBJECTS; i++)
		if (bloomfilter_check(bf, data_object[i], data_object_len[i]) != 0)
			return 0;
	
	return 1;
}

int check_for_data_objects_2(struct bloomfilter *bf)
{
	long i;
	
	// Check for presence of inserted objects:
	for (i = NUMBER_OF_DATA_OBJECTS_1; i < NUMBER_OF_DATA_OBJECTS; i++)
		if (bloomfilter_check(bf, data_object[i], data_object_len[i]) == 0)
			return 0;
	
	// Check for non-presence of non-inserted objects:
	for (i = 0; i < NUMBER_OF_DATA_OBJECTS_1; i++)
		if (bloomfilter_check(bf, data_object[i], data_object_len[i]) != 0)
			return 0;
	
	return 1;
}
int check_for_data_objects_all(struct bloomfilter *bf)
{
	long i;
	
	// Check for presence of inserted objects:
	for (i = 0; i < NUMBER_OF_DATA_OBJECTS; i++)
		if (bloomfilter_check(bf, data_object[i], data_object_len[i]) == 0)
			return 0;
	
	return 1;
}

#if defined(OS_WINDOWS)
int haggle_test_bloom(void)
#else
int main(int argc, char *argv[])
#endif
{
	struct bloomfilter *bf1, *bf2, *bf_copy;
	char *b64_bf_copy_1, *b64_bf_copy_2;
	long i,j;
	int success = (1==1), tmp_succ;
	
	prng_init();
	
	print_over_test_str_nl(0, "Bloomfilter test: ");
	// Create random data objects:
	for (i = 0; i < NUMBER_OF_DATA_OBJECTS; i++) {
		data_object_len[i] = 
			((prng_uint8() << 8) | prng_uint8()) % DATA_OBJECT_BYTES;
		for (j = 0; j < data_object_len[i]; j++)
			data_object[i][j] = prng_uint8();
	}
	
	print_over_test_str(1, "Create bloomfilter: ");
	bf1 = bloomfilter_new((float)0.01, 1000);
	bf2 = bloomfilter_copy(bf1);
	
	// Check that it worked
	if (bf1 == NULL)
		return 1;
	
	if (bf2 == NULL) {
		bloomfilter_free(bf1);
                return 1;
        }
	
	print_passed();
	print_over_test_str(1, "Add data objects to filter 1: ");

	// Insert objects:
	for (i = 0; i < NUMBER_OF_DATA_OBJECTS_1; i++)
		bloomfilter_add(bf1, data_object[i], data_object_len[i]);
	
	print_passed();

	print_over_test_str(1, "Add data objects to filter 2: ");
	// Add some other data objects:
	for (i = NUMBER_OF_DATA_OBJECTS_1; i < NUMBER_OF_DATA_OBJECTS; i++)
		bloomfilter_add(bf2, data_object[i], data_object_len[i]);
	
	print_passed();

	print_over_test_str(1, "Filters contain those data objects: ");
	// Check filter contents:
	tmp_succ = check_for_data_objects_1(bf1);
	success &= tmp_succ;
	tmp_succ = check_for_data_objects_2(bf2);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Copy: ");
	bf_copy = bloomfilter_copy(bf1);
	
	// Check that it worked
	if(bf_copy == NULL)
		return 1;
	
	print_passed();
	print_over_test_str(1, "Copy contains data objects: ");
	// Check filter contents:
	tmp_succ = check_for_data_objects_1(bf_copy);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	bloomfilter_free(bf_copy);
	
	
	print_over_test_str(1, "To Base64: ");
	b64_bf_copy_1 = bloomfilter_to_base64(bf1);
	
	// Check that it worked
	if(b64_bf_copy_1 == NULL)
		return 1;
	
	print_passed();
	print_over_test_str(1, "From Base64: ");
	bf_copy = 
		base64_to_bloomfilter(
			b64_bf_copy_1, 
			strlen(b64_bf_copy_1));
	
	// Check that it worked
	if(bf_copy == NULL)
		return 1;
	
	print_passed();
	print_over_test_str(1, "To Base64 match: ");
	b64_bf_copy_2 = bloomfilter_to_base64(bf_copy);
	
	// Check that it worked
	if (b64_bf_copy_2 == NULL)
		return 1;
	
	// Check that the lengths are the same:
	if (strlen(b64_bf_copy_2) != strlen(b64_bf_copy_1))
		tmp_succ = (1==0);
	else {
		// Check that they are equal:
		if (strcmp(b64_bf_copy_2, b64_bf_copy_1) != 0)
			tmp_succ = (1==0);
	}
	
	success &= tmp_succ;
	print_pass(tmp_succ);
	print_over_test_str(1, "Copy contains data objects: ");
	// Check filter contents:
	tmp_succ = check_for_data_objects_1(bf_copy);
	success &= tmp_succ;
	print_pass(tmp_succ);

	print_over_test_str(1, "Merge filters 1 & 2: ");
	// Check filter contents:
	tmp_succ = (bloomfilter_merge(bf1, bf2) == MERGE_RESULT_OK);
	success &= tmp_succ;
	print_pass(tmp_succ);
        
        print_over_test_str(1, "Merged filter contains all data objects: ");
	// Check filter contents:
	tmp_succ = check_for_data_objects_all(bf1);
	success &= tmp_succ;
	print_pass(tmp_succ);

	print_over_test_str(1, "Release: ");
	bloomfilter_free(bf_copy);
	
	bloomfilter_free(bf1);
	bloomfilter_free(bf2);
	
	print_passed();
	print_over_test_str(1, "Total: ");
	// Success?
	return (success?0:1);
}
