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

#ifndef __PRNG_H__
#define __PRNG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
	Pseudo-random number generation library
*/

/*
	Initializes the PRNG library. Needs to be done at least once before the
	other functions are executed. There's no harm in calling this function more
	than once.
*/
void prng_init(void);

/*
	Returns an 8-bit random number.
*/
unsigned char prng_uint8(void);

/*
	Returns a 32-bit random number.
*/
unsigned long prng_uint32(void);

#ifdef __cplusplus
}
#endif


#endif
