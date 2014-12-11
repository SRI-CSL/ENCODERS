/* base64.h -- Encode binary data using printable characters.
   Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.
   Written by Simon Josefsson.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef BASE64_H
#define BASE64_H

#ifdef __cplusplus
extern "C" {
#endif
/* Get size_t. */
#include <stddef.h>

#ifdef _WIN32
/* Added for WIN32 compatibility */
#define restrict 

#ifndef __cplusplus
#define true 1
#define false 0
typedef int bool;
#endif

#else
/* Get bool. */
# include <stdbool.h>

#ifdef __cplusplus
// In case we do not have gnu extensions when including from C++
#define restrict
#endif

#endif
/* This uses that the expression (n+(k-1))/k means the smallest
   integer >= n/k, i.e., the ceiling of n/k.  */
#define BASE64_LENGTH(inlen) ((((inlen) + 2) / 3) * 4)

struct base64_decode_context
{
  unsigned int i;
  char buf[4];
};

extern bool isbase64 (char ch);

extern void base64_encode (const char *restrict in, size_t inlen,
			   char *restrict out, size_t outlen);

extern size_t base64_encode_alloc (const char *in, size_t inlen, char **out);

extern void base64_decode_ctx_init (struct base64_decode_context *ctx);
extern bool base64_decode (struct base64_decode_context *ctx,
			   const char *restrict in, size_t inlen,
			   char *restrict out, size_t *outlen);

extern bool base64_decode_alloc (struct base64_decode_context *ctx,
				 const char *in, size_t inlen,
				 char **out, size_t *outlen);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* BASE64_H */
