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

#if defined(WIN32) || defined(WINCE)
#define CLOSE_SOCKET(s) closesocket(s)
#include <winsock2.h>

typedef HANDLE thread_handle_t;
typedef DWORD thread_handle_attr_t;
typedef DWORD thread_handle_id_t;
#define cleanup_ret_t void
#define start_ret_t DWORD WINAPI

#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

typedef pthread_t thread_handle_t;
typedef unsigned int thread_handle_id_t;
typedef pthread_attr_t thread_handle_attr_t;
#define cleanup_ret_t void
#define start_ret_t void*

#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LIBHAGGLE_INTERNAL
#include <libhaggle/platform.h>
#include "metadata.h"
#include <libhaggle/debug.h>
#include <libhaggle/ipc.h>
#include <libhaggle/attribute.h>
#include <libhaggle/node.h>
#include <libhaggle/dataobject.h>

#define PID_FILE libhaggle_platform_get_path(PLATFORM_PATH_HAGGLE_PRIVATE, "/haggle.pid")
#define VALGRIND_PROCESS_NAME "valgrind"

#if defined(OS_ANDROID)
#define HAGGLE_PROCESS_NAME "org.haggle.kernel"
#else
#define HAGGLE_PROCESS_NAME "haggle"
#endif

#ifdef USE_UNIX_APPLICATION_SOCKET
#define HAGGLE_UNIX_SOCK_PATH "/tmp/haggle.sock"
#else
#define HAGGLE_SERVICE_DEFAULT_PORT 8787
#endif /* USE_UNIX_APPLICATION_SOCKET */

#include "sha1.h"
#include "base64.h"

#define DATA_BUFLEN (10000) /* What would be a suitable max size */
#define EVENT_BUFLEN (50000)

#define ID_LEN SHA1_DIGEST_LENGTH
#define ID_BASE64_LEN ((((ID_LEN) + 2) / 3) * 4 + 1)

static unsigned char databuffer[DATA_BUFLEN];

struct handler_data {
	void *arg;
	haggle_event_handler_t handler;
};

struct haggle_handle {
	list_t l;
	SOCKET sock;
#if defined(OS_UNIX)
        int signal[2];
#elif defined(OS_WINDOWS)
        HANDLE signal;
	DWORD th_id;
#endif
	int session_id;
	int num_handlers;
	int event_loop_running;
	int handle_free_final;
	thread_handle_t th;
	char *name;
	char id[ID_LEN];
	char id_base64[ID_BASE64_LEN];
        haggle_event_loop_start_t start;
        haggle_event_loop_stop_t stop;
        void *arg; // argument to pass to start and stop
	struct handler_data handlers[_LIBHAGGLE_NUM_EVENTS];
};

struct sockaddr_in haggle_addr;

HAGGLE_API char *haggle_directory = NULL;

HAGGLE_API int libhaggle_errno = 0;

static int num_handles = 0;

LIST(haggle_handles);

/*
	A function that sends a data object to Haggle.

	The caller may optionally block and wait for a reply with a timeout given
	in milli seconds. 

	In the case of a reply, the reply data object is returned through the
	given data object "pointer to pointer", which can be NULL otherwise. The
	timeout can be set to IO_REPLY_BLOCK for no timeout (i.e., block until
	a reply is received), or IO_REPLY_NON_BLOCK in case the function should
	try to receive a reply without waiting for it. With a positive timeout
	value, the function will block for the specified time, or until a reply
	is received.

	If no reply is asked for, the caller should set the reply data object
	pointer to NULL and set the timeout to IO_NO_REPLY.

*/
static int haggle_ipc_send_dataobject(struct haggle_handle *h, haggle_dobj_t *dobj, 
				      haggle_dobj_t **dobj_reply, long msecs_timeout);

/*
	A generic send function that takes a list of attributes and adds to a new data object
	which it subsequently sends to Haggle. It will not add any additional attributes.
*/
static int haggle_ipc_generate_and_send_control_dataobject(haggle_handle_t hh, control_type_t type);

static int is_event_loop_thread(haggle_handle_t hh);
static void haggle_handle_free_final(haggle_handle_t hh);

enum {
        EVENT_LOOP_ERROR = -1,
        EVENT_LOOP_TIMEOUT = 0,
        EVENT_LOOP_SHOULD_EXIT = 1,
        EVENT_LOOP_SOCKET_READABLE = 2,
};

#if defined(OS_WINDOWS)

// This function is defined in platform.c, but we do not want to expose it in a header file
extern wchar_t *strtowstr_alloc(const char *str);

typedef int socklen_t;

/* DLL entry point */
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	int iResult = 0;
	WSADATA wsaData;

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
#if defined(DEBUG)
		libhaggle_debug_init();
#endif
		// Make sure Winsock is initialized
		iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

		if (iResult != 0) {
			fprintf(stderr, "WSAStartup failed\n");
			return FALSE;
		}
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
#if defined(DEBUG)
		libhaggle_debug_fini();
#endif
		WSACleanup();
		break;
	}
	return TRUE;
}

int wait_for_event(haggle_handle_t hh, struct timeval *tv)
{
	DWORD timeout = tv ? (tv->tv_sec * 1000) + tv->tv_usec / 1000 : INFINITE; 
	WSAEVENT eventArr[2]; 
	DWORD waitres;
	WSAEVENT socketEvent = WSACreateEvent();

	eventArr[0] = hh->signal;
	eventArr[1] = socketEvent;

	WSAEventSelect(hh->sock, socketEvent, FD_READ);

	LIBHAGGLE_DBG("Waiting for timeout, or socket or signal event\n");
	waitres = WSAWaitForMultipleEvents(2, eventArr, FALSE, timeout, FALSE);

	if (waitres >= WSA_WAIT_EVENT_0 && waitres < (DWORD)(WSA_WAIT_EVENT_0 + 2)) {
		int i = (unsigned int)(waitres - WSA_WAIT_EVENT_0);

		LIBHAGGLE_DBG("Got WSAEvent i=%d\n", i);

		if (i == 0) {
			return EVENT_LOOP_SHOULD_EXIT;
		} else {
			WSANETWORKEVENTS netEvents;
			DWORD res;

			// This call will automatically reset the Event as well
			res = WSAEnumNetworkEvents(hh->sock, socketEvent, &netEvents);

			if (res == 0) {
				if (netEvents.lNetworkEvents & FD_READ) {
					// Socket is readable
					if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
						LIBHAGGLE_ERR("Read error\n");
						return EVENT_LOOP_ERROR;
					}
					LIBHAGGLE_DBG("FD_READ on socket %d\n", hh->sock);
					return EVENT_LOOP_SOCKET_READABLE;
				} 
			} else {
				// Error occurred... do something to handle.
				LIBHAGGLE_DBG("WSAEnumNetworkEvents ERROR\n");
				return EVENT_LOOP_ERROR;
			}
		}
	
	} else if (waitres == WSA_WAIT_TIMEOUT) {
		return EVENT_LOOP_TIMEOUT;	
	} else if (waitres == WSA_WAIT_FAILED) {
		LIBHAGGLE_DBG("WSA_WAIT_FAILED Error=%d\n", WSAGetLastError());
		return EVENT_LOOP_ERROR;
	} 
	return EVENT_LOOP_ERROR;
}


int event_loop_signal_is_raised(haggle_handle_t hh)
{
        if (!hh || !hh->signal)
                return -1;

        return WaitForSingleObject(hh->signal, 0) == WAIT_OBJECT_0 ? 1 : 0;
}

void event_loop_signal_raise(haggle_handle_t hh)
{
        if (!hh || !hh->signal)
                return;
        
        SetEvent(hh->signal);
}

void event_loop_signal_lower(haggle_handle_t hh)
{
    if (!hh || !hh->signal)
                return;
        
        ResetEvent(hh->signal);
}
#elif defined(OS_UNIX)
#include <fcntl.h>

int wait_for_event(haggle_handle_t hh, struct timeval *tv)
{
        fd_set readfds;
        int maxfd = 0, ret = 0;

        FD_ZERO(&readfds);
        FD_SET(hh->signal[0], &readfds);
        FD_SET(hh->sock, &readfds);
        
        if (hh->signal[0] > hh->sock)
                maxfd = hh->signal[0];
        else
                maxfd = hh->sock;
        
        ret = select(maxfd + 1, &readfds, NULL, NULL, tv);

        if (ret < 0) {
                return EVENT_LOOP_ERROR;
        } else if (ret == 0) {
                return EVENT_LOOP_TIMEOUT;
        } else if (FD_ISSET(hh->signal[0], &readfds)) {
                return EVENT_LOOP_SHOULD_EXIT;
        } else if (FD_ISSET(hh->sock, &readfds)) {
                return EVENT_LOOP_SOCKET_READABLE;
        }
        return EVENT_LOOP_ERROR;
}


int event_loop_signal_is_raised(haggle_handle_t hh)
{
        char c;

        if (!hh || !hh->signal[0])
                return -1;

        return recv(hh->signal[0], &c, 1, MSG_PEEK) ? 1 : 0;
}

void event_loop_signal_raise(haggle_handle_t hh)
{
        char c = '1';
        int res;

        if (!hh || !hh->signal[1])
                return;
        
        res = write(hh->signal[1], &c, 1);

	if (res == -1) {
		fprintf(stderr, "%s: could not raise signal\n", __func__);
	}
}


void event_loop_signal_lower(haggle_handle_t hh)
{
        char c;
        int res;

        if (!hh || !hh->signal[0])
                return;
        
        fcntl(hh->signal[0], F_SETFD, O_NONBLOCK);
        res = read(hh->signal[0], &c, 1);
        fcntl(hh->signal[0], F_SETFD, ~O_NONBLOCK);

	if (res == -1) {
		perror("event_loop_signal_lower:");
	}
}
#endif

static char *intTostr(int n) 
{
	static char intStr[5];
	
	sprintf(intStr, "%d", n);

	return intStr;
}

int haggle_get_error()
{
	return libhaggle_errno;
}

static void haggle_set_socket_error()
{
#if defined(WIN32) && defined(WINCE)
	libhaggle_errno = WSAGetLastError();
#else
	libhaggle_errno = errno;
#endif
}

#if defined(OS_WINDOWS)
typedef DWORD pid_t;
#endif

/**
   Check if the Haggle daemon is running and return its pid.
   Return 0 if Haggle is not running, -1 on error, or 1 if
   there is a valid pid of a running Haggle instance.
 */
int haggle_daemon_pid(unsigned long *pid)
{
#define PIDBUFLEN 200
        char buf[PIDBUFLEN];
        size_t ret;
        FILE *fp;
	unsigned long _pid;
#if defined(OS_WINDOWS)
        HANDLE p;
#endif
	int old_instance_is_running = 0;
	LIBHAGGLE_ERR("checking if haggle daemon can be found\n");
	if (pid)
	        *pid = 0;

        fp = fopen(PID_FILE, "r");

        if (!fp) {
                /* The Pid file does not exist --> Haggle not running. */
                LIBHAGGLE_ERR("haggle pid file does not exist\n");
                return HAGGLE_DAEMON_NOT_RUNNING;
        }

        memset(buf, 0, PIDBUFLEN);

        /* Read process id from pid file. */
        ret = fread(buf, 1, PIDBUFLEN, fp);

        fclose(fp);

        if (ret == 0) {
                LIBHAGGLE_ERR("unable to read pid file\n");
                return HAGGLE_DAEMON_ERROR;
        }

        _pid = strtoul(buf, NULL, 10);
        
	if (pid)
	        *pid = _pid;

        /* Check whether there is a process matching the pid */
#if defined(OS_LINUX)
        /* On Linux, do not use kill to figure out whether there is a
        process with the matching PID. This is because we run Haggle
        with root privileges on Android, and kill() can only signal a
        process running as the same user. Therefore, an application
        running with non-root privileges would not be able to use
        kill(). */

        /* Check /proc file system for a process with the matching
         * Pid */
	    LIBHAGGLE_ERR("checking proc filesystem\n");
        snprintf(buf, PIDBUFLEN, "/proc/%ld/cmdline", _pid);

        fp = fopen(buf, "r");

        if (fp != NULL) {
                size_t nitems = fread(buf, 1, PIDBUFLEN, fp);
                if (nitems &&
                        (strstr(buf, HAGGLE_PROCESS_NAME) != NULL) || strstr(buf,VALGRIND_PROCESS_NAME)) {
                    old_instance_is_running = 1;
                }
                fclose(fp);
	}
       
#elif defined(OS_UNIX)
	old_instance_is_running = (kill(_pid, 0) != -1);
#elif defined(OS_WINDOWS)
	p = OpenProcess(0, FALSE, _pid);

	if (p != NULL) {
		DWORD exitcode = 0;
		if (GetExitCodeProcess(p, &exitcode) != 0) {
			old_instance_is_running = (exitcode == STILL_ACTIVE);
		}
		CloseHandle(p);
	} 
#endif
	/* If there was a process, return its pid */
	if (old_instance_is_running)
		return HAGGLE_DAEMON_RUNNING;
        
        /* No process matching the pid --> Haggle is not running and
         * previously quit without cleaning up (e.g., Haggle crashed,
         * or the phone ran out of battery, etc.)
         */
	    LIBHAGGLE_ERR("haggle daemon not found returning haggle daemon crashed\n");
        return HAGGLE_DAEMON_CRASHED;
}

static int spawn_daemon_internal(const char *daemonpath, 
				 daemon_spawn_callback_t callback)
{
	int ret = HAGGLE_ERROR;
	int status;
	SOCKET sock;
	fd_set readfds;
	struct timeval timeout;
	unsigned int time_left, time_passed = 0;
	int maxfd = 0;
	
#if defined(OS_WINDOWS)
#if defined(OS_WINDOWS_MOBILE) || defined(OS_WINDOWS_VISTA)
	wchar_t *path;
#else
	const char *path;
#endif
	PROCESS_INFORMATION pi;
#endif
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock == INVALID_SOCKET) {
		ret = HAGGLE_SOCKET_ERROR;
		haggle_set_socket_error();
		LIBHAGGLE_ERR("Could not open launch callback socket\n");
		goto fail_sock;
	}
	
	haggle_addr.sin_family = AF_INET;
	haggle_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	haggle_addr.sin_port = htons(50888);

	if (bind(sock, (struct sockaddr *) &haggle_addr, sizeof(haggle_addr)) == SOCKET_ERROR) {
		ret = HAGGLE_SOCKET_ERROR;
		haggle_set_socket_error();
		LIBHAGGLE_ERR("Could not bind launch callback socket\n");
		goto fail_start;
	}
#if defined(OS_UNIX)
#define PATH_LEN 200
	char cmd[PATH_LEN];

#if defined(OS_ANDROID)
	/* Should probably figure out a cleaner way of launching the Haggle service */
	snprintf(cmd, PATH_LEN, "am startservice -a android.intent.action.MAIN -n org.haggle.kernel/org.haggle.kernel.Haggle");
#else
	if (!daemonpath) {
		ret = HAGGLE_ERROR;
		goto fail_start;
	}
	
	snprintf(cmd, PATH_LEN, "%s -d", daemonpath);
#endif /* OS_ANDROID */
	LIBHAGGLE_DBG("Trying to spawn daemon using %s\n", cmd);
	
	ret = system(cmd);
	
	if (ret == -1 || ret != 0) {
		LIBHAGGLE_ERR("could not start Haggle daemon, err=%d\n", ret);
		ret = HAGGLE_ERROR;
		goto fail_start;
	}
	
#elif defined(OS_WINDOWS)
#if defined(OS_WINDOWS_MOBILE) || defined(OS_WINDOWS_VISTA)
	path = strtowstr_alloc(daemonpath);
#else
	path = daemonpath;
#endif
	if (!path) {
		ret = HAGGLE_ERROR;
		goto fail_start;
	}
	ret = CreateProcess(path, L"", NULL, NULL, 0, 0, NULL, NULL, NULL, &pi);

#if defined(OS_WINDOWS_MOBILE) || defined(OS_WINDOWS_VISTA)
	free(path);	
#endif
	if (ret == 0) {
		LIBHAGGLE_ERR("Could not create process\n");
		ret = HAGGLE_ERROR;
		goto fail_start;
	}
#endif
	
	// Wait for 90 seconds max.
	time_left = 90;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	ret = HAGGLE_ERROR;
	
	LIBHAGGLE_DBG("Waiting for Haggle to start...\n");

	while (1) {
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		
		maxfd = sock;
		
		status = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

		if (status < 0) {
			LIBHAGGLE_DBG("Socket error while waiting for Haggle to start\n");
			ret = HAGGLE_SOCKET_ERROR;
			haggle_set_socket_error();
			break;
		} else if (status == 0) {
			// Timeout!
			
			LIBHAGGLE_DBG("Timeout while waiting for Haggle to start\n");

			time_left--;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			time_passed += timeout.tv_sec * 1000 + timeout.tv_usec / 1000;

			if (time_left == 0) {
				ret = HAGGLE_DAEMON_ERROR;
				break;
			}
			if (callback) {
				LIBHAGGLE_DBG("Calling daemon spawn callback, time_passed=%u\n", 
					      time_passed);

				if (callback(time_passed) == -1) {
					LIBHAGGLE_DBG("Application wants to stop waiting\n");
					ret = HAGGLE_DAEMON_ERROR;
					break;
				}
			}
		} else if (FD_ISSET(sock, &readfds)) {
			// FIXME: should probably check that the message is a correct data 
			// object first...
			LIBHAGGLE_DBG("Haggle signals it is running\n");
			if (callback) {
				LIBHAGGLE_DBG("Done! Calling daemon spawn callback\n");
				callback(0);
			}
			ret = HAGGLE_NO_ERROR;
			break;
		}
	}
	
fail_start:
	CLOSE_SOCKET(sock);
fail_sock:
	return ret;
}

int haggle_daemon_spawn(const char *daemonpath)
{
	return haggle_daemon_spawn_with_callback(daemonpath, NULL);
}

int haggle_daemon_spawn_with_callback(const char *daemonpath, daemon_spawn_callback_t callback)
{
        int i = 0;

	if (haggle_daemon_pid(NULL) == HAGGLE_DAEMON_RUNNING)
                return HAGGLE_NO_ERROR;

        if (daemonpath) {
                return spawn_daemon_internal(daemonpath, callback);
        }

	if (!callback) {
		LIBHAGGLE_DBG("No daemon spawn callback\n");
	}

#if defined(OS_ANDROID)
	return spawn_daemon_internal(NULL, callback);
#else
	while (1) {
		/*
		Need to put the definition of haggle_paths here. Otherwise,
		the call to haggle_daemon_pid() will overwrite the static 
		memory used to get the platform path.
		*/
#if defined(OS_WINDOWS)
		const char *haggle_paths[] = { 
			libhaggle_platform_get_path(PLATFORM_PATH_HAGGLE_EXE, "\\Haggle.exe"), 
			NULL 
		};
#elif defined(OS_UNIX)
		// Do some qualified guessing
		const char *haggle_paths[] = { 
			"./haggle", 
			"./bin/haggle", 
			"/bin/haggle",
			"/usr/bin/haggle", 
			"/usr/local/bin/haggle", 
			"/opt/bin/haggle", 
			"/opt/local/bin/haggle", 
			NULL 
		};
#endif
		FILE *fp = fopen(haggle_paths[i], "r");

		if (fp) {
			fclose(fp);
			return spawn_daemon_internal(haggle_paths[i], callback);
		}

		if (!haggle_paths[++i])
			break;
	}

	return HAGGLE_ERROR;
#endif /* OS_ANDROID */
}

static const char *ctrl_type_names[] = { 
	"registration_request", 
	"registration_reply", 
	"deregistration", 
	"register_interest",
	"remove_interest",
	"get_interests",
	"register_event_interest", 
	"matching_dataobject",
	"delete_dataobject",
	"get_dataobjects",
	"send_node_description",
	"shutdown", 
	"event", 
	"set_param", 
    "configure_security", // CBMEN, HL
    "dynamic_configure",
	NULL 
};

static control_type_t ctrl_name_to_type(const char *name)
{
	unsigned int i = 0;

	if (!name)
		return CTRL_TYPE_INVALID;

	while (ctrl_type_names[i]) {
		if (strcmp(ctrl_type_names[i], name) == 0) {
			return (control_type_t)i;
		}
		i++;
	}

	return CTRL_TYPE_INVALID;
}

static struct dataobject *create_control_dataobject(haggle_handle_t hh, const control_type_t type, metadata_t **ctrl_m)
{
	struct dataobject *dobj;
	metadata_t *m, *mc;
	
	if (!hh)
		return NULL;
	
	dobj = haggle_dataobject_new();
	
	if (!dobj)
		return NULL;

	haggle_dataobject_set_createtime(dobj, NULL);
	
	/* Control data objects are non-persistent, i.e., should not
	 * be added to the data store */
        haggle_dataobject_unset_flags(dobj, DATAOBJECT_FLAG_PERSISTENT);
	
	/*
	 Add control attribute. Haggle needs this to filter the message.
	 */
	haggle_dataobject_add_attribute(dobj, HAGGLE_ATTR_CONTROL_NAME, hh->name);
	
	
	m = metadata_new(DATAOBJECT_METADATA_APPLICATION, NULL, NULL);
	
	if (!m) {
		haggle_dataobject_free(dobj);
		return NULL;
	}
	
	if (metadata_set_parameter(m, DATAOBJECT_METADATA_APPLICATION_NAME_PARAM, hh->name) < 0)
		goto out_error;
	
	if (metadata_set_parameter(m, DATAOBJECT_METADATA_APPLICATION_ID_PARAM, hh->id_base64) < 0)
		goto out_error;
	
	mc = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL, NULL, m);
	
	if (!mc || metadata_set_parameter(mc, DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM, ctrl_type_names[type]) < 0) 
		goto out_error;
	
	if (ctrl_m)
		*ctrl_m = mc;
	
	haggle_dataobject_add_metadata(dobj, m);
	
	return dobj;
	
out_error:
	haggle_dataobject_free(dobj);
	metadata_free(m);
	return NULL;
	
}

int haggle_handle_get_internal(const char *name, haggle_handle_t *handle, 
			       int ignore_busy_signal)
{
	int ret;
	struct haggle_handle *hh = NULL;
	struct dataobject *dobj, *dobj_reply;
	metadata_t *m, *mc;
	control_type_t ctrl_type;
	SHA1_CTX ctxt;
	LIBHAGGLE_ERR("entering haggle_handle_get_internal\n");
#ifdef USE_UNIX_APPLICATION_SOCKET
#define AF_ADDRESS_FAMILY AF_UNIX
	struct sockaddr_un haggle_addr;
	socklen_t addrlen = sizeof(struct sockaddr_un);
#else
#define AF_ADDRESS_FAMILY AF_INET
	/* struct sockaddr_in local_addr; */
	/* unsigned long addrlen = sizeof(struct sockaddr_in); */
#endif

#if !defined(OS_MACOSX_IPHONE)
        if (haggle_daemon_pid(NULL) != HAGGLE_DAEMON_RUNNING) {
                LIBHAGGLE_ERR("haggle daemon not found\n");
                return HAGGLE_DAEMON_ERROR;
        }
#endif

	hh = (struct haggle_handle *)malloc(sizeof(struct haggle_handle));

	if (!hh)
		return HAGGLE_ALLOC_ERROR;

	memset(hh, 0, sizeof(struct haggle_handle));

	hh->num_handlers = 0;
	hh->handle_free_final = 0;

	INIT_LIST(&hh->l);

	hh->sock = socket(AF_ADDRESS_FAMILY, SOCK_DGRAM, 0);

	// setsockopt(hh->sock, SOL_SOCKET, SO_RCVBUF, 500000, 4); // MOS

	if (hh->sock == INVALID_SOCKET) {
		free(hh);
		haggle_set_socket_error();
#if defined(OS_UNIX)
		LIBHAGGLE_ERR("Could not open haggle socket: %s\n", strerror(errno));
#else
		LIBHAGGLE_ERR("Could not open haggle socket\n");
#endif
		return HAGGLE_SOCKET_ERROR;
	}
	
#if defined(OS_WINDOWS)
        hh->signal = CreateEvent(NULL, TRUE, FALSE, NULL);
#elif defined(OS_UNIX)
        ret = pipe(hh->signal);
        
        if (ret != 0) {
		free(hh);
		return HAGGLE_ERROR;
        }
#endif
	// Generate the application's SHA1 hash
	SHA1_Init(&ctxt);

	SHA1_Update(&ctxt, (unsigned char *)name, strlen(name));
	
	SHA1_Final((unsigned char *)hh->id, &ctxt);

	// Compute the base64 representation
	base64_encode(hh->id, ID_LEN, hh->id_base64, ID_BASE64_LEN);

	// Save name
	hh->name = (char *)malloc(strlen(name) + 1);

	if (!hh->name) {
		free(hh);
		return HAGGLE_ALLOC_ERROR;
	}

	strcpy(hh->name, name);

#ifdef USE_UNIX_APPLICATION_SOCKET
	haggle_addr.sun_family = AF_UNIX;
	strcpy(haggle_addr.sun_path, HAGGLE_UNIX_SOCK_PATH);
#else
	haggle_addr.sin_family = AF_INET;
	haggle_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	haggle_addr.sin_port = htons(HAGGLE_SERVICE_DEFAULT_PORT);
#endif

	LIBHAGGLE_DBG("creating control dataobject\n");
	dobj = create_control_dataobject(hh, CTRL_TYPE_REGISTRATION_REQUEST, 
					 NULL);

	if (!dobj) {
		ret = HAGGLE_ALLOC_ERROR;
		goto out_error;
	}

	ret = haggle_ipc_send_dataobject(hh, dobj, &dobj_reply, 30000); // MOS - increased from 10000 - should become API parameter
	
	haggle_dataobject_free(dobj);

	if (ret != HAGGLE_NO_ERROR) {
		LIBHAGGLE_ERR("Could not register with haggle daemon\n");

		if (ret >= 0)
			ret = HAGGLE_INTERNAL_ERROR;

		goto out_error;
	}
	
	
	LIBHAGGLE_DBG("Got reply, checking...\n");
	
	ret = HAGGLE_REGISTRATION_ERROR;
	
	LIBHAGGLE_DBG("checking metadata...\n");
	
	/*
	 Check the reply message.
	 */	
	m = haggle_dataobject_get_metadata(dobj_reply, 
					   DATAOBJECT_METADATA_APPLICATION);

// CBMEN, HL, Begin
//#if defined(DEBUG)
	//haggle_dataobject_print(stdout, dobj_reply);
//#endif
// CBMEN, HL, End

	if (!m) {
		haggle_dataobject_free(dobj_reply);
		LIBHAGGLE_ERR("Error: No application metadata!\n");
		goto out_error;
	}
	if (!metadata_get_parameter(m, DATAOBJECT_METADATA_APPLICATION_NAME_PARAM)) {
		haggle_dataobject_free(dobj_reply);
		LIBHAGGLE_ERR("Error: No application name metadata\n");
		goto out_error;
		
	}
	
	mc = metadata_get(m, DATAOBJECT_METADATA_APPLICATION_CONTROL);
	
	if (!mc) {
		haggle_dataobject_free(dobj_reply);
		LIBHAGGLE_ERR("Error: No control metadata\n");
		goto out_error;
	}

	ctrl_type = ctrl_name_to_type(metadata_get_parameter(mc, DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM));

	if (ctrl_type != CTRL_TYPE_REGISTRATION_REPLY) {
		haggle_dataobject_free(dobj_reply);
		LIBHAGGLE_ERR("Error: Invalid control metadata: NOT valid registration reply\n");
		goto out_error;
	}

	m = metadata_get(mc, DATAOBJECT_METADATA_APPLICATION_CONTROL_MESSAGE);

	if (!m || !metadata_get_content(m)) {
		LIBHAGGLE_ERR("Error: Invalid control metadata: NO control message\n");
		haggle_dataobject_free(dobj_reply);
		goto out_error;
	}
	
	if (strcmp(metadata_get_content(m), "OK") == 0) {

	} else if (strcmp(metadata_get_content(m), "Already registered") == 0) {
		if (ignore_busy_signal == 0) {
			LIBHAGGLE_ERR("Unable to get haggle handle - already registered!\n");
			haggle_dataobject_free(dobj_reply);
			ret = HAGGLE_BUSY_ERROR;
			goto out_error;
		}
	} else {
		// Unrecognized message
		haggle_dataobject_free(dobj_reply);
		goto out_error;
	}
	
	m = metadata_get(mc, DATAOBJECT_METADATA_APPLICATION_CONTROL_DIRECTORY);

	if (m) {
		if (!haggle_directory) {
			haggle_directory = malloc(strlen(metadata_get_content(m)) + 1);
			if (!haggle_directory) {
				LIBHAGGLE_ERR("Unable to allocate memory\n");
				haggle_dataobject_free(dobj_reply);
				ret = HAGGLE_ALLOC_ERROR;
				goto out_error;
			}
			strcpy(haggle_directory, metadata_get_content(m));
		}
	} else {
		LIBHAGGLE_ERR("Bad registration reply - no directory\n");
		haggle_dataobject_free(dobj_reply);
		goto out_error;
	}
	
	if (ignore_busy_signal == 0) {
		
		m = metadata_get(mc, DATAOBJECT_METADATA_APPLICATION_CONTROL_SESSION);

		if (!m) {
			LIBHAGGLE_ERR("Bad registration reply, no session metadata\n");
			haggle_dataobject_free(dobj_reply);
			goto out_error;
		}

		hh->session_id = atoi(metadata_get_content(m));
	}

	num_handles++;

	list_add(&hh->l, &haggle_handles);

	haggle_dataobject_free(dobj_reply);

	//if (shutdown_handler) {
	//	haggle_ipc_register_event_interest(h->id, LIBHAGGLE_EVENT_HAGGLE_SHUTDOWN, shutdown_handler);
	//}
	
	*handle = hh;

	return HAGGLE_NO_ERROR;

out_error:
	CLOSE_SOCKET(hh->sock);

	if (hh->name)
		free(hh->name);
	free(hh);
	return ret;
}

int haggle_handle_get(const char *name, haggle_handle_t *handle)
{
	return haggle_handle_get_internal(name, handle, 0);
}

void haggle_handle_free_final(haggle_handle_t hh)
{
	num_handles--;
	list_detach(&hh->l);
	CLOSE_SOCKET(hh->sock);
#if defined(OS_WINDOWS)
        CloseHandle(hh->signal);
#elif defined(OS_UNIX)
        close(hh->signal[0]);
        close(hh->signal[1]);
#endif

	if (hh->name)
		free(hh->name);

	free(hh);
	hh = NULL;
#ifdef DEBUG
	haggle_dataobject_leak_report_print();
#endif
}

void haggle_handle_free(haggle_handle_t hh)
{
	struct dataobject *dobj;
	int ret;

	if (!hh) {
		return;
	}
	
	if (hh->event_loop_running)
		haggle_event_loop_stop(hh);

	dobj = create_control_dataobject(hh, 
					 CTRL_TYPE_DEREGISTRATION_NOTICE, NULL);
	
	if (dobj) {
		ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);

		haggle_dataobject_free(dobj);		

		if (ret != HAGGLE_NO_ERROR) {
			LIBHAGGLE_ERR("failed to send deregistration\n");
		}
	}
	
	if (is_event_loop_thread(hh)) {
		/* Since we the event loop thread and are destroying
		 * the handle, we cannot expect the "main" thread to
		 * join later. Therefore, detach the thread to avoid
		 * leaks. */
#if defined(OS_UNIX)
		pthread_detach(hh->th);
#elif defined(OS_WINDOWS)
		/* TODO */
#endif
		/* Indicate that the handle should be freed when event
		 * loop exits. */
		hh->handle_free_final = 1;
	} else {
		haggle_handle_free_final(hh);
	}
}

int haggle_unregister(const char *name)
{
	haggle_handle_t	hh = NULL;
	
	haggle_handle_get_internal(name, &hh, 1);

	if (hh) {
		haggle_handle_free(hh);
		return HAGGLE_NO_ERROR;
	} 
        
        return HAGGLE_INTERNAL_ERROR;
}

int haggle_handle_get_session_id(haggle_handle_t hh)
{
	return hh->session_id;
}

typedef enum {
	INTEREST_OP_ADD,
	INTEREST_OP_REMOVE
} interest_op_t;


static int haggle_ipc_add_or_remove_interests(haggle_handle_t hh, 
					      interest_op_t op, 
					      const haggle_attrlist_t *al)
{
	struct dataobject *dobj;
	metadata_t *ctrl_m = NULL;
	list_t *pos;
	int ret;
	
	if (!hh || !al)
		return HAGGLE_PARAM_ERROR;

	if (op == INTEREST_OP_ADD)
		dobj = create_control_dataobject(hh, CTRL_TYPE_REGISTER_INTEREST, &ctrl_m);	
	else
		dobj = create_control_dataobject(hh, CTRL_TYPE_REMOVE_INTEREST, &ctrl_m);
				      
	if (!dobj)
		return HAGGLE_ALLOC_ERROR;

	list_for_each(pos, &al->attributes) {
		haggle_attr_t *a = (haggle_attr_t *)pos;
		metadata_t *interest = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST, haggle_attribute_get_value(a), ctrl_m);
		
		if (interest) {
			char weight[10];
			snprintf(weight, 10, "%lu", haggle_attribute_get_weight(a));
			metadata_set_parameter(interest, DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_NAME_PARAM, haggle_attribute_get_name(a));
			metadata_set_parameter(interest, DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_WEIGHT_PARAM, weight);
		}
		
		/* printf("Adding attribute %s:%s:%lu\n", haggle_attribute_get_name(a), haggle_attribute_get_value(a), haggle_attribute_get_weight(a)); */
	}
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);
	
	return ret;
}


int haggle_ipc_add_application_interests(haggle_handle_t hh, const haggle_attrlist_t *al)
{	
	return haggle_ipc_add_or_remove_interests(hh, INTEREST_OP_ADD, al);
}

int haggle_ipc_remove_application_interests(haggle_handle_t hh, const haggle_attrlist_t *al)
{
	return haggle_ipc_add_or_remove_interests(hh, INTEREST_OP_REMOVE, al);
}

int haggle_ipc_add_or_remove_application_interest_weighted(haggle_handle_t hh, interest_op_t op, const char *name, const char *value, const unsigned long weight)
{
	haggle_attrlist_t *al;
	int ret = 0;
	
	al = haggle_attributelist_new_from_attribute(haggle_attribute_new_weighted(name, value, weight));

	if (!al)
		return HAGGLE_INTERNAL_ERROR;

	ret =  haggle_ipc_add_or_remove_interests(hh, op, al);
	
	haggle_attributelist_free(al);
	
	return ret;
}

int haggle_ipc_add_application_interest_weighted(haggle_handle_t hh, const char *name, const char *value, const unsigned long weight)
{
	return haggle_ipc_add_or_remove_application_interest_weighted(hh, INTEREST_OP_ADD, name, value, weight);
}

int haggle_ipc_add_application_interest(haggle_handle_t hh, const char *name, const char *value)
{
	return haggle_ipc_add_or_remove_application_interest_weighted(hh, INTEREST_OP_ADD, name, value, 1);
}

int haggle_ipc_remove_application_interest(haggle_handle_t hh, const char *name, const char *value)
{
	return haggle_ipc_add_or_remove_application_interest_weighted(hh, INTEREST_OP_REMOVE, name, value, 1);
}

// SW: - new API function delete application state on deregistration
int haggle_ipc_set_delete_app_state_on_deregister(haggle_handle_t hh, int deleteOnDeReg)
{
	struct dataobject *dobj;
	metadata_t *ctrl_m = NULL;
	int ret;
	
	if (!hh)
		return HAGGLE_PARAM_ERROR;

	dobj = create_control_dataobject(hh, CTRL_TYPE_SET_PARAM, &ctrl_m);	
				      
	if (!dobj)
		return HAGGLE_ALLOC_ERROR;

	const char * shouldDelete = deleteOnDeReg ? "true" : "false";

	metadata_set_parameter(ctrl_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_DEREG_DELETE_PARAM, shouldDelete); 
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);
	
	return ret;
}

// MOS - new API function to modify the applications maximum number of data objects for a match

int haggle_ipc_set_max_data_objects_in_match(haggle_handle_t hh, 
				      unsigned long maxDataObjectsInMatch)
{
	struct dataobject *dobj;
	metadata_t *ctrl_m = NULL;
	int ret;
	
	if (!hh)
		return HAGGLE_PARAM_ERROR;

	dobj = create_control_dataobject(hh, CTRL_TYPE_SET_PARAM, &ctrl_m);	
				      
	if (!dobj)
		return HAGGLE_ALLOC_ERROR;

	char maximum[10];
	snprintf(maximum, 10, "%lu", maxDataObjectsInMatch);
	metadata_set_parameter(ctrl_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_MAX_DATAOBJECTS_IN_MATCH_PARAM, maximum); 
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);
	
	return ret;
}

// MOS - new API function to modify the applications matching threshold

int haggle_ipc_set_matching_threshold(haggle_handle_t hh, 
				      unsigned long matchingThreshold)
{
	struct dataobject *dobj;
	metadata_t *ctrl_m = NULL;
	int ret;
	
	if (!hh)
		return HAGGLE_PARAM_ERROR;

	dobj = create_control_dataobject(hh, CTRL_TYPE_SET_PARAM, &ctrl_m);	
				      
	if (!dobj)
		return HAGGLE_ALLOC_ERROR;

	char threshold[10];
	snprintf(threshold, 10, "%lu", matchingThreshold);
	metadata_set_parameter(ctrl_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_MATCHING_THRESHOLD_PARAM, threshold); 
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);
	
	return ret;
}

// CBMEN, HL - new API function to dynamically update the config
HAGGLE_API int haggle_ipc_update_configuration_dynamic(haggle_handle_t hh,
                                                       const char *config, size_t len)
{
    struct dataobject *dobj;
    metadata_t *dynamic_m = NULL;
    metadata_t *config_m = NULL;
    metadata_t *ctrl_m = NULL;
    int ret;
    
    if (!hh)
        return HAGGLE_PARAM_ERROR;

    dobj = create_control_dataobject(hh, CTRL_TYPE_DYNAMIC_CONFIGURE, &ctrl_m); 
                      
    if (!dobj)
        return HAGGLE_ALLOC_ERROR;

    dynamic_m = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_DYNAMIC_CONFIGURATION, NULL, ctrl_m);
    if (!dynamic_m)
    {
        return HAGGLE_ALLOC_ERROR;
    }

    config_m = metadata_new_from_raw(config, len);

    if (!config_m)
        return HAGGLE_PARAM_ERROR;

    if (metadata_add(dynamic_m, config_m)) {
        metadata_free(config_m);
        haggle_dataobject_free(dobj);
        return HAGGLE_ERROR;
    }

    ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
    
    haggle_dataobject_free(dobj);
    
    return ret;
}

int haggle_ipc_get_application_interests_async(haggle_handle_t hh)
{
	return haggle_ipc_generate_and_send_control_dataobject(hh, CTRL_TYPE_GET_INTERESTS);
}

int haggle_ipc_get_data_objects_async(haggle_handle_t hh)
{
	return haggle_ipc_generate_and_send_control_dataobject(hh, CTRL_TYPE_GET_DATAOBJECTS);
}

int haggle_ipc_send_node_description(haggle_handle_t hh)
{
	return haggle_ipc_generate_and_send_control_dataobject(hh, CTRL_TYPE_SEND_NODE_DESCRIPTION);
}

int haggle_ipc_shutdown(haggle_handle_t hh)
{
	return haggle_ipc_generate_and_send_control_dataobject(hh, CTRL_TYPE_SHUTDOWN);
}

int STDCALL haggle_ipc_register_event_interest(haggle_handle_t hh, const int eventId, haggle_event_handler_t handler)
{
	return haggle_ipc_register_event_interest_with_arg(hh, eventId, handler, NULL);
}

int STDCALL haggle_ipc_register_event_interest_with_arg(haggle_handle_t hh, const int eventId, haggle_event_handler_t handler, void *arg)
{
	struct dataobject *dobj;
	metadata_t *ctrl_m, *event_m;
	int ret;

	if (eventId < 0 || eventId >= _LIBHAGGLE_NUM_EVENTS || handler == NULL)
		return HAGGLE_PARAM_ERROR;
		
	if (!hh)
		return HAGGLE_PARAM_ERROR;
	
	dobj = create_control_dataobject(hh, CTRL_TYPE_REGISTER_EVENT_INTEREST, &ctrl_m);
	
	if (!dobj) {
		return HAGGLE_ALLOC_ERROR;
	}
	
	event_m = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT, NULL, ctrl_m);
	
	if (!event_m) {
		haggle_dataobject_free(dobj);
		return HAGGLE_ALLOC_ERROR;
	}
	if (metadata_set_parameter(event_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM, intTostr(eventId)) < 0) {
		haggle_dataobject_free(dobj);
		return HAGGLE_ALLOC_ERROR;
	}
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);
	
	hh->handlers[eventId].arg = arg;
	hh->handlers[eventId].handler = handler;
	hh->num_handlers++;
	
	return ret;
}

int haggle_ipc_publish_dataobject(haggle_handle_t hh, haggle_dobj_t *dobj)
{
	if (!hh) {
		libhaggle_errno = LIBHAGGLE_ERR_BAD_HANDLE;
		LIBHAGGLE_DBG("Bad handle\n");
		return HAGGLE_PARAM_ERROR;
	}

	// MOS - better make sure that create time is set
        // in this way each data object will have a unique id
	if (haggle_dataobject_get_createtime(dobj) == NULL) {
	  haggle_dataobject_set_createtime(dobj, NULL);
	}

	return haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
}

int haggle_ipc_generate_and_send_control_dataobject(haggle_handle_t hh, 
						    control_type_t type)
{
	struct dataobject *dobj;
	int ret;

	if (!hh)
		return HAGGLE_PARAM_ERROR;

	dobj = create_control_dataobject(hh, type, NULL);
	
	if (!dobj) {
		return HAGGLE_ALLOC_ERROR;
	}
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);

	return ret;
}

int haggle_ipc_send_dataobject(struct haggle_handle *hh, haggle_dobj_t *dobj, 
			       haggle_dobj_t **dobj_reply, long msecs_timeout)
{
	int ret = 0;
	struct dataobject *dobj_recv;
	unsigned char *data;
        size_t datalen;

	if (!dobj || !hh)
		return HAGGLE_PARAM_ERROR;

	/* Generate raw xml from dataobject */
        ret = haggle_dataobject_get_raw_alloc(dobj, &data, &datalen);
	
	if (ret != HAGGLE_NO_ERROR || datalen == 0) {
		LIBHAGGLE_ERR("Could not allocate raw metadata\n");
		return HAGGLE_ALLOC_ERROR;
	}
        
	ret = sendto(hh->sock, data, datalen, 0, 
		     (struct sockaddr *)&haggle_addr, sizeof(haggle_addr));

	free(data);

	if (ret < 0) {
#if defined(WIN32) || defined(WINCE)
		LIBHAGGLE_DBG("send failure %d\n", WSAGetLastError());
#else
		LIBHAGGLE_DBG("send failure %s\n", strerror(errno));
#endif
		return HAGGLE_SOCKET_ERROR;
	}

	if (msecs_timeout != IO_NO_REPLY && dobj_reply != NULL) {
		struct timeval timeout;
		struct timeval *to_ptr = NULL;
		struct sockaddr_in peer_addr;
		socklen_t addrlen = sizeof(peer_addr);
                fd_set readfds;

		if (msecs_timeout == IO_REPLY_BLOCK) {
			/* We block indefinately */
			to_ptr = NULL;
		} else if (msecs_timeout == IO_REPLY_NON_BLOCK) {
			/* Non-blocking mode, return immediately */
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
			to_ptr = &timeout;
		} else if (msecs_timeout > 0) {
			/* We block with timeout */
			timeout.tv_sec = msecs_timeout / 1000;
			timeout.tv_usec = (msecs_timeout * 1000) % 1000000;
			to_ptr = &timeout;
		} else {
			/* Bad value */
			return HAGGLE_INTERNAL_ERROR;
		}

                FD_ZERO(&readfds);
                FD_SET(hh->sock, &readfds);

                ret = select(hh->sock + 1, &readfds, NULL, NULL, to_ptr);

		if (ret < 0) {
			LIBHAGGLE_ERR("select error\n");
			return HAGGLE_SOCKET_ERROR;
		} else if (ret == 0) {
			LIBHAGGLE_ERR("Receive timeout!\n");
			return HAGGLE_TIMEOUT_ERROR;
		} 
                             
                ret = recvfrom(hh->sock, databuffer, DATA_BUFLEN, 0, 
			       (struct sockaddr *)&peer_addr, &addrlen);
                
                if (ret == SOCKET_ERROR) {
                        LIBHAGGLE_DBG("Receive error = %d\n", ret);
                        haggle_set_socket_error();
                        return HAGGLE_SOCKET_ERROR;
                }
		
		/*
		printf("Received raw data on socket: %s\n", 
			      (char *)databuffer);
		*/
                dobj_recv = haggle_dataobject_new_from_raw(databuffer, ret);
                
                if (dobj_recv == NULL) {
                        dobj_reply = NULL;
                        haggle_set_socket_error();
                        return HAGGLE_ALLOC_ERROR;
                }
                *dobj_reply = dobj_recv;
        }
        return HAGGLE_NO_ERROR;
}

int haggle_ipc_delete_data_object_by_id_bloomfilter(haggle_handle_t hh, 
						    const dataobject_id_t id, 
						    int keep_in_bloomfilter)
{
	char base64str[BASE64_LENGTH(sizeof(dataobject_id_t)) + 1];
	struct dataobject *dobj;
	metadata_t *ctrl_m, *dobj_m;
	int ret;

	if (!hh)
		return HAGGLE_PARAM_ERROR;
	
	dobj = create_control_dataobject(hh, CTRL_TYPE_DELETE_DATAOBJECT, &ctrl_m);
	
	if (!dobj) {
		return HAGGLE_ALLOC_ERROR;
	}
		
	dobj_m = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT, NULL, ctrl_m);
	
	if (!dobj_m) {
		haggle_dataobject_free(dobj);
		return HAGGLE_ALLOC_ERROR;
	}
	
	memset(base64str, '\0', sizeof(base64str));
	base64_encode((char *)id, sizeof(dataobject_id_t), base64str, sizeof(base64str));
	
	if (metadata_set_parameter(dobj_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_ID_PARAM, base64str) < 0) {
		haggle_dataobject_free(dobj);
		return HAGGLE_ALLOC_ERROR;
	}

	if (metadata_set_parameter(ctrl_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_BLOOMFILTER_PARAM, 
				   keep_in_bloomfilter ? "yes" : "no") < 0) {
		haggle_dataobject_free(dobj);
		return HAGGLE_ALLOC_ERROR;
	}
	
	ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
	
	haggle_dataobject_free(dobj);
	
	return ret;
}

int haggle_ipc_delete_data_object_by_id(haggle_handle_t hh, const dataobject_id_t id)
{
	return haggle_ipc_delete_data_object_by_id_bloomfilter(hh, id, 0);
}

int haggle_ipc_delete_data_object_bloomfilter(haggle_handle_t hh, const struct dataobject *dobj, int keep_in_bloomfilter)
{
	dataobject_id_t id;

	// Calculate the id from the data object
	haggle_dataobject_calculate_id(dobj, &id);

	return haggle_ipc_delete_data_object_by_id_bloomfilter(hh, id, keep_in_bloomfilter);
}

int haggle_ipc_delete_data_object(haggle_handle_t hh, const struct dataobject *dobj)
{
	return haggle_ipc_delete_data_object_bloomfilter(hh, dobj, 0);
}

// CBMEN, HL, Begin

static int haggle_ipc_configure_security(haggle_handle_t hh, metadata_t *params) {
    struct dataobject *dobj;
    metadata_t *ctrl_m = NULL, *config = NULL;

    int ret = 0;

    if (!hh || !params)
        return HAGGLE_PARAM_ERROR;

    dobj = create_control_dataobject(hh, CTRL_TYPE_CONFIGURE_SECURITY, &ctrl_m); 

    if (!dobj)
        return HAGGLE_ALLOC_ERROR;

    config = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION, NULL, ctrl_m);

    if (!config)
        return HAGGLE_ALLOC_ERROR;

    metadata_add(config, params);

    ret = haggle_ipc_send_dataobject(hh, dobj, NULL, IO_NO_REPLY);
    
    haggle_dataobject_free(dobj);
    
    return ret;
}

// API call to add single role
HAGGLE_API int haggle_ipc_add_role_shared_secret(haggle_handle_t hh, const char *name, const char *secret) {
    haggle_attrlist_t *al;
    int ret = 0;
    
    al = haggle_attributelist_new_from_attribute(haggle_attribute_new_weighted(name, secret, 1));

    if (!al)
        return HAGGLE_INTERNAL_ERROR;

    ret = haggle_ipc_add_role_shared_secrets(hh, al);
    
    haggle_attributelist_free(al);
    
    return ret;
}

// API call to add multiple roles
HAGGLE_API int haggle_ipc_add_role_shared_secrets(haggle_handle_t hh, const struct attributelist *al) {
    metadata_t *roles = NULL;
    list_t *pos;
    int ret;
    
    if (!hh || !al)
        return HAGGLE_PARAM_ERROR;

    roles = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLES_METADATA, NULL, NULL);
    if (!roles)
        return HAGGLE_ALLOC_ERROR;

    list_for_each(pos, &al->attributes) {
        haggle_attr_t *a = (haggle_attr_t *)pos;
        metadata_t *role = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_METADATA, NULL, roles);
        
        if (role) {
            metadata_set_parameter(role, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_NAME_PARAM, haggle_attribute_get_name(a));
            metadata_set_parameter(role, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_SECRET_PARAM, haggle_attribute_get_value(a));
        }
        
    }

    ret = haggle_ipc_configure_security(hh, roles);
    
    return ret;
}

// API call to add single node shared secret
HAGGLE_API int haggle_ipc_add_node_shared_secret(haggle_handle_t hh, const char *id, const char *secret) {
    haggle_attrlist_t *al;
    int ret = 0;
    
    al = haggle_attributelist_new_from_attribute(haggle_attribute_new_weighted(id, secret, 1));

    if (!al)
        return HAGGLE_INTERNAL_ERROR;

    ret = haggle_ipc_add_node_shared_secrets(hh, al);
    
    haggle_attributelist_free(al);
    
    return ret;
}

// API call to add multiple node shared secrets
HAGGLE_API int haggle_ipc_add_node_shared_secrets(haggle_handle_t hh, const struct attributelist *al) {
    metadata_t *nodes = NULL;
    list_t *pos;
    int ret;
    
    if (!hh || !al)
        return HAGGLE_PARAM_ERROR;

    nodes = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODES_METADATA, NULL, NULL);
    if (!nodes)
        return HAGGLE_ALLOC_ERROR;

    list_for_each(pos, &al->attributes) {
        haggle_attr_t *a = (haggle_attr_t *)pos;
        metadata_t *node = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_METADATA, NULL, nodes);
        
        if (node) {
            metadata_set_parameter(node, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_ID_PARAM, haggle_attribute_get_name(a));
            metadata_set_parameter(node, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_SECRET_PARAM, haggle_attribute_get_value(a));
        }
        
    }

    ret = haggle_ipc_configure_security(hh, nodes);
    
    return ret;
}

// API call to add single authority
HAGGLE_API int haggle_ipc_add_authority(haggle_handle_t hh, const char *name, const char *id) {
    haggle_attrlist_t *al;
    int ret = 0;
    
    al = haggle_attributelist_new_from_attribute(haggle_attribute_new_weighted(name, id, 1));

    if (!al)
        return HAGGLE_INTERNAL_ERROR;

    ret = haggle_ipc_add_authorities(hh, al);
    
    haggle_attributelist_free(al);
    
    return ret;
}

// API call to add multiple authorities
HAGGLE_API int haggle_ipc_add_authorities(haggle_handle_t hh, const struct attributelist *al) {
    metadata_t *authorities = NULL;
    list_t *pos;
    int ret;
    
    if (!hh || !al)
        return HAGGLE_PARAM_ERROR;

    authorities = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITIES_METADATA, NULL, NULL);
    if (!authorities)
        return HAGGLE_ALLOC_ERROR;

    list_for_each(pos, &al->attributes) {
        haggle_attr_t *a = (haggle_attr_t *)pos;
        metadata_t *authority = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_METADATA, NULL, authorities);
        
        if (authority) {
            metadata_set_parameter(authority, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_NAME_PARAM, haggle_attribute_get_name(a));
            metadata_set_parameter(authority, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_ID_PARAM, haggle_attribute_get_value(a));
        }
        
    }

    ret = haggle_ipc_configure_security(hh, authorities);
    
    return ret;
}

// API call to authorize single node
HAGGLE_API int haggle_ipc_authorize_node_for_certification(haggle_handle_t hh, const char *id) {
    haggle_attrlist_t *al;
    int ret = 0;
    
    al = haggle_attributelist_new_from_attribute(haggle_attribute_new_weighted(id, NULL, 1));

    if (!al)
        return HAGGLE_INTERNAL_ERROR;

    ret = haggle_ipc_authorize_nodes_for_certification(hh, al);
    
    haggle_attributelist_free(al);
    
    return ret;
}

// API call to authorize multiple nodes
HAGGLE_API int haggle_ipc_authorize_nodes_for_certification(haggle_handle_t hh, const struct attributelist *al) {
    metadata_t *authority = NULL;
    list_t *pos;
    int ret;
    
    if (!hh || !al)
        return HAGGLE_PARAM_ERROR;

    authority = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_METADATA, NULL, NULL);
    if (!authority)
        return HAGGLE_ALLOC_ERROR;

    list_for_each(pos, &al->attributes) {
        haggle_attr_t *a = (haggle_attr_t *)pos;
        metadata_t *node = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_METADATA, NULL, authority);
        
        if (node) {
            metadata_set_parameter(node, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_ID_PARAM, haggle_attribute_get_name(a));
            metadata_set_parameter(node, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_CERTIFY_PARAM, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_CERTIFY_TRUE_PARAM);
        }
        
    }

    ret = haggle_ipc_configure_security(hh, authority);
    
    return ret;
}

// API call to authorize role for attributes
HAGGLE_API int haggle_ipc_authorize_role_for_attributes(haggle_handle_t hh, const char *roleName, const struct attributelist *encryption, const struct attributelist *decryption) {
    metadata_t *authority = NULL, *role = NULL;
    list_t *pos;
    int ret;
    
    if (!hh || !encryption | !decryption)
        return HAGGLE_PARAM_ERROR;

    authority = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_METADATA, NULL, NULL);
    if (!authority)
        return HAGGLE_ALLOC_ERROR;

    role = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_METADATA, NULL, authority);
    if (!role)
        return HAGGLE_ALLOC_ERROR;

    metadata_set_parameter(role, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_NAME_PARAM, roleName);

    // We will get two entries per attribute if it is authorized for both encryption and decryption.
    // But this saves some logic as othewise we have to reconcile both lists.
    list_for_each(pos, &encryption->attributes) {
        haggle_attr_t *a = (haggle_attr_t *)pos;
        metadata_t *attr = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_METADATA, NULL, role);

        if (attr) {
            metadata_set_parameter(attr, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_NAME_PARAM, haggle_attribute_get_name(a));
            metadata_set_parameter(attr, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_ENCRYPTION_PARAM, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_AUTHORIZED_PARAM);
        }
    }

    list_for_each(pos, &decryption->attributes) {
        haggle_attr_t *a = (haggle_attr_t *)pos;
        metadata_t *attr = metadata_new(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_METADATA, NULL, role);

        if (attr) {
            metadata_set_parameter(attr, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_NAME_PARAM, haggle_attribute_get_name(a));
            metadata_set_parameter(attr, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_DECRYPTION_PARAM, DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_AUTHORIZED_PARAM);
        }
    }

    ret = haggle_ipc_configure_security(hh, authority);
    
    return ret;
}

// CBMEN, HL, End

int haggle_event_loop_is_running(haggle_handle_t hh)
{
        return (hh ? hh->event_loop_running : HAGGLE_HANDLE_ERROR);
}

int is_event_loop_thread(haggle_handle_t hh)
{
#if defined(OS_WINDOWS)
	return GetCurrentThreadId() == hh->th_id;
#else
	return pthread_equal(hh->th, pthread_self()) != 0;
#endif
}

int haggle_event_loop_stop(haggle_handle_t hh)
{
	LIBHAGGLE_DBG("Stopping event loop\n");

	if (!hh) {
		LIBHAGGLE_DBG("Bad haggle handle\n");
		libhaggle_errno = LIBHAGGLE_ERR_BAD_HANDLE;
		return HAGGLE_PARAM_ERROR;
	}
	if (!hh->event_loop_running) {
		LIBHAGGLE_DBG("Event loop not running\n");
		return HAGGLE_EVENT_LOOP_ERROR;
	}
                
        event_loop_signal_raise(hh);
	
	if (is_event_loop_thread(hh)) {
		/* We are the event loop thread. Exit the loop by
		 * setting event_loop_running to 0 */ 
		hh->event_loop_running = 0;
	} else {
		/* We are not the event loop thread. Exit by 
		   signalling the other thread to exit in case
		   it is waiting.
		*/
#if defined(OS_WINDOWS)
		if (hh->th) {
			WaitForSingleObject(hh->th, INFINITE);
			/* Is this really necessary here or will the handle be closed by the event loop? */
			CloseHandle(hh->th);
			hh->th = NULL;
		}
#elif defined(OS_UNIX)
		if (hh->th) {
			LIBHAGGLE_DBG("Joining with event loop thread...\n");
			pthread_join(hh->th, NULL);
			hh->th = 0;
		}
#endif
	} 

	LIBHAGGLE_DBG("Event loop successfully stopped\n");

	return HAGGLE_NO_ERROR;
}

/*
 handle_event:
 Returns less than 1 on error, or 0 or 1 on success.
 If the return value is -1 or 0 the data object should be freed, if the value is 1 or greater it should not be freed.
*/
static int handle_event(struct haggle_handle *hh, haggle_event_type_t type, struct dataobject *dobj, metadata_t *app_m, metadata_t *event_m)
{
	haggle_event_t e;
	int ret = 0;
	
	LIBHAGGLE_DBG("Handling event type %u\n", type);

	if (!event_m)
		return -1;

	if (!hh->handlers[type].handler)
		return 0;
	
	e.type = type;
	
	switch (type) {
		case LIBHAGGLE_EVENT_INTEREST_LIST:
			e.interests = haggle_attributelist_new();
			
			if (e.interests) {
				metadata_t *m = metadata_get(event_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST);
				
				while (m) {
					const char *name = metadata_get_parameter(m, DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_NAME_PARAM);
					const char *weight_str = metadata_get_parameter(m, DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_WEIGHT_PARAM);
					const unsigned long weight = weight_str ? strtoul(weight_str, NULL, 10) : 1;
					struct attribute *a = haggle_attribute_new_weighted(name, metadata_get_content(m), weight);
					haggle_attributelist_add_attribute(e.interests, a);
					m = metadata_get_next(event_m);
				}
					
				ret = hh->handlers[type].handler(&e, hh->handlers[type].arg);
                                
				if (ret != 1 && e.interests)
					haggle_attributelist_free(e.interests);
                                
				ret = 0;
			} else {
				ret = -1;
			}
			break;
		case LIBHAGGLE_EVENT_NEW_DATAOBJECT:
			e.dobj = dobj;
			/* Remove the application metadata */
			if (app_m)
				metadata_free(app_m);

			ret = hh->handlers[type].handler(&e, hh->handlers[type].arg);
			break;
		case LIBHAGGLE_EVENT_NEIGHBOR_UPDATE:
			e.neighbors = haggle_nodelist_new_from_metadata(event_m);
			
			if (e.neighbors) {
				
				ret = hh->handlers[type].handler(&e, hh->handlers[type].arg);
				
				if (ret != 1 && e.neighbors)
					haggle_nodelist_free(e.neighbors);

				ret = 0;
			} else {
				ret = -1;
			}
			break;
		case LIBHAGGLE_EVENT_SHUTDOWN:
			// TODO: Reason not implemented yet.
			e.shutdown_reason = 0;
			hh->handlers[type].handler(&e, hh->handlers[type].arg);
			
			ret = 0;
			break;
		default:
			break;
	}
		
	return ret;
}

start_ret_t haggle_event_loop(void *arg)
{
	struct haggle_handle *hh = (struct haggle_handle *)arg;
	unsigned char *eventbuffer;
	int ret;
	unsigned int error_retries = 0;

	eventbuffer = (unsigned char *)malloc(EVENT_BUFLEN);

	if (!eventbuffer) {
		LIBHAGGLE_ERR("Could not allocated event buffer\n");
		goto done;
	}

	hh->event_loop_running = 1;

	if (hh->start) {
                hh->start(hh->arg);
	}

	while (hh->event_loop_running) {
		struct dataobject *dobj;
		metadata_t *app_m, *ctrl_m, *event_m;
		const char *event_type_str;
		int event_type;
		
		LIBHAGGLE_DBG("Event loop running, waiting for data object...\n");

		memset(eventbuffer, 0, EVENT_BUFLEN);
       
		ret = wait_for_event(hh, NULL);

		if (ret == EVENT_LOOP_ERROR) {
			LIBHAGGLE_ERR("Haggle event loop wait error!\n");
                        // Break or try again?

			if (error_retries++ == 4) {
				hh->event_loop_running = 0;
			}
			continue;
		} else if (ret == EVENT_LOOP_TIMEOUT) {
			LIBHAGGLE_DBG("Event loop timeout!\n");
			break;
                } else if (ret == EVENT_LOOP_SHOULD_EXIT) {
			LIBHAGGLE_DBG("Event loop should exit!\n");
                        event_loop_signal_lower(hh);
			hh->event_loop_running = 0;
			break;
                } else if (ret == EVENT_LOOP_SOCKET_READABLE)  {
                       
                        ret = recv(hh->sock, eventbuffer, EVENT_BUFLEN, 0);
                        
                        if (ret == SOCKET_ERROR) {
                                LIBHAGGLE_ERR("Haggle event loop recv() error!\n");

				if (error_retries++ == 4) {
					hh->event_loop_running = 0;
				}
                                continue;
                        }
                        
                        dobj = haggle_dataobject_new_from_raw(eventbuffer, ret);
                        
                        if (!dobj) {
                                LIBHAGGLE_ERR("Haggle event loop ERROR: could not create data object\n");
                                continue;
                        }
                       
			LIBHAGGLE_DBG("Received data object\n%s\n", (char *)eventbuffer);
			
			app_m = haggle_dataobject_get_metadata(dobj, DATAOBJECT_METADATA_APPLICATION);
			
			if (!app_m) {
				LIBHAGGLE_ERR("Data object contains no valid application metadata!\n");
                                haggle_dataobject_free(dobj);
                                continue;
			}
			
			if (strcmp(metadata_get_parameter(app_m, DATAOBJECT_METADATA_APPLICATION_NAME_PARAM), hh->name) != 0 &&
			    strcmp(metadata_get_parameter(app_m, DATAOBJECT_METADATA_APPLICATION_NAME_PARAM), "All Applications") != 0) {
				LIBHAGGLE_DBG("Data object is not for application %s\n", hh->name);
				haggle_dataobject_free(dobj);
				continue;
			}
				       
			ctrl_m = metadata_get(app_m, DATAOBJECT_METADATA_APPLICATION_CONTROL);
			
                        if (!ctrl_m) {
                                LIBHAGGLE_ERR("Data object contains no control information!\n");
                                haggle_dataobject_free(dobj);
                                continue;			
                        }
			
			if (strcmp(metadata_get_parameter(ctrl_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM), ctrl_type_names[CTRL_TYPE_EVENT]) != 0) {
                                LIBHAGGLE_ERR("Data object has no control type!\n");
                                haggle_dataobject_free(dobj);
                                continue;			
			}
			
			event_m = metadata_get(ctrl_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
			
			if (!event_m) {
				LIBHAGGLE_ERR("Data object has no event information!\n");
                                haggle_dataobject_free(dobj);
                                continue;
			}
			
			event_type_str = metadata_get_parameter(event_m, DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM);
			
			if (!event_type_str) {
				LIBHAGGLE_ERR("Data object has no event type!\n");
                                haggle_dataobject_free(dobj);
                                continue;
			}
                                               
                        event_type = atoi(event_type_str);
                        			
			if (handle_event(hh, event_type, 
					 dobj, app_m, event_m) <= 0) {
				haggle_dataobject_free(dobj);
			}
		}
		error_retries = 0;
	}

        if (hh->stop)
                hh->stop(hh->arg);

	free(eventbuffer);
done:
	if (hh->handle_free_final) 
		haggle_handle_free_final(hh);

#if defined(WIN32) || defined(WINCE)
	return 0;
#else
	return NULL;
#endif
}

/* A blocking event loop. Application does threading if necessary. */
int haggle_event_loop_run(haggle_handle_t hh)
{
#if defined(OS_LINUX) || defined(OS_MACOSX)
	/* pthread_attr_t attr; */
#endif
	if (!hh) {
		libhaggle_errno = LIBHAGGLE_ERR_BAD_HANDLE;
		return HAGGLE_PARAM_ERROR;
	}

	if (hh->num_handlers == 0)
		return HAGGLE_EVENT_HANDLER_ERROR;

	if (hh->event_loop_running)
		return HAGGLE_EVENT_LOOP_ERROR;

	haggle_event_loop(hh);

	return HAGGLE_NO_ERROR;
}

/* An asynchronous event loop. The application does not have to care
 * about threading */
int haggle_event_loop_run_async(haggle_handle_t hh)
{
	int ret = 0;

	if (!hh) {
		libhaggle_errno = LIBHAGGLE_ERR_BAD_HANDLE;
		return HAGGLE_PARAM_ERROR;
	}

	if (hh->num_handlers == 0)
		return HAGGLE_EVENT_HANDLER_ERROR;

	if (hh->event_loop_running)
		return HAGGLE_EVENT_LOOP_ERROR;

#if defined(WIN32) || defined(WINCE)
	hh->th = CreateThread(NULL,  0, haggle_event_loop, (void *)hh, 0, &hh->th_id);

	if (hh->th == NULL)
		return HAGGLE_INTERNAL_ERROR;
#else
	ret = pthread_create(&hh->th, NULL, haggle_event_loop, (void *)hh);

	if (ret != 0)
		return HAGGLE_INTERNAL_ERROR;
#endif
	return HAGGLE_NO_ERROR;
}

int haggle_event_loop_register_callbacks(haggle_handle_t hh, haggle_event_loop_start_t start, haggle_event_loop_stop_t stop, void *arg)
{
        if (!hh || (!start && !stop))
                return -1;
        
        if (hh->start) {
                LIBHAGGLE_ERR("Start callback already set\n");
                return -1;
        }
        if (hh->stop) {
                LIBHAGGLE_ERR("Stop callback already set\n");
                return -1;
        }

        hh->start = start;
        hh->stop = stop;
        hh->arg = arg;

        return HAGGLE_NO_ERROR;
}
