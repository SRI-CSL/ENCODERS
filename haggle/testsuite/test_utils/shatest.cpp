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
#if 0
#include <openssl/sha.h>
#include "base64.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(OS_MACOSX)
#include <stdlib.h>
#elif defined(OS_LINUX)
#include <time.h>
#include <stdlib.h>
#elif defined(OS_WINDOWS)
#include <windows.h>
#include <time.h>
#pragma warning( disable: 4996 )
#endif

#define NUMBER_OF_DIGESTS	8
static char		correct_digest[NUMBER_OF_DIGESTS][SHA1_DIGEST_LENGTH] = 
	{
		// FIPS PUB 180-1 test 1 result:
		{
			0xA9, 0x99, 0x3E, 0x36,
			0x47, 0x06, 0x81, 0x6A,
			0xBA, 0x3E, 0x25, 0x71,
			0x78, 0x50, 0xC2, 0x6C,
			0x9C, 0xD0, 0xD8, 0x9D
		},
		// FIPS PUB 180-1 test 2 result:
		{
			0x84, 0x98, 0x3E, 0x44,
			0x1C, 0x3B, 0xD2, 0x6E,
			0xBA, 0xAE, 0x4A, 0xA1,
			0xF9, 0x51, 0x29, 0xE5,
			0xE5, 0x46, 0x70, 0xF1
		},
		// FIPS PUB 180-1 test 3 result:
		{
			0x34, 0xAA, 0x97, 0x3C,
			0xD4, 0xC4, 0xDA, 0xA4,
			0xF6, 0x1E, 0xEB, 0x2B,
			0xDB, 0xAD, 0x27, 0x31,
			0x65, 0x34, 0x01, 0x6F
		},
		{
			0xDA, 0x39, 0xA3, 0xEE, 
			0x5E, 0x6B, 0x4B, 0x0D, 
			0x32, 0x55, 0xBF, 0xEF, 
			0x95, 0x60, 0x18, 0x90, 
			0xAF, 0xD8, 0x07, 0x09
		},
		{
			0x31, 0x67, 0xBC, 0xD3,
			0xED, 0x22, 0xA9, 0x90,
			0xD9, 0xE0, 0x01, 0x88,
			0x2C, 0x10, 0x00, 0x84,
			0x7A, 0x13, 0x64, 0x26
		},
		{
			0x63, 0xF0, 0x43, 0xE6,
			0xE8, 0x3D, 0x4A, 0xF0,
			0x95, 0x4D, 0xDB, 0xAC,
			0xF3, 0xA1, 0xC1, 0x14,
			0x6F, 0x7D, 0x1F, 0xCC
		},
		{
			0x19, 0x88, 0x6A, 0x5E, 
			0x00, 0x72, 0x8A, 0xB3,
			0x2D, 0x01, 0x6D, 0xCF,
			0x0C, 0x2C, 0xC0, 0x4C,
			0x82, 0x5C, 0x86, 0x07
		},
		{
			0xA6, 0x48, 0xC2, 0x09, 
			0x6D, 0x9E, 0xEF, 0x67, 
			0x4F, 0x42, 0xB9, 0x0A, 
			0x25, 0x5B, 0xA5, 0x22, 
			0x0E, 0x0E, 0xBC, 0x52
		}
	};

static char		*digest_data[NUMBER_OF_DIGESTS] = 
	{
		// FIPS PUB 180-1 test 1:
		"abc",
		// FIPS PUB 180-1 test 2:
		"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
		// FIPS PUB 180-1 test 3:
		"a",
		// Private tests:
		NULL,
		"2jmj7l5rSw0yVb/vlWAYkK/YBwk=",
		"MWe80+0iqZDZ4AGILBAAhHoTZCY=",
		"MWe80+0iqZDZ4AGILBAAhHoTZCYY/BD5ug9SvCVTdus86HBFG99H8w",
		"GYhqXgByirMY/BD5ug9SvCVTdus86HBF"
			"G99H8wtAW3PDCzATIJchgc=MWe80+0iq"
			"BD5ug9SvCVTdus86HBFG99H8wZDZ4AGI"
			"LBAAhHoTZCY"
	};

static long		digest_len[NUMBER_OF_DIGESTS] = 
	{
		3,
		56,
		-1, // Means "insert one million copies"
		0,
		28 + 1,
		28 + 1,
		27 * 2 + 1,
		3*32 + 11
	};

static void prng_init(void)
{
#if defined(OS_MACOSX)
	// No need for initialization
#elif defined(OS_LINUX)
	srandom(time(NULL));
#elif defined(OS_WINDOWS)
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);

	srand(sysTime.wSecond + sysTime.wMilliseconds);
#endif
}

static unsigned char prng_uint8(void)
{
#if defined(OS_MACOSX)
	return arc4random() & 0xFF;
#elif defined(OS_LINUX)
	return random() & 0xFF;
#elif defined(OS_WINDOWS)
	return rand() & 0xFF;
#endif
}
#endif
#if defined(OS_WINDOWS)
int haggle_test_sha(void)
#else
int main(int argc, char *argv[])
#endif
{
#if 0
	SHA1_CTX		ctx;
	unsigned char	digest[SHA1_DIGEST_LENGTH];
	char			str[32];
	int				success = (1==1), tmp_succ;
	long			test[NUMBER_OF_DIGESTS];
	long			i,j,k,l;
	
	prng_init();
	
	// Create a random sub-test ordering:
	for(i = 0; i < NUMBER_OF_DIGESTS; i++)
		test[i] = i;
	
	for(j = 0; j < 3; j++)
		for(i = 0; i < NUMBER_OF_DIGESTS; i++)
		{
			k = prng_uint8() % NUMBER_OF_DIGESTS;
			
			l = test[i];
			test[i] = test[k];
			test[k] = l;
		}
	
	print_over_test_str_nl(0, "SHA1 test: ");
	
	for(i = 0; i < NUMBER_OF_DIGESTS; i++)
	{
		sprintf(str, "Digest %ld: ", test[i]);
		// Initialize context:
		SHA1_Init(&ctx);
		
		print_over_test_str(1, str);
		// Insert data (if any data is to be inserted)
		if(digest_len[test[i]] > 0)
			SHA1_Update(
				&ctx, 
				(unsigned char *)digest_data[test[i]], 
				digest_len[test[i]]);
		else if(digest_len[test[i]] == -1)
		{
			for(j = 0; j < 1000000; j++)
			{
				SHA1_Update(
					&ctx, 
					(unsigned char *) digest_data[test[i]], 
					1);
			}
		}
		// Finalize context:
		SHA1_Final(digest, &ctx);
		
		// Check result
		tmp_succ = 
			(memcmp(digest, correct_digest[test[i]], SHA1_DIGEST_LENGTH) == 0);
		success &= tmp_succ;
		if(!tmp_succ)
		{
			printf("{");
			for(j = 0; j < SHA1_DIGEST_LENGTH; j++)
				printf("%02X", (unsigned char) digest[j]);
			printf("} vs. {");
			for(j = 0; j < SHA1_DIGEST_LENGTH; j++)
				printf("%02X", (unsigned char) correct_digest[test[i]][j]);
			printf("} ");
		}
		print_pass(tmp_succ);
	}
	
	print_over_test_str(1, "Total: ");
	// Success:
	return (success?0:1);
#else
	return 0;
#endif
}
