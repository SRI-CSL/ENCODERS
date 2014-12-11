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

#include <utils.h>
#include <libhaggle/haggle.h>

#include "smtpserver.h"
#include "smtpd.h"
#include "thread.h"

#include "mailattr.h"
#include "databuf.h"

#include "prng.h"

#include <string.h>
#include <stdio.h>
#ifndef OS_WINDOWS_MOBILE
#include <signal.h>
#endif
#include <time.h>

#include "mini_base64.h"

//#if defined(OS_WINDOWS_MOBILE)
//#define DEFAULT_SMTP_PORT 25
//#else
#define DEFAULT_SMTP_PORT 2525
//#endif

static haggle_handle_t haggle_;

void smtp_new_email(char *from, char *to, char *message, long message_len)
{
	char UID[256];
	struct dataobject *dObj;
	unsigned long random_id_number;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	printf("Received message to send\n");
	
	random_id_number = prng_uint32();
	dObj = haggle_dataobject_new_from_buffer(
			(const unsigned char *) message, 
			message_len);
	
	// Make sure the data object is permanent:
	haggle_dataobject_set_flags(dObj, DATAOBJECT_FLAG_PERSISTENT);
	haggle_dataobject_set_createtime(dObj, &tv);
	haggle_dataobject_add_attribute(dObj, haggle_email_attribute_from, from);
	haggle_dataobject_add_attribute(dObj, haggle_email_attribute_to, to);
	haggle_dataobject_add_hash(dObj);
	
	// Create a UID for this email.
	/*
		This may or may not be unique, since it relies on two 32-bit random 
		numbers for uniqueness, but the likelihood of two 64-bit values to be 
		the same is very slim.
	*/
	sprintf(UID, "%lu.%lu.%s", tv.tv_sec, random_id_number, from);
	// Make sure that all characters are valid UID characters:
	{
		long	i;
		
		for(i = 0; UID[i] != '\0'; i++)
			if(UID[i] > 0x7E || UID[i] < 0x21)
				UID[i] = '#';
	}
	haggle_dataobject_add_attribute(dObj, haggle_email_attribute_UID, UID);

	haggle_ipc_publish_dataobject(haggle_, dObj);
	
	haggle_dataobject_free(dObj);

	printf("Message sent\n\n");
}

bool smtp_server_start(haggle_handle_t _haggle)
{
	haggle_ = _haggle;
	
	// Initialize the pseudo-random number generator:
	prng_init();
	
	// Start the SMTP library:
	if(!smtp_startup(DEFAULT_SMTP_PORT))
		goto fail_smtpd;
	
	// Start accepting connections:
	if(!thread_start((void (*)(int))smtp_accept_connections, 0))
		goto fail_thread;
	
	return true;
	
fail_thread:
	smtp_shutdown();
	if(0)
fail_smtpd:
		printf("Unable to start SMTP library.\n");
	return false;
}

void smtp_server_stop(void)
{
	smtp_shutdown();
}
