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
	This program tests to make sure that the multiple apps are able to register
	the same interests, and all of them still get data objects intended for 
	them.
*/
static bool has_shut_down;

#define MY_INTEREST_NAME	((char *) "Test DO interest")
#define MY_INTEREST_VALUE	((char *) "Test DO value")

static int has_gotten_do_1, has_gotten_do_2;

static Condition	*my_cond;
static Mutex		*mutex;

static int STDCALL multipleapp_1_dataobject_event_handler(haggle_event_t *e, void *arg)
{
	has_gotten_do_1++;

	return 0;
}

static int STDCALL multipleapp_2_dataobject_event_handler(haggle_event_t *e, void *arg)
{
	has_gotten_do_2++;

	return 0;
}

#if defined(OS_WINDOWS)
int haggle_test_multipleapp(void)
#else
int main(int argc, char *argv[])
#endif
{
	bool				success = true, 
						tmp_succ;
	haggle_handle_t		haggle_handle_1, haggle_handle_2;
	struct dataobject	*my_do;
		
	// Disable tracing
	trace_disable(true);

	set_trace_level(0);
	mutex = new Mutex();
	mutex->lock();
	my_cond = new Condition();
	
	has_shut_down = false;
	print_over_test_str_nl(0, "Multiple application test: ");
	
	// Can we start a haggle kernel?
	tmp_succ = 
		hagglemain_start(
			// All managers:
			hagglemain_have_all_managers 
			// Except the connectivity manager, since it most likely starts a 
			// Bluetooth scan:
			^ hagglemain_have_connectivity_manager,
			// No condition variable:
			my_cond);
	success &= tmp_succ;
	print_over_test_str(1, "Start haggle: ");
	print_pass(tmp_succ);
	if(!tmp_succ)
		goto fail_start_haggle;
	
	// Wait for haggle to start proplery:
	my_cond->timedWaitSeconds(mutex, 15);
	
	// Get one haggle handle:
	print_over_test_str(1, "Get first haggle handle: ");
	tmp_succ = (haggle_handle_get("Multiple application test 1", &haggle_handle_1) == HAGGLE_NO_ERROR);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Get another haggle handle:
	print_over_test_str(1, "Get second haggle handle: ");
	tmp_succ = (haggle_handle_get("Multiple application test 2", &haggle_handle_2) == HAGGLE_NO_ERROR);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Test the handles against eachother:
	print_over_test_str(1, "Handles are different: ");
	tmp_succ = (haggle_handle_2 != haggle_handle_1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Register event interest:
	
	print_over_test_str(1, "Register event interest 1: ");
	tmp_succ = 
		(haggle_ipc_register_event_interest(
			haggle_handle_1,
			LIBHAGGLE_EVENT_NEW_DATAOBJECT,
			multipleapp_1_dataobject_event_handler) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Register event interest 2: ");
	tmp_succ = 
		(haggle_ipc_register_event_interest(
			haggle_handle_2,
			LIBHAGGLE_EVENT_NEW_DATAOBJECT,
			multipleapp_2_dataobject_event_handler) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Start haggle event loop:
	
	print_over_test_str(1, "Start hagglelib event loop 1: ");
	tmp_succ = 
		(haggle_event_loop_run_async(haggle_handle_1) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Start hagglelib event loop 2: ");
	tmp_succ = 
		(haggle_event_loop_run_async(haggle_handle_2) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Register application interest:
	
	print_over_test_str(1, "Register application interest 1: ");
	tmp_succ = 
		(haggle_ipc_add_application_interest(
			haggle_handle_1,
			MY_INTEREST_NAME,
			MY_INTEREST_VALUE) >= 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Register application interest 2: ");
	tmp_succ = 
		(haggle_ipc_add_application_interest(
			haggle_handle_2,
			MY_INTEREST_NAME,
			MY_INTEREST_VALUE) >= 0);
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
	
	print_over_test_str(1, "Publish data object: ");
	tmp_succ = 
		(haggle_ipc_publish_dataobject(haggle_handle_1, my_do) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Wait for a while before checking if data object was received:
	
	milli_sleep(1000);
	
	// Received data object:
	
	print_over_test_str(1, "Application 1 received data object: ");
	tmp_succ = (has_gotten_do_1 != 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Application 1 received only one data object: ");
	tmp_succ = (has_gotten_do_1 == 1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Application 2 received data object: ");
	tmp_succ = (has_gotten_do_2 != 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Application 2 received only one data object: ");
	tmp_succ = (has_gotten_do_2 == 1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Stop event loop 1:
	print_over_test_str(1, "Application 1 event loop shutdown: ");
	tmp_succ = 
		(haggle_event_loop_stop(haggle_handle_1) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Create data object:
	
	print_over_test_str(1, "Create another data object: ");
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
	success &= tmp_succ;
	tmp_succ = 
		(haggle_dataobject_add_attribute(
			my_do, 
			MY_INTEREST_VALUE,
			MY_INTEREST_NAME) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Publish data object:
	
	print_over_test_str(1, "Publish data object: ");
	tmp_succ = 
		(haggle_ipc_publish_dataobject(haggle_handle_1, my_do) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Wait for a while before checking if data object was received:
	
	milli_sleep(2000);
	
	// Received data object:
	
	print_over_test_str(1, "Application 1 did not receive data object: ");
	tmp_succ = (has_gotten_do_1 == 1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Application 2 received data object: ");
	tmp_succ = (has_gotten_do_2 > 1);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	print_over_test_str(1, "Application 2 received only one data object: ");
	tmp_succ = (has_gotten_do_2 == 2);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Stop event loop 2:
	print_over_test_str(1, "Application 2 event loop shutdown: ");
	tmp_succ = 
		(haggle_event_loop_stop(haggle_handle_2) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Release the handles:
	haggle_handle_free(haggle_handle_1);
	haggle_handle_free(haggle_handle_2);
	
	// Stop haggle:
	hagglemain_stop_and_wait();
fail_start_haggle:
	print_over_test_str(1, "Total: ");
	
	return (success?0:1);
}

