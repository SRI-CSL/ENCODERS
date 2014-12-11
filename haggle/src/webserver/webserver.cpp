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
*  webserver.cpp
*
*  Created by Christian Rohner on 2008-02-18.
*  Copyright 2008 Haggle. All rights reserved.
*
*/


// usage: webserver -s path_to_server -d path_to_content
//	path_to_server: location of index.html and related files
//	path_to_content: location from where content can be loaded (todo: read from dataobject)
// http://localhost:8081 to enter application interest
// results load dynamically in firefox and safari (using javascript)

#include <libhaggle/haggle.h>

#if defined(OS_UNIX)

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include <cerrno>
#include "regex.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <strings.h>

#define ERRNO errno

#elif defined(OS_WINDOWS)

#include <windows.h>
#include <time.h>
#include <winioctl.h>
#include <winerror.h>
#include <iphlpapi.h>

#define ERRNO WSAGetLastError()
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

// ---
#define NUM_CONNECTIONS 1
#define HTTP_PORT 8081

using namespace std;

string filepath = "/";
string serverpath;

SOCKET httpListenSock;
SOCKET httpCommSock;
haggle_handle_t haggleHandle;

// This mutex protects the ResultString and NeighborString
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
static void mutex_lock(mutex_t *m)
{

#ifdef OS_WINDOWS
	EnterCriticalSection(m);
#elif defined(OS_UNIX)
	pthread_mutex_lock(m);
#endif
}

static void mutex_unlock(mutex_t *m)
{

#ifdef OS_WINDOWS
	LeaveCriticalSection(m);
#elif defined(OS_UNIX)
	pthread_mutex_unlock(m);
#endif
}

stringstream ResultString;
stringstream NeighborString;
stringstream DebugString;

int countDataObject = 0;

string decodeURL(string* str) {
	stringstream result;
	string::iterator it;
	for( it = str->begin(); it != str->end(); it++ ) {
		if (*it != '%') {
			result << *it;
		} else {
			char x1 = *(++it);
			char x2 = *(++it);
			// FIXME: check if this is correct  (this is a quick hack to get the %20 work)
			unsigned int x = ((x1-48)<<4) + (x2-48);
			result << (char)x;
		}
	}
	return result.str();

}


int onInterestList(haggle_event_t *e, void *arg)
{
        list_t *pos;
        
        list_for_each(pos, &e->interests->attributes) {
                haggle_attr_t *a = (haggle_attr_t *)pos; 
                printf("interest: %s=%s\n", haggle_attribute_get_name(a), haggle_attribute_get_value(a));
        }

        return 0;
}

int onDataObject(haggle_event_t *e, void *arg)
{
	cerr << endl << "Received data object!!!" << endl;
	const char *filepath = haggle_dataobject_get_filepath(e->dobj);

	if (!filepath || strlen(filepath) == 0) {
		cerr << endl <<  "--- No filepath in dataobject" << endl;
		return -1;		
	}
	cerr << "Filepath is \'" << filepath << "\'" << endl;

	struct attribute *attr = NULL;
	int cnt = 0;
	
	mutex_lock(&mutex);

	if (countDataObject % 3 == 0) {
		ResultString << "<br/><br/>";
	}
	countDataObject++;

//	ResultString << "<a style=\"padding-right:20px; border:0 solid white; outline:0 solid white;\" href=\"http://localhost:8081/" << filepath << "\" target=_new><img width=300 src=\"http://localhost:8081/" << filepath << "\" title=\""; 
	ResultString << "<a style=\"padding-right:20px; border:0 solid white; outline:0 solid white;\" href=\"" << filepath << "\" target=_new><img width=300 src=\"" << filepath << "\" title=\""; 
	while ((attr = haggle_dataobject_get_attribute_n(e->dobj, cnt++))) {
		ResultString << haggle_attribute_get_name(attr) << "=" << haggle_attribute_get_value(attr) << " \r\n";
	}
	ResultString << "\" /></a>";

	mutex_unlock(&mutex);
        
        return 0;
}


int onNeighborUpdate(haggle_event_t *e, void *arg)
{
	list_t *pos;

	printf("Neighbor update event!\n");
	
	mutex_lock(&mutex);
	
	NeighborString.str(""); 

	list_for_each(pos, &e->neighbors->nodes) {
		list_t *ppos;
		haggle_node_t *node = (haggle_node_t *)pos;

		printf("Neighbor: %s\n", haggle_node_get_name(node));
		
		NeighborString << "[" << haggle_node_get_name(node) << "] ";

		list_for_each(ppos, &node->interfaces) {
			haggle_interface_t *iface = (haggle_interface_t *)ppos;
			printf("\t%s : [%s] %s\n", haggle_interface_get_type_name(iface), 
			       haggle_interface_get_identifier_str(iface),
			       haggle_interface_get_status_name(iface)
			);
		}
	}
	
	mutex_unlock(&mutex);

        return 0;
}

// -----

void http(SOCKET sock, char* buf, int num)
{
	stringstream Request;
	stringstream Response;
	stringstream Query;
	bool readQuery = false;

	DebugString.clear();

	if (sock == httpListenSock) {
		// cout << "got request: " << endl;
	} else if (sock == httpCommSock) {
		Request << buf;
		
#define MAX_LENGTH 256

		char line[MAX_LENGTH];

		while (Request.getline(line, MAX_LENGTH, '\r')) {
			if (readQuery == true) {
				/* reading the post parameters inserted by the <form> in index.html */ 
				/* q=[value]&meta=[name] */
				char *begin = NULL; 
				char *end = NULL; 
				char *name = NULL;
				char *value = NULL;
				begin = strstr(line, "=");
				if (begin) {
					end = strstr(begin, "&");
					*end = '\0';
					value = begin+1;
				}
				if (end) {
					begin = strstr(end+1, "=");
					if (begin) {
						name = begin+1;
					}

					if (!strcmp(name, "advanced")) {
						cout << value << endl;
						name = value;
						if ((end = strstr(value, "%3D"))) {
							*end = '\0';
							value = end+3;
						}
					}
					printf("Adding interest: %s=%s\n", name, value);
					haggle_ipc_add_application_interest(haggleHandle, name, value);
					mutex_lock(&mutex);
					ResultString.clear();
					mutex_unlock(&mutex);
				}

				readQuery = false;
			}
			if (strlen(line) < 2) {
				readQuery = true;
			}
			if (strstr(line, "GET") || strstr(line, "POST")) {
				stringstream urlpath;

				/* we have to return the file requested in the url.
				   special cases: result.html and neighbour.html are dynamically generated */
				
				if (strstr(line, " /result.html HTTP")) {
					/* request for result.html > generate from ResultString */
					mutex_lock(&mutex);
					Response << "HTTP/1.1 200 OK\r\n" << "Cache-control: public\r\nContent-Length: " << ResultString.str().length() << "\r\n\r\n" << ResultString.str();
					mutex_unlock(&mutex);
					send(httpCommSock, Response.str().c_str(), Response.str().length(), 0);
				} else if (strstr(line, " /neighbor.html HTTP")) {
					/* request for neighbor.html > generate from NeighborString */
					mutex_lock(&mutex);
					Response << "HTTP/1.1 200 OK\r\n" << "Content-Length: " << NeighborString.str().length() << "\r\n\r\n" << NeighborString.str();
					mutex_unlock(&mutex);
					send(httpCommSock, Response.str().c_str(), Response.str().length(), 0);
				} else {
					if (strstr(line, " / HTTP")) {
						/* '/' means index.html */
						urlpath  << "index.html";
					} else {
						/* extract urlpath (including '/') */
						char *begin = strstr(line, " /") + 1;
						char *end = strstr(line, " HTTP");
						*end = '\0';

						// if there is only a beginning slash, it is a file in the webserver catalogue. 
						cout << "--------------" << begin << "--------------" << endl;
						if (!strstr(begin+1, "/")) begin++;
						
						urlpath << begin;
					}
					string tmp = urlpath.str();
					string filename = decodeURL(&tmp);
					cout << filename << endl;

					ifstream FileStream(filename.c_str(), ios::binary);
					FileStream.seekg (0, ios::end);
					int filesize = FileStream.tellg();

					stringstream HTTPparams;
					HTTPparams.clear();
					if (strstr(line, "jpg")) {
						HTTPparams << "Content-Type: image/jpeg\r\n";
						HTTPparams << "Accept-Ranges: bytes\r\n";
					}
					
					Response << "HTTP/1.1 200 OK\r\n" << "Connection: Keep-Alive\r\n" << HTTPparams.str() << "Content-Length: " << filesize << "\r\n\r\n";
					cout << Response.str() << endl;
					send(httpCommSock, Response.str().c_str(), Response.str().length(), 0);
					FileStream.close();

					if (filesize > 0) {
						FILE *f = fopen(filename.c_str(), "r");

						if (!f)  {
							cout << "fopen error" << endl;
							break;
						}

#define MAX_SIZE 1400
						char buf[MAX_SIZE];
						int sent, tot_sent = 0;
						char *p;

						while(true) {
							size_t len = MAX_SIZE;

							size_t fstart = ftell(f);
							size_t nitems = fread(buf, MAX_SIZE, 1, f);

							if (nitems != 1) {
								if (feof(f) != 0) {
									// This is ok, but we didn't read the whole buffer
									// so we need to set the length
									len = ftell(f) - fstart;
								} else if (ferror(f) != 0) {
									fprintf(stderr, "File read error?\n");
									break;
								} else {
									fprintf(stderr, "Unkown fread state\n");
									break;
								}
							}
							
							if (len == 0)
								break;

							//printf("read %d bytes data\n", len);

							p = buf;

							do {
								sent = send(httpCommSock, p, len, 0);

								if (sent < 0) {
									if (
#if defined(OS_WINDOWS_MOBILE)
										ERRNO == EAGAIN || 
#endif
										ERRNO == EWOULDBLOCK) {
										continue;
									}
									fprintf(stderr, "Send error\n");
									goto send_done;
								}
								len -= sent;
								p += sent;
								tot_sent += sent;
								//printf("sent %d bytes data\n", sent);

							} while (len);
						}
send_done:
						printf("Total bytes sent %d\n", tot_sent);
						fclose(f);	
					}

				}
			}
		};

		CLOSE_SOCKET(httpCommSock);
		httpCommSock = 0;
	}
}


// ----- Networking functions -----------------------------------

int openTcpSock(int port) 
{
	SOCKET s;
	int reuse_addr = 1;  /* Used so we can re-bind to our port
			     while a previous connection is still
			     in TIME_WAIT state. */
	struct sockaddr_in server_address; /* bind info structure */
	struct protoent* tcp_level;


	/* Obtain a file descriptor for our "listening" socket */
	s = socket(AF_INET, SOCK_STREAM, 0);

	if (!s) {
		fprintf(stderr, "socket error\n");
		return -1;
	}
	/* So that we can re-bind to it without TIME_WAIT problems */
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_addr, sizeof(reuse_addr));

	/* set tcp_nodelay */
	if ((tcp_level = getprotobyname("TCP"))) {
		setsockopt(s, tcp_level->p_proto, TCP_NODELAY, (char *)&reuse_addr, sizeof(reuse_addr));
	}

	/* Get the address information, and bind it to the socket */
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	if (bind(s, (struct sockaddr *) &server_address, sizeof(server_address)) != 0) {
		fprintf(stderr, "bind error : %d\n", ERRNO);
		CLOSE_SOCKET(s);
		return -1;
	}

	/* Set up queue for incoming connections. */
	listen(s,1);

	return s;
}


void build_select_list(unsigned int* nfds, fd_set* readfds) {

	if (*nfds < (unsigned int)httpListenSock + 1) 
		*nfds = httpListenSock + 1;

	FD_SET(httpListenSock, readfds);

	if (httpCommSock != 0) {
		if (*nfds < (unsigned int)httpCommSock + 1) { *nfds = (unsigned int)httpCommSock + 1; }
		FD_SET(httpCommSock, readfds);
	}
}


int handle_new_connection(SOCKET sock) {
	int connection;		/* Socket file descriptor for incoming connections */
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	connection = accept(sock, (struct sockaddr*)&addr, &len);

	if (connection < 0) {
		fprintf(stderr, "accept error : %d\n", ERRNO);
		return connection;
	}
	if (sock == httpListenSock && httpCommSock == 0) {
		httpCommSock = connection;
		http(sock, NULL, 0);
		connection = -1;
	}
	if (connection != -1) {
		/* No room left in the queue! */
		CLOSE_SOCKET(connection);
		return connection;
	}
	return 0;
}


void deal_with_data(SOCKET sock) {
#define BUFLEN 1024
	static char buf[BUFLEN];     /* Buffer for socket reads */
	//char *msg = 0;
	int num = 0;

	if ((num = recv(sock, buf, BUFLEN, 0)) == 0) {
		printf("Connection lost\n");
		CLOSE_SOCKET(sock);
		sock = 0;
	} else {
		/* We got some data, so we answer to it */
		buf[num] = 0x00;
		//printf("\n[%d]: %s", listnum, buf);
		//printf("\n[%d]: ");
		http(sock, buf, num);
		//fflush(stdout);
	}
}

void read_socks(fd_set* sock) {
	/* test for socket */
	if (FD_ISSET(httpListenSock, sock)) {
		handle_new_connection(httpListenSock);
	}
	/* Now check connectlist for available data */
	if (FD_ISSET(httpCommSock, sock)) {
		deal_with_data(httpCommSock);
	}
}

void closeConnections(int i) {
	printf("\nshutting down...\n");
	
	if (haggleHandle)
		haggle_handle_free(haggleHandle);

	if (httpCommSock) 
		CLOSE_SOCKET(httpCommSock);
	if (httpListenSock) 
		CLOSE_SOCKET(httpListenSock);

	exit(0);
}

void eventLoop() {
	fd_set readfds;
	int result = 0;
	struct timeval timeout;
	static volatile int StopNow = 0;

	while (!StopNow) {
		unsigned int nfds = 0;
		timeout.tv_sec = 60;
		timeout.tv_usec = 0;

		FD_ZERO(&readfds);
		// mDNSPosixGetFDSet(&mDNSStorage, &nfds, &readfds, &timeout);
		build_select_list(&nfds, &readfds);

		result = select(nfds, &readfds, NULL, NULL, &timeout);
		if (result < 0) {
			if (ERRNO != EINTR) StopNow = 1;
		} 
		if (result > 0) {
			// mDNSPosixProcessFDSet(&mDNSStorage, &readfds);
			read_socks(&readfds);
		}
	}
}

#if defined(OS_WINDOWS_MOBILE)
int wmain()
{
#elif defined(OS_WINDOWS_DESKTOP)
int main()
{
#else
int main (int argc, char *argv[]) 
{
	string serverpath = argv[0];  // default filepath is the current directory

	serverpath = serverpath.substr(0, serverpath.find_last_of("/\\"));

	while (argc) {
		if (strcmp(argv[0], "-d") == 0) {
			if (!argv[1]) {
				fprintf(stderr, "Bad argument\n");
				return -1;
			}
			filepath = argv[1];
			argc--;
			argv++;
			printf("File path is %s\n", filepath.c_str());
		}
		if (strcmp(argv[0], "-s") == 0) {
			if (!argv[1]) {
				fprintf(stderr, "Bad argument\n");
				return -1;
			}
			serverpath = argv[1];
			argc--;
			argv++;
			printf("Server path is %s\n", serverpath.c_str());
		}

		argc--;
		argv++;		       
	}

	signal(SIGINT,  closeConnections);      // SIGINT is what you get for a Ctrl-C
#endif
	printf("serverpath=%s\n", serverpath.c_str());
	printf("Serving Haggle page on port %d (e.g., http://localhost:%d).\n", HTTP_PORT, HTTP_PORT);

	mutex_init(&mutex);

        int ret = haggle_daemon_pid(NULL) ;

        if (ret == HAGGLE_ERROR)
                goto done;

	if (ret == 0) {
		printf("Trying to spawn daemon\n");
		haggle_daemon_spawn(NULL);
	}
	// libhaggle will initialize winsock for us
	if (haggle_handle_get("Haggle Webserver", &haggleHandle) != HAGGLE_NO_ERROR)
		goto done;

	haggle_ipc_register_event_interest(haggleHandle, LIBHAGGLE_EVENT_INTEREST_LIST, onInterestList);
	haggle_ipc_register_event_interest(haggleHandle, LIBHAGGLE_EVENT_NEW_DATAOBJECT, onDataObject);
	haggle_ipc_register_event_interest(haggleHandle, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, onNeighborUpdate);

        haggle_ipc_get_application_interests_async(haggleHandle);

	httpListenSock = openTcpSock(HTTP_PORT);

	if (!httpListenSock)
		goto done;

	haggle_event_loop_run_async(haggleHandle);

	httpCommSock = 0;

	eventLoop();

done:
	mutex_del(&mutex);

	closeConnections(0);

	return 1;
}





