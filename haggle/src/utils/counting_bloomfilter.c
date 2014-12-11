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
#include <string.h>
#include <math.h>       
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"
#include "base64.h"
#include "counting_bloomfilter.h"
#include <openssl/sha.h>
#include "bloomfilter.h"

struct counting_bloomfilter *counting_bloomfilter_new(double error_rate, unsigned int capacity)
{
	struct counting_bloomfilter *bf;
	unsigned int m, k, i;
	unsigned long bflen;
	counting_salt_t *salts;
	struct timeval tv;

	bloomfilter_calculate_length(capacity, error_rate, &m, &k);

	bflen = sizeof(struct counting_bloomfilter) + (k * COUNTING_SALT_SIZE + m / COUNTING_VALUES_PER_BIN * COUNTING_BIN_SIZE);

	bf = (struct counting_bloomfilter *)malloc(bflen);

	if (!bf)
		return NULL;

	memset(bf, 0, bflen);

	bf->m = m;
	bf->k = k;
	bf->n = 0;
	
	salts = COUNTING_BLOOMFILTER_GET_SALTS(bf);

	// Seed the rand() function's state. rand() should probably be replaced
	// by prng_uint8() or prnguint32(), but I don't know if there would be any
	// bad effects of doing that.
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	
	/* Create salts for hash functions */
	for (i = 0; i < k; i++) {
		salts[i] = (counting_salt_t)rand();
	}	
	return bf;
}

struct counting_bloomfilter *counting_bloomfilter_copy(const struct counting_bloomfilter *bf)
{
	struct counting_bloomfilter *bf_copy;

	if (!bf)
		return NULL;

	bf_copy = (struct counting_bloomfilter *)malloc(COUNTING_BLOOMFILTER_TOT_LEN(bf));

	if (!bf_copy)
		return NULL;

	memcpy(bf_copy, bf, COUNTING_BLOOMFILTER_TOT_LEN(bf));

	return bf_copy;
}

void counting_bloomfilter_print(struct counting_bloomfilter *bf)
{
	unsigned int i;
	
	if (!bf)
		return;

	printf("Bloomfilter m=%u k=%u n=%u\n", bf->m, bf->k, bf->n);

	for (i = 0; i < bf->m; i++) {
		counting_bin_t *bins = COUNTING_BLOOMFILTER_GET_FILTER(bf);
		printf("%u", bins[i]);
	}
	printf("\n");
}

/* This function converts the counting bloomfilter byte-by-byte into a
 * hexadecimal string. NOTE: It cannot be used as an encoding over the
 * network, since the integer fields are currently not converted to
 * network byte order. */
char *counting_bloomfilter_to_str(struct counting_bloomfilter *bf)
{
	char *str;
	char *ptr;
	unsigned int i;
	int len = 0;

	if (!bf)
		return NULL;

	//printf("bf len is %d K_SIZE=%ld M_SIZE=%ld N_SIZE=%ld SALTS_LEN=%ld FILTER_LEN=%u\n", len, K_SIZE, M_SIZE, N_SIZE, COUNTING_BLOOMFILTER_SALTS_LEN(bf), CB_FILTER_LEN(bf));

	str = (char *)malloc(COUNTING_BLOOMFILTER_TOT_LEN(bf) + 1);

	if (!str)
		return NULL;

	ptr = (char *)bf;
	
	/* First, format the 'k', 'm' and 'n' into the string */
	for (i = 0; i < COUNTING_BLOOMFILTER_TOT_LEN(bf); i++)
		len += sprintf(str+len, "%02x", ptr[i] & 0xff);

	return str;
}

struct bloomfilter *counting_bloomfilter_to_noncounting(const struct counting_bloomfilter *bf)
{
        struct bloomfilter *bf_nc;
        unsigned long bflen;
	counting_salt_t *salts;
	counting_bin_t *bins;
        unsigned long i, j;
	salt_t *salts_nc;
	bin_t *bins_nc;
        unsigned int bits_to_shift;

        if (!bf)
                return NULL;

        bflen = sizeof(struct bloomfilter) + (bf->k * SALT_SIZE + bf->m / VALUES_PER_BIN * BIN_SIZE);

	bf_nc = (struct bloomfilter *)malloc(bflen);

	if (!bf_nc)
		return NULL;

	memset(bf_nc, 0, bflen);

	bf_nc->m = bf->m;
	bf_nc->k = bf->k;
	bf_nc->n = bf->n;

	/* Get pointers */
	salts = COUNTING_BLOOMFILTER_GET_SALTS(bf);
	salts_nc = BLOOMFILTER_GET_SALTS(bf_nc);
	bins = COUNTING_BLOOMFILTER_GET_FILTER(bf);
	bins_nc = BLOOMFILTER_GET_FILTER(bf_nc);

	j = 0;

        // Copy the salts
        for (i = 0; i < bf->k; i++) {
                salts_nc[i] = salts[i]; 
        }

        /* Set the bins */
	for (i = 0; i < bf->m; i++) {
                bits_to_shift = (i % VALUES_PER_BIN);

                if (bins[i]) {
                        bins_nc[j] |= (1 << bits_to_shift);
                }

                if (bits_to_shift == (VALUES_PER_BIN - 1)) {
                        unsigned long k = VALUES_PER_BIN - 1;
                        while (1) {
                                if (k-- == 0)
                                        break;
                        }
                        j++;
                }
	}

        return bf_nc;
}

char *counting_bloomfilter_to_base64(const struct counting_bloomfilter *bf)
{
	char *b64str;
	int len = 0;
	unsigned int i = 0;
	struct counting_bloomfilter *bf_net;
	counting_salt_t *salts, *salts_net;
	counting_bin_t *bins, *bins_net;
	
	if (!bf)
		return NULL;

	bf_net = (struct counting_bloomfilter *)malloc(COUNTING_BLOOMFILTER_TOT_LEN(bf));

	if (!bf_net)
		return NULL;

	/* First set the values in host byte order so that we can get
	the pointers to the salts and the filter */
	bf_net->k = bf->k;
	bf_net->m = bf->m;
	bf_net->n = bf->n;
	
	/* Get pointers */
	salts = COUNTING_BLOOMFILTER_GET_SALTS(bf);
	salts_net = COUNTING_BLOOMFILTER_GET_SALTS(bf_net);
	bins = COUNTING_BLOOMFILTER_GET_FILTER(bf);
	bins_net = COUNTING_BLOOMFILTER_GET_FILTER(bf_net);

	/* Now convert into network byte order */
	bf_net->k = htonl(bf->k);
	bf_net->m = htonl(bf->m);
	bf_net->n = htonl(bf->n);

	for (i = 0; i < bf->k; i++)
		salts_net[i] = htonl(salts[i]);
	
	for (i = 0; i < bf->m; i++) {
		bins_net[i] = htons(bins[i]);
	}
	len = base64_encode_alloc((const char *)bf_net, COUNTING_BLOOMFILTER_TOT_LEN(bf), &b64str);

	counting_bloomfilter_free(bf_net);

	if (b64str == NULL && len == 0) {
		fprintf(stderr, "Bloomfilter ERROR: input too long to base64 encoder\n");
		return NULL;
	}
	if (b64str == NULL) {
		fprintf(stderr, "Bloomfilter ERROR: base64 encoder allocation error\n");
		return NULL;
	}
	if (len < 0)
		return NULL;

	return b64str;
}

char *counting_bloomfilter_to_noncounting_base64(const struct counting_bloomfilter *bf)
{
	char *b64str;
	struct bloomfilter *bf_other;
	
	if (!bf)
		return NULL;
	
	bf_other = counting_bloomfilter_to_noncounting(bf);
	
	if (!bf_other)
		return NULL;
	
	b64str = bloomfilter_to_base64(bf_other);

	bloomfilter_free(bf_other);
	
	return b64str;
}

struct counting_bloomfilter *base64_to_counting_bloomfilter(const char *b64str, const size_t b64len)
{
	struct base64_decode_context b64_ctx;
	struct counting_bloomfilter *bf_net;
	char *ptr;
	size_t len;
	counting_salt_t *salts;
	unsigned int i;

	base64_decode_ctx_init(&b64_ctx);

	if (!base64_decode_alloc (&b64_ctx, b64str, b64len, &ptr, &len)) {
		return NULL;
	}

	bf_net = (struct counting_bloomfilter *)ptr;

	bf_net->k = ntohl(bf_net->k);
	bf_net->m = ntohl(bf_net->m);
	bf_net->n = ntohl(bf_net->n);

	salts = COUNTING_BLOOMFILTER_GET_SALTS(bf_net);

	for (i = 0; i < bf_net->k; i++)
		salts[i] = ntohl(salts[i]);
	
	for (i = 0; i < bf_net->m; i++) {
		counting_bin_t *bins = COUNTING_BLOOMFILTER_GET_FILTER(bf_net);
		bins[i] = ntohs(bins[i]);
	}

	return bf_net;
}

int counting_bloomfilter_operation(struct counting_bloomfilter *bf, const char *key, 
			  const unsigned int len, unsigned int op)
{
	unsigned char *buf;
	unsigned int i;
	unsigned short removed = 0;
	int res = 1;
	counting_salt_t *salts;

	if (!bf || !key)
		return -1;

	if (op >= COUNTING_BF_OP_MAX) {
		return -1;
	}

	buf = malloc(len + CB_SALTS_LEN(bf));
	
	if (!buf)
		return -1;
	
	memcpy(buf, key, len);
	
	salts = COUNTING_BLOOMFILTER_GET_SALTS(bf);

	/* Increment number of objects in filter */
/* MOS - this is already happening below
	if (op == COUNTING_BF_OP_ADD)
		bf->n++;
*/

	for (i = 0; i < bf->k; i++) {
		SHA_CTX ctxt;
		unsigned int md[SHA_DIGEST_LENGTH];
		unsigned int hash = 0;
		int j, index;
		
		/* Salt the input */
		memcpy(buf + len, salts + i, COUNTING_SALT_SIZE);

		SHA1_Init(&ctxt);
		SHA1_Update(&ctxt, buf, len + COUNTING_SALT_SIZE);
		SHA1_Final((unsigned char *)md, &ctxt);
				
		for (j = 0; j < 5; j++) {
			hash = hash ^ md[j];			
		}
		
		/* Maybe there is a more efficient way to set the
		   correct bits? */
		index = hash % bf->m;

		//printf("index%d=%u\n", i, index);

		switch(op) {
		case COUNTING_BF_OP_CHECK:
			if (COUNTING_BLOOMFILTER_GET_FILTER(bf)[index] == 0) {
				res = 0;
				goto out;
			}
			break;
		case COUNTING_BF_OP_ADD:
			COUNTING_BLOOMFILTER_GET_FILTER(bf)[index]++;
			break;
		case COUNTING_BF_OP_REMOVE:
			if (COUNTING_BLOOMFILTER_GET_FILTER(bf)[index] > 0) {
				COUNTING_BLOOMFILTER_GET_FILTER(bf)[index]--;
				removed++;
			}
			
			/* 
			 else 
				fprintf(stderr, "Cannot remove item, because it is not in filter\n");
			 */
			break;
		default:
			fprintf(stderr, "Unknown Bloomfilter operation\n");
		}
		
	}
	/* Increment or decrement the number of objects in the filter depending on operation */
	if (op == COUNTING_BF_OP_ADD)
		bf->n++;
	else if (op == COUNTING_BF_OP_REMOVE && removed > 0)
		bf->n--;
	
out:
	free(buf);

	return res;
}

void counting_bloomfilter_free(struct counting_bloomfilter *bf)
{
	if (!bf)
		return;

	free(bf);	
}

#ifdef COUNTING_BLOOMFILTER_MAIN

int main(int argc, char **argv)
{
	struct counting_bloomfilter *bf, *bf2;
	char *key = "John McEnroe";
	char *key2= "John";
	int res;
	
	bf = counting_bloomfilter_new(0.01, 1000);
	bf2 = counting_bloomfilter_new(0.01, 1000);
	
	if (!bf || !bf2)
		return -1;

//	printf("bloomfilter m=%u, k=%u\n", bf->m, bf->k);

	counting_bloomfilter_add(bf, key, strlen(key));
	//printf("bf1:\n");
	//counting_bloomfilter_print(bf);
	counting_bloomfilter_add(bf, key, strlen(key));
	//printf("bf1:\n");
	//counting_bloomfilter_print(bf);
	counting_bloomfilter_remove(bf, key, strlen(key));
	
	printf("bf1:\n");
	counting_bloomfilter_print(bf);

	counting_bloomfilter_add(bf2, key2, strlen(key2));
	
	//printf("bf2:\n");
	//counting_bloomfilter_print(bf2);
	
//	res = counting_bloomfilter_check(bf, key2, strlen(key2));
	res = counting_bloomfilter_check(bf2, key2, strlen(key2));

	if (res)
		printf("\"%s\" is in filter\n", key2);
	else
		printf("\"%s\" is NOT in filter\n", key2);

	char *bfstr = counting_bloomfilter_to_base64(bf);

	counting_bloomfilter_free(bf2);

	bf2 = base64_to_bloomfilter(bfstr, strlen(bfstr));

	printf("Base64: %s\n", bfstr);

	//printf("bf2:\n");
	//counting_bloomfilter_print(bf2);

	free(bfstr);
	counting_bloomfilter_free(bf);
	counting_bloomfilter_free(bf2);

	return 0;
}

#endif
