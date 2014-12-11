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

#include <libhaggle/haggle.h>

#include <stdio.h>
#include "testhlp.h"

/*
	This program tests to make sure that data objects with very large amounts 
	of metadata are transferred between haggle and libhaggle properly.
*/
#define MY_INTEREST_NAME	((char *) "Large DO interest")
#define MY_ATTRIBUTE		((char *) "Large DO attr")
#define MY_VALUE_SIZE		65536
static char my_value[MY_VALUE_SIZE + 1];

static int has_gotten_do;
static bool	has_attrib, has_shut_down;
static int do_interest_size;

static Condition	*my_cond;
static Mutex		*mutex;

class largedoRunnable : public Runnable {
public:
	largedoRunnable() {}
	~largedoRunnable() {}

	Mutex *getMutex() { return &mutex; }
	bool run()
	{
		// Wait for a tiny while so the main thread will be waiting on the 
		// condition variable
		milli_sleep(100);
		has_shut_down = hagglemain_stop_and_wait();
		my_cond->signal();
#if defined(OS_WINDOWS)
		// Wait for the testsuite to finish before terminating this thread:
		// This removes garbage output in the middle of the testsuite results
		milli_sleep(100000);
#endif
		return false;
	}
	void cleanup() {} 
};

static int STDCALL largedo_dataobject_event_handler(haggle_event_t *e, void *arg)
{
	struct attribute *attrib;

	has_gotten_do++;
	attrib = haggle_dataobject_get_attribute_by_name( e->dobj, MY_ATTRIBUTE);
	has_attrib = (attrib != NULL);
	if (has_attrib)
		do_interest_size = strlen(haggle_attribute_get_value(attrib));
	
	my_cond->signal();

	return 0;
}

#if defined(OS_WINDOWS)
int haggle_test_largedo(void)
#else
int main(int argc, char *argv[])
#endif
{
	bool success = true, tmp_succ, loop_succ;
	haggle_handle_t	my_haggle_handle;
	struct dataobject *my_do;
	long i, current_size;
	int old_has_gotten_do;
	
	// Disable tracing
	trace_disable(true);

	old_has_gotten_do = 0;
	has_gotten_do = 0;
	has_attrib = false;
	do_interest_size = 0;
	set_trace_level(0);
	mutex = new Mutex();
	mutex->lock();
	my_cond = new Condition();
	
	has_shut_down = false;
	print_over_test_str_nl(0, "Large data object test: ");
	
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
	
	// Wait for haggle to start properly:
	my_cond->timedWaitSeconds(mutex, 15);
	
	print_over_test_str(1, "Get haggle handle: ");
	tmp_succ = (haggle_handle_get("Large data object test", &my_haggle_handle) == HAGGLE_NO_ERROR);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Register event interest:
	
	print_over_test_str(1, "Register event interest: ");
	tmp_succ = 
		(haggle_ipc_register_event_interest(
			my_haggle_handle,
			LIBHAGGLE_EVENT_NEW_DATAOBJECT,
			largedo_dataobject_event_handler) > 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Start haggle event loop:
	
	print_over_test_str(1, "Start hagglelib event loop: ");
	tmp_succ = 
		(haggle_event_loop_run_async(my_haggle_handle) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Register application interest:
	
	print_over_test_str(1, "Register application interest: ");
	tmp_succ = 
		(haggle_ipc_add_application_interest(
			my_haggle_handle,
			MY_INTEREST_NAME,
			"*") >= 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	for(current_size = 2; 
		current_size <= MY_VALUE_SIZE; 
		current_size = (i<<1))
	{
		char	str[256];
		
		// Fill in attribute:
		for(i = 0; i < current_size; i++)
			my_value[i] = 'A';
		my_value[current_size] = '\0';
		
		loop_succ = true;
		// Reset testing variables:
		has_attrib = false;
		do_interest_size = 0;
	
		// Create data object:
		
		sprintf(str, "Data object with %ld bytes metadata: ", current_size);
		print_over_test_str_nl(1, str);
		
		print_over_test_str(2, "Create data object: ");
		my_do = haggle_dataobject_new();
		tmp_succ = (my_do != NULL);
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);
		
		// Add attribute:
		
		print_over_test_str(2, "Add attributes: ");
		tmp_succ = 
			(haggle_dataobject_add_attribute(
				my_do, 
				MY_INTEREST_NAME, 
				"*") > 0);
		loop_succ &= tmp_succ;
		tmp_succ = 
			(haggle_dataobject_add_attribute(
				my_do, 
				MY_ATTRIBUTE, 
				my_value) > 0);
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);
		
		// Publish data object:
		
		print_over_test_str(2, "Publish data object: ");
		tmp_succ = 
			(haggle_ipc_publish_dataobject(my_haggle_handle, my_do) >= 0);
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);
		
		haggle_dataobject_free(my_do);
		
		// If sending failed, go directly to subtotal - don't try to test
		// reception.
		if(!tmp_succ)
			goto fail_send_do;
		
		// Wait for a while before checking if data object was received:
		
		my_cond->timedWaitSeconds(mutex, 2);
		
		// Received data object:
		
		print_over_test_str(2, "Received data object: ");
		tmp_succ = (has_gotten_do != old_has_gotten_do);
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(2, "Received only one data object: ");
		tmp_succ = (has_gotten_do == old_has_gotten_do+1);
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);
		
		print_over_test_str(2, "Received data object has attribute: ");
		tmp_succ = has_attrib;
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);

		print_over_test_str(2, "Received attribute has right size: ");
		tmp_succ = (do_interest_size == current_size);
		loop_succ &= tmp_succ;
		print_pass(tmp_succ);
		
fail_send_do:
		old_has_gotten_do = has_gotten_do;
		print_over_test_str(1, "Subtotal: ");
		success &= loop_succ;
		print_pass(loop_succ);
	}
	
	// Stop event loop:
	print_over_test_str(1, "Stopping event loop: ");
	tmp_succ = (haggle_event_loop_stop(my_haggle_handle) == 0);
	success &= tmp_succ;
	print_pass(tmp_succ);
	
	// Release the handle:
	haggle_handle_free(my_haggle_handle);
	
	// Stop haggle:
	print_over_test_str(1, "Stopping haggle: ");
	(new largedoRunnable())->start();
	my_cond->timedWaitSeconds(mutex, 5);
	success &= has_shut_down;
	print_pass(has_shut_down);
	
fail_start_haggle:
	print_over_test_str(1, "Total: ");
	
	return (success?0:1);
}

