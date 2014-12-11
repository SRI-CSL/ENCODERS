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

#include "smtpd.h"
#include "databuf.h"
#include "smtpserver.h"

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
#include <time.h>
#include <winioctl.h>
#include <winerror.h>
#include <iphlpapi.h>
#endif

// Removes the define of printf from utils.h 
#undef printf 

#define ENABLE_SMTP_CHANNEL_DEBUG 0

static SOCKET sock;
static SOCKET client_socket;

bool smtp_startup(int port)
{
	struct sockaddr_in my_addr;
	int optval;
	
	// Set up local port:
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	
	// Open a TCP socket:
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		goto fail_socket;
	
	// Reuse address (less problems when shutting down/restarting server):
	optval = 1;
	if(	setsockopt(
			sock, 
			SOL_SOCKET, 
			SO_REUSEADDR, 
			(char *)&optval, 
			sizeof(optval)) == -1)
		goto fail_sockopt;

	// Bind the socket to the address:
	if(bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		goto fail_bind;
	
	// Listen on the socket:
	if(listen(sock, 20) == -1)
		goto fail_listen;
	
	return true;
fail_listen:
fail_bind:
fail_sockopt:
	CLOSE_SOCKET(sock);
fail_socket:
	sock = -1;
	return false;
}

void smtp_shutdown(void)
{
	CLOSE_SOCKET(sock);
	sock = -1;
	if(client_socket != -1)
		CLOSE_SOCKET(client_socket);
}

#if ENABLE_SMTP_CHANNEL_DEBUG
static char *char_name[32] =
	{
		(char *) "<NUL>",
		(char *) "<SOH>",
		(char *) "<STX>",
		(char *) "<ETX>",
		(char *) "<EOT>",
		(char *) "<ENQ>",
		(char *) "<ACK>",
		(char *) "<BEL>",
		(char *) "<BS>",
		(char *) "<HT>",
		(char *) "<LF>",
		(char *) "<VT>",
		(char *) "<NP>",
		(char *) "<CR>",
		(char *) "<SO>",
		(char *) "<SI>",
		(char *) "<DLE>",
		(char *) "<DC1>",
		(char *) "<DC2>",
		(char *) "<DC3>",
		(char *) "<DC4>",
		(char *) "<NAK>",
		(char *) "<SYN>",
		(char *) "<ETB>",
		(char *) "<CAN>",
		(char *) "<EM>",
		(char *) "<SUB>",
		(char *) "<ESC>",
		(char *) "<FS>",
		(char *) "<GS>",
		(char *) "<RS>",
		(char *) "<US>"
	};

static void debug_str(char *prefix, char *str)
{
	long	i, len;
	
	len = strlen(str);
	fwrite(prefix, strlen(prefix), 1, stdout);
	for(i = 0; i < len; i++)
	{
		if(str[i] < 0x20)
			fwrite(
				(const void *) char_name[(unsigned char) (str[i])], 
				strlen(char_name[(unsigned char) (str[i])]), 
				1, 
				stdout);
		else
			fwrite((const void *) &(str[i]), 1, 1, stdout);
	}
	fwrite((char *) "\n", 1, 1, stdout);
}
#endif

static void my_send(char *str)
{
	send(client_socket, str, strlen(str), 0);
#if ENABLE_SMTP_CHANNEL_DEBUG
	debug_str((char *) "S: ", str);
#endif
}

/*
	This takes a string in UPPERCASE, and a string and checks to see if either 
	the uppercase or lowercase version of that string begins the other string.
*/
static bool test_cmd(char *cmd_str, char *line)
{
	unsigned long	len, i;
	
	len = strlen(cmd_str);
	if(len > strlen(line))
	{
		return false;
	}
	
	for(i = 0; i < len; i++)
	{
		if(cmd_str[i] >= 'A' && cmd_str[i] <= 'Z')
		{
			// Check both uppercase and lowercase values:
			if(	line[i] == cmd_str[i] ||
				line[i] == (cmd_str[i] - 'A' + 'a'))
				;
			else
				return false;
		}else if(cmd_str[i] != line[i])
			return false;
	}
	return true;
}

void smtp_accept_connections(void)
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
#define MAX_LINE_LEN	100
			char	line[MAX_LINE_LEN+1];
			char	from_addr[MAX_LINE_LEN];
			char	to_addr[MAX_LINE_LEN];
			bool	not_done;
			long	cmd_len;
			bool	is_bad_user;
			
			bool	has_received_hello;
			bool	has_received_from;
			bool	has_received_to;
			
			is_bad_user = false;
			// check address and make sure it's local. 
			if(((struct sockaddr_in *)&addr)->sin_addr.s_addr != htonl(0x7F000001))
			{
				// Otherwise: 554/disconnect
#if ENABLE_SMTP_CHANNEL_DEBUG
				printf((char *) "Rejecting connection attempt from %s\n",
					inet_ntoa(((struct sockaddr_in *)(&addr))->sin_addr));
#endif
				my_send((char *) "554 Haggle SMTP server: bad client IP address\r\n");
				is_bad_user = true;
			}else{
				my_send((char *) "220 Haggle SMTP server ready\r\n");
			}
			not_done = true;
			has_received_hello = false;
			has_received_from = false;
			has_received_to = false;
			while(not_done)
			{
				ssize_t	i;
				bool	line_not_read;
				bool	line_was_too_long;
				
				// Read a line:
read_another_line:
				line_was_too_long = false;
				
read_next_line:
				cmd_len = 0;
				line_not_read = true;
				// Repeat until an entire line has been read
				do{
					int				ret;
					fd_set			read_set, exception_set;
					struct timeval	timeout;
					
					FD_ZERO(&read_set);
					FD_ZERO(&exception_set);
					FD_SET(client_socket, &read_set);
					FD_SET(client_socket, &exception_set);
					// Inactivity timeout value from SMTP RFC
					// This is the largest minimum timeout value I could find in
					// the RFC
					timeout.tv_sec = 10*60;
					timeout.tv_usec = 0;
					
					// Use select to wait for more data:
					ret = 
						select(
							client_socket+1, 
							&read_set, 
							NULL, 
							&exception_set, 
							&timeout);
					if(ret == 0)
					{
#if ENABLE_SMTP_CHANNEL_DEBUG
						printf("10 Minute inactivity - automatic logoff.\n");
#endif
						// Inactivity for 10 minutes - automatic logoff.
						goto fail_sock_err;
					}
					if(ret == -1)
					{
#if ENABLE_SMTP_CHANNEL_DEBUG
						printf("Socket error\n");
#endif
						// Socket error, terminate.
						goto fail_sock_err;
					}
					if(FD_ISSET(client_socket, &exception_set))
					{
#if ENABLE_SMTP_CHANNEL_DEBUG
						printf("Socket exception.\n");
#endif
						// Client closed socket
						goto fail_sock_err;
					}
					if(FD_ISSET(client_socket, &read_set))
					{
						i = recv(
								client_socket, 
								&(line[cmd_len]), 
								1, 
#if defined(OS_WINDOWS)
								0
#else
								MSG_WAITALL
#endif
								);
						if(i != 1)
						{
#if ENABLE_SMTP_CHANNEL_DEBUG
							printf("Socket error when reading.\n");
#endif
							// Socket error, terminate.
							goto fail_sock_err;
						}
						cmd_len++;
						// End of terminal? That means the other side quit.
						if(line[cmd_len-1] == 4)
						{
#if ENABLE_SMTP_CHANNEL_DEBUG
							printf("End of terminal character read.\n");
#endif
							goto fail_sock_err;
						}
						
						// End of line?
						if(	line[cmd_len-2] == '\r' &&
							line[cmd_len-1] == '\n')
							line_not_read = false;
						// Line too long?
						if(cmd_len >= MAX_LINE_LEN)
						{
#if ENABLE_SMTP_CHANNEL_DEBUG
							line[cmd_len] = '\0';
							debug_str((char *) "C: ", line);
#endif
							// Tell ourselves not to process the next line as a 
							// command
							line_was_too_long = true;
							// Go back and start reading the rest of the line
							goto read_next_line;
						}
					}
				}while(line_not_read);
				
				// null-terminate the line.
				line[cmd_len] = '\0';
#if ENABLE_SMTP_CHANNEL_DEBUG
				debug_str((char *) "C: ", line);
#endif
				// If this is the end of a line that was too long, go back
				// and read the next line, ignoring this one.
				if(line_was_too_long)
				{
					// Tell the client
					my_send((char *) "500 Line too long\r\n");
					goto read_another_line;
				}
				
				if(test_cmd((char *) "QUIT", line))
				{
					// Client is quitting.
					my_send((char *) 
						"221 Haggle SMTP server closing connection\r\n");
					not_done = false;
				}else if(is_bad_user)
				{
					my_send((char *) "503 bad sequence of commands\r\n");
				}else if(test_cmd((char *) "EHLO", line))
				{
					// Client is saying hi.
					has_received_hello = true;
					my_send((char *) "250 Welcome to Haggle mail service.\r\n");
				}else if(test_cmd((char *) "HELO", line))
				{
					// Client is saying hi. Old style.
					has_received_hello = true;
					my_send((char *) "250 Welcome to Haggle mail service.\r\n");
				}else if(test_cmd((char *) "MAIL FROM:", line))
				{
					// This is the start of a new email message
					if(!has_received_hello || has_received_from)
					{
						my_send((char *) "503 Bad sequence of commands\r\n");
					}else{
						long	i, j;
						
						i = 0;
						while(line[i] != '<' && line[i] != '\0')
							i++;
						if(line[i] == '\0')
							goto mail_from_syntax_error;
						i++;
						j = 0;
						do{
							from_addr[j] = line[i];
							i++;
							j++;
						}while(line[i] != '>' && line[i] != '\0');
						from_addr[j] = '\0';
						if(line[i] == '\0')
						{
mail_from_syntax_error:
							my_send((char *) "500 SYNTAX ERROR\r\n");
						}else{
							my_send((char *) "250 OK, go ahead\r\n");
							has_received_from = true;
						}
					}
				}else if(test_cmd((char *) "RCPT TO:", line))
				{
					// This is the start of a new email message
					if(!has_received_hello || !has_received_from)
					{
						my_send((char *) "503 Bad sequence of commands\r\n");
					}else if(has_received_to)
					{
						my_send((char *) "452 Too many recipients\r\n");
					}else{
						long	i, j;
						
						i = 0;
						while(line[i] != '<' && line[i] != '\0')
							i++;
						if(line[i] == '\0')
							goto rcpt_to_syntax_error;
						i++;
						j = 0;
						do{
							to_addr[j] = line[i];
							i++;
							j++;
						}while(line[i] != '>' && line[i] != '\0');
						to_addr[j] = '\0';
						if(line[i] == '\0')
						{
rcpt_to_syntax_error:
							my_send((char *) "500 SYNTAX ERROR\r\n");
						}else{
							my_send((char *) "250 OK, go ahead\r\n");
							has_received_to = true;
						}
					}
				}else if(test_cmd((char *) "DATA", line))
				{
					if(	!has_received_hello ||
						!has_received_from ||
						!has_received_to)
					{
						my_send((char *) "503 Bad sequence of commands\r\n");
					}else if(strlen(line) == 4 + 2)
					{
						char	*message;
						long	message_allocated_len;
						long	i;
						bool	more_data;
						
						my_send((char *) 
							"354 Start mail input; "
							"end with <CRLF>.<CRLF>\r\n");
						
						message = NULL;
						message_allocated_len = 0;
						i = 0;
						more_data = true;
						do{
							if(i == message_allocated_len)
							{
								char	*tmp;
								
								tmp = (char *)
									realloc(
										message, 
										message_allocated_len+10);
								if(tmp != NULL)
								{
									message = tmp;
									message_allocated_len = 
										message_allocated_len + 10;
								}else{
									my_send((char *) 
										"452 Requested action not taken: "
										"insufficient system storage\r\n");
									free(message);
									goto message_alloc_failed;
								}
							}
							
							if(recv(
								client_socket, 
								&(message[i]), 
								1, 
#ifdef OS_WINDOWS
								0
#else
								MSG_WAITALL
#endif
								) != 1)
							{
								free(message);
								goto fail_sock_err;
							}
							
							i++;
							if(	message[i-5] == '\r' &&
								message[i-4] == '\n' &&
								message[i-3] == '.' &&
								message[i-2] == '\r' &&
								message[i-1] == '\n')
							{
								message[i-3] = '\0';
								message_allocated_len = i-4;
								more_data = false;
							}
						}while(more_data);
						
						smtp_new_email(
							from_addr, 
							to_addr, 
							message, 
							message_allocated_len);
						my_send((char *) "250 OK, message sent\r\n");
						free(message);
						has_received_from = false;
						has_received_to = false;
message_alloc_failed:;
					}else{
						my_send((char *) "500 SYNTAX ERROR\r\n");
					}
				}else if(test_cmd((char *) "NOOP", line))
				{
					my_send((char *) "250 OK, go ahead\r\n");
				}else if(test_cmd((char *) "VRFY", line))
				{
					my_send((char *) "553 Unable to verify\r\n");
				}else if(test_cmd((char *) "RSET", line))
				{
					my_send((char *) "250 OK\r\n");
					has_received_from = false;
					has_received_to = false;
				}else{
					my_send((char *) "502 Command not implemented\r\n");
				}
				
			}
fail_sock_err:
#if ENABLE_SMTP_CHANNEL_DEBUG
			printf("Client socket disconnected.\n");
#endif
			CLOSE_SOCKET(client_socket);
			client_socket = -1;
		}else{
			switch(ERRNO)
			{
#if !defined(OS_WINDOWS_DESKTOP)
				case EBADF:
				case EINVAL:
#endif
				case ENOTSOCK:
				case EOPNOTSUPP:
					CLOSE_SOCKET(sock);
					sock = -1;
				break;
				
				default:
				break;
			}
		}
	}
}
