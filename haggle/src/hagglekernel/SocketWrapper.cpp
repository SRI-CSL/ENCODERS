/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/times.h>
//#include <sys/sendfile.h>

#include "SocketWrapper.h"

#if defined(ENABLE_IPv6)
#define SOCKADDR_SIZE sizeof(struct sockaddr_in6)
#else
#define SOCKADDR_SIZE sizeof(struct sockaddr_in)
#endif

SocketWrapper::SocketWrapper(
    HaggleKernel *_kernel,
    Manager *_manager,
    SOCKET _sock) 
    :   kernel(_kernel), 
        manager(_manager), 
        registered(false), 
        connected(false),
        nonblock(false),
        bound(false),
        listening(false),
        sock(_sock)
{
    if (INVALID_SOCKET != sock) {
        // NOTE: we assume that a socket that is non-default 
        // has already been connected
        setConnected(true);
    }
}

SocketWrapper::~SocketWrapper() 
{
    if (INVALID_SOCKET != sock) {
        closeSocket();
    }
}

SOCKET 
SocketWrapper::getSOCKET() 
{
    return sock;
}

bool 
SocketWrapper::multiplyRcvBufferSize(int factor)
{
    bool res = false;
#if defined(OS_LINUX) 
    int ret = 0;
    long optval = 0;
    socklen_t optlen = sizeof(optval);

    ret = ::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);

    if (-1 == ret) {
        return res;
    }

    HAGGLE_DBG("Original recv buffer set to %ld bytes\n", optval); 

    optval = optval * factor;

    ret = ::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &optval, optlen);

    if (-1 == ret) {
        HAGGLE_ERR("Could not set recv buffer to %ld bytes\n", optval);
    } else {
        HAGGLE_DBG("Recv buffer set to %ld bytes\n", optval); 
        res = true;
    }
#endif

	return res;
}

bool 
SocketWrapper::multiplySndBufferSize(int factor)
{
    bool res = false;
#if defined(OS_LINUX) 
    int ret = 0;
    long optval = 0;
    socklen_t optlen = sizeof(optval);

    ret = ::getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &optval, &optlen);

    if (-1 == ret) {
        return res;
    }

    HAGGLE_DBG("Original send buffer set to %ld bytes\n", optval); 

    optval = optval * factor;

    ret = ::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &optval, optlen);

    if (-1 == ret) {
        HAGGLE_ERR("Could not set send buffer to %ld bytes\n", optval);
    } else {
        HAGGLE_DBG("Send buffer set to %ld bytes\n", optval); 
        res = true;
    }
#endif

	return res;
}

void 
SocketWrapper::closeSocket() 
{
	if (INVALID_SOCKET == sock) {
		HAGGLE_ERR("Cannot close non-open socket\n"); 
		return;
	}

    if (isRegistered()) {
        kernel->unregisterWatchable(sock);
    }
	CLOSE_SOCKET(sock);
    sock = INVALID_SOCKET;
    setRegistered(false);
}

bool 
SocketWrapper::isNonblock()
{
    return nonblock;
}

bool 
SocketWrapper::setNonblock(bool _nonblock)
{
#ifdef OS_WINDOWS
    unsigned long on = _nonblock ? 1 : 0;

    if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR) {
        HAGGLE_ERR("Could not set %s mode on socket %d : %s\n", 
                    _nonblock ? "nonblocking" : "blocking", 
                    sock, 
                    STRERROR(ERRNO));
        return false;
    }

#elif defined(OS_UNIX)
    long mode = fcntl(sock, F_GETFL, 0);
	
    if (-1 == mode) {
        HAGGLE_ERR("Could not get socket flags : %s\n", STRERROR(ERRNO));
        return false;
    }

    if (_nonblock) {
        mode = mode | O_NONBLOCK;
    } else {
        mode = mode & ~O_NONBLOCK;
    }

    if (-1 == fcntl(sock, F_SETFL, mode)) {
        HAGGLE_ERR("Could not set %s mode on socket %d : %s\n", 
                    _nonblock ? "nonblocking" : "blocking", 
                    sock, 
                    STRERROR(ERRNO));
        return false;
    }

#endif
    nonblock = _nonblock;

    return true;
}

bool 
SocketWrapper::isRegistered()
{
    return registered;
}

void 
SocketWrapper::setRegistered(bool _registered)
{
    registered = _registered;
}

bool 
SocketWrapper::isConnected()
{
    return connected;
}

void 
SocketWrapper::setConnected(bool _connected)
{
    connected = _connected;
}

bool 
SocketWrapper::registerSocket()
{
    if (sock == INVALID_SOCKET) {
        HAGGLE_ERR("Cannot register invalid socket\n"); 
        return false;
    } 

    if (isRegistered()) {
        HAGGLE_ERR("Socket already registered\n"); 
        return false;
    }

    if (0 >= kernel->registerWatchable(sock, manager)) {
        HAGGLE_ERR("Could not register socket with kernel\n"); 
        return false;
    }
	
    setRegistered(true);
	
    return true;
}

bool
SocketWrapper::openSocket(
    int domain, 
    int type, 
    int protocol, 
    bool registersock, 
    bool nonblock)
{
    if (INVALID_SOCKET != sock) {
        HAGGLE_ERR("socket already open\n");
        return false;
    }

    sock = ::socket(domain, type, protocol);

    if (INVALID_SOCKET == sock) {
        HAGGLE_ERR("could not open socket\n"); 
        return false;
    } 

    if (!setNonblock(nonblock)) {
        closeSocket();
        return false;
    }

    if (registersock && !registerSocket()) {
        closeSocket();
		return false;
	}
   
    return true;
}

ProtocolEvent
SocketWrapper::openConnection(
    const struct sockaddr *saddr, 
    socklen_t addrlen)
{

    bool wasNonBlock = isNonblock();

    if (INVALID_SOCKET == sock) {
        HAGGLE_ERR("Socket is invalid\n");
        return PROT_EVENT_ERROR;
    }

    if (!saddr) {
        HAGGLE_ERR("Address is invalid\n");
        return PROT_EVENT_ERROR;
    } 

    // block while connecting

    if (nonblock) {
        setNonblock(false);
    }

    bool hasError = false;

    int ret = ::connect(sock, saddr, addrlen);

    if (SOCKET_ERROR == ret) {
        hasError = true;
        HAGGLE_ERR("Problems connecting: %s\n", 
            SocketWrapper::getProtocolErrorStr());
    }

    if (wasNonBlock) {
        setNonblock(true);
    }

    if (!hasError) {
        HAGGLE_DBG("Succesfully connected to socket.\n");
        setConnected(true);
        return PROT_EVENT_SUCCESS;
    } else {
        HAGGLE_DBG("An error occurred connecting: %s\n", 
            getProtocolErrorStr());
    }

    switch (getProtocolError()) {
    case PROT_ERROR_BAD_HANDLE:
    case PROT_ERROR_INVALID_ARGUMENT:
    case PROT_ERROR_NO_MEMORY:
    case PROT_ERROR_NOT_A_SOCKET:
    case PROT_ERROR_NO_STORAGE_SPACE:
        return PROT_EVENT_ERROR_FATAL;
    default: 
        return PROT_EVENT_ERROR;
    }    
}

void
SocketWrapper::setBound(bool _bound)
{
    bound = _bound;
} 

bool
SocketWrapper::isBound()
{
    return bound;
}

bool
SocketWrapper::isOpen()
{
    return INVALID_SOCKET != sock;
}

bool 
SocketWrapper::bind(
    const struct sockaddr *saddr, 
    socklen_t addrlen)
{
    if (!saddr) {
        return false;
    }

    if (isBound()) {
        HAGGLE_ERR("Already bound.\n");
        return false;
    }

    if (::bind(sock, saddr, addrlen) == SOCKET_ERROR) {
        HAGGLE_ERR("Could not bind socket: %s\n", 
                    STRERROR(ERRNO));
        return false;
    }

    setBound(true);

    return true;
}

SOCKET
SocketWrapper::accept(
    struct sockaddr *saddr,
    socklen_t *addrlen)
{
    if (INVALID_SOCKET == sock) {
        HAGGLE_ERR("Invalid server socket\n");
        return INVALID_SOCKET;
    }

    if (!saddr || !addrlen) {
        HAGGLE_ERR("Invalid address\n");
        return INVALID_SOCKET;
    }

    SOCKET clientsock = ::accept(sock, saddr, addrlen);

    if (clientsock == INVALID_SOCKET) {
        HAGGLE_ERR("Accept failed : %s\n",
                    STRERROR(ERRNO));
        return INVALID_SOCKET;
    }

    return clientsock;
}

void
SocketWrapper::setListening(bool _listening) 
{
    listening = _listening;
}

bool
SocketWrapper::isListening()
{
    return listening;
}

bool 
SocketWrapper::setListen(int backlog)
{
    if (isListening()) {
        HAGGLE_DBG("Already listening.");
        return false;
    }
    
    if (SOCKET_ERROR == listen(sock, backlog)) {
        HAGGLE_DBG("Could not set listen.");
        return false;
    }
	
    setListening(true);
    return true;
}

ssize_t 
SocketWrapper::sendTo(
    const void *buf, 
    size_t len, 
    int flags, 
    const struct sockaddr *to, 
    socklen_t tolen) 
{
    if ((NULL == to) || (NULL == buf)) {
        HAGGLE_ERR("Invalid argument\n");
        return -1;
    }

    if (INVALID_SOCKET == sock) {
        HAGGLE_ERR("Cannot sendto on closed socket\n");
        return -1;
    }

    ssize_t ret = sendto(sock, (const char *)buf, len, flags, to, tolen);

    if (-1 == ret) {
        HAGGLE_ERR("Sendto failed : %s\n", STRERROR(ERRNO));
    }

    return ret;
}

ssize_t
SocketWrapper::recvFrom(
    void *buf, 
    size_t len, 
    int flags, 
    struct sockaddr *from, 
    socklen_t *fromlen) 
{

    if ((NULL == from) || (NULL == buf)) {
        HAGGLE_ERR("Invalid argument\n");
        return -1;
    }

    if (INVALID_SOCKET == sock) {
        HAGGLE_ERR("Cannot recvfrom on closed socket\n"); 
        return -1;
    }

    ssize_t ret = recvfrom(sock, (char *)buf, len, flags, from, fromlen);

    if (-1 == ret) {
        HAGGLE_ERR("Recvfrom failed : %s\n", STRERROR(ERRNO));
    }

    return ret;
}

bool 
SocketWrapper::hasWatchable(const Watchable &wbl)
{
    return wbl == sock;
}

bool 
SocketWrapper::setSocketOption(
    int level, 
    int optname, 
    void *optval, 
    socklen_t optlen)
{
    if (SOCKET_ERROR == setsockopt(sock, level, optname, 
        (char *)optval, optlen)) {
		HAGGLE_ERR("Setsockopt failed : %s\n", STRERROR(ERRNO));

		return false;
	}

	return true;
}

SOCKET
SocketWrapper::dupeSOCKET()
{
    if (INVALID_SOCKET == sock) {
        HAGGLE_ERR("Cannot dupe invalid socket\n");

        return INVALID_SOCKET;
    } 

    return dup(sock);
}

void
SocketWrapper::waitForEvent(
    Timeval *timeout, 
    bool writeEvent,
    bool *o_readable,
    bool *o_writable,
    int *o_status)
{
    *o_readable = false;
    *o_writable = false;
    *o_status = 0;

    Watch w;
    int ret, index;

    index = w.add(sock, writeEvent ? WATCH_STATE_WRITE : WATCH_STATE_READ);

    ret = w.wait(timeout);
        
    *o_status = ret;    

    if ((Watch::TIMEOUT == ret) 
        || (Watch::FAILED == ret) 
        || (Watch::ABANDONED == ret)) {
        return;
    }

    if (w.isReadable(index)) {
        *o_readable = true;
    }

    if (w.isWriteable(index)) {
        *o_writable = true;
    }

    return;
}

ProtocolEvent 
SocketWrapper::receiveData(
    void *buf,
    size_t len,
    const int flags,
    ssize_t *bytes)
{
    ssize_t ret;
    *bytes = 0;
    if (!isConnected()) {
        HAGGLE_ERR("Not connected\n");
        return PROT_EVENT_ERROR;
    }
    ret = recv(sock, (char *)buf, len, flags);

    if (0 > ret) {
        return PROT_EVENT_ERROR;
    }

    if (0 == ret) {
        return PROT_EVENT_PEER_CLOSED;
    }
    *bytes = ret;

    return PROT_EVENT_SUCCESS;
}

ProtocolEvent
SocketWrapper::sendData(
    const void *buf,
    size_t len,
    const int flags,
    ssize_t *bytes)
{
    
    *bytes = 0;
    if (!isConnected()) {
        HAGGLE_ERR("Not connected\n");
        return PROT_EVENT_ERROR;
    }
    ssize_t ret;
    ret = send(sock, (const char *)buf, len, flags);

    if (0 > ret) {
        return PROT_EVENT_ERROR;
    }

    if (0 == ret) {
        return PROT_EVENT_PEER_CLOSED;
    }
    *bytes = ret;

    return PROT_EVENT_SUCCESS;
}

#if defined(OS_LINUX) || defined (OS_MACOSX)

ProtocolError SocketWrapper::getProtocolError() {
    return getProtocolError(errno);
}

ProtocolError
SocketWrapper::getProtocolError(int errno_status)
{
    ProtocolError error;
    switch (errno_status) {
    case EAGAIN:
        error = PROT_ERROR_WOULD_BLOCK;
        break;
    case EBADF:
#if defined(OS_LINUX)
    case EBADFD:
#endif
        error = PROT_ERROR_BAD_HANDLE;
        break;
    case ECONNREFUSED:
        error = PROT_ERROR_CONNECTION_REFUSED;
        break;
    case EINTR:
        error = PROT_ERROR_INTERRUPTED;
        break;
    case EINVAL:
        error = PROT_ERROR_INVALID_ARGUMENT;
        break;
    case ENOMEM:
        error = PROT_ERROR_NO_MEMORY;
        break;
    case ENOTCONN:
        error = PROT_ERROR_NOT_CONNECTED;
        break;
    case ECONNRESET:
        error = PROT_ERROR_CONNECTION_RESET;
        break;
    case ENOTSOCK:
        error = PROT_ERROR_NOT_A_SOCKET;
        break;
    case ENOSPC:
        error = PROT_ERROR_NO_STORAGE_SPACE;
        break;
    default:
        error = PROT_ERROR_UNKNOWN;
        break;
    }

    return error;
}

const char *
SocketWrapper::getProtocolErrorStr() {
    return getProtocolErrorStr(errno);
}

const char *
SocketWrapper::getProtocolErrorStr(int errno_status)
{
    return strerror(errno_status);
}
    
/* OS_LINUX OS_MACOSX */
#else
/* windows */

ProtocolError 
SocketWrapper::getProtocolError() 
{
    return getProtocolError(0);
}

ProtocolError 
SocketWrapper::getProtocolError(int nop)
{
    int error = 0;
    switch (WSAGetLastError()) {
    case WSAEWOULDBLOCK:
    case WSAEINPROGRESS:
        error = PROT_ERROR_WOULD_BLOCK;
        break;
    case WSA_INVALID_HANDLE:
    case WSAEBADF:
        error = PROT_ERROR_BAD_HANDLE;
        break;
    case WSAECONNREFUSED:
        error = PROT_ERROR_CONNECTION_REFUSED;
        break;
    case WSAEINTR:
        error = PROT_ERROR_INTERRUPTED;
        break;
    case WSAEINVAL:
        error = PROT_ERROR_INVALID_ARGUMENT;
        break;
    case WSA_NOT_ENOUGH_MEMORY:
        error = PROT_ERROR_NO_MEMORY;
        break;
    case WSAENOTCONN:
        error = PROT_ERROR_NOT_CONNECTED;
        break;
    case WSAECONNRESET:
        error = PROT_ERROR_CONNECTION_RESET;
        break;
    case WSAENOTSOCK:
        error = PROT_ERROR_NOT_A_SOCKET;
        break;
    default:
        error = PROT_ERROR_UNKNOWN;
        break;
    }

    return error;
}

const char *
ProtocolSocket::getProtocolErrorStr()
{
    return getProtocolErrorStr(0);
}

const char *
ProtocolSocket::getProtocolErrorStr(int nop)
{
    static char *errStr = NULL;
    static const char *unknownErrStr = "Unknown error";
    LPVOID lpMsgBuf;

    DWORD len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    WSAGetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) & lpMsgBuf,
                    0,
                    NULL);

    if (!len) {
        return unknownErrStr;
    }

    if (errStr && (strlen(errStr) < len)) {
        delete [] errStr;
        errStr = NULL;
    }

    if (errStr == NULL) {
        errStr = new char[len + 1];
    }

    sprintf(errStr, "%s", reinterpret_cast < TCHAR * >(lpMsgBuf));
    LocalFree(lpMsgBuf);

    return errStr;
}

#endif /* windows */

#define MIN(a,b) (((a) > (b)) ? (a) : (b))

#define MAX_MESSAGE_SIZE 65536

bool SocketWrapper::clearMessage()
{
    char buf[MAX_MESSAGE_SIZE];
    ProtocolEvent pEvent;
    ssize_t bytes_sent = 0;

    pEvent = receiveData(buf, MAX_MESSAGE_SIZE, MSG_DONTWAIT, &bytes_sent);

    if (PROT_EVENT_SUCCESS != pEvent && errno != EAGAIN) {
        HAGGLE_ERR("Could not clear message - error: %s (%d)\n", strerror(errno), errno);
        return false;
    }

    return true;
}

bool SocketWrapper::spliceSockets(SocketWrapper *in, SocketWrapper *out)
{
    // SW: explicit synchronization to avoid waking up child before 
    // the entire message has been added to the socket
	// NOTE: splice is only called in thread of ProtocolManager,
	// removing this for now. 
    //Mutex::AutoLocker l(out->getProxyLock());

    if (NULL == in) {
        HAGGLE_ERR("In socket is NULL\n");
        return false;
    }

    if (NULL == out) {
        HAGGLE_ERR("Out socket is NULL\n");
        return false;
    }
    
    // get msg size
    int msg_size = in->getMessageSize();

    if (msg_size <= 0) {
        HAGGLE_ERR("Nothing to splice\n");
        return false;
    }

    char *page;
    //int page_size = getpagesize();
    int buffer_size = msg_size; // page_size * ((msg_size / page_size) + 1); // MOS
    if (!(page = (char *)malloc(buffer_size))) {
    //if (posix_memalign((void **) &page, page_size, buffer_size)) {
        HAGGLE_ERR("Could not allocate buffer for splice: %s\n", strerror(errno));
        return false;
    }

    bzero(page, buffer_size);

    bool success = true;

    struct iovec iov[1];
    bzero(iov, sizeof(struct iovec));
    iov[0].iov_base = page;
    iov[0].iov_len = buffer_size;

    struct msghdr msgh;
    bzero(&msgh, sizeof(struct msghdr));
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;

    int ret = 0;
    // msg_control
    // msg_controllen
    if (msg_size != recvmsg(in->getSOCKET(), &msgh, 0)) {
        HAGGLE_ERR("Could not recv message - error: %s (%d)\n", strerror(errno), errno);
        success = false;
    } 
    else if (buffer_size != (ret = sendmsg(out->getSOCKET(), &msgh, 0))) {
        HAGGLE_ERR("Could not send message - result: %d - error: %s (%d)\n", ret, strerror(errno), errno);
        success = false;
    }

    free(page);

    HAGGLE_DBG("Proxied %d bytes.\n", buffer_size);

    return success;
}

bool SocketWrapper::peek(struct sockaddr *o_saddr, socklen_t *o_addrlen) 
{
    if ((NULL == o_saddr) || (NULL == o_addrlen)) {
        HAGGLE_ERR("Invalid params\n");
        return false;
    }

    char b;
    ssize_t rc = recvFrom(&b, 1, MSG_PEEK, o_saddr, o_addrlen);

    if (1 != rc) {
        HAGGLE_ERR("Error: could not peek on socket\n");
        return false;
    }

    return true;
}

int SocketWrapper::peek(char *o_buf, int buf_size) 
{
    if (NULL == o_buf) {
        HAGGLE_ERR("Invalid params\n");
        return 0;
    }

    struct sockaddr saddr; 
    bzero(&saddr, sizeof(struct sockaddr));
    socklen_t addrlen = sizeof(struct sockaddr);

    ssize_t rc = recvFrom(o_buf, buf_size, MSG_PEEK, &saddr, &addrlen);

    if (0 >= rc) {
        HAGGLE_ERR("Error: could not peek on socket\n");
    }

    return rc;
}

int SocketWrapper::getMessageSize() 
{
    char b;    

    struct sockaddr saddr; 
    bzero(&saddr, sizeof(struct sockaddr));
    socklen_t addrlen = sizeof(struct sockaddr);

    ssize_t rc = recvFrom(&b, 1, MSG_PEEK | MSG_TRUNC, &saddr, &addrlen);

    if (0 >= rc) {
        HAGGLE_ERR("Error: could not peek on socket\n");
    }

    return rc;
}
