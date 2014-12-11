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

/*
 LuckyMe challenges your luck. 
 How many random dataobjects floating around in the Haggle network
 do you attract? You are yourself contributing to the luck of other
 players in the network! 
 
 Besides of being a research application to evalute the search based
 networking paradigm of Haggle, this code can be taken as a skeletton 
 for many fun application or game scenarios.
 Think creative, why not use puzzle parts as random objects? Make a 
 competition with you buddies to collect as many objects as possible. 
 */
#include <libhaggle/haggle.h>
#include "luckyme.h"

#if defined(OS_UNIX)

#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define ERRNO errno

#elif defined(OS_WINDOWS)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <winioctl.h>
#include <winerror.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#define ERRNO WSAGetLastError()
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

unsigned long grid_size = 0;			// overwrite with -g
char *filename = "\\luckyme.jpg";		// overwrite with -f
char *data_trace_filename = NULL;		// overwrite with -l
FILE *data_trace_fp = NULL;
char *single_source_name = NULL;		// overwrite with -s
unsigned long create_data_interval = 120000;	// milliseconds, overwrite with -t (given in seconds)
unsigned long repeatableSeed = 0;		// overwrite with -r
unsigned long use_node_number = 0;		// overwrite with -n
unsigned long num_dataobjects = 0;		// overwrite with -N

#if defined(OS_WINDOWS_MOBILE)
unsigned long attribute_pool_size = 100;
unsigned long num_dataobject_attributes = 4; 
unsigned long num_interest_attributes = 10;
#else
unsigned long attribute_pool_size = 100;	// overwrite with -A
unsigned long num_dataobject_attributes = 4;	// overwrite with -d
unsigned long num_interest_attributes = 10;	// overwrite with -i
#endif

unsigned long node_number = 0;

#define APP_NAME "LuckyMe"

haggle_handle_t hh;

char hostname[128];
unsigned long num_dobj_received = 0;
unsigned long num_dobj_created = 0;
unsigned long num_neighbors = 0;
char **neighbors = NULL;

int called_haggle_shutdown = 0;
static int stop_now = 0;

#if defined(OS_WINDOWS_MOBILE)
DWORD WINAPI luckyme_run(void *);
static callback_function_t callback = NULL;

#define NOTIFY() { if (callback) { callback(EVENT_TYPE_STATUS_UPDATE); } }
#define NOTIFY_ERROR() { if (callback) { callback(EVENT_TYPE_ERROR); } }

static HANDLE luckyme_thread_handle = NULL;
static HANDLE haggle_start_thread_handle = NULL;
static HANDLE test_loop_event = NULL;
static HANDLE luckyme_start_event = NULL;
static int test_is_running = 0;

#else
#define NOTIFY()
#define NOTIFY_ERROR()
static int test_loop_event[2];
#endif
#ifdef OS_WINDOWS
typedef CRITICAL_SECTION mutex_t;
#elif defined(OS_UNIX)
#include <pthread.h>
#include <libgen.h>
typedef pthread_mutex_t mutex_t;
#endif

mutex_t mutex;

static void mutex_init(mutex_t *m)
{
#ifdef OS_WINDOWS
	InitializeCriticalSection(m);
#elif defined(OS_UNIX)
	pthread_mutex_init(m, NULL);
#endif
}
static void mutex_del(mutex_t *m)
{
#ifdef OS_WINDOWS
	DeleteCriticalSection(m);
#elif defined(OS_UNIX)
	pthread_mutex_destroy(m);
#endif
}
static inline int mutex_lock(mutex_t *m)
{
#ifdef OS_WINDOWS
	EnterCriticalSection(m);
	return 0;
#elif defined(OS_UNIX)
	return pthread_mutex_lock(m);
#endif
}

static inline void mutex_unlock(mutex_t *m)
{
#ifdef OS_WINDOWS
	LeaveCriticalSection(m);
#elif defined(OS_UNIX)
	pthread_mutex_unlock(m);
#endif
}

static double fac(unsigned long n)
{
	unsigned long i, t = 1;
	
	for (i = n; i > 1; i--)
		t *= i;
	
	return (double)t;
}

static const char *ulong_to_str(unsigned long n)
{
	static char buf[32];
	
	snprintf(buf, 32, "%lu", n);
	
	return buf;
}

static void luckyme_prng_init()
{
#ifdef OS_UNIX
	if (repeatableSeed) {
		return srandom(node_number+1);
	}
#endif
	prng_init();
}

static unsigned int luckyme_prng_uint32()
{
#ifdef OS_UNIX
	if (repeatableSeed) {
		return random();
	}
#endif
	return prng_uint32();
}


void luckyme_dataobject_set_createtime(struct dataobject *dobj)
{
#ifdef OS_UNIX
	if (repeatableSeed) {
		struct timeval ct;
		ct.tv_sec = num_dobj_created;
		ct.tv_usec = node_number;
		haggle_dataobject_set_createtime(dobj, &ct);
	} else {
		haggle_dataobject_set_createtime(dobj, NULL);
	}
#else
	haggle_dataobject_set_createtime(dobj, NULL);
#endif
}

#ifdef OS_WINDOWS_MOBILE

/* DLL entry point */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			test_loop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			luckyme_start_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			mutex_init(&mutex);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			CloseHandle(test_loop_event);
			CloseHandle(luckyme_start_event);
			mutex_del(&mutex);
			break;
	}
	return TRUE;
}

int luckyme_test_start()
{
	DWORD ret;
	test_is_running = 1;
	num_dobj_received = 0;
	
	LIBHAGGLE_DBG("Checking test_loop_event\n");

	ret = WaitForSingleObject(test_loop_event, 0);
	
	switch (ret) {
		case WAIT_TIMEOUT:
			LIBHAGGLE_DBG("Setting test_loop_event\n");
			SetEvent(test_loop_event);
			break;
		case WAIT_OBJECT_0:
			return 0;
		case WAIT_FAILED:
			return -1;
	}
	return 1;
}

int luckyme_test_stop()
{
	DWORD ret;
	test_is_running = 0;
	
	ret = WaitForSingleObject(test_loop_event, 0);
	
	switch (ret) {
		case WAIT_TIMEOUT:
			SetEvent(test_loop_event);
			break;
		case WAIT_OBJECT_0:
			return 0;
		case WAIT_FAILED:
			return -1;
	}
	return 1;
}

int luckyme_is_running(void)
{
	if (luckyme_thread_handle != NULL && hh && haggle_event_loop_is_running(hh))
		return 1;
	
	return 0;
}

int luckyme_is_test_running()
{
	return test_is_running ? 1 : 0;
}

unsigned long luckyme_get_num_dataobjects_received(void)
{
	return num_dobj_received;
}

unsigned long luckyme_get_num_dataobjects_created(void)
{
	return num_dobj_created;
}

unsigned long luckyme_get_num_neighbors(void)
{
	return num_neighbors;
}

char *luckyme_get_neighbor_unlocked(unsigned int i)
{
	if (i < num_neighbors) {
		return neighbors[i];
	}
	return NULL;
}

char *luckyme_get_neighbor_locked(unsigned int i)
{
	char *neigh, *neighbor = NULL;

	mutex_lock(&mutex);

	neigh = luckyme_get_neighbor_unlocked(i);

	if (neigh) {
		neighbor = (char *)malloc(strlen(neigh) + 1);
		strcpy(neighbor, neigh);
	}
	
	mutex_unlock(&mutex);

	return NULL;
}

void luckyme_free_neighbor(char *neigh)
{
	if (neigh)
		free(neigh);
}

int luckyme_neighborlist_lock()
{
	return mutex_lock(&mutex);
}

void luckyme_neighborlist_unlock()
{
	mutex_unlock(&mutex);
}

int luckyme_haggle_start(daemon_spawn_callback_t callback)
{
	if (haggle_daemon_pid(NULL) == HAGGLE_DAEMON_RUNNING) {
		return 0;
	}

	LIBHAGGLE_DBG("callback is %s\n", callback ? "set" : "NULL");
	
	return haggle_daemon_spawn_with_callback(NULL, callback) == HAGGLE_NO_ERROR;
}


int luckyme_haggle_stop(void)
{
	if (haggle_daemon_pid(NULL) == HAGGLE_DAEMON_RUNNING) {
		if (!hh)
			return -1;

		haggle_ipc_shutdown(hh);

		return 1;
	}
	return 0;
}

int luckyme_start(void)
{
	DWORD ret;

	if (luckyme_is_running())
		return 0;
		
	luckyme_thread_handle = CreateThread(NULL, 0, luckyme_run, (void *)NULL, 0, 0);
	
	if (luckyme_thread_handle == NULL) {
		LIBHAGGLE_ERR("Could not start luckyme thread\n");
		return -1;
	}

	ret = WaitForSingleObject(luckyme_start_event, 90000);
	
	switch (ret) {
		case WAIT_TIMEOUT:
			LIBHAGGLE_ERR("Timeout while waiting for luckyme thread to start\n");
			return -1;
			break;
		case WAIT_FAILED:
			LIBHAGGLE_ERR("Wait error while waiting for luckyme thread to start\n");
			return -1;
			break;
		case WAIT_OBJECT_0:
			break;
	}
	return 1;
}

int luckyme_stop(int stop_haggle)
{
	int retval = 0;

	if (luckyme_is_running()) {
		DWORD ret;

		stop_now = 1;
		
		if (luckyme_test_stop() < 0)
			return -1;
		
		if (haggle_daemon_pid(NULL) == HAGGLE_DAEMON_RUNNING && stop_haggle) {
			called_haggle_shutdown = 1;
			if (luckyme_haggle_stop())
				retval++;
		}

		/* 
		  Check again if test is running since Haggle shutdown event
		  could have shut us down.
		  */
		if (luckyme_is_running()) {
			// Wait for 10 seconds
			ret = WaitForSingleObject(luckyme_thread_handle, 10000);

			if (ret == WAIT_OBJECT_0) {
				/* Is this really necessary here or will the handle be closed by the event loop? */
				CloseHandle(luckyme_thread_handle);
				retval++;
			} else if (ret == WAIT_FAILED) {
			} else if (ret == WAIT_TIMEOUT) {
				LIBHAGGLE_DBG("Wait timeout when stopping test...\n");
				return -1;
			} else {
				// Should not happen
			}
		} else {
			retval++;
		}
	}
	return retval;
}

int set_callback(callback_function_t _callback)
{
	if (callback != NULL)
		return 0;
	
	callback = _callback;
	
	return 1;
}

#endif /* OS_WINDOWS_MOBILE */

// --- luckyMe functions (create interest and data) 

/* 
 we are interested in a number of random attributes.
 */
int create_interest_binomial() 
{
	unsigned long n, u, luck = 0, i = 0;
	struct attributelist *al;
	double p;

	// define luck (mean of binomial distribution)
	if (use_node_number == 1) {
		luck = node_number;
	} else {
		luck = luckyme_prng_uint32() % attribute_pool_size;
	}

	// use binomial distribution to approximate normal distribution (sum of weights = 100)
	// mean = u = np, variance = np(1-p)

	// chosing p and n:
	// to get non-skewed distribution, we use p = 0.5  (skewness = (1-2p)/sqrt(np(1-p)))
	// n is given by num_interest_attributes. variance: n = variance/(p(1-p)) (for p=0.5: variance=n/4)
	// u is the mean value (u=np, for p=0.5:u=n/2)
	//   this gives us n interests (note the limitation below)
	p = 0.5;
	n = num_interest_attributes - 1;
	u = (unsigned long)(n * p);
	
	LIBHAGGLE_DBG("create interest (luck=%ld)\n", luck);
	LIBHAGGLE_DBG("binomial distribution  n=%lu, p=%lf\n", n, p);
	
	al = haggle_attributelist_new();
	
	if (!al)
		return -1;
	
	// calculate P(X=i) = (n over i) p^i (1-p)^(n-i) as weight
	// in our case P(X=i) = (n over i) 0.5^n
	for (i = 0; (unsigned long)i <= n; i++) {
		struct attribute *attr;
		unsigned long interest, weight;
		// create n interests, take the highest weight for luck (i=u)
		interest = (luck + i - u + attribute_pool_size) % attribute_pool_size;
		weight = (unsigned long)(100 * fac(n)/(fac(n-i)*fac(i))*pow(p,i)*pow(1-p,n-i));

		// do not add interest with weight < 1 (corresponds to P(X=i) < 0.01)
		if (weight < 1) 
			weight = 1;

		attr = haggle_attribute_new_weighted(APP_NAME, ulong_to_str(interest), weight);
		haggle_attributelist_add_attribute(al, attr);
		LIBHAGGLE_DBG("   %s=%s:%lu\n", haggle_attribute_get_name(attr), haggle_attribute_get_value(attr), haggle_attribute_get_weight(attr));
	}
	
	haggle_ipc_add_application_interests(hh, al);
	
	haggle_attributelist_free(al);
	
	return (i + 1);
}


/* 
 we are interested in two attributes,
 corresponding to row and column(+gridSize) in the grid. 
 */
int create_interest_grid() 
{
	int i = 0;
	struct attributelist *al;
#define NUM_INTERESTS (2)
	unsigned int interests[NUM_INTERESTS];
	
	interests[0] = node_number / grid_size;
	interests[1] = (node_number % grid_size) + grid_size;
	
	LIBHAGGLE_DBG("create interest\n");
	
	al = haggle_attributelist_new();
	
	if (!al)
		return -1;
	
	for (i = 0; i < NUM_INTERESTS; i++) {
		struct attribute *attr = haggle_attribute_new_weighted(APP_NAME, ulong_to_str(interests[i]), 1);
		haggle_attributelist_add_attribute(al, attr);
		LIBHAGGLE_DBG("   %s=%s:%lu\n", haggle_attribute_get_name(attr), haggle_attribute_get_value(attr), haggle_attribute_get_weight(attr));

	}
	
	haggle_ipc_add_application_interests(hh, al);
	
	haggle_attributelist_free(al);

	return i;
}


int create_interest_node_number() 
{
	struct attribute *attr;
	struct attributelist *al = haggle_attributelist_new();
	
	if (!al)
		return -1;
	
	LIBHAGGLE_DBG("create interest based on node number\n");
	
	attr = haggle_attribute_new_weighted(APP_NAME, ulong_to_str(node_number), 1);
	
	haggle_attributelist_add_attribute(al, attr);
	
	LIBHAGGLE_DBG("   %s=%s:%lu\n", haggle_attribute_get_name(attr), haggle_attribute_get_value(attr), haggle_attribute_get_weight(attr));
	
	haggle_ipc_add_application_interests(hh, al);

	haggle_attributelist_free(al);
	
	return node_number;
}



/*
 we create a dataobject with a number of random attributes.
 at the moment the attributes are uniformly distributed.
 the intention is to build in some correlation to the own
 interests and/or the dataobjects that one received.
 */
int create_dataobject_random() 
{
	unsigned int i = 0;
	unsigned long *values;
	struct dataobject *dobj = NULL;
	
	if (num_dataobject_attributes == 0)
		return -1;

	if (filename) {
		dobj = haggle_dataobject_new_from_file(filename);
		
		if (dobj)
			haggle_dataobject_add_hash(dobj);
		else
			dobj = haggle_dataobject_new();
	} else {
		dobj = haggle_dataobject_new();
	}
	
	if (!dobj)
		return -1;
	
	LIBHAGGLE_DBG("create data object %lu\n", num_dobj_created);
	
	values = (unsigned long *)malloc(sizeof(unsigned long) * num_dataobject_attributes);

	if (!values) {
		haggle_dataobject_free(dobj);
		return -1;
	}

	i = 0;

	while (i < num_dataobject_attributes) {
		unsigned int j, have_value = 0;
		unsigned long value = luckyme_prng_uint32() % attribute_pool_size;
		
		// Check if we alread generated this attribute
		for (j = 0; j < i; j++) {
			if (value == values[j]) {
				have_value = 1;
			}
		}

		// see if the value was found, and in that case try again
		if (have_value)
			continue;

		haggle_dataobject_add_attribute(dobj, APP_NAME, ulong_to_str(value));
		printf("   %s=%s\n", APP_NAME, ulong_to_str(value));
		values[i] = value;
		i++;
	}

	free(values);

	luckyme_dataobject_set_createtime(dobj);
	haggle_ipc_publish_dataobject(hh, dobj);
	haggle_dataobject_free(dobj);
	
	num_dobj_created++;
	
	return 1;
}


/*
 we create a dataobject with a number of attributes.
 all combinations from 1 attribute to 2*gridSize attributes generated.
 */
int create_dataobject_grid() 
{
	struct dataobject *dobj = NULL;
	unsigned int max_dataobject_number = (1 << (2*grid_size));	
	int i = 0;
	
	if (num_dobj_created > max_dataobject_number-1) {
		//shutdown(0);
	}
	
	if (filename) {
		dobj = haggle_dataobject_new_from_file(filename);

		if (dobj)
			haggle_dataobject_add_hash(dobj);
		else
			dobj = haggle_dataobject_new();
	} else {
		dobj = haggle_dataobject_new();
	}
	
	if (!dobj)
		return -1;
	
	LIBHAGGLE_DBG("create data object %lu\n", num_dobj_created);
	
	for (i = 0; i < 6; i++) {
		if ((num_dobj_created>>i) & 0x01) {
			haggle_dataobject_add_attribute(dobj, APP_NAME, ulong_to_str((unsigned long)i));
			LIBHAGGLE_DBG("   %s=%s\n", APP_NAME, ulong_to_str((unsigned long)i));
		}
	}
	
	luckyme_dataobject_set_createtime(dobj);
	haggle_ipc_publish_dataobject(hh, dobj);
	haggle_dataobject_free(dobj);
	
	num_dobj_created++;
	
	return 1;
}

int read_interest_from_trace()
{
	int ret = 0;
	char buf[4096];
	char *line = NULL;	
	long off_start, off_end;
	int bytes = 0;
	unsigned long total_bytes = 0;
	struct attributelist *al;
	
	if (!data_trace_fp)
		return -1;
	
	LIBHAGGLE_DBG("Reading interest\n");
	
	while (1) {
		unsigned long pos;
		off_start = ftell(data_trace_fp);
		
		// Read entire line into buffer
		line = fgets(buf, 4096, data_trace_fp);
		
		if (!line) {
			if (feof(data_trace_fp) != 0) {
				LIBHAGGLE_DBG("EOF when reading data trace\n");
				return 0;
			} else {
				LIBHAGGLE_ERR("An error occured when reading data trace\n");
				return -1;
			}
		}
		
		off_end = ftell(data_trace_fp);

		if (line[0] != '#') {
			// if what we read is not what we were looking for,
			// reset the stream to the position before the read.
			LIBHAGGLE_ERR("Line was not an interest\n");
			fseek(data_trace_fp, off_start - off_end, SEEK_CUR);
			return 0;
		}
		
		// read until first whitespace -- there we should find the end of the node name
		pos = 2;
		
		while (line[pos] != ' ' && pos < strlen(line)) {
			pos++;
		}

		// skip lines that do not match our hostname
		if (strlen(hostname) != pos-2)								// different length
			continue;
		if (strncmp(&line[2], hostname, strlen(hostname)) != 0)		// different name
			continue;
		
		break;
	}
	// Skip the # character and hostname and one whitespace
	total_bytes = 1 + strlen(hostname) + 1;
	
	al = haggle_attributelist_new();

	if (!al)
		return -1;
	
	
	do {
		char name[50], value[50];
		unsigned long weight;
		struct attribute *attr;
		
		ret = sscanf(line + total_bytes, " %[^=]=%[^:]:%lu%n", name, value, &weight, &bytes);
		
		if (ret == 3) {
			LIBHAGGLE_DBG("Read interest %s=%s:%lu\n", name, value, weight);
			total_bytes += bytes;
			
			attr = haggle_attribute_new_weighted(name, value, weight);
		
			haggle_attributelist_add_attribute(al, attr);
		}
		
	} while (ret == 3);
	
	haggle_ipc_add_application_interests(hh, al);
	
	haggle_attributelist_free(al);
	
	return 1;
}

int read_dataobject_from_trace(struct dataobject **dobj, struct timeval *timeout)
{
	char buf[4096];
	char *line = NULL;
	int ret;	
	char node[50];
	int bytes = 0;
	unsigned long total_bytes = 0;
	struct timeval traceTime;
	static struct timeval lastTime = { 0, 0 };
	
	
	if (!dobj || !timeout || !data_trace_fp)
		return -1;
	
	while (1) {
		long off_start;
		unsigned long i = 0, bytes_read = 0, pos;
		
		off_start = ftell(data_trace_fp);
		
		// Read entire line into buffer
		line = fgets(buf, 4096, data_trace_fp);
		
		bytes_read = ftell(data_trace_fp) - off_start;
		
		if (!line) {
			if (feof(data_trace_fp) != 0) {
				LIBHAGGLE_DBG("EOF when reading data trace\n");
				return 0;
			} else {
				LIBHAGGLE_ERR("An error occured when reading data trace\n");
				return -1;
			}
		}
		
		if (line[0] == '#') {
			// skip these lines if they haven't already been read by read_interest_from_trace()
			continue;
		} 
				
		// read until first whitespace -- there we should find the node name
		while (line[i] != ' ' && i < bytes_read - 1) {
			i++;
		}
		
		// skip whitespace
		i++;

		pos = i;
		
		while (line[pos] != ' ' && pos < strlen(line)) {
			pos++;
		}
		
		
		// Skip lines that do not match our hostname
		if (strlen(hostname) != pos-i)
			continue;
		if (strncmp(hostname, &line[i], strlen(hostname)) != 0) 
			continue;
		
		break;
	}	
	
	LIBHAGGLE_DBG("reading data object\n");
	
	if (filename) {
		*dobj = haggle_dataobject_new_from_file(filename);
		
		if (*dobj)
			haggle_dataobject_add_hash(*dobj);
		else
			*dobj = haggle_dataobject_new();
	} else {
		*dobj = haggle_dataobject_new();
	}
	
	if (!*dobj)
		return -1;
	
	// conversion format: create_time|node-nr|attribute list|
	ret = sscanf(line, "%ld.%ld %[^ ]%n", 
		     (long *)&traceTime.tv_sec, 
		     (long *)&traceTime.tv_usec, 
		     node, &bytes);
	
	
	// calculate timeout from traceTime and lastTime
	timeout->tv_sec = traceTime.tv_sec - lastTime.tv_sec;
	timeout->tv_usec = traceTime.tv_usec - lastTime.tv_usec;
	if (timeout->tv_usec < 0) {
		timeout->tv_sec--;
		timeout->tv_usec = timeout->tv_usec + 1000000;
	}
	memcpy(&lastTime, &traceTime, sizeof(struct timeval));
		
	
	if (ret == EOF) {
		LIBHAGGLE_DBG("EOF when reading data trace\n");
		haggle_dataobject_free(*dobj);
		*dobj = NULL;
		return 0;
	} else if (ret < 3) {
		LIBHAGGLE_ERR("A conversion error occured when reading data trace\n");
		haggle_dataobject_free(*dobj);
		*dobj = NULL;
		return -1;
	}
	
	total_bytes += bytes;
	
	LIBHAGGLE_DBG("%ld.%06ld %s\n", (long)timeout->tv_sec, (long)timeout->tv_usec, node);
	
	do {
		char name[50], value[50];
		unsigned long weight;
		
		ret = sscanf(line + total_bytes, " %[^=]=%[^:]:%lu%n", name, value, &weight, &bytes);
		
		if (ret == 3) {
			LIBHAGGLE_DBG("\tadding attribute %s=%s:%lu\n", name, value, weight);
			total_bytes += bytes;
			haggle_dataobject_add_attribute_weighted(*dobj, name, value, weight);
		}
	} while (ret == 3);
	
	luckyme_dataobject_set_createtime(*dobj);
	
	num_dobj_created++;
			
	return 1;
}

int on_dataobject(haggle_event_t *e, void* nix)
{
	num_dobj_received++;

#if defined(OS_WINDOWS_MOBILE)
	if (callback)
		callback(EVENT_TYPE_NEW_DATAOBJECT);
#endif
	
	if (e->dobj) {
		const unsigned char *xml = haggle_dataobject_get_raw(e->dobj);
		
		if (xml) {
			LIBHAGGLE_DBG("Received data object:\n%s\n", (const char *)xml);
		}
	}
	
	return 0;
}

static void neighbor_list_clear()
{
	unsigned long i;

	mutex_lock(&mutex);

	if (num_neighbors) {
		for (i = 0; i < num_neighbors; i++) {
			free(neighbors[i]);
		}
		free(neighbors);
	}
	num_neighbors = 0;
	neighbors = NULL;

	mutex_unlock(&mutex);
}

int on_neighbor_update(haggle_event_t *e, void* nix)
{
	haggle_nodelist_t *nl = e->neighbors;
	unsigned long i;
	
	neighbor_list_clear();
	
	mutex_lock(&mutex);

	num_neighbors = haggle_nodelist_size(nl);

	LIBHAGGLE_DBG("number of neighbors is %lu\n", num_neighbors);
	
	if (num_neighbors > 0) {
		neighbors = (char **)malloc(sizeof(char *) * num_neighbors);
		
		if (!neighbors) {
			num_neighbors = 0;
		} else {
			list_t *pos;
			i = 0;
			list_for_each(pos, &nl->nodes) {
				haggle_node_t *n = (haggle_node_t *)pos;
				neighbors[i] = (char *)malloc(strlen(haggle_node_get_name(n)) + 1);
				
				if (neighbors[i]) {
					strcpy(neighbors[i], haggle_node_get_name(n));
				}
				i++;
			}
		}
	} 
	
	mutex_unlock(&mutex);

#if defined(OS_WINDOWS_MOBILE)
	if (callback)
		callback(EVENT_TYPE_NEIGHBOR_UPDATE);
#endif
	
	return 0;
}

int on_shutdown(haggle_event_t *e, void* nix)
{
#if defined(OS_WINDOWS_MOBILE)
	
	LIBHAGGLE_DBG("Got shutdown event\n");
	
	if (hh && called_haggle_shutdown) {
		if (haggle_event_loop_is_running(hh)) {
			LIBHAGGLE_DBG("Stopping event loop\n");
			haggle_event_loop_stop(hh);
		}
	}
	
	if (callback)
		callback(EVENT_TYPE_SHUTDOWN);
	
#else
	ssize_t ret;
	stop_now = 1;
	ret = write(test_loop_event[1], "x", 1);
	
	if (ret == -1)
		fprintf(stderr, "shutdown signal error\n");
#endif
	return 0;
}

int on_interests(haggle_event_t *e, void* nix)
{
	LIBHAGGLE_DBG("Received application interests event\n");

	if (haggle_attributelist_get_attribute_by_name(e->interests, APP_NAME) != NULL) {
#if defined(DEBUG)
		list_t *pos;

		LIBHAGGLE_DBG("Checking existing interests\n");

		// We already have some interests, so we don't create any new ones.
		
		// In the future, we might want to delete the old interests, and
		// create new ones... depending on the circumstances.
		// If so, that code would fit here. 

		list_for_each(pos, &e->interests->attributes) {
			struct attribute *attr = (struct attribute *)pos;
			LIBHAGGLE_DBG("interest: %s=%s:%lu\n", 
			       haggle_attribute_get_name(attr), 
			       haggle_attribute_get_value(attr), 
			       haggle_attribute_get_weight(attr));
		}
#endif
	} else {
		LIBHAGGLE_DBG("No existing interests, generating new ones\n");

		// No old interests: Create new interests.
		// Read interests from file, if trace filename is set
		if (data_trace_filename) {
			// Read interests
// christian: had to do a hack because the callback on_interest is too late.
			// while (read_interest_from_trace() > 0) {}
		} else {
			if (grid_size > 0) {
				create_interest_grid();
			} else {
				create_interest_binomial();
			}
		}
	}
	
	return 0;
}



// ----- USAGE (parse arguments)

#if defined(OS_UNIX)

static void print_usage()
{	
	fprintf(stderr, 
		"Usage: ./%s [-A num] [-d num] [-i num] [-t interval] [-n] [-g gridSize] [-N num] [-s hostname] [-f path] [-l data_trace]\n", 
		APP_NAME);
	fprintf(stderr, "          -A attribute pool (default %lu)\n", attribute_pool_size);
	fprintf(stderr, "          -d number of attributes per data object (default %lu)\n", num_dataobject_attributes);
	fprintf(stderr, "          -i number of interests (default %lu)\n", num_interest_attributes);
	fprintf(stderr, "          -t interval to create data objects [s] (default %lu)\n", create_data_interval);
	fprintf(stderr, "          -N number of data objects to be generated (no limit: 0, default %lu)\n", num_dataobjects);
	fprintf(stderr, "          -s singe source (create data objects only on node 'name', default off)\n");
	fprintf(stderr, "          -f data file to be sent (default off)\n");
	fprintf(stderr, "          -n use node number as interest (default off)\n");
	fprintf(stderr, "          -g use grid topology (gridSize x gridSize, overwrites -Adi, default off)\n");
	fprintf(stderr, "          -r repeatable experiments (constant seed, incremental createtime, default off)\n");
	fprintf(stderr, "          -l data trace log used to generate data objects\n");
}

static void parse_commandline(int argc, char **argv)
// Parses our command line arguments into the global variables 
// listed above.
{
	int ch;
	
	// Parse command line options using getopt.
	
	do {
		ch = getopt(argc, argv, "A:d:i:t:g:nrs:f:l:N:");
		if (ch != -1) {
			switch (ch) {
				case 'A':
					attribute_pool_size = strtoul(optarg, NULL, 10);
					break;
				case 'd':
					num_dataobject_attributes = strtoul(optarg, NULL, 10);
					break;
				case 'i':
					num_interest_attributes = strtoul(optarg, NULL, 10);
					break;
				case 't':
					create_data_interval = strtoul(optarg, NULL, 10) * 1000;
					break;
				case 'g':
					grid_size = strtoul(optarg, NULL, 10);
					attribute_pool_size = 2 * grid_size;
					break;
				case 'r':
					repeatableSeed = 1;
					break;
				case 'n':
					use_node_number = 1;
					break;
				case 's':
					single_source_name = optarg;
					break;
				case 'f':
					filename = optarg;
					break;
				case 'l':
					data_trace_filename = optarg;
					break;
				case 'N':
					num_dataobjects = strtoul(optarg, NULL, 10);
					break;
				default:
					print_usage();
					exit(1);
					break;
			}
		}
	} while (ch != -1);
	
	// Check for any left over command line arguments.
	
	if (optind != argc) {
		print_usage();
		fprintf(stderr, "%s: Unexpected argument '%s'\n", argv[0], argv[optind]);
		exit(1);
	}
	
}

#endif // OS_UNIX

void milliseconds_to_timeval(struct timeval *tv, unsigned long milliseconds)
{
    tv->tv_sec = 0;
	while (milliseconds >= 1000) {
		tv->tv_sec++;
		milliseconds -= 1000;
	}
	
	tv->tv_usec = milliseconds;
}

void test_loop() {
	int result = 0;
		
#if defined(OS_WINDOWS)
	DWORD wait, ret;
	// Signal that we are running
	SetEvent(luckyme_start_event);
#else
	unsigned int nfds = test_loop_event[0] + 1;
	fd_set readfds;
#endif

	if (data_trace_filename) {
		// Read interests
		while (read_interest_from_trace() > 0) {}
	}


	while (!stop_now) {
		struct dataobject *dobj = NULL;
		struct timeval *t = NULL, timeout = { 0, 0 };
		
		if (data_trace_filename) {
			// Read next data object from trace
			result = read_dataobject_from_trace(&dobj, &timeout);
			
			if (result > 0) {
				create_data_interval = timeout.tv_sec * 1000 + (timeout.tv_usec / 1000);
				
				LIBHAGGLE_DBG("publishing data object in %ld.%06ld seconds\n", (long)timeout.tv_sec, (long)timeout.tv_usec);
				t = &timeout;
				
			} else if (result == 0) {
#if defined(OS_WINDOWS)
				create_data_interval = INFINITE;
#else
				t = NULL;
#endif
			} else {
				LIBHAGGLE_ERR("Error when reading data trace\n");
				return;
			}
		} else {
			milliseconds_to_timeval(&timeout, create_data_interval);
			t = &timeout;
		}
#if defined(OS_WINDOWS)
		wait = test_is_running ? create_data_interval : INFINITE;
		
		ret = WaitForSingleObject(test_loop_event, wait);
		
		switch (ret) {
			case WAIT_OBJECT_0:
				// Set result 1, i.e., create no data object.
				// We just check whether the test is running or not, or
				// if we should exit
				result = 1;
				break;
			case WAIT_FAILED:
				stop_now = 1;
				result = -1;
				LIBHAGGLE_DBG("Wait failed!!\n");
				break;
			case WAIT_TIMEOUT:
				result = 0;
				break;
		}
#else
		
		FD_ZERO(&readfds);
		FD_SET(test_loop_event[0], &readfds);
		
		result = select(nfds, &readfds, NULL, NULL, t);
#endif
		if (result < 0) {
			if (ERRNO != EINTR) {
				LIBHAGGLE_ERR("Stopping due to error!\n");
				stop_now = 1;
			}
		} else if (result == 0) {
			if ((num_dataobjects == 0) || (num_dobj_created < num_dataobjects)) {
				if (single_source_name == NULL || strcmp(hostname, single_source_name) == 0) {
					LIBHAGGLE_DBG("Creating data object\n");
					if (dobj) {
						haggle_ipc_publish_dataobject(hh, dobj);
						haggle_dataobject_free(dobj);
					} else {
						if (grid_size > 0) {
							create_dataobject_grid();
						} else {
							create_dataobject_random();
						}
					}
				}
#if defined(OS_WINDOWS_MOBILE)
				if (callback)
					callback(EVENT_TYPE_DATA_OBJECT_GENERATED);
#endif
			}
		} else if (result == 1) {
			// Check whether we should exit
		}
	}
	LIBHAGGLE_DBG("test loop done!\n");
}


void on_event_loop_start(void *arg)
{	
	haggle_handle_t hh = (haggle_handle_t)arg;

	luckyme_prng_init();
	/* retreive interests: */
	LIBHAGGLE_DBG("Requesting application interests.\n");

	if (haggle_ipc_get_application_interests_async(hh) != HAGGLE_NO_ERROR) {
		LIBHAGGLE_DBG("Could not request interests\n");
	}
}

void on_event_loop_stop(void *arg)
{
	haggle_handle_t hh = (haggle_handle_t)arg;

	if (hh) {
		LIBHAGGLE_DBG("Freeing Haggle handle\n");
		haggle_handle_free(hh);
		hh = NULL;
	}
	
	// Clean up neighbor list
	neighbor_list_clear();
}

#if defined(OS_WINDOWS)
DWORD WINAPI luckyme_run(void *arg)
#else
int luckyme_run()
#endif
{
	int ret = 0, i;
	unsigned int retry = 3;
	
	stop_now = 0;
	// get hostname (used to set interest)
	gethostname(hostname, 128);
	
	for (i = strlen(hostname) - 1; i >= 0; i--) {
		if (hostname[i] == '-') {
			node_number = strtoul(&hostname[i + 1], NULL, 10);
		}
	}
	printf("hostname = %s\n", hostname);
	printf("node_number = %lu\n", node_number);

	// reset random number generator
	// note: set node_number before the call of luckyme_prng_init();
	luckyme_prng_init();
		
	do {
		LIBHAGGLE_DBG("Trying to register with Haggle\n");

		ret = haggle_handle_get(APP_NAME, &hh);
		
		// Busy?
		if (ret == HAGGLE_BUSY_ERROR) {
			// Unregister and try again.
			LIBHAGGLE_DBG("Application is already registered.\n");
			haggle_unregister(APP_NAME);
		}
#ifdef OS_WINDOWS_MOBILE
		Sleep(5000);
#else
		sleep(1);
#endif
	} while (ret != HAGGLE_NO_ERROR && retry-- != 0);
	
	if (ret != HAGGLE_NO_ERROR || hh == NULL) {
		LIBHAGGLE_ERR("Could not get Haggle handle\n");
		goto out_error;
	}
	
	ret = haggle_event_loop_register_callbacks(hh, on_event_loop_start, on_event_loop_stop, hh);
	
	if (ret != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not register start and stop callbacks\n");
		goto out_error;
	}
	// register callback for new data objects
	ret = haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_SHUTDOWN, on_shutdown);

	if (ret != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not register shutdown event interest\n");
		goto out_error;
	}

	ret = haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, on_neighbor_update);

	if (ret != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not register neighbor update event interest\n");
		goto out_error;
	}

	ret = haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_NEW_DATAOBJECT, on_dataobject);

	if (ret != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not register new data object event interest\n");
		goto out_error;
	}

	ret = haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_INTEREST_LIST, on_interests);

	if (ret != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not register interest list event interest\n");
		goto out_error;
	}
	
	if (data_trace_filename) {
		data_trace_fp = fopen(data_trace_filename, "r");
		
		if (!data_trace_fp) {
			LIBHAGGLE_ERR("Could not open data trace \'%s\'\n", data_trace_filename);
			goto out_error;
		}
	}
	
	if (haggle_event_loop_run_async(hh) != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not start event loop\n");
		goto out_error;
	}
		
	NOTIFY();
	
	test_loop();
	
	// Join with libhaggle thread
	if (haggle_daemon_pid(NULL) == HAGGLE_DAEMON_RUNNING && called_haggle_shutdown) {
		// if we called shutdown, wait to free the haggle handle until
		// we get the shutdown callback
		LIBHAGGLE_DBG("Deferring event loop stop until shutdown event\n");
	} else {
		if (haggle_event_loop_is_running(hh)) {
			LIBHAGGLE_DBG("Stopping event loop\n");
			haggle_event_loop_stop(hh);
		}
	}
	
	if (data_trace_fp)
		fclose(data_trace_fp);
	
	return 0;
	
out_error:
	NOTIFY_ERROR();

	if (hh)
		haggle_handle_free(hh);
	
	if (data_trace_fp)
		fclose(data_trace_fp);
	
	return ret;
}

#if defined(OS_UNIX)
void signal_handler()
{
	ssize_t ret;
	stop_now = 1;
	ret = write(test_loop_event[1], "x", 1);

	if (ret == -1)
		fprintf(stderr, "could not signal shutdown event\n");
}

int main(int argc, char **argv)
{
	int res;

	signal(SIGINT, signal_handler);      // SIGINT is what you get for a Ctrl-C
	parse_commandline(argc, argv);
	
	res = pipe(test_loop_event);
	
	if (res == -1) {
		LIBHAGGLE_ERR("Could not open pipe\n");
		return -1;
	}
	
	mutex_init(&mutex);

	res = luckyme_run();
	
	mutex_del(&mutex);
	
	if (test_loop_event[0])
		close(test_loop_event[0]);
	
	if (test_loop_event[1])
		close(test_loop_event[1]);
	
	return res;
}
#elif defined(OS_WINDOWS_MOBILE) && defined(CONSOLE)
int wmain()
{
	test_is_running = 1;
	test_loop_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	luckyme_run(NULL);
	CloseHandle(test_loop_event);
}
#endif
