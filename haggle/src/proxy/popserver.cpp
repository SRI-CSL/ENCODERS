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

#include "popserver.h"
#include "popd.h"
#include "thread.h"

#include "mailattr.h"
#include "databuf.h"

#include "prng.h"

#include <string.h>
#include <stdio.h>
#ifndef OS_WINDOWS_MOBILE
#include <signal.h>
#endif
#include <list>

static haggle_handle_t haggle_;

//#if defined(OS_WINDOWS_MOBILE)
//#define DEFAULT_POP_PORT 110
//#else
#define DEFAULT_POP_PORT 11011
//#endif

#if defined(OS_WINDOWS)
#pragma warning( push )
#pragma warning( disable: 4200 )
#endif

typedef struct email_s {
	// This is the email:
	char *message;
	// This is true iff the message has been tagged for deletion.
	bool deleted;
	// This is the size of the email:
	long size;
	// This is the UID of the email:
	char UID[256];
} *email;

typedef struct user_account_s {
	// To avoid having to go through the whole list:
	// This count does NOT include messages tagged for deletion:
	long messages_in_inbox;
	// These are the not yet deleted messages:
	std::list<email> *messages;
	// FIXME: how to do message deletion better?
	// These are deleted messages:
	std::list<email> *deleted_messages;
	// The user name:
	char user_name[0];
} *user_account;

#if defined(OS_WINDOWS)
#pragma warning( pop )
#endif

static email email_create(struct dataobject *dObj)
{
	email				m;
	struct attribute	*attr;
	
	m = (email) malloc(sizeof(struct email_s));
	if(m == NULL)
		return NULL;
	
	// Determine email size:
	if (haggle_dataobject_get_data_size(dObj, (size_t *)&m->size) < 0)
	{
		// No email
		free(m);
		return NULL;
	}
	
	// Get the message:
	m->message = (char *) haggle_dataobject_get_data_all(dObj);
	if(m->message == NULL)
	{
		free(m);
		return NULL;
	}
	// Not yet deleted.
	m->deleted = false;
	// Set UIDL:
	// Find the message's UID:
	attr = 
		haggle_dataobject_get_attribute_by_name(
			dObj, 
			haggle_email_attribute_UID);

	if(attr == NULL)
	{
		// Make up a random UID, since none was found:
		sprintf(
			m->UID, 
			"%lu.%lu.john.doe@haggle", 
			prng_uint32(), 
			prng_uint32());
	}else{
		// Copy the UID:
		strncpy(m->UID, (char *) haggle_attribute_get_value(attr), 256);
	}
	// Make sure that all characters are valid UID characters:
	{
		long	i;
		
		for(i = 0; m->UID[i] != '\0'; i++)
			if(m->UID[i] > 0x7E || m->UID[i] < 0x21)
				m->UID[i] = '#';
	}
	
	return m;
}

static void email_destroy(email m)
{
	free(m->message);
	free(m);
}

static user_account user_account_create(char *name)
{
	user_account u;
	long len;
	
	len = strlen(name);

	u = (user_account) 
		malloc(sizeof(struct user_account_s) + sizeof(char) * (len+1));

	if(u == NULL)
		return NULL;
	
	strcpy(u->user_name, name);
	u->messages = new std::list<email>();
	u->deleted_messages = new std::list<email>();
	u->messages_in_inbox = 0;
	
	return u;
}

#if 0
// Not used at the moment
static void user_account_destroy(user_account u)
{
	while (u->messages->size() != 0) {
		email	m;
		
		m = u->messages->front();
		u->messages->pop_front();
		email_destroy(m);
	}
	delete u->messages;
	while (u->deleted_messages->size() != 0) {
		email	m;
		
		m = u->deleted_messages->front();
		u->deleted_messages->pop_front();
		email_destroy(m);
	}
	delete u->deleted_messages;
	free(u);
}
#endif

std::list<user_account>		accounts;

mutex_t	pop3_mutex;

// Returns NULL if the email can't be found in the inbox
// WARNING: requires the mutex to be locked!
static email pop3_find_email(char *user, long message)
{
	long	i;
	
	for (std::list<user_account>::iterator it = accounts.begin(); 
		it != accounts.end(); it++) {
		if (strcmp((*it)->user_name, user) == 0) {
			i = 0;
			for (std::list<email>::iterator jt = (*it)->messages->begin(); 
				jt != (*it)->messages->end(); jt++) {
				if( i == message) {
					return (*jt);
				}
				i++;
			}
		}
	}
	return NULL;
}

bool pop3_login(char *user, char *password)
{
	static bool email_address_added_as_interest = false;
	/*
		Add the user name to our own interests as a weighted attribute.
		
		The principle is that if a user logs on and wants a list of emails for a
		specific address, then that user also wishes to receive any messages to
		that address. We also assume that a user always has only one address at 
		a time.
		
		FIXME: There is probably a cleaner solution for this...
	*/

	if (!email_address_added_as_interest) {
		// We pick some weight higher than 1 (the default), 
		// until we have defined what type of weighting makes sense.
		haggle_ipc_add_application_interest_weighted(
			haggle_, 
			haggle_email_attribute_to, 
			user, 
			10);
		email_address_added_as_interest = true;
	}
	
	// Anyone may log in with any password:
	return true;
}

long pop3_list(char *user)
{
	long retval;
	
	retval = 0;
	mutex_lock(pop3_mutex);

	for (std::list<user_account>::iterator it = accounts.begin(); 
		 it != accounts.end(); 
		 it++) {
		if(strcmp((*it)->user_name, user) == 0) {
			retval = (*it)->messages_in_inbox;
			goto done;
		}
	}
done:
	mutex_unlock(pop3_mutex);
	return retval;
}

long pop3_message_size(char *user, long message)
{
	long	retval;
	email	m;
	
	retval = 0;
	mutex_lock(pop3_mutex);
	m = pop3_find_email(user, message);
	if(m)
		retval = m->size;
	mutex_unlock(pop3_mutex);
	return retval;
}

char *pop3_get_UIDL(char *user, long message)
{
	email m;
	char *retval;
	
	retval = NULL;
	mutex_lock(pop3_mutex);
	m = pop3_find_email(user, message);
	if(m)
		retval = m->UID;
	mutex_unlock(pop3_mutex);
	return retval;
}

char *pop3_get_message(char *user, long message)
{
	email	m;
	char	*retval;
	
	retval = NULL;
	mutex_lock(pop3_mutex);
	m = pop3_find_email(user, message);

	if(m)
	{
		retval = (char *)malloc(sizeof(char) * (m->size + 1));
		
		if(retval != NULL)
		{
			memcpy(retval, m->message, m->size);
			retval[m->size] = '\0';
		}
	}
	mutex_unlock(pop3_mutex);
	return retval;
}

char *pop3_get_top_of_message(char *user, long message, long lines)
{
	long	i, len, line_count;
	char	*retval;
	
	retval = pop3_get_message(user, message);
	if(retval == NULL)
		return NULL;
	
	len = strlen(retval);
	
	line_count = -1;
	for(i = 3; i < len; i++)
	{
		if(line_count >= 0)
		{
			if(	retval[i-1] == '\r' &&
				retval[i-0] == '\n')
			{
				line_count++;
				if(line_count == lines)
					goto cut;
			}
		}else if(
			retval[i-3] == '\r' &&
			retval[i-2] == '\n' &&
			retval[i-1] == '\r' &&
			retval[i-0] == '\n')
		{
			line_count = 0;
			if(lines == 0)
				goto cut;
		}
	}
	if(0)
	{
cut:
		retval[i+1] = '\0';
	}
	return retval;
}

bool pop3_delete_message(char *user, long message)
{
	bool	retval;
	email	m;
	
	retval = false;
	mutex_lock(pop3_mutex);
	m = pop3_find_email(user, message);
	if(m)
	{
		m->deleted = true;
		retval = true;
	}
	mutex_unlock(pop3_mutex);
	return retval;
}

bool pop3_delete_messages(char *user)
{
	mutex_lock(pop3_mutex);
	for(std::list<user_account>::iterator it = accounts.begin();
		it != accounts.end();
		it++)
	{
		if(strcmp((*it)->user_name, user) == 0)
		{
handle_next:
			for(std::list<email>::iterator jt = (*it)->messages->begin();
				jt != (*it)->messages->end();
				jt++)
			{
				if((*jt)->deleted)
				{
					email	m;
					
					m = (*jt);
					(*it)->messages_in_inbox--;
					(*it)->deleted_messages->push_front(m);
					(*it)->messages->remove(m);
					goto handle_next;
				}
			}
		}
	}
	mutex_unlock(pop3_mutex);
	return true;
}

bool pop3_undelete_messages(char *user)
{
	mutex_lock(pop3_mutex);
	for(std::list<user_account>::iterator it = accounts.begin();
		it != accounts.end();
		it++)
	{
		if(strcmp((*it)->user_name, user) == 0)
		{
			for(std::list<email>::iterator jt = (*it)->messages->begin();
				jt != (*it)->messages->end();
				jt++)
			{
				if((*jt)->deleted)
				{
					(*jt)->deleted = false;
				}
			}
		}
	}
	mutex_unlock(pop3_mutex);
	return true;
}

/*
	This is called when haggle tells us there's a new data object we might be 
	interested in.
*/
static int onDataObject(haggle_event_t *e, void *arg)
{
	email				m;
	struct attribute	*attr;
	char				*to;
	user_account		user;
	
	// Create new message:
	m = email_create(e->dobj);

	if(m == NULL)
	{
		printf("Got dataobject that was not an email.\n");
		goto fail_message;
	}
	
	// Find who the message is for:
	attr = 
		haggle_dataobject_get_attribute_by_name(
			e->dobj, 
			haggle_email_attribute_to);

	if(attr == NULL)
		goto fail_mailto;

	to = (char *) haggle_attribute_get_value(attr);
	
	// Now we're accessing the POP3 server stuff, so we lock the mutex:
	mutex_lock(pop3_mutex);
	
	// Does the user exist?
	user = NULL;
	
	for(std::list<user_account>::iterator it = accounts.begin();
		it != accounts.end() && user == NULL;
		it++)
	{
		if(strcmp((*it)->user_name, to) == 0)
		{
			user = (*it);
		}
	}
	if(user == NULL)
	{
		// No? Create new user:
		user = user_account_create(to);
		if(user == NULL)
			goto fail_create_user;
		
		// Insert into user list:
		accounts.push_back(user);
	}
	
	// Check that this message is not in the "trashcan":
	for(std::list<email>::iterator it = user->deleted_messages->begin();
		it != user->deleted_messages->end();
		it++)
	{
		// Are the UIDs a match?
		if(strncmp((*it)->UID, m->UID, 256) == 0)
		{
			// Yes. Discard.
			printf("Already delivered email received:\n"
				"\tTo: %s\n"
				"\tUID: %s\n",
				to,
				m->UID);
			goto fail_message_is_duplicate;
		}
	}
	
	// Check that this message is not in the "inbox":
	for(std::list<email>::iterator it = user->messages->begin();
		it != user->messages->end();
		it++)
	{
		// Are the UIDs a match?
		if(strncmp((*it)->UID, m->UID, 256) == 0)
		{
			printf("Duplicate email received:\n"
				"\tTo: %s\n"
				"\tUID: %s\n",
				to,
				m->UID);
			// Yes. Discard.
			goto fail_message_is_duplicate;
		}
	}
	printf("New email received:\n"
		"\tTo: %s\n"
		"\tUID: %s\n",
		to,
		m->UID);
	
	// Put the message in (last):
	user->messages->push_back(m);
	user->messages_in_inbox++;

	printf("user %s now has %ld messages in the inbox\n", 
	       user->user_name, user->messages_in_inbox);

	m = NULL;
	
fail_message_is_duplicate:

fail_create_user:
	// Now we're done accessing the POP3 server stuff, so we unlock the mutex:
	mutex_unlock(pop3_mutex);
	
fail_mailto:
	if(m != NULL)
		email_destroy(m);
fail_message:
        return 0;
}

bool pop3_server_start(haggle_handle_t _haggle)
{
	haggle_ = _haggle;
	
	// Initialize the PRNG library:
	prng_init();
	
	// Create the global POP3 mutex:
	pop3_mutex = mutex_create();

	if(pop3_mutex == NULL)
		goto fail_mutex;
	
	// Start the popd library:
	if(!pop3_startup(DEFAULT_POP_PORT))
		goto fail_popd;
	
	// Tell haggle we want to know about new data objects:
	haggle_ipc_register_event_interest(
		haggle_, 
		LIBHAGGLE_EVENT_NEW_DATAOBJECT, 
		onDataObject);
	
	// Start accepting connections:
	if(!thread_start((void (*)(int))pop3_accept_connections,0))
		goto fail_thread;
	
	return true;
	
fail_thread:
	pop3_shutdown();
	if(0)
fail_popd:
		printf("Unable to start POP3 library.\n");
fail_mutex:
	return false;
}

void pop3_server_stop(void)
{
	pop3_shutdown();
}
