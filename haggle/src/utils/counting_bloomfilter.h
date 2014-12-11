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
#ifndef _COUNTING_BLOOMFILTER_H
#define _COUNTING_BLOOMFILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windef.h>
#include <winsock2.h>
#ifndef HAS_DEFINED_U_INTN_T
#define HAS_DEFINED_U_INTN_T
typedef	BYTE u_int8_t;
typedef USHORT u_int16_t;
typedef DWORD32 u_int32_t;
#endif
#else
#include <sys/types.h>
#include <netinet/in.h>
#endif

typedef u_int32_t counting_salt_t;

struct counting_bloomfilter {
	u_int32_t k; /* Number of salts = # hash functions. 32-bits for this is probably overkill, 
		     but it keeps word alignment in the struct so that it is easier to serialize and send over the net */
	u_int32_t m; /* Number of bins in filter */
	u_int32_t n; /* Number of inserted objects */
	/* Here follows k salts */
	/* Then follows the actual filter */
};

/* Counting bloomfilter bin size in bits */
typedef u_int16_t counting_bin_t;
#define COUNTING_BIN_SIZE (sizeof(counting_bin_t))
#define COUNTING_VALUES_PER_BIN (1)

#define K_SIZE sizeof(u_int32_t)
#define M_SIZE sizeof(u_int32_t)
#define N_SIZE sizeof(u_int32_t)
#define COUNTING_SALT_SIZE sizeof(counting_salt_t)

#define CB_FILTER_LEN(bf) ((bf)->m / COUNTING_VALUES_PER_BIN * COUNTING_BIN_SIZE)
#define CB_SALTS_LEN(bf) ((bf)->k * COUNTING_SALT_SIZE)
#define COUNTING_BLOOMFILTER_TOT_LEN(bf) (sizeof(struct counting_bloomfilter) + CB_SALTS_LEN(bf) + CB_FILTER_LEN(bf))

#define COUNTING_BLOOMFILTER_GET_SALTS(bf) ((counting_salt_t *)((unsigned char *)(bf) + sizeof(struct counting_bloomfilter)))
#define COUNTING_BLOOMFILTER_GET_FILTER(bf) ((counting_bin_t *)((unsigned char *)(bf) + sizeof(struct counting_bloomfilter) + CB_SALTS_LEN(bf)))

enum counting_bf_op {
	counting_bf_op_check,
#define COUNTING_BF_OP_CHECK   counting_bf_op_check
	counting_bf_op_add,
#define COUNTING_BF_OP_ADD     counting_bf_op_add
	counting_bf_op_remove,
#define COUNTING_BF_OP_REMOVE  counting_bf_op_remove
	COUNTING_BF_OP_MAX,
};

#if defined(WIN32) || defined(WINCE)
#if !defined(inline)
#define inline __inline
#endif
#endif

struct counting_bloomfilter *counting_bloomfilter_new(double error_rate, unsigned int capacity);
int counting_bloomfilter_operation(struct counting_bloomfilter *bf, const char *key, const unsigned int len, unsigned int op);
void counting_bloomfilter_free(struct counting_bloomfilter *bf);
struct counting_bloomfilter *counting_bloomfilter_copy(const struct counting_bloomfilter *bf);
struct bloomfilter *counting_bloomfilter_to_noncounting(const struct counting_bloomfilter *bf);
char *counting_bloomfilter_to_base64(const struct counting_bloomfilter *bf);
char *counting_bloomfilter_to_noncounting_base64(const struct counting_bloomfilter *bf);
struct counting_bloomfilter *base64_to_counting_bloomfilter(const char *b64str, const size_t b64len);

static inline unsigned long counting_bloomfilter_get_n(const struct counting_bloomfilter *bf)
{
	return (unsigned long)bf->n;
}

static inline int counting_bloomfilter_check(struct counting_bloomfilter *bf, const char *key, const unsigned int len)
{
	return counting_bloomfilter_operation(bf, key, len, COUNTING_BF_OP_CHECK);
}
static inline int counting_bloomfilter_add(struct counting_bloomfilter *bf, const char *key, const unsigned int len)
{
	return counting_bloomfilter_operation(bf, key, len, COUNTING_BF_OP_ADD);
}
#ifdef DEBUG
void counting_bloomfilter_print(struct counting_bloomfilter *bf);
#endif

static inline int counting_bloomfilter_remove(struct counting_bloomfilter *bf, const char *key, const unsigned int len)
{
	return counting_bloomfilter_operation(bf, key, len, COUNTING_BF_OP_REMOVE);
}

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /*_COUNTING_BLOOMFILTER_H */
