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

#include "libhaggle/haggle.h"
#include <stdio.h>
#include <string.h>

static void print_useage(char *error, char *progname)
{
	if(error)
		printf("%s\n\n", error);
	
	printf(
		"%s <-A|-B>\n"
		"\n"
		"\t-A\tCauses this program to behave as it should on node A.\n"
		"\t-B\tCauses this program to behave as it should on node B.\n"
		"\n",
		progname);
}

int main(int argc, char *argv[])
{
	long			i;
	enum {
		node_A,
		node_B,
		node_NONE
	}				which_node = node_NONE;
	int				retval = 1;
	haggle_handle_t	haggle_;
	
	// Check command line parameters to figure out which node this program is 
	// running on:
	for(i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-A") == 0)
		{
			if(which_node == node_NONE)
				which_node = node_A;
			else{
				print_useage(
					(char *) "ERROR: do not supply more than one parameter "
					"to this program!",
					argv[0]);
				goto fail_param;
			}
		}else if(strcmp(argv[i], "-B") == 0)
		{
			if(which_node == node_NONE)
				which_node = node_B;
			else{
				print_useage(
					(char *) "ERROR: do not supply more than one parameter "
					"to this program!",
					argv[0]);
				goto fail_param;
			}
		}
	}
	
	if(which_node == node_NONE)
	{
		print_useage(
			(char *) "ERROR: You must supply one of the -A or -B command line "
			"parameters.\n",
			argv[0]);
		goto fail_param;
	}
	
	// Find Haggle:
	if(	haggle_handle_get("Haggle test 01 application", &haggle_) != 
			HAGGLE_NO_ERROR)
	{
		printf("ERROR: Haggle test application already running!\n");
		goto fail_haggle;
	}
	
	switch(which_node)
	{
		case node_A:
		{
			struct dataobject *dO;
			
			// Create a new data object:
			dO = haggle_dataobject_new();
			// Add the correct attribute:
			haggle_dataobject_add_attribute(dO, "Picture", "Bild1");
			// Make sure the data object is permanent:
			haggle_dataobject_set_flags(dO, DATAOBJECT_FLAG_PERSISTENT);
			// Put it into haggle:
			haggle_ipc_publish_dataobject(haggle_, dO);
		}
		break;
		
		case node_B:
			// Add the attribute to our interests:
			haggle_ipc_add_application_interest(
				haggle_, 
				"Picture", 
				"Bild1");
		break;
		
		default:
			printf("CODING ERROR: SHOULD NOT HAVE GOTTEN HERE!\n");
			goto fail_err;
		break;
	}
	
	// Make sure to return success:
	retval = 0;
	
fail_err:
	// Release the haggle handle:
	haggle_handle_free(haggle_);
fail_haggle:
fail_param:
	return retval;
}
