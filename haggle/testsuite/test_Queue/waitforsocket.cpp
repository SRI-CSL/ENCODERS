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
#include <libcpphaggle/Platform.h>
#include "testhlp.h"
#include <libcpphaggle/GenericQueue.h>
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;
/*
	This program tests the function Queue::retrieve_within_socket.
*/

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#endif
#if defined(OS_MACOSX)
#include <net/route.h>
#include <net/if_dl.h>
#include <sys/kern_event.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#elif defined(OS_LINUX)
#include <unistd.h>
#include <net/ethernet.h>
#include <string.h>
#elif defined(OS_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <winioctl.h>
#include <winerror.h>
#include <iphlpapi.h>
#endif

static GenericQueue<long> *myQueue;
static int	recv_sock;
static int	send_sock;
static bool pass_1, pass_2, pass_3;
static Condition myCondition;
Mutex	myMutex;	

class waitforsocketRunnable : public Runnable {
public:
	waitforsocketRunnable() {}
	~waitforsocketRunnable() {}

	bool run()
	{
		long elem;
		QueueEvent_t qev;
	
		Timeval t(5);

		if (myQueue->retrieve(&elem, recv_sock, &t) == QUEUE_ELEMENT)
		{
			pass_1 = true;
		}

		// Make sure the other thread is on the condition
		myCondition.signal();
		
		qev = myQueue->retrieve(&elem, recv_sock, &t);
		
		if (qev == QUEUE_WATCH_READ)
		{
			char	c;
			
			pass_2 = true;
			// Receive network packet:
			recv(recv_sock, &c, 1, 0);
		} else if (qev == QUEUE_ELEMENT)
		{
		}

		myCondition.signal();
		
		qev = myQueue->retrieve(&elem, recv_sock, &t);

		if (qev == QUEUE_TIMEOUT)
		{
			pass_3 = true;
		} else if (qev == QUEUE_ELEMENT)
		{
		}

		myCondition.signal();
		
#if defined(OS_WINDOWS)
		// Wait for the testsuite to finish before terminating this thread:
		// This removes garbage output in the middle of the testsuite results
		milli_sleep(100000);
#endif
		return false;
	}
	void cleanup() { } 
};

#if defined(OS_WINDOWS)
int haggle_test_waitforsocket(void)
#else
int main(int argc, char *argv[])
#endif
{
	waitforsocketRunnable *thr = NULL;

	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Socket waiting test: ");
	try{
		struct sockaddr_in my_addr;
		
		recv_sock = -1;
		send_sock = -1;
		
		print_over_test_str(1, "Opened sockets: ");
		memset(&my_addr, 0, sizeof(struct sockaddr_in));
		// Set up send socket:
		my_addr.sin_family = AF_INET;
		my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		my_addr.sin_port = htons(65286);
		
		// Open a UDP socket:
		send_sock = socket(AF_INET, SOCK_DGRAM, 0);
		if(send_sock == -1)
		{
			print_pass(false);
			goto fail;
		}
		// Bind the socket to the address:
		if(bind(send_sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		{
			print_pass(false);
			goto fail;
		}

		// Set up receive socket:
		my_addr.sin_family = AF_INET;
		my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		my_addr.sin_port = htons(65285);
		
		// Open a UDP socket:
		recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
		if(recv_sock == -1)
		{
			print_pass(false);
			goto fail;
		}
		// Bind the socket to the address:
		if(bind(recv_sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		{
			print_pass(false);
			goto fail;
		}
#if defined(OS_WINDOWS)
		my_addr.sin_addr.s_addr = htonl(0x7F000001);
#endif

		print_pass(true);
		
		myQueue = new GenericQueue<long>();

		pass_1 = false;
		pass_2 = false;
		pass_3 = false;

		myMutex.lock();

		// Star the thread:
		thr = new waitforsocketRunnable();
		thr->start();
		
		// Wait for the thread to be waiting on the queue:
		milli_sleep(2000);
		
		// Insert a data object:
		myQueue->insert(0);
	
		// Wait for the thread to be waiting on the queue:
		myCondition.timedWaitSeconds(&myMutex, 5);

		// Send a packet:
		sendto(send_sock, 
			"1", 
			1, 
			0, 
			(struct sockaddr *)&my_addr, 
			sizeof(my_addr));
		
		// Wait for the thread to finish:
		myCondition.timedWaitSeconds(&myMutex, 5);

		myCondition.timedWaitSeconds(&myMutex, 5);

		myMutex.unlock();

		print_over_test_str(1,"Got data object: ");
		print_pass(pass_1);
		print_over_test_str(1,"Got packet: ");
		print_pass(pass_2);
		print_over_test_str(1,"Timed out: ");
		print_pass(pass_3);
fail:
		print_over_test_str(1,"Total: ");
		
		if (send_sock != -1)
			CLOSE_SOCKET(send_sock);
		if (recv_sock != -1)
			CLOSE_SOCKET(recv_sock);
		
		return (pass_1&&pass_2&&pass_3?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
