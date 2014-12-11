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
#include "DataObject.h"
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;

/*
	This program tests the functionality of the putData and getData_* functions
	for creating data objects.
*/

#define number_of_data_object_buffers	3
#define data_object_buffer_size			1024

static char	data_object_buffer
				[number_of_data_object_buffers]
				[data_object_buffer_size] =
	{
		/*
			The minimum correct data object:
		*/
		"<Haggle></Haggle>"
		,
		/*
			A data object with data
		*/
		"<Haggle>"
			"<Attr name=\"FileName\">"
				"test_object2.do"
			"</Attr>"
			"<Attr name=\"DataLen\">"
				"10"
			"</Attr>"
		"</Haggle>"
		"abcdefghij"
		,
		/*
			This data object will have the same SHA1 id as the previous object.
		*/
		"<Haggle>"
			"<Attr name=\"FileName\">"
				"test_object2.do"
			"</Attr>"
			"<Attr name=\"DataLen\">"
				"10"
			"</Attr>"
		"</Haggle>"
		"jihgfedcba"
	};

#if defined(OS_WINDOWS)
int haggle_test_getputData(void)
#else
int main(int argc, char *argv[])
#endif
{
	bool success;//, tmp_succ;
	
	// Disable tracing
	trace_disable(true);

	success = true;
	print_over_test_str(0, "putData/getData test: ");
	fprintf(stdout, "OUTDATED! DO NOT RELY ON THESE RESULTS!\n");

	try{
		/*DataObject		*dObj1, *dObj2;
		size_t			data_object_len[number_of_data_object_buffers], i;
		size_t			len;
		char			tmp_data[data_object_buffer_size];
		
		for(i = 0; i < number_of_data_object_buffers; i++)
			data_object_len[i] = strlen(data_object_buffer[i]);
		
#define put_obj_whole(dobj, which) \
		try { \
			(dobj) = new DataObject(NULL); \
			(void) \
				(dobj)->putData( \
					(unsigned char *) data_object_buffer[which],  \
					data_object_len[which], \
					&len); \
			tmp_succ = (len == 0); \
		}catch (Exception &e) { \
			printf("Threw exception - %s", e.getErrorMsg()); \
			tmp_succ = false; \
		} \
		success &= tmp_succ;
		
#define get_obj_whole_ns(dobj) \
		try { \
			tmp_succ = (dobj)->getData_begin(); \
			if(tmp_succ) \
			{ \
				len = (dobj)->getData_retrieve( \
						(unsigned char *)tmp_data, \
						data_object_buffer_size); \
				(dobj)->getData_end(); \
				tmp_succ = (len > 0); \
			} \
		}catch (Exception &) { \
			printf("Threw exception - "); \
			tmp_succ = false; \
		}

#define get_obj_whole(dobj) \
	get_obj_whole_ns(dobj); \
	success &= tmp_succ;
		
#define put_obj_1byte(dobj, which) \
		try { \
			(dobj) = new DataObject(NULL); \
			tmp_succ = true; \
			for(i = 0; i < data_object_len[which] && tmp_succ; i++) \
			{ \
				(void) \
					(dobj)->putData( \
						(unsigned char *) (&(data_object_buffer[which][i])),  \
						1, \
						&len); \
						\
				tmp_succ =  \
					( \
						(len > 0) &&  \
						(len <= (data_object_len[which] - (i + 1))) &&  \
						(i != (data_object_len[which]-1))) || \
					( \
						(len == 0) &&  \
						(i == (data_object_len[which]-1)) \
					); \
			} \
		}catch (Exception &) { \
			printf("Threw exception - "); \
			tmp_succ = false; \
		} \
		success &= tmp_succ;

#define put_obj_10byte(dobj, which) \
		try { \
			(dobj) = new DataObject(NULL); \
			tmp_succ = true; \
			for(i = 0; i < data_object_len[which] && tmp_succ; i+=10) \
			{ \
				ssize_t written; \
				written = \
					(dobj)->putData( \
						(unsigned char *) (&(data_object_buffer[which][i])),  \
						10, \
						&len); \
				if(len > 0) \
					written = len; \
				else \
					written = written - 10; \
				tmp_succ = \
					( \
						(len > 0) &&  \
						(len <= (data_object_len[which] - (i + 1))) &&  \
						(i + 10 < (data_object_len[which]))) || \
					( \
						(len <= 0) &&  \
						(i + 10 >= (data_object_len[which]-1)) && \
						(i + 10 + len == (data_object_len[which])) \
					); \
			} \
		}catch (Exception &) { \
			printf("Threw exception - "); \
			tmp_succ = false; \
		} \
		success &= tmp_succ;
		
#define get_obj_1byte_ns(dobj) \
		try { \
			tmp_succ = (dobj)->getData_begin(); \
			if(tmp_succ) \
			{ \
				i = 0; \
				do { \
					(void) \
						(dobj)->getData_retrieve( \
							(unsigned char *) (&(tmp_data[i])),  \
							1, \
							&len); \
					i++; \
				}while(len > 0) \
				len = i; \
				(dobj)->getData_end(); \
				tmp_succ = (len > 0); \
			} \
		}catch (Exception &) { \
			printf("Threw exception - "); \
			tmp_succ = false; \
		}

#define get_obj_1byte(dobj) \
	get_obj_1byte_ns(dobj); \
	success &= tmp_succ;
		
		// Test 1:
		print_over_test_str(1, "putData test 1: ");
		// Put in the entire data object all at once:
		put_obj_whole(dObj1, 0);
		print_pass(tmp_succ);
		delete dObj1;
		
		
		// Test 2a:
		print_over_test_str(1, "putData test 2a: ");
		// Put in the data object 1 byte at a time:
		put_obj_1byte(dObj1, 0);
		print_pass(tmp_succ);
		delete dObj1;
		
		
		// Test 2b:
		print_over_test_str(1, "putData test 2b: ");
		// Put in the data object 10 bytes at a time:
		put_obj_10byte(dObj1, 0);
		print_pass(tmp_succ);
		delete dObj1;
		
		
		// Test 3:
		print_over_test_str(1, "putData test 3: ");
		// Put in the data object all at once (data object with file):
		put_obj_whole(dObj1, 1);
		print_pass(tmp_succ);
		delete dObj1;
		
		
		// Test 4a:
		print_over_test_str(1, "putData test 4a: ");
		// Put in the data object 1 byte at a time (data object with file):
		put_obj_1byte(dObj1, 1);
		print_pass(tmp_succ);
		delete dObj1;
		
		
		// Test 4b:
		print_over_test_str(1, "putData test 4b: ");
		// Put in the data object 10 bytes at a time:
		put_obj_10byte(dObj1, 1);
		print_pass(tmp_succ);
		delete dObj1;
		
		
		// Test 5:
		print_over_test_str(1, "ID test 1: ");
		// Put in the entire data object all at once:
		put_obj_whole(dObj1, 1);
		if(!tmp_succ)
			goto fail_test_5_1;
		// Put in the entire data object all at once:
		put_obj_whole(dObj2, 2);
		if(!tmp_succ)
			goto fail_test_5_2;
		
		// Test the IDs against each other, they should be equal, even though 
		// the objects aren't exactly equal:
		tmp_succ = (strcmp(dObj1->getIdStr(), dObj2->getIdStr()) == 0);
		success &= tmp_succ;
		
fail_test_5_2:
		delete dObj2;
fail_test_5_1:
		delete dObj1;
		print_pass(tmp_succ);
		
		
		// Test 6:
		// Put in the entire data object all at once:
		put_obj_whole(dObj1, 1);
		if(!tmp_succ)
			goto fail_test_6_1;
		// Put in the entire data object all at once:
		put_obj_whole(dObj2, 1);
		if(!tmp_succ)
			goto fail_test_6_2;
		
		print_over_test_str(1, "ID test 2: ");
		// Test the paths against each other, they should not be equal, just 
		// because the objects are:
		tmp_succ = 
			(strcmp((const char *)dObj1->getId(), (const char *)dObj2->getId()) 
				== 0);
		success &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(1, "File path test: ");
		// Test the paths against each other, they should not be equal, just 
		// because the objects are:
		tmp_succ = 
			(strcmp(dObj1->getFilePath().c_str(), dObj2->getFilePath().c_str()) 
				!= 0);
		success &= tmp_succ;
		print_pass(tmp_succ);

		print_over_test_str(1, "File name test: ");
		// Test the names against each other, they should be equal because the 
		// objects are:
		tmp_succ = 
			(strcmp(dObj1->getFileName().c_str(), dObj2->getFileName().c_str()) 
				== 0);
		success &= tmp_succ;
		print_pass(tmp_succ);
		
fail_test_6_2:
		delete dObj2;
fail_test_6_1:
		delete dObj1;
		
		
		// Test 7:
		print_over_test_str(1, "getData test 1: ");
		// Put in the entire data object all at once:
		put_obj_whole(dObj1, 1);
		if(!tmp_succ)
			goto fail_test_7_1;
		
		// Retrieve the entire data object:
		get_obj_whole(dObj1);
		if(!tmp_succ)
			goto fail_test_7_2;
		
fail_test_7_2:
fail_test_7_1:
		delete dObj1;
		print_pass(tmp_succ);
		
		
		// Test 8:
		print_over_test_str(1, "getData test 2: ");
		// Put in the entire data object all at once:
		put_obj_whole(dObj1, 1);
		if(!tmp_succ)
			goto fail_test_8_1;
		
		// Begin retreival:
		dObj1->getData_begin();
		len = 3;
		// Retrieve the entire data object (this should fail!):
		get_obj_whole_ns(dObj1);
		// This test is true iff the getData_begin() function failed:
		tmp_succ = (!tmp_succ && (len == 3));
		success &= tmp_succ;
		if(!tmp_succ)
			goto fail_test_8_2;
		
fail_test_8_2:
fail_test_8_1:
		delete dObj1;
		print_pass(tmp_succ);
		
		*/
		print_over_test_str(1, "Total: ");
		
		return (success?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
