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

#include "hagglemain.h"
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>

using namespace haggle;

#include "libhaggle/haggle.h"

#include <stdio.h>
#include "testhlp.h"

/*
	This program tests to make sure that communication between haggle and 
	libhaggle works.
*/
static bool has_shut_down;

#define MY_INTEREST_NAME	((char *) "Test DO interest")
#define MY_INTEREST_VALUE	((char *) "Test DO value")

static int has_gotten_do;

static Condition	*my_cond;
static Mutex		*mutex;

static int STDCALL singleapp_dataobject_event_handler(haggle_event_t *e, void *arg)
{
	has_gotten_do++;
	my_cond->signal();

	return 0;
}

#if defined(OS_WINDOWS)
int haggle_test_singleapp(void)
#else
int main(int argc, char *argv[])
#endif
{
	bool				success = true, 
						tmp_succ;
	haggle_handle_t		my_haggle_handle;
	struct dataobject	*my_do;
	int					old_object_count;
		
	// Disable tracing
	trace_disable(true);

	has_gotten_do = 0;
	set_trace_level(0);
	mutex = new Mutex();
	mutex->lock();
	my_cond = new Condition();
	
	has_shut_down = false;
	print_over_test_str_nl(0, "Single application test: ");
	
	// Can we start a haggle kernel?
	tmp_succ = 
		hagglemain_start(
			// All managers:
			hagglemain_have_all_managers 
			// Except the connectivity manager, since it most likely starts a 
			// Bluetooth scan:
			^ hagglemain_have_connectivity_manager,
			my_cond);
	success &= tmp_succ;
	print_over_test_str(1, "Start haggle: ");
	print_pass(tmp_succ);
	if(!tmp_succ)
		goto fail_start_haggle;
	
	// Wait for haggle to start properly:
	my_cond->timedWaitSeconds(mutex, 15);
	
	print_over_test_str(1, "Get haggle handle: ");
	tmp_succ = (haggle_handle_get("Single application test", &my_haggle_handle) == HAGGLE_NO_ERROR);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Register event interest:
	
	print_over_test_str(1, "Register event interest: ");
	tmp_succ = 
		(haggle_ipc_register_event_interest(
			my_haggle_handle,
			LIBHAGGLE_EVENT_NEW_DATAOBJECT,
			singleapp_dataobject_event_handler) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Start haggle event loop:
	
	print_over_test_str(1, "Start hagglelib event loop: ");
	tmp_succ = 
		(haggle_event_loop_run_async(my_haggle_handle) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Create data object:
	
	print_over_test_str(1, "Create data object: ");
	my_do = haggle_dataobject_new();
	tmp_succ = (my_do != NULL);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Add attribute:
	
	print_over_test_str(1, "Add attributes: ");
	tmp_succ = 
		(haggle_dataobject_add_attribute(
			my_do, 
			MY_INTEREST_NAME, 
			MY_INTEREST_VALUE) > 0);
	tmp_succ &= 
		(haggle_dataobject_add_attribute(
			my_do, 
			"abcd", 
			"efgh") > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Publish data object:
	
	print_over_test_str(1, "Publish data object: ");
	tmp_succ = 
		(haggle_ipc_publish_dataobject(my_haggle_handle, my_do) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Wait for a while so haggle can store the data object before we're 
	// interested in it:
	
	my_cond->timedWaitSeconds(mutex, 2);
	
	// Register application interest:
	
	print_over_test_str(1, "Register application interest: ");
	tmp_succ = 
		(haggle_ipc_add_application_interest(
			my_haggle_handle,
			MY_INTEREST_NAME,
			MY_INTEREST_VALUE) >= 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Wait for a while before checking if data object was received:
	
	my_cond->timedWaitSeconds(mutex, 2);
	
	// Received data object:
	
	print_over_test_str(1, "Received \"old\" data object: ");
	tmp_succ = (has_gotten_do == 1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Create data object:
	
	print_over_test_str(1, "Create data object: ");
	my_do = haggle_dataobject_new();
	tmp_succ = (my_do != NULL);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Add attribute:
	
	print_over_test_str(1, "Add attribute: ");
	tmp_succ = 
		(haggle_dataobject_add_attribute(
			my_do, 
			MY_INTEREST_NAME, 
			MY_INTEREST_VALUE) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Publish data object:
	
	old_object_count = has_gotten_do;
	
	print_over_test_str(1, "Publish data object: ");
	tmp_succ = 
		(haggle_ipc_publish_dataobject(my_haggle_handle, my_do) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Wait for a while before checking if data object was received:
	
	my_cond->timedWaitSeconds(mutex, 2);
	
	// Received data object:
	
	print_over_test_str(1, "Received data object: ");
	tmp_succ = (has_gotten_do != old_object_count);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Wait for a while in case the data object is delayed:
	my_cond->timedWaitSeconds(mutex, 1);
	
	print_over_test_str(1, "Received only one data object: ");
	tmp_succ = (has_gotten_do == old_object_count+1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Stop event loop:
	haggle_event_loop_stop(my_haggle_handle);
	
	// Release the handle:
	haggle_handle_free(my_haggle_handle);
	
	// Stop haggle:
	hagglemain_stop_and_wait();
fail_start_haggle:
	print_over_test_str(1, "Total: ");
	
	return (success?0:1);
}

