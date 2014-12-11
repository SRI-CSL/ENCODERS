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

#ifndef HAGGLEMAIN_H
#define HAGGLEMAIN_H

#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Condition.h>

using namespace haggle;

/*
	The different managers a haggle kernel should have for the running 
	experiment:
*/
enum {
	hagglemain_have_application_manager		=	(1<<0),
	hagglemain_have_connectivity_manager	=	(1<<1),
	hagglemain_have_data_manager			=	(1<<2),
	hagglemain_have_forwarding_manager		=	(1<<3),
	hagglemain_have_node_manager			=	(1<<4),
	hagglemain_have_protocol_manager		=	(1<<5),
	hagglemain_have_security_manager		=	(1<<6),
	hagglemain_have_resource_manager		=	(1<<7),
	
	hagglemain_have_all_managers			=	(1<<8)-1
};

/*
	This function sets up (in a separate thread) a haggle kernel with the given
	managers.
	
	The Condition variable is optional, and if it is given, the condition
	will be signalled when haggle has started.
	
	Returns true iff it was successful.
	
	This function will fail if there was already a haggle kernel running.
*/
bool hagglemain_start(long flags, Condition *cond);

/*
	This function returns true iff there is a haggle kernel running.
*/
bool hagglemain_is_running(void);

/*
	If there is a hagglekernel running, this function will tell it to shut down.
	
	When this function returns, the haggle kernel may not have finished.
*/
bool hagglemain_stop(void);

/*
	If there is a hagglekernel running, this function will tell it to shut down.
	The function then waits for the kernel to shut down before returning.
	
	When this function returns, the haggle kernel will have finished running.
*/
bool hagglemain_stop_and_wait(void);

#endif

