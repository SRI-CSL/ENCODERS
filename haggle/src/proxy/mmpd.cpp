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

/*
	Man in the Middle Protocol Dumper
	
	Minimalistic implementation.
*/

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
#include <netdb.h>
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

#include <utils.h>
#include <stdio.h>

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

static void debug_str(const char *prefix, char *str)
{
	long i, len;
        size_t count;
	
	len = strlen(str);

	count = fwrite(prefix, strlen(prefix), 1, stdout);

	if (count < 1)
		return;

	for (i = 0; i < len; i++) {
		if (str[i] < 0x20)
			count = fwrite((const void *) char_name[(unsigned char) (str[i])], 
                                       strlen(char_name[(unsigned char) (str[i])]), 
                                       1, 
                                       stdout);
		else
			count = fwrite((const void *) &(str[i]), 1, 1, stdout);

		if (count < 1)
			return;
	}
	count = fwrite((char *) "\n", 1, 1, stdout);
}

int accept_socket, client_socket, server_socket;
#define MAX_TEXT 4096
char current_text[MAX_TEXT];
long len;
int printing_socket = -1;

void dump_now(void)
{
	if(len == 0)
		return;
	
	current_text[len] = '\0';
	if(printing_socket == server_socket)
		debug_str("S: ", current_text);
	else if(printing_socket == client_socket)
		debug_str("C: ", current_text);
	else
		debug_str("*: ", current_text);
	len = 0;
	printing_socket = -1;
}

int read_write(int read_socket, int write_socket)
{
	int	retval;
	
	retval = 
		recv(read_socket, 
			&(current_text[len]), 
			1, 
#if defined(OS_WINDOWS)
			0
#else
			MSG_WAITALL
#endif
			);
	if(retval < 1)
		return retval;
	
	retval = 
		send(write_socket, 
			&(current_text[len]), 
			1, 
#if defined(OS_WINDOWS)
			0
#else
			MSG_WAITALL
#endif
			);
	
	return retval;
}

int main(int argc, char *argv[])
{
	// Default values - should be overwritten:
	struct sockaddr_in	my_addr;
	struct sockaddr_in	server_addr;
	struct sockaddr_in	addr;
	socklen_t			addr_len;
	int					optval;
	long				i;
	bool				not_done = true;
	
	// Set up local address:
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(0);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	
	// Set up server address:
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(0);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
	
	for(i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "-s") == 0)
		{
			struct hostent *he;
			
			he = gethostbyname(argv[i+1]);
			if(he)
				server_addr.sin_addr = *((struct in_addr *)he->h_addr);
			i++;
		}
		if(strcmp(argv[i], "-p") == 0)
		{
			server_addr.sin_port = htons(atoi(argv[i+1]));
			i++;
		}
		if(strcmp(argv[i], "-P") == 0)
		{
			my_addr.sin_port = htons(atoi(argv[i+1]));
			i++;
		}
	}
	
	// Check that we were set up correctly
	if(server_addr.sin_addr.s_addr == htonl(INADDR_ANY))
	{
		printf("Server address not set. Please specify server address using -s <server name>.\n");
		goto fail_param;
	}
	
	if(server_addr.sin_port == htons(0))
	{
		printf("Server port not set. Please specify server port using -p <number>.\n");
		goto fail_param;
	}
	
	if(my_addr.sin_port == htons(0))
	{
		printf("Local port not set. Please specify local port using -P <number>.\n");
		goto fail_param;
	}
	
	// Open a local TCP socket:
	accept_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(accept_socket == -1)
		goto fail_socket;
	
	// Reuse address (less problems when shutting down/restarting server):
	optval = 1;
	if (setsockopt(accept_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) == -1)
		goto fail_sockopt;
	
	// Bind the socket to the address:
	if (bind(accept_socket, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
		goto fail_bind;
	
	// Listen on the socket:
	if (listen(accept_socket, 10) == -1)
		goto fail_listen;
	
	do{
		printf("Accepting connection at %s:%d\n", inet_ntoa(my_addr.sin_addr), ntohs(my_addr.sin_port));
		
		addr_len = sizeof(addr);
		// Accept a client:
		client_socket = accept(accept_socket, (struct sockaddr *)&addr, &addr_len);
		if(client_socket == -1)
			goto fail_accept;
		
		printf("Opening connection to %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
		// Open a TCP socket:
		server_socket = socket(AF_INET, SOCK_STREAM, 0);

		if(server_socket == -1)
			goto fail_socket_2;
		
		// Bind the socket to the address:
		if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
			goto fail_connect;
		
		len = 0;
		not_done = true;
		do{
			fd_set	read_set, exception_set;
			int		retval;
			
			FD_ZERO(&read_set);
			FD_ZERO(&exception_set);
			
			FD_SET(server_socket, &read_set);
			FD_SET(client_socket, &read_set);
			FD_SET(server_socket, &exception_set);
			FD_SET(client_socket, &exception_set);
			
			retval =
				select(
					(server_socket>client_socket?server_socket:client_socket)+1,
					&read_set,
					NULL,
					&exception_set,
					NULL);
			if(retval == -1)
			{
				switch(errno)
				{
					case EINTR:
					break;
					
					default:
						not_done = false;
					break;
				}
			}else{
				if(FD_ISSET(server_socket, &exception_set))
				{
					dump_now();
					printf("Server connection closed\n");
					not_done = false;
				}else if(FD_ISSET(client_socket, &exception_set))
				{
					dump_now();
					printf("Client connection closed\n");
					not_done = false;
				}else if(FD_ISSET(server_socket, &read_set))
				{
					if(printing_socket == client_socket)
						dump_now();
					printing_socket = server_socket;
					retval = read_write(server_socket, client_socket);
					if(retval < 1)
					{
						dump_now();
						printf("Server read error\n");
						not_done = false;
					}else{
						len++;
						if(current_text[len-1] == '\n')
						{
							dump_now();
						}else if(current_text[len-1] == 4)
						{
							// End of terminal:
							not_done = false;
							dump_now();
							printf("Server closed connection\n");
						}
					}
				}else if(FD_ISSET(client_socket, &read_set))
				{
					if(printing_socket == server_socket)
						dump_now();
					printing_socket = client_socket;
					retval = read_write(client_socket, server_socket);
					if(retval < 1)
					{
						dump_now();
						printf("Client read error\n");
						not_done = false;
					}else{
						len++;
						if(current_text[len-1] == '\n')
						{
							dump_now();
						}else if(current_text[len-1] == 4)
						{
							// End of terminal:
							not_done = false;
							dump_now();
							printf("Client closed connection\n");
						}
					}
				}
			}
		}while(not_done);
		
		CLOSE_SOCKET(server_socket);
		CLOSE_SOCKET(client_socket);
	}while(true);
	
	CLOSE_SOCKET(accept_socket);
	return 0;
	
	if(0)
fail_connect:
		printf("server socket connect() failed!\n");
	CLOSE_SOCKET(server_socket);
	if(0)
fail_socket_2:
		printf("server socket socket() failed!\n");
	CLOSE_SOCKET(client_socket);
	if(0)
fail_accept:
		printf("accept() failed!\n");
	if(0)
fail_listen:
		printf("accept socket listen() failed!\n");
	if(0)
fail_bind:
		printf("accept socket bind() failed!\n");
	if(0)
fail_sockopt:
		printf("accept socket setsockopt() failed!\n");
	CLOSE_SOCKET(accept_socket);
	if(0)
fail_socket:
		printf("accept socket socket() failed!\n");
fail_param:

	return 1;
}
