/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "SocketWrapperTCP.h"

SocketWrapperTCP::SocketWrapperTCP(
    HaggleKernel *_kernel,
    Manager *_manager,
    SOCKET _sock)
    :   SocketWrapper(_kernel, _manager, _sock) 
{
}

bool SocketWrapperTCP::openLocalSocket(
    struct sockaddr *local_addr,
    socklen_t addrlen,
    bool registersock,
    bool nonblock)
{
    int optval = 1;

    if (isConnected()) {
        HAGGLE_ERR("Already connected\n");
        return true;
    }

    if (!openSocket(
            local_addr->sa_family, 
            SOCK_STREAM, 
            IPPROTO_TCP, 
            registersock, 
            nonblock)) {
        HAGGLE_ERR("Could not create TCP socket\n");
        return false;
	}

    if (!setSocketOption(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
        closeSocket();
        HAGGLE_ERR("setsockopt SO_REUSEADDR failed\n");
        return false;
    }

    if (!setSocketOption(SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval))) {
        closeSocket();
        HAGGLE_ERR("setsockopt SO_KEEPALIVE failed\n");
        return false;
    }

    if (!bind(local_addr, addrlen)) {
        closeSocket();
        HAGGLE_ERR("Could not bind TCP socket\n");
        return false;
    }
  
    return true;
}
