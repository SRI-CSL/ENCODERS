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

#include "httpd.h"
#include "databuf.h"

#include <stdio.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#endif
#if defined(OS_MACOSX)
#include <net/if_dl.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(OS_LINUX)
#include <unistd.h>
#include <net/ethernet.h>
#include <string.h>
#elif defined(OS_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <winioctl.h>
#include <winerror.h>
#include <iphlpapi.h>
#endif

static char *httpd_error_msg[2][27] =
	{
		{
			(char *) "400 Bad Request",
			(char *) "401 Unauthorized",
			(char *) "402 Payment Required",
			(char *) "403 Forbidden",
			(char *) "404 Not Found",
			(char *) "405 Method Not Allowed",
			(char *) "406 Not Acceptable",
			(char *) "407 Proxy Authentication Required",
			(char *) "408 Request Timeout",
			(char *) "409 Conflict",
			(char *) "410 Gone",
			(char *) "411 Length Required",
			(char *) "412 Precondition Failed",
			(char *) "413 Request Entity Too Large",
			(char *) "414 Request-URI Too Long",
			(char *) "415 Unsupported Media Type",
			(char *) "416 Requested Range Not Satisfiable",
			(char *) "417 Expectation Failed",
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) "421 There are too many connections from your internet address",
			(char *) "422 Unprocessable Entity",
			(char *) "423 Locked",
			(char *) "424 Failed Dependency",
			(char *) "425 Unordered Collection",
			(char *) "426 Upgrade Required"
		},
		{
			(char *) "500 Internal Server Error",
			(char *) "501 Not Implemented",
			(char *) "502 Bad Gateway",
			(char *) "503 Service Unavailable",
			(char *) "504 Gateway Timeout",
			(char *) "505 HTTP Version Not Supported",
			(char *) "506 Variant Also Negotiates",
			(char *) "507 Insufficient Storage",
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL,
			(char *) NULL
		}
	};

static int sock;

bool httpd_startup(int port)
{
	struct sockaddr_in	my_addr;
	
	// Set up local port:
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	
	// Open a TCP socket:
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		goto fail_socket;
	
	// Bind the socket to the address:
	if(bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		goto fail_bind;
	
	// Listen on the socket:
	if(listen(sock, 20) == -1)
		goto fail_listen;
	
	return true;
fail_listen:
fail_bind:
	close(sock);
fail_socket:
	sock = -1;
	return false;
}

void httpd_shutdown(void)
{
	close(sock);
	sock = -1;
}

static char *content_length_header = (char *) "Content-Length: ";

static int client_socket;
// YES, the receive buffer has a static size, see how this is handled below.
#define RECV_BUF_MAX 4096
static char recv_buf[RECV_BUF_MAX+1];
void httpd_accept_connections(void)
{
	struct sockaddr	addr;
	socklen_t		addr_len;
	
	// Keep going while the socket is ok:
	while(sock != -1)
	{
		addr_len = sizeof(addr);
		client_socket = accept(sock, &addr, &addr_len);
		// Should really fork() here, or create a separate thread, but in the
		// interest of making a lightweight implementation:
		if(client_socket != -1)
		{
			long	recv_buf_len;
			bool	not_done;
			long	err;
			
			// Read incoming request:
			recv_buf_len = 0;
			not_done = true;
			err = 500;
			do{
				ssize_t	i;
				
				i = recv(
						client_socket, 
						&(recv_buf[recv_buf_len]), 
						1, 
						MSG_WAITALL);
				if(i != 1)
				{
					err = 400;
					goto report_err;
				}
				
				recv_buf_len++;
				
				if(recv_buf_len >= 4)
				{
					if(	recv_buf[recv_buf_len-4] == '\r' &&
						recv_buf[recv_buf_len-3] == '\n' &&
						recv_buf[recv_buf_len-2] == '\r' &&
						recv_buf[recv_buf_len-1] == '\n')
						not_done = false;
				}
				if(not_done)
				{
					if(recv_buf_len >= RECV_BUF_MAX)
					{
						err = 500;
						goto report_err;
					}
				}
			}while(not_done);
			
			recv_buf[recv_buf_len] = '\0';
			
			// Entire incoming request gotten. Parse:
			
			if(strncmp(recv_buf, "GET ", 4) == 0)
			{
				// GET request:
				int i;
				
				i = 4;
				
				// Find the end of the line:
				// This is guaranteed to eventually be true, since we only get
				// here if we succeed in receiving a header above.
				while(!(recv_buf[i] == '\r' && recv_buf[i+1] == '\n'))
				{
					i++;
				}
				
				// Back up to before the HTTP/#.# part:
				
				while(!(recv_buf[i] == ' '))
					i--;
				
				// Null-terminate the resource identifier:
				recv_buf[i] = '\0';
				
				// Process the GET message:
				httpd_process_GET(&(recv_buf[4]));
				
			}else if(strncmp(recv_buf, "POST ", 5) == 0)
			{
				// POST request:
				int i, j;
				long	data_len;
				
				i = 5;
				
				// Find the end of the line:
				// This is guaranteed to eventually be true, since we only get
				// here if we succeed in receiving a header above.
				while(!(recv_buf[i] == '\r' && recv_buf[i+1] == '\n'))
				{
					i++;
				}
				
				// Back up to before the HTTP/#.# part:
				
				while(!(recv_buf[i] == ' '))
					i--;
				
				// Null-terminate the resource identifier:
				recv_buf[i] = '\0';
				
				j = strlen(content_length_header);
				
				for(; i < recv_buf_len; i++)
					if(strncmp(content_length_header, &(recv_buf[i]), j) == 0)
					{
						i += strlen(content_length_header);
						goto found_it;
					}
				
				err = 411;
				goto report_err;
found_it:
				data_len = atoi(&(recv_buf[i]));
				
				if(data_len + recv_buf_len >= RECV_BUF_MAX)
				{
					err = 507;
					goto report_err;
				}
				
				if(recv(
					client_socket, 
					&(recv_buf[recv_buf_len]), 
					data_len, 
					MSG_WAITALL) != data_len)
				{
					err = 500;
					goto report_err;
				}
				
				// Process the POST message:
				httpd_process_POST(
					&(recv_buf[5]), 
					&(recv_buf[recv_buf_len]), 
					data_len);
			}else
				err = 501;
			
			if(client_socket != -1)
			{
report_err:
				http_reply_err(err);
			}
		}else{
			switch(errno)
			{
				case EBADF:
				case EINVAL:
				case ENOTSOCK:
				case EOPNOTSUPP:
					close(sock);
					sock = -1;
				break;
				
				default:
				break;
			}
		}
	}
}

static void my_send(char *str)
{
	send(client_socket, str, strlen(str), 0);
}

static void send_std_headers(
				long content_length, 
				char *content_type)
{
	char	str[32];
	
	// FIXME: SEND DATE!
	my_send(
		(char *) "Connection: close\r\n");
	if(content_length > 0)
	{
		my_send(
			(char *) "Content-Length: ");
		sprintf(str, "%ld", content_length);
		my_send(str);
		my_send((char *) "\r\n"
			"Content-Type: ");
		my_send(content_type);
		my_send((char *) 
				"\r\n");
	}
	my_send((char *) "\r\n");
}

static char *http_err_html_begin = 
	(char *) "<HTML><BODY>There was an error. The error was:<BR><B>";
static char *http_err_html_end = 
	(char *) "</B></BODY></HTML>";

void http_reply_err(int err)
{
	char		*the_err;
	data_buffer	buf;
	if(client_socket == -1)
		return;
	
	my_send((char *) "HTTP/1.1 ");
	if(err >= 400 && err < 427)
		the_err = httpd_error_msg[0][err - 400];
	else if(err >= 500 && err < 527)
		the_err = httpd_error_msg[1][err - 500];
	else
		the_err = httpd_error_msg[1][500 - 500];
	my_send(the_err);
	my_send((char *) "\r\n");
	
	buf =
		data_buffer_cons_free(
			data_buffer_cons_free(
				data_buffer_create_copy(
					http_err_html_begin, 
					strlen(http_err_html_begin)),
				data_buffer_create_copy(
					the_err,
					strlen(the_err))),
			data_buffer_create_copy(
				http_err_html_end, 
				strlen(http_err_html_end)));
	
	send_std_headers(buf.len, (char *) "text/html; charset=UTF-8");
	my_send(buf.data);
	
	free(buf.data);
	
	close(client_socket);
	client_socket = -1;
}

void http_reply_data(char *data, long len, char *content_type)
{
	if(client_socket == -1)
		return;
	
	my_send((char *) "HTTP/1.1 200 OK\r\n");
	send_std_headers(len, content_type);
	
	send(client_socket, data, len, 0);
	
	close(client_socket);
	client_socket = -1;
}

/*
	Function to undo url-encoding. Modifies the input data.
	
	When done, the text will be a null-terminated string.
	
	Returns the length of the resulting string.
*/
static long un_url_encode(char *text, long max_len)
{
	long	i, len;
	
	len = 0;
	while(len < max_len && text[len] != '&')
		len++;
	
	// Null terminate:
	text[len] = '\0';
	
	// Go through it all:
	for(i = 0; i < len; i++)
	{
		// A %XX character description?
		if(text[i] == '%' && i + 2 < len)
		{
			unsigned char	c;
			long			j;
			
			// Decode what character is really supposed to be here:
			if(text[i+1] >= '0' && text[i+1] <= '9')
				c = (text[i+1] - '0');
			else if(text[i+1] >= 'a' && text[i+1] <= 'f')
				c = (text[i+1] - 'a' + 10);
			else if(text[i+1] >= 'A' && text[i+1] <= 'F')
				c = (text[i+1] - 'A' + 10);
			else
				c = 0;
			
			c = c << 4;
			
			if(text[i+2] >= '0' && text[i+2] <= '9')
				c = c | (text[i+2] - '0');
			else if(text[i+2] >= 'a' && text[i+2] <= 'f')
				c = c | (text[i+2] - 'a' + 10);
			else if(text[i+2] >= 'A' && text[i+2] <= 'F')
				c = c | (text[i+2] - 'A' + 10);
			else
				c = c | 0;
			
			// Put that character in:
			text[i] = c;
			
			// Move the string back 2 positions:
			for(j = i+1; j <= len-2; j++)
				text[j] = text[j+2];
			
			// The string is now shorter.
			len -= 2;
		}else if(text[i] == '+')
			// A '+' character? Means ' '.
			text[i] = ' ';
	}
	
	return len;
}

data_buffer httpd_post_data_get_data_for_input(
				char *input, 
				char *data, 
				long data_len)
{
	data_buffer	retval;
	long		i, inlen;
	
	retval.data = NULL;
	retval.len = 0;
	
	inlen = strlen(input);
	
	for(i = 0; i < data_len; i++)
		if(	strncmp(input, &(data[i]), inlen) == 0 &&
			data[i+inlen] == '=')
		{
			retval.data = &(data[i+inlen+1]);
			retval.len = un_url_encode(retval.data, data_len - (i+inlen+1)) + 1;
			goto done;
		}
	
done:
	return retval;
}
