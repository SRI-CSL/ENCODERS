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

#ifndef _smtpd_h_
#define _smtpd_h_

/*
	Minimalistic SMTP helper library.
	
	This is not exactly inteded as a complete SMTP implementation, but 
	something that mimics SMTP enough to make most mail clients able to receive
	email.
*/


/**
	Sets up the popd helper library, and starts listening for connections on 
	the given port.
	
	Returns true iff the helper library can be further used.
*/
bool smtp_startup(int port);

/**
	Starts accepting client connections. Will not return until smtp_shutdown
	is called.
*/
void smtp_accept_connections(void);

/**
	Closes the SMTP port opened using smtp_startup. Shuts down the popd helper 
	library.
	
	The helper library can not be used further after calling this function.
*/
void smtp_shutdown(void);

/*
	Called whenever a new email is sent by a client.
	
	This function has to be implemented by whatever uses this library.
	
	The strings are null-terminated.
	
	The message string might contain null characters, that's why the length is 
	specified.
	
	Modifying the given strings is fine, but do not assume you can write to any
	byte beyond them.
*/
void smtp_new_email(char *from, char *to, char *message, long message_len);

#endif
