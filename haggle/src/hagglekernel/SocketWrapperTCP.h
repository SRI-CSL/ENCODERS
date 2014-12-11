/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _SOCKETWRAPPERTCP_H
#define _SOCKETWRAPPERTCP_H

class SocketWrapperTCP;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>

#include "Protocol.h"
#include "Manager.h"
#include "SocketWrapper.h"

class SocketWrapperTCP : public SocketWrapper {

public:
    SocketWrapperTCP( 
        HaggleKernel *_kernel,
        Manager *_manager,
        SOCKET _sock = INVALID_SOCKET);

    ~SocketWrapperTCP();

    bool openLocalSocket(
        struct sockaddr *local_addr,
        socklen_t addrlen,
        bool registersock,
        bool nonblock);
};

#endif /* SOCKETWRAPPERTCP_H */
