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
 *
 * Parts of this code is under different licenses, please read those licenses
 * below.
 */ 

/*
	This file contains some private implementational details of the SHA1 
	implementation. Those files using SHA1 should not look here for any 
	information.
*/
#ifndef _SHA1_PRIVATE_H
#define _SHA1_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

#define SHA1_USE_ORIGINAL_IMPLEMENTATION	0
#define SHA1_USE_ALTERNATIVE_IMPLEMENTATION	1

#if SHA1_USE_ORIGINAL_IMPLEMENTATION && SHA1_USE_ALTERNATIVE_IMPLEMENTATION
#error "Can't use both the original and alternative implementation at once!"
#elif !SHA1_USE_ORIGINAL_IMPLEMENTATION && !SHA1_USE_ALTERNATIVE_IMPLEMENTATION
#error "Have to have one implementation active!"
#endif

#if SHA1_USE_ORIGINAL_IMPLEMENTATION
/*
 * sha1.h
 *
 * Originally taken from the public domain SHA1 implementation
 * written by by Steve Reid <steve@edmweb.com>
 * 
 * Modified by Aaron D. Gifford <agifford@infowest.com>
 *
 * NO COPYRIGHT - THIS IS 100% IN THE PUBLIC DOMAIN
 *
 * The original unmodified version is available at:
 *    ftp://ftp.funet.fi/pub/crypt/hash/sha/sha1.c
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(WIN32) || defined(WINCE)
/* Make sure you define these types for your architecture: */
typedef unsigned int sha1_quadbyte;	/* 4 byte type */
typedef unsigned char sha1_byte;	/* single byte type */
#else
#include <sys/types.h>
/* Make sure you define these types for your architecture: */
typedef u_int32_t sha1_quadbyte;	/* 4 byte type */
typedef unsigned char sha1_byte;	/* single byte type */
#endif

#if defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN)
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#else /* arch is __BIG_ENDIAN__ */
#undef LITTLE_ENDIAN
#endif
/*
 * Be sure to get the above definitions right.  For instance, on my
 * x86 based FreeBSD box, I define LITTLE_ENDIAN and use the type
 * "unsigned long" for the quadbyte.  On FreeBSD on the Alpha, however,
 * while I still use LITTLE_ENDIAN, I must define the quadbyte type
 * as "unsigned int" instead.
 */

#define SHA1_BLOCK_LENGTH	64

/* The SHA1 structure: */
struct _SHA1_CTX {
	sha1_quadbyte	state[5];
	sha1_quadbyte	count[2];
	sha1_byte	buffer[SHA1_BLOCK_LENGTH];
};

/* Changed the name of these functions slightly to avoid name clashes. */
void _SHA1_Init(SHA1_CTX* context);
void _SHA1_Update(SHA1_CTX *context, sha1_byte *data, unsigned int len);
void _SHA1_Final(sha1_byte digest[SHA1_DIGEST_LENGTH], struct _SHA1_CTX *context);

#elif SHA1_USE_ALTERNATIVE_IMPLEMENTATION
/*
 *  sha1.h
 *
 *  Description:
 *      This is the header file for code which implements the Secure
 *      Hashing Algorithm 1 as defined in FIPS PUB 180-1 published
 *      April 17, 1995.
 *
 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 */

#if defined(WIN32) || defined(WINCE)
#include <windows.h>
typedef DWORD uint32_t;
typedef unsigned char uint8_t;
typedef WORD int_least16_t;
#else
#include <stdint.h>
#endif
/*
 * If you do not have the ISO standard stdint.h header file, then you
 * must typdef the following:
 *    name              meaning
 *  uint32_t         unsigned 32 bit integer
 *  uint8_t          unsigned 8 bit integer (i.e., unsigned char)
 *  int_least16_t    integer of >= 16 bits
 *
 */

#ifndef _SHA_enum_
#define _SHA_enum_
enum
{
    shaSuccess = 0,
    shaNull,            /* Null pointer parameter */
    shaInputTooLong,    /* input data too long */
    shaStateError       /* called Input after Result */
};
#endif
#define SHA1HashSize 20

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct _SHA1_CTX
{
    uint32_t Intermediate_Hash[SHA1HashSize/4]; /* Message Digest  */

    uint32_t Length_Low;            /* Message length in bits      */
    uint32_t Length_High;           /* Message length in bits      */

                               /* Index into message block array   */
    int_least16_t Message_Block_Index;
    uint8_t Message_Block[64];      /* 512-bit message blocks      */

    int Computed;               /* Is the digest computed?         */
    int Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

/*
 *  Function Prototypes
 */

int SHA1Reset(  SHA1Context *);
int SHA1Input(  SHA1Context *,
                const uint8_t *,
                unsigned int);
int SHA1Result( SHA1Context *,
                uint8_t Message_Digest[SHA1HashSize]);
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _SHA1_PRIVATE_H */
