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

#ifndef _mini_base64_h_
#define _mini_base64_h_

/*
	These are c-to-c++ easy to use wrappers for the base64 library in src/utils
*/

#include "databuf.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	Encodes the data using base64 encoding scheme. The returned buffer is 
	allocated on the heap, and will need free()ing.
	
	Returns NULL/0 in case of error.
*/
data_buffer mini_base64_encode(char *data, size_t len);
/*
	Decodes the data using base64 encoding scheme. The returned buffer is 
	allocated on the heap, and will need free()ing.
	
	Returns NULL/0 in case of error.
*/
data_buffer mini_base64_decode(char *data, size_t len);

#ifdef	__cplusplus
}
#endif

#endif

