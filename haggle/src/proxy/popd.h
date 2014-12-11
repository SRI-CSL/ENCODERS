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

#ifndef _popd_h_
#define _popd_h_

/*
	Minimalistic POP3 helper library.
	
	This is not exactly inteded as a complete POP3 implementation, but 
	something that mimics POP3 enough to make most mail clients able to receive
	email.
*/


/**
	Sets up the popd helper library, and starts listening for connections on 
	the given port.
	
	Returns true iff the helper library can be further used.
*/
bool pop3_startup(int port);

/**
	Starts accepting client connections. Will not return until pop3_shutdown
	is called.
*/
void pop3_accept_connections(void);

/**
	Closes the POP3 port opened using pop3_startup. Shuts down the popd helper 
	library.
	
	The helper library can not be used further after calling this function.
*/
void pop3_shutdown(void);

/**
	Returns true iff the user is allowed to log on with this username/password 
	combination.
	
	This function has to be implemented by whatever uses this library.
	
	The strings are null-terminated.
*/
bool pop3_login(char *user, char *password);

/**
	Returns the number of messages not marked as deleted.
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
*/
long pop3_list(char *user);

/**
	Returns the UIDL of the user's message with the given number, not marked as
	deleted.
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
	
	Returns NULL if no such message was found. Returns a C string if the 
	message was found.
	
	The returned string must be null-terminated, no longer than 70 characters,
	not shorter than 1 character, and only contain characters in the range
	0x21 - 0x7E.
	
	The returned string will not be modified or read beyond the initial 
	null-terminating character (or the first 70 characters, whichever comes 
	first).
*/
char *pop3_get_UIDL(char *user, long i);

/**
	Returns the size in bytes of the user's message with the given number.
	(Deleted messages do not count). Shall return 0 if the message does not 
	exist.
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
	The message value is a zero-based index.
*/
long pop3_message_size(char *user, long message);

/**
	Returns the user's message with the given number. (Deleted messages do not 
	count). Shall return NULL if the message does not exist.
	
	This function has to be implemented by whatever uses this library.
	
	The returned string must be null-terminated. It will not be modified. It 
	will, however, be free()d.
	
	The string is null-terminated.
	The message value is a zero-based index.
*/
char *pop3_get_message(char *user, long message);

/**
	Returns the user's message with the given number. (Deleted messages do not 
	count). Shall return NULL if the message does not exist.
	
	Must only return the first few lines of the message, 0 meaning just the 
	headers and the separating line between headers and message. The lines 
	argument tells how many lines to get.
	
	This function has to be implemented by whatever uses this library.
	
	The returned string must be null-terminated. It will not be modified. It 
	will, however, be free()d.
	
	The string is null-terminated.
	The message value is a zero-based index.
*/
char *pop3_get_top_of_message(char *user, long message, long lines);

/**
	Marks the user's message with the given number as one that should be 
	deleted. Does not delete the message directly.
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
	The message value is a zero-based index.
	
	Returns true iff the message existed and has been marked as deleted.
*/
bool pop3_delete_message(char *user, long message);

/**
	Deletes the user's messages that have already been marked for deletion. 
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
	
	Returns true iff the messages could be deleted.
*/
bool pop3_delete_messages(char *user);

/**
	Marks all the user's messages as ones that should not be deleted.
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
	
	Returns true iff the message existed and has been marked as deleted.
*/
bool pop3_undelete_messages(char *user);

#endif
