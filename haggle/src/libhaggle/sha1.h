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

#ifndef _SHA1_H
#define _SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

// This is how long an SHA1 digest is:
#define SHA1_DIGEST_LENGTH	20

/*
	The internal representation of this struct is hidden.
*/
typedef struct _SHA1_CTX SHA1_CTX;

#include "sha1_private.h"

/**
	This function must be called before using an SHA1_CTX struct.
*/
void SHA1_Init(SHA1_CTX *context);
/**
	This function is called to process more data.
	
	Given a pointer "buf", containing 20 bytes, the following produces
	identical results:
		
		SHA1_Update(&ctx, buf, 20);
	
	and
		SHA1_Update(&ctx, &(buf[0]), 10);
		SHA1_Update(&ctx, &(buf[10]), 10);
*/
void SHA1_Update(SHA1_CTX *context, unsigned char *data, unsigned int len);

/**
	This function is called to generate the digest.
	
	After a call to this function, the context is INVALID and may not be used 
	again.
*/
void SHA1_Final(unsigned char digest[SHA1_DIGEST_LENGTH], SHA1_CTX* context);

#ifdef	__cplusplus
}
#endif

#endif /* _SHA1_H */
