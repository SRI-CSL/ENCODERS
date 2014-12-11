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

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#if defined(OS_MACOSX)
#include <net/if_dl.h>
#include <sys/types.h>
#elif defined(OS_LINUX)
#include <net/ethernet.h>
#include <string.h>
#elif defined(OS_WINDOWS)
#include <windows.h>
#include <time.h>
#include <winioctl.h>
#include <winerror.h>
#include <iphlpapi.h>
#endif

#include "popd.h"
#include "databuf.h"

#include <stdio.h>

// Removes the define of printf from utils.h 
#undef printf 

#define ENABLE_POP3_CHANNEL_DEBUG 1

static SOCKET sock;
static SOCKET client_socket;

bool pop3_startup(int port)
{
	struct sockaddr_in my_addr;
	int optval;
	
	// Set up local port:
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	
	// Open a TCP socket:
	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock == -1)
		goto fail_socket;
	
	// Reuse address (less problems when shutting down/restarting server):
	optval = 1;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) == -1)
		goto fail_sockopt;
	
	// Bind the socket to the address:
	if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		goto fail_bind;
	
	// Listen on the socket:
	if (listen(sock, 20) == -1)
		goto fail_listen;
	
#if ENABLE_POP3_CHANNEL_DEBUG
	printf("pop server started on port %d\n", port);
#endif

	return true;
fail_listen:
fail_bind:
fail_sockopt:
	CLOSE_SOCKET(sock);
fail_socket:
	sock = -1;

	return false;
}

void pop3_shutdown(void)
{
	CLOSE_SOCKET(sock);
	sock = -1;
}

#if ENABLE_POP3_CHANNEL_DEBUG
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

static void debug_str(const char *prefix, const char *str)
{
	long i, len;
	
	len = strlen(str);

        // Capture return value in order to make gcc happy. 
	size_t n = fwrite(prefix, strlen(prefix), 1, stdout);

	if (n < 1)
		return;

	for (i = 0; i < len; i++) {
		if (str[i] < 0x20)
			n = fwrite((const void *) char_name[(unsigned char) (str[i])], 
				   strlen(char_name[(unsigned char) (str[i])]), 
				   1, 
				   stdout);
		else
			n = fwrite((const void *) &(str[i]), 1, 1, stdout);

		if (n < 1)
			return;
	}
	n = fwrite((char *) "\n", 1, 1, stdout);
}
#endif

static void my_send(const char *str)
{
	send(client_socket, str, strlen(str), 0);
#if ENABLE_POP3_CHANNEL_DEBUG
	debug_str("S: ", str);
#endif
}

static void my_send_l(const char *str, long len)
{
	send(client_socket, str, len, 0);
#if ENABLE_POP3_CHANNEL_DEBUG
	debug_str("S: ", (char *) "<message>");
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

void pop3_accept_connections(void)
{
	struct sockaddr_in addr;
	socklen_t addr_len;
	
	// Keep going while the socket is ok:
	while(sock != -1) {
#define MAX_LINE_LEN 100
		char line[MAX_LINE_LEN+1];
		char user_name[MAX_LINE_LEN];
		long cmd_len;
		bool got_user_name = false;
		bool not_done = true;
		enum {
			pop3_state_authorization,
			pop3_state_transaction,
			pop3_state_update
		} current_state;
		addr_len = sizeof(addr);

#if ENABLE_POP3_CHANNEL_DEBUG
		printf("Waiting for pop connections\n");
#endif

		client_socket = accept(sock, (struct sockaddr *)&addr, &addr_len);
		// Should really fork() here, or create a separate thread, but in the
		// interest of making a lightweight implementation:
#if ENABLE_POP3_CHANNEL_DEBUG
		printf("pop connection from %s:%d\n", ip_to_str(addr.sin_addr), addr.sin_port);
#endif

		if (client_socket == -1) {
			switch (ERRNO) {
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
			continue;
		}

		// check address and make sure it's local. 
		if (((struct sockaddr_in *)&addr)->sin_addr.s_addr != htonl(0x7F000001))
		{
			// Otherwise: -ERR/disconnect
#if ENABLE_POP3_CHANNEL_DEBUG
			printf((char *) "Rejecting connection attempt from %s\n",
				inet_ntoa(((struct sockaddr_in *)(&addr))->sin_addr));
#endif
			my_send((char *) "-ERR Bad originating IP address\r\n");
		} else {
			my_send((char *) "+OK Haggle POP3 server ready\r\n");
		}

		current_state = pop3_state_authorization;

		while(not_done)
		{
			bool line_not_read;
			bool line_was_too_long;

			// Read a line:
read_another_line:
			line_was_too_long = false;

read_next_line:
			cmd_len = 0;
			line_not_read = true;
			// Repeat until an entire line has been read
			do {
				ssize_t			i;
				int				ret;
				fd_set			read_set, exception_set;
				struct timeval	timeout;
				
				FD_ZERO(&read_set);
				FD_ZERO(&exception_set);
				FD_SET(client_socket, &read_set);
				FD_SET(client_socket, &exception_set);
				// Inactivity timeout value from POP3 RFC
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
#if ENABLE_POP3_CHANNEL_DEBUG
					printf("10 Minute inactivity - automatic logoff.\n");
#endif
					// Inactivity for 10 minutes - automatic logoff.
					goto fail_sock_err;
				}
				if(ret == -1)
				{
#if ENABLE_POP3_CHANNEL_DEBUG
					printf("Socket error\n");
#endif
					// Socket error, terminate.
					goto fail_sock_err;
				}
				if(FD_ISSET(client_socket, &exception_set))
				{
#if ENABLE_POP3_CHANNEL_DEBUG
					printf("Socket exception.\n");
#endif
					// Client closed socket
					goto fail_sock_err;
				}
				if(FD_ISSET(client_socket, &read_set))
				{
					i = recv(client_socket, 
						&(line[cmd_len]), 
						1, 
#if defined(OS_WINDOWS)
						0
#else
						MSG_WAITALL
#endif
						);
					if (i == 0) {

#if ENABLE_POP3_CHANNEL_DEBUG
						printf("Client disconnected\n");
#endif
						goto client_disconnect;
					}
					if (i != 1)
					{
#if ENABLE_POP3_CHANNEL_DEBUG
						printf("Socket error when reading.\n");
						
#endif
						// Socket error, terminate.
						goto fail_sock_err;
					}
					cmd_len++;
					// End of terminal? That means the other side quit.
					if (line[cmd_len-1] == 4)
					{
#if ENABLE_POP3_CHANNEL_DEBUG
						printf("End of terminal character read.\n");
#endif
						goto fail_sock_err;
					}

					// End of line?
					if (line[cmd_len-2] == '\r' && line[cmd_len-1] == '\n')
						line_not_read = false;

					// Line too long?
					if (cmd_len >= MAX_LINE_LEN)
					{
#if ENABLE_POP3_CHANNEL_DEBUG
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
			} while (line_not_read);

			// null-terminate the line.
			line[cmd_len] = '\0';
#if ENABLE_POP3_CHANNEL_DEBUG
			debug_str((char *) "C: ", line);
#endif
			// If this is the end of a line that was too long, go back
			// and read the next line, ignoring this one.
			if (line_was_too_long)
			{
				// Tell the client
				my_send((char *) "-ERR Line too long\r\n");
				goto read_another_line;
			}

			if (current_state == pop3_state_authorization)
			{
				if (test_cmd((char *) "QUIT", line))
				{
					not_done = false;
					my_send((char *) "+OK Haggle POP3 server signing off\r\n");
				} else if (test_cmd((char *) "USER", line))
				{
					long	i;

					for(i = 5; i < cmd_len-2; i++)
						user_name[i-5] = line[i];
					user_name[i-5] = '\0';
#if ENABLE_POP3_CHANNEL_DEBUG
					printf((char *) "User \"%s\" logged on.\n", user_name);
#endif
					got_user_name = true;

					my_send((char *) "+OK\r\n");
				} else if (test_cmd((char *) "PASS", line))
				{
					if (got_user_name)
					{
						if(pop3_login(user_name, &(line[5])))
						{
							my_send((char *) "+OK Welcome\r\n");
							current_state = pop3_state_transaction;
						}else{
							my_send((char *) "-ERR Access denied.\r\n");
							not_done = false;
						}
					} else {
						my_send((char *) "-ERR no username specified\r\n");
					}
				} else{
					my_send((char *) "-ERR\r\n");
				}
			} else if (current_state == pop3_state_transaction)
			{
				if (test_cmd((char *) "QUIT", line))
				{
					my_send((char *) "+OK\r\n");
					current_state = pop3_state_update;
				} else if (test_cmd((char *) "STAT", line))
				{
					// Return number of messages for this user
					long num, i, size;
					char str[64];

					num = pop3_list(user_name);
					size = 0;
					for (i = 0; i < num; i++)
					{
						size += pop3_message_size(user_name, i);
					}
					sprintf(str, (char *) "+OK %ld %ld\r\n", num, size);
					my_send(str);
				} else if (test_cmd((char *) "LIST", line))
				{
					// List all messages for the user
					long	num, i;
					char	str[64];
					
					if(cmd_len > 6)
					{
						i = atoi(&(line[5]))-1;
						num = pop3_message_size(user_name, i);
						if(num != 0)
						{
							sprintf(str, (char *) "+OK %ld %ld\r\n", i+1, num);
							my_send(str);
						}else{
							my_send("-ERR no such message in mailbox.\r\n");
						}
					}else{
						num = pop3_list(user_name);
						sprintf(str, (char *) "+OK %ld messages\r\n", num);
						my_send(str);
						for (i = 0; i < num; i++)
						{
							sprintf(str,
								(char *) "%ld %ld\r\n", 
								i+1, 
								pop3_message_size(user_name, i));
							
							my_send(str);
						}
						my_send((char *) ".\r\n");
					}
				} else if (test_cmd((char *) "RETR", line))
				{
					// Retrieve message:
					long	num, len;
					char	*data;

					num = atoi(&(line[5]))-1;
					data = pop3_get_message(user_name, num);
					if (data != NULL)
					{
						char str[64];
						
						len = strlen(data);
						sprintf(str, (char *) "+OK %ld octets\r\n", len);
						my_send(str);
						my_send_l(data, len);
						if(len < 2 || 
							data[len-2] != '\r' || 
							data[len-1] != '\n')
							my_send((char *) "\r\n.\r\n");
						else
							my_send((char *) ".\r\n");

						free(data);
					} else
						my_send((char *) "-ERR no such message\r\n");
				} else if (test_cmd((char *) "TOP", line))
				{
					// Retrieve top of message:
					long	num, lines = 0, i;
					bool	parsed_ok;
					
					num = atoi(&(line[4]))-1;
					parsed_ok = false;
					for(i = 4; i < cmd_len && !parsed_ok; i++)
						if(line[i] == ' ')
						{
							lines = atoi(&(line[i+1]))-1;
							parsed_ok = true;
						}
					
					if(parsed_ok)
					{
						char	*data;
						long	len;
						
						data = pop3_get_top_of_message(user_name, num, lines);
						if (data != NULL)
						{
							char str[64];

							len = strlen(data);
							sprintf(str, (char *) "+OK %ld octets\r\n", len);
							my_send(str);
							my_send_l(data, len);
							if(len < 2 || 
								data[len-2] != '\r' || 
								data[len-1] != '\n')
								my_send((char *) "\r\n.\r\n");
							else
								my_send((char *) ".\r\n");

							free(data);
						} else
							my_send((char *) "-ERR no such message\r\n");
					}else
						my_send((char *) "-ERR unable to parse command\r\n");
				} else if (test_cmd((char *) "UIDL", line))
				{
					if (cmd_len < 7)
					{
						// UIDL listing:
						long	i, num;

						num = pop3_list(user_name);
						my_send((char *)"+OK unique-id listing follows\r\n");
						for (i = 0; i < num; i++)
						{
							char	*uidl;
							char	str[100];

							uidl = pop3_get_UIDL(user_name, i);
							if(uidl != NULL)
							{
								sprintf(str, "%ld %s\r\n", i+1, uidl);
								my_send(str);
							}
						}
						my_send((char *) ".\r\n");
					} else {
						// UIDL specific message listing:
						char *uidl;
						long num;
						char str[100];

						num = atoi(&(line[5]))-1;
						uidl = pop3_get_UIDL(user_name, num);
						if (uidl == NULL)
						{
							my_send((char *)"-ERR no such message\r\n");
						} else {
							sprintf(str, "+OK %ld %s\r\n", num+1, uidl);
							my_send(str);
						}
					}
				} else if (test_cmd((char *) "DELE", line))
				{
					// _Mark_ message to be deleted
					if (pop3_delete_message(user_name, atoi(&(line[5]))-1))
						my_send((char *) "+OK\r\n");
					else
						my_send((char *) "-ERR no such message\r\n");
				} else if (test_cmd((char *) "NOOP", line))
				{
					my_send((char *) "+OK\r\n");
				} else if (test_cmd((char *) "RSET", line))
				{
					// _Mark_ all messages to not be deleted
					if (pop3_undelete_messages(user_name))
						my_send((char *) "+OK\r\n");
					else
						my_send((char *) "-ERR no such message\r\n");
				} else {
					my_send((char *) "-ERR\r\n");
				}
			} else if (current_state == pop3_state_update)
			{
				if (test_cmd((char *) "QUIT", line))
				{
					not_done = false;
					my_send((char *) "+OK Haggle POP3 server signing off\r\n");
				} else {
					my_send((char *) "-ERR\r\n");
				}
			} else {
				my_send((char *) "-ERR Unknown POP3 state.\r\n");
			}
		}

		if (0)
		{
fail_sock_err:;
		client_disconnect:
#if ENABLE_POP3_CHANNEL_DEBUG
			printf((char *) "Client socket disconnected.\n");
#endif
		}
		CLOSE_SOCKET(client_socket);
		pop3_delete_messages(user_name);
	} 
}
