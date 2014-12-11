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

#ifndef _httpd_h_
#define _httpd_h_

#include "databuf.h"

/*
	Minimalistic httpd helper library.
	
	This is not exactly inteded as a complete HTTP implementation, but 
	something that mimics HTTP enough to fool a web browser such as Safari,
	IE, or Firefox.
*/

/**
	Sets up the httpd helper library, and starts listening for HTTP connections
	on the given port.
	
	Returns true iff the helper library can be further used.
*/
bool httpd_startup(int port);

/**
	Starts accepting client connections. Will not return until httpd_shutdown
	is called.
*/
void httpd_accept_connections(void);

/**
	Closes the http port opened using httpd_startup. Shuts down the httpd 
	helper library.
	
	The helper library can not be used further after calling this function.
*/
void httpd_shutdown(void);


/**
	Returns a error message to the client and shuts the connection down.
	
	Only meant for 4XX,5XX errors like: 404/Not found, 500/Infernal server 
	error.
*/
void http_reply_err(int err);

/**
	Returns the given data to the client, then closes the connection.
	
	The client will get a 200/OK, then the given data.
	
	The content_type variable should point to a null-terminated string 
	containing a content type, (such as "text/html"), it should not contain
	any carriage returns or line feeds.
*/
void http_reply_data(char *data, long len, char *content_type);

/*
	Called whenever a HTTP GET request is made by a client.
	
	This function has to be implemented by whatever uses this library.
	
	The string is null-terminated.
*/
void httpd_process_GET(char *resource_name);

/*
	Called whenever a HTTP POST request is made by a client.
	
	This function has to be implemented by whatever uses this library.
	
	The resource name string is null-terminated.
	
	The post_data string is null-terminated, but the null-terminating character
	is not included in the data_len count.
	
	Modifying the given strings is fine, but do not assume you can write to any
	byte beyond them.
*/
void httpd_process_POST(char *resource_name, char *post_data, long data_len);

/*
	Function to get the content of the input field with the given name.
	
	The returned data should not be deallocated (it's just a reference into
	the original data buffer).
	
	The returned data will not need decoding.
	
	The given data buffer will be modified (but not beyond the given bounds).
	
	Returns NULL/0 if there was no such input field.
*/
data_buffer httpd_post_data_get_data_for_input(
				char *input, 
				char *data, 
				long data_len);

#endif

