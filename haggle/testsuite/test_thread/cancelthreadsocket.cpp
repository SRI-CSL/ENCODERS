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
#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Platform.h>
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;
/*
	This program tests if it is possible to cancel a thread while doing sockets
	stuff.
	
	It first creates the thread, then starts it, waits for a second to allow it
	to start executing, then cancels it, then waits another 3 seconds. The 
	thread is supposed to wait three seconds and then set the success flag to 
	false, so after one second it is still in the call to sleep. If the cancel 
	function is successful, success is never set to 0.
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


#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Watch.h>

static volatile bool started, selected, success, ran_cleanup;

class cancelthreadsocketRunnable : public Runnable {
	SOCKET sock;
public:
	cancelthreadsocketRunnable() {}
	~cancelthreadsocketRunnable() {}
	
	bool run()
	{
		struct sockaddr_in my_addr;
		Timeval timeout(3);
		Watch w;
		
		success = true;
		
		memset(&my_addr, 0, sizeof(struct sockaddr_in));
		// Set up local port:
		my_addr.sin_family = AF_INET;
		my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		my_addr.sin_port = htons(65285);
		
		// Open a UDP socket:
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		// Bind the socket to the address:
		if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		{
			goto fail;
		}

		w.add(sock);

		started = true;
		selected = true;

		w.wait(&timeout);
fail:
		success = false;
		started = true;

		return false;
	}
	void cleanup()
	{
		CLOSE_SOCKET(sock);
		ran_cleanup = true;
	}
};

#if defined(OS_WINDOWS)
int haggle_test_cancelthreadsocket(void)
#else
int main(int argc, char *argv[])
#endif
{
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Sockets cancel test: ");
	try{
	cancelthreadsocketRunnable	*thr;
	selected = false;
	started = false;
	success = false;
	ran_cleanup = false;
	selected = false;
	started = false;
	success = false;
	ran_cleanup = false;
	
	thr = new cancelthreadsocketRunnable();
	thr->start();
	// Wait for the thread to start: 
	// (Can't use a mutex - they haven't been tested yet.)
	do{
		milli_sleep(1000);
	}while(!started);
	// The thread should now be sleeping, cancel it:
	thr->cancel();
	// Wait for longer than it would take the thread to end normally:
	milli_sleep(5*1000);
	print_over_test_str(1, "Ran select(): ");
	print_pass(selected);
	print_over_test_str(1, "Thread cancelled: ");
	print_pass(success);
	print_over_test_str(1, "Thread ran cleanup: ");
	print_pass(ran_cleanup);
	print_over_test_str(1, "Total: ");
	
	return ((started&&success&&ran_cleanup)?0:1);
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
