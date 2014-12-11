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

#include <libhaggle/haggle.h>

#include "smtpserver.h"
#include "thread.h"

#include <string.h>
#include <signal.h>

static mutex_t	shutdown_mutex;

// Signal handler: shuts down the program nicely upon Ctrl-C, rather than
// letting the system crash with open network ports, etc.
static void signal_handler(int signal)
{
	switch(signal)
	{
#if defined(OS_UNIX)
		case SIGKILL:
		case SIGHUP:
#endif
		case SIGINT: // Not supported by OS_WINDOWS?
		case SIGTERM:
			// Tell the main thread it's time to shut down:
			mutex_unlock(shutdown_mutex);
		break;
		
		default:
		break;
	}
}

/*
	This is called when haggle tells us it's shutting down
*/
static int onDataShutdown(haggle_event_t *e, void *arg)
{
	// Tell the main thread it's time to shut down:
	mutex_unlock(shutdown_mutex);

        return 0;
}

int main(int argc, char *argv[])
{
	int	retval = 1;
	haggle_handle_t haggle_;
	
	// This is to catch Ctrl-C:
#if defined(OS_UNIX)
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
#endif
#if defined(OS_WINDOWS)
	signal(SIGTERM, &signal_handler);
	signal(SIGINT, &signal_handler);
#elif defined(OS_UNIX)
	sigact.sa_handler = &signal_handler;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
#endif
	
	// Create a mutex to handle shutdown with:
	shutdown_mutex = mutex_create();
	if(shutdown_mutex == NULL)
		goto fail_mutex;
	
	// Lock the mutex:
	mutex_lock(shutdown_mutex);
	
	// Find Haggle:
	if(haggle_handle_get("Haggle SMTP Proxy", &haggle_) != HAGGLE_NO_ERROR)
		goto fail_haggle;
	
	// Tell haggle we want to know when haggle goes down, so we can shut down:
	haggle_ipc_register_event_interest(
		haggle_, 
		LIBHAGGLE_EVENT_SHUTDOWN, 
		onDataShutdown);
	
	// Start the haggle event loop:
	if(haggle_event_loop_run_async(haggle_) != 0)
		goto fail_start_haggle;
	
	// Start SMTP server:
	if(!smtp_server_start(haggle_))
		goto fail_server;
	
	// Wait for someone to wake us up:
	mutex_lock(shutdown_mutex);
	retval = 0;
	
	// Stop the SMTP server:
	smtp_server_stop();
fail_server:
	// Stop the haggle event loop:
	haggle_event_loop_stop(haggle_);
fail_start_haggle:
	// Release the haggle handle:
	haggle_handle_free(haggle_);
fail_haggle:
fail_mutex:
	return retval;
}
