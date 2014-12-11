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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "haggle-udp.h"

#define BUF_SIZE 2048

int haggle_udp_create_socket(void)
{
	int sock = 0;
	int ret = 0;
	int reuse_addr = 1;
	struct sockaddr_in in_addr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		fprintf(stderr, "Could not create Haggle udp socket\n");
		return -1;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

	memset(&in_addr, 0, sizeof(struct sockaddr_in));

	in_addr.sin_family = AF_INET;
	in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	in_addr.sin_port = htons(HAGGLE_UDP_SOCKET_PORT);

	ret = bind(sock, (struct sockaddr *)&in_addr, sizeof(struct sockaddr_in));

	if (ret < 0) {
		fprintf(stderr, "Could not bind Haggle control socket\n");
		return -1;
	}

	return sock;	
}

int haggle_udp_read(int sock)
{
	int len;
	char buf[BUF_SIZE];
	struct haggle_udp *udp;

	len = recv(sock, buf, BUF_SIZE, 0);
	
	printf("Read %d from socket %d \n", len, sock);
	
	udp = (struct haggle_udp *)buf;

	switch (udp->type) {
	case HAGGLE_UDP_TYPE_DISCOVER:
		printf("got DISCOVER message\n");
		
		break;
	case HAGGLE_UDP_TYPE_DATA:
		printf("Data\n");
		break;
	default:
		fprintf(stderr, "Unknown Haggle udp %u\n", udp->type);

	}
	return len;
}

void haggle_udp_close_socket(int sock)
{
	printf ("Closed socket\n");
	close (sock);
	// unlink(HAGGLE_UDP_SOCKET_PORT);
}


int haggle_udp_sendDiscovery(int sock) 
{
	int ret = 0;
	struct haggle_udp hudp;
	struct sockaddr_in in_addr;
	
	hudp.type = htons(HAGGLE_UDP_TYPE_DISCOVER);
	hudp.len = 0;
	hudp.data[0] = 0;

	in_addr.sin_family = AF_INET;
	in_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	in_addr.sin_port = htons(HAGGLE_UDP_SOCKET_PORT);

	ret = sendto(sock, &hudp, sizeof(struct haggle_udp), 0, (struct sockaddr *)&in_addr, sizeof(struct sockaddr_in));

	if (ret < 0) {
		perror("Sendto failed");
      	}

	printf("SEND UDP DISCOVERY\n");

	// add new discovery event to event queue
	// Event *e = new Event(EVENT_TYPE_DISCOVERY_EVENT, 1.0, &haggle_udp_sendDiscovery, &sock);
	// addEvent(e);


	return ret;

}
