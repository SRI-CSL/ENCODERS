/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _SOCKETWRAPPER_H
#define _SOCKETWRAPPER_H

class SocketWrapper;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>

#include "Protocol.h" 
#include "Manager.h"

class SocketWrapper {
    HaggleKernel *kernel;
    Manager *manager;
    bool registered;
    bool connected;
    bool nonblock;
    bool bound;
    bool listening;
    Mutex proxyLock;
protected:
    SOCKET sock;
    void setRegistered(bool _registered);
    bool setNonblock(bool _nonblock);
    void setBound(bool _bound);
    void setListening(bool _listening);

    virtual bool openSocket(
        int domain, 
        int type, 
        int protocol, 
        bool registersock, 
        bool nonblock);

    bool isRegistered();
    bool isNonblock();
    bool isBound();
    bool isListening();

public:
    SocketWrapper( 
        HaggleKernel *_kernel,
        Manager *_manager,
        SOCKET _sock = INVALID_SOCKET); 

    ~SocketWrapper();

    void setConnected(bool _connected);

    bool registerSocket();

    bool multiplySndBufferSize(int factor);
    bool multiplyRcvBufferSize(int factor);

    SOCKET getSOCKET();

	bool isConnected();

	bool isOpen();

    void closeSocket();

    ProtocolEvent openConnection(
        const struct sockaddr *saddr, 
        socklen_t addrlen);

    bool bind(const struct sockaddr *saddr, socklen_t addrlen);
        
    virtual SOCKET accept(struct sockaddr *saddr, socklen_t *addrlen);

    bool setListen(int backlog);

    virtual ssize_t sendTo(
        const void *buf, 
        size_t len, 
        int flags, 
        const struct sockaddr *to, 
        socklen_t tolen);

    ssize_t recvFrom(
        void *buf, 
        size_t len, 
        int flags, 
        struct sockaddr *from, 
        socklen_t *fromlen);

    bool hasWatchable(const Watchable &wbl);

    bool setSocketOption(
        int level, 
        int optname, 
        void *optval, 
        socklen_t optlen);

    SOCKET dupeSOCKET();

    void waitForEvent(
        Timeval *timeout, 
        bool writeEvent,
        bool *o_readable,
        bool *o_writable,
        int *o_status);

    ProtocolEvent receiveData(
        void *buf,
        size_t len,
        const int flags,
        ssize_t *bytes);

    ProtocolEvent sendData(
        const void *buf,
        size_t len,
        const int flags,
        ssize_t *bytes);

    void clearBuffer();

    static ProtocolError getProtocolError();

    static const char *getProtocolErrorStr();

    static ProtocolError getProtocolError(int errno_status);

    static const char *getProtocolErrorStr(int errno_status);

    virtual bool openLocalSocket(
        struct sockaddr *local_addr,
        socklen_t addrlen,
        bool registersock,
        bool nonblock) { return false; };

    static bool spliceSockets(SocketWrapper *in, SocketWrapper *out);

    bool peek(struct sockaddr *o_saddr, socklen_t *o_addrlen); 

    int peek(char *o_buf, int buf_size);

    int getMessageSize();
    
    bool clearMessage();

    Mutex *getProxyLock() { return &proxyLock; }
};

#endif /* SOCKETWRAPPER_H */
