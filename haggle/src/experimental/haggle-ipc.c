#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "haggle-ipc.h"

#define BUF_SIZE 2048

int haggle_ipc_create_socket(void)
{
	int sock;
	int ret;

	struct sockaddr_un un_addr;

	sock = socket(PF_LOCAL, SOCK_DGRAM, 0);

	if (sock < 0) {
		fprintf(stderr, "Could not create Haggle control socket\n");
		return -1;
	}
	memset(&un_addr, 0, sizeof(struct sockaddr_un));

	un_addr.sun_family = PF_LOCAL;
	strcpy(un_addr.sun_path, HAGGLE_CTRL_SOCKET_PATH);

	ret = bind(sock, (struct sockaddr *)&un_addr, sizeof(struct sockaddr_un));

	if (ret < 0) {
		fprintf(stderr, "Could not bind Haggle control socket\n");
		return -1;
	}

	return sock;	
}

int haggle_ipc_read(int sock)
{
	int len;
	char buf[BUF_SIZE];
	struct haggle_cmd *cmd;

	len = recv(sock, buf, BUF_SIZE, 0);
	
	printf("Read %d from socket %d \n", len, sock);
	
	cmd = (struct haggle_cmd *)buf;

	switch (cmd->type) {
	case HAGGLE_CMD_ADD_DO:
		printf("Add DO command\n");
		
		break;
	case HAGGLE_CMD_ADD_FAKE_EVENT:
		printf("Add Fake Event command\n");
		break;
	default:
		fprintf(stderr, "Unknown Haggle cmd %u\n", cmd->type);

	}
	return len;
}

void haggle_ipc_close_socket(int sock)
{
	printf ("Closed socket\n");
	close (sock);
	unlink(HAGGLE_CTRL_SOCKET_PATH);
}
