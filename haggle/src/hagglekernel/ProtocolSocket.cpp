/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 */

/* Copyright 2008-2009 Uppsala University
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
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Watch.h>

#include "ProtocolSocket.h"

#define MAX(a,b) (a > b ? a : b)

#if defined(ENABLE_IPv6)
#define SOCKADDR_SIZE sizeof(struct sockaddr_in6)
#else
#define SOCKADDR_SIZE sizeof(struct sockaddr_in)
#endif

ProtocolSocket::ProtocolSocket(const ProtType_t _type, 
			       const char *_name, InterfaceRef _localIface, 
			       InterfaceRef _peerIface, const int _flags, 
			       ProtocolManager * m, SOCKET _sock, 
			       size_t bufferSize) : 
	Protocol(_type, _name, _localIface, _peerIface, _flags, m, bufferSize), 
        sock(_sock), socketIsRegistered(false), nonblock(false)
{
	if (sock != INVALID_SOCKET) {
                setSocketOptions();
	}
}

bool ProtocolSocket::multiplyReceiveBufferSize(unsigned int x)
{
	bool res = false;
#if defined(OS_LINUX) 
        int ret = 0;
        long optval = 0;
        socklen_t optlen = sizeof(optval);

        ret = ::getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &optval, &optlen);

        if (ret != -1) {
                optval = optval * x;

                ret = ::setsockopt(sock, SOL_SOCKET, 
				   SO_RCVBUF, &optval, optlen);

                if (ret == -1) {
                        HAGGLE_ERR("Could not set recv buffer to %ld bytes\n", 
				   optval);
                } else {
                        HAGGLE_DBG("recv buffer set to %ld bytes on %s\n", 
				   optval, getName());
			res = true;
                }
        }
#endif
	return res;
}

void ProtocolSocket::setSocketOptions()
{
	// No default options currently
}

ProtocolSocket::~ProtocolSocket()
{
	if (sock != INVALID_SOCKET)
		closeSocket();
}

bool ProtocolSocket::openSocket(int domain, int type, 
				int protocol, bool registersock, 
				bool nonblock)
{
	if (sock != INVALID_SOCKET) {
		HAGGLE_ERR("%s: socket already open\n", getName());
		return false;
	}

	sock = ::socket(domain, type, protocol);

	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: could not open socket : %s\n", 
			   getName(), STRERROR(ERRNO));
		return false;
	}
        else {
	  HAGGLE_DBG2("%s: Opening socket %d\n", getName(), sock);
	}

	if (!setNonblock(nonblock)) {
		CLOSE_SOCKET(sock);
		return false;
	}
        
        setSocketOptions();

	if (registersock && !registerSocket()) {
		CLOSE_SOCKET(sock);
		return false;
	}
	return true;
}

bool ProtocolSocket::setSocket(SOCKET _sock, bool registersock)
{
	if (sock != INVALID_SOCKET)
		return false;
	
	sock = _sock;
	
	if (registersock && !registerSocket()) {
		CLOSE_SOCKET(sock);
		return false;
	}
	return true;
}

bool ProtocolSocket::bind(const struct sockaddr *saddr, socklen_t addrlen)
{
	if (!saddr)
		return false;

	if (::bind(sock, saddr, addrlen) == SOCKET_ERROR) {
		HAGGLE_ERR("%s: could not bind socket: %s\n", 
			   getName(), STRERROR(ERRNO));
		return false;
	}
	return true;
}

SOCKET ProtocolSocket::accept(struct sockaddr *saddr, socklen_t *addrlen)
{
	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: invalid server socket\n", getName());
		return INVALID_SOCKET;
	}
	if (!saddr || !addrlen) {
		HAGGLE_ERR("%s: invalid address\n", getName());
		return INVALID_SOCKET;
	}
	if (getMode() != PROT_MODE_LISTENING) {
		HAGGLE_ERR("%s: non-listening socket\n", getName());
		return INVALID_SOCKET;
	}
	SOCKET clientsock = ::accept(sock, saddr, addrlen);

	if (clientsock == INVALID_SOCKET) {
	        int lasterrno = errno;

		HAGGLE_ERR("%s: accept failed : %s\n", 
			   getName(), STRERROR(ERRNO));

	        if(lasterrno == ENFILE) {
		  HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n"); // MOS
		  getKernel()->setFatalError();
		  getKernel()->shutdown();
		}

		return INVALID_SOCKET;
	}
	return clientsock;
}

bool ProtocolSocket::setListen(int backlog)
{
	if (!localIface)
		return false;

	if (listen(sock, backlog) == SOCKET_ERROR) {
		HAGGLE_DBG("Could not set listen on %s\n", getName());
		return false;
	}
	
	setMode(PROT_MODE_LISTENING);

	return true;
}

bool ProtocolSocket::setNonblock(bool _nonblock)
{
	
#ifdef OS_WINDOWS
	unsigned long on = _nonblock ? 1 : 0;

	if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR) {
		HAGGLE_ERR("%s: Could not set %s mode on socket %d : %s\n", 
			   getName(), _nonblock ? "nonblocking" : "blocking", 
			   sock, STRERROR(ERRNO));
		return false;
	}
#elif defined(OS_UNIX)
	long mode = fcntl(sock, F_GETFL, 0);
	
	if (mode == -1) {
		HAGGLE_ERR("%s: could not get socket flags : %s\n", 
			   getName(), STRERROR(ERRNO));
		return false;
	}

	if (_nonblock)
		mode = mode | O_NONBLOCK;
	else
		mode = mode & ~O_NONBLOCK;

	if (fcntl(sock, F_SETFL, mode) == -1) {
		HAGGLE_ERR("%s: Could not set %s mode on socket %d : %s\n", 
			   getName(), _nonblock ? "nonblocking" : "blocking", 
			   sock, STRERROR(ERRNO));
		return false;
	}
#endif
        nonblock = _nonblock;

	return true;
}

bool ProtocolSocket::isNonblock()
{
        return nonblock;
}

void ProtocolSocket::closeSocket()
{
	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: cannot close non-open socket\n", 
			   getName());
		return;
	}

	if (socketIsRegistered) {
		getKernel()->unregisterWatchable(sock);
		socketIsRegistered = false;
	}
	CLOSE_SOCKET(sock);
	setMode(PROT_MODE_DONE);
}

ssize_t ProtocolSocket::sendTo(const void *buf, size_t len, int flags, 
			       const struct sockaddr *to, socklen_t tolen)
{
	if (!to || !buf) {
		HAGGLE_ERR("%s: invalid argument\n", getName());
		return -1;
	}
	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: cannot sendto on closed socket\n", getName());
		return -1;
	}
	ssize_t ret =  sendto(sock, (const char *)buf, len, flags, to, tolen);

	if (ret == -1) {
		HAGGLE_ERR("%s: sendto failed : %s\n", 
			   getName(), STRERROR(ERRNO));
	}
	return ret;
}

ssize_t ProtocolSocket::recvFrom(void *buf, size_t len, 
				 int flags, struct sockaddr *from, 
				 socklen_t *fromlen)
{
	if (!from || !buf) {
		HAGGLE_ERR("%s: invalid argument\n", getName());
		return -1;
	}
	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: cannot recvfrom on closed socket\n", 
			   getName());
		return -1;
	}
	ssize_t ret = recvfrom(sock, (char *)buf, len, flags, from, fromlen);

	if (ret == -1) {
		HAGGLE_ERR("%s: recvfrom failed : %s\n", 
			   getName(), STRERROR(ERRNO));
	}
	return ret;
}

InterfaceRef ProtocolSocket::resolvePeerInterface(const SocketAddress& addr)
{
	int res;
	InterfaceRef pIface = getKernel()->getInterfaceStore()->retrieve(addr);

	if (pIface) {
		HAGGLE_DBG("Peer interface is [%s]\n", 
			   pIface->getIdentifierStr());
	} else if (addr.getType() == Address::TYPE_IPV4
#if defined(ENABLE_IPv6)
		   || addr.getType() == Address::TYPE_IPV6
#endif
		) {
                char buf[SOCKADDR_SIZE];
                unsigned char mac[6];
		// const char *ifname = isServer() ? NULL : localIface->getName();
		const char *ifname = localIface->getName(); // MOS - NULL leads to seg fault below
                struct sockaddr *peer_addr = (struct sockaddr *)buf;
                addr.fillInSockaddr(peer_addr);
                
                HAGGLE_DBG("%s: isServer=%s mac2IP %s on interface %s\n", 
			   getName(), isServer() ? 
			   "true" : "false", 
			   addr.getStr(), ifname ? 
			   localIface->getName() : "unspecified");
		
                res = get_peer_mac_address(peer_addr, ifname, mac, 6);

		if (res < 0) {
			HAGGLE_ERR("peer %s, error=%d\n", 
				   addr.getStr(), res);
		} else if (res == 0) {
			HAGGLE_ERR("No mac address for peer %s\n", 
				   addr.getStr());
		} else {
			EthernetAddress eth_addr(mac);
			pIface = 
				Interface::create<EthernetInterface>(mac, 
								     "TCP peer",
								     eth_addr, 
								     IFFLAG_UP);
			
			if (pIface) {
				pIface->addAddress(eth_addr);
				pIface->addAddress(addr);
				HAGGLE_DBG("Peer interface is [%s]\n", 
					   pIface->getIdentifierStr());
			}
		}
	}

	return pIface;
}

bool ProtocolSocket::registerSocket()
{
	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: Cannot register invalid socket\n", 
			   getName());
		return false;
	} 
	if (socketIsRegistered) {
		HAGGLE_ERR("%s: Socket already registered\n", 
			   getName());
		return false;
	}
	if (getKernel()->registerWatchable(sock, getManager()) <= 0) {
		HAGGLE_ERR("%s: Could not register socket with kernel\n", 
			   getName());
		return false;
	}
	
	socketIsRegistered = true;
	
	return true;
}

bool ProtocolSocket::hasWatchable(const Watchable &wbl)
{
	return wbl == sock;
}

void ProtocolSocket::handleWatchableEvent(const Watchable &wbl)
{
	if (wbl != sock) {
		HAGGLE_ERR("ERROR! : %s does not belong to Protocol %s\n", 
			   wbl.getStr(), getName());
		return;
	}

	if (isClient())
		receiveDataObject();
	else if (isServer())
		acceptClient();
}

ProtocolEvent ProtocolSocket::openConnection(const struct sockaddr *saddr, 
					     socklen_t addrlen)
{
        bool wasNonblock = nonblock;

	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: socket is invalid\n", 
			   getName());
		return PROT_EVENT_ERROR;
	}

	if (!saddr) {
		HAGGLE_ERR("%s:  address is invalid\n", getName());
		return PROT_EVENT_ERROR;
	}

	if (isConnected()) {
		HAGGLE_ERR("%s: connection is already open\n", 
			   getName());
		return PROT_EVENT_ERROR;
	}

        // Make sure that we block while trying to connect
        if (nonblock)
                setNonblock(false);

	if (::connect(sock, saddr, addrlen) == SOCKET_ERROR) {
	        int lasterrno = errno;

		HAGGLE_ERR("%s - %s\n", 
			   getName(), 
			   STRERROR(ERRNO));
		
                if (wasNonblock)
                        setNonblock(true);

	        if(lasterrno == ENFILE) {
		  HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n"); // MOS
		  getKernel()->setFatalError();
		  getKernel()->shutdown();
		}

		switch (getProtocolError()) {
		case PROT_ERROR_BAD_HANDLE:
		case PROT_ERROR_INVALID_ARGUMENT:
		case PROT_ERROR_NO_MEMORY:
		case PROT_ERROR_NOT_A_SOCKET:
		case PROT_ERROR_NO_STORAGE_SPACE:
			return PROT_EVENT_ERROR_FATAL;
		default:
			break;
		}

		return PROT_EVENT_ERROR;
	}

        if (wasNonblock)
                setNonblock(true);

	setFlag(PROT_FLAG_CONNECTED);

	return PROT_EVENT_SUCCESS;
}

void ProtocolSocket::closeConnection()
{
	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("%s: socket is not valid\n", 
			   getName());
	}	return;
	
	unSetFlag(PROT_FLAG_CONNECTED);
	
	// TODO: should consider calling closeSocket() here since that
	// is basically doing the same thing
	if (socketIsRegistered) {
		getKernel()->unregisterWatchable(sock);
		socketIsRegistered = false;
	}
	CLOSE_SOCKET(sock);
}

bool ProtocolSocket::setSocketOption(int level, int optname, 
				     void *optval, socklen_t optlen)
{
	if (setsockopt(sock, level, optname, 
		       (char *)optval, optlen) == SOCKET_ERROR) {
		HAGGLE_ERR("%s: setsockopt failed : %s\n", 
			   getName(), STRERROR(ERRNO));
		return false;
	}
	return true;
}

ProtocolEvent ProtocolSocket::receiveData(void *buf, size_t len, 
					  const int flags, size_t *bytes)
{
	ssize_t ret;
	
	*bytes = 0;
	
	ret = recv(sock, (char *)buf, len, flags);
	
	if (ret < 0) {
		return PROT_EVENT_ERROR;
	} else if (ret == 0)
		return PROT_EVENT_PEER_CLOSED;
	
	*bytes = ret;
	
	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolSocket::sendData(const void *buf, size_t len, 
				       const int flags, size_t *bytes)
{
	ssize_t ret;

	*bytes = 0;

    int extra_flags = 0;
#if defined(MSG_NOSIGNAL)
    extra_flags |= MSG_NOSIGNAL; // MOS - mask SIGPIPE broken pipe signal
#endif
	ret = send(sock, (const char *)buf, len, flags|extra_flags);

	if (ret < 0) {
		return PROT_EVENT_ERROR;
	} else if (ret == 0)
		return PROT_EVENT_PEER_CLOSED;

	*bytes = ret;
	
	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolSocket::waitForEvent(Timeval *timeout, 
					   bool writeevent)
{
	Watch w;
	int ret, index;

	index = w.add(sock, writeevent ? 
		      WATCH_STATE_WRITE : WATCH_STATE_READ);

	ret = w.wait(timeout);
	
	if (ret == Watch::TIMEOUT)
		return PROT_EVENT_TIMEOUT;
	else if (ret == Watch::FAILED)
		return PROT_EVENT_ERROR;
	else if (ret == Watch::ABANDONED)
                return PROT_EVENT_SHOULD_EXIT;

	if (w.isReadable(index))
		return PROT_EVENT_INCOMING_DATA;
	else if (w.isWriteable(index)) 
		return PROT_EVENT_WRITEABLE;

	return PROT_EVENT_ERROR;
}

ProtocolEvent ProtocolSocket::waitForEvent(DataObjectRef &dObj, 
					   Timeval *timeout, 
					   bool writeevent)
{
	QueueElement *qe = NULL;
	Queue *q = getQueue();

	if (!q)
		return PROT_EVENT_ERROR;

	QueueEvent_t qev = q->retrieve(&qe, sock, timeout, writeevent);

	switch (qev) {
	case QUEUE_TIMEOUT:
		return  PROT_EVENT_TIMEOUT;
	case QUEUE_WATCH_ABANDONED:
		return PROT_EVENT_SHOULD_EXIT;
	case QUEUE_WATCH_READ:
		return PROT_EVENT_INCOMING_DATA;
	case QUEUE_WATCH_WRITE:
		return PROT_EVENT_WRITEABLE;
	case QUEUE_ELEMENT:
	        if(!qe) {
		  HAGGLE_ERR("Ignoring NULL queue element when reading protocol queue\n");
		  break;
		}
		dObj = qe->getDataObject();
		delete qe;
		return PROT_EVENT_TXQ_NEW_DATAOBJECT;
	case QUEUE_EMPTY:
		return PROT_EVENT_TXQ_EMPTY;
	default:
		break;
	}

	return PROT_EVENT_ERROR;
}

void ProtocolSocket::hookShutdown()
{
	closeConnection();
}

#if defined(OS_LINUX) || defined (OS_MACOSX)
ProtocolError ProtocolSocket::getProtocolError()
{
	switch (errno) {
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
	case EPIPE: // MOS - treat broken pipe same as reset
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
const char *ProtocolSocket::getProtocolErrorStr()
{
	// We could append the system error string from strerror
	//return (error < _PROT_ERROR_MAX && error > _PROT_ERROR_MIN) ? errorStr[error] : "Bad error";
	return strerror(errno);
}

#else

ProtocolError ProtocolSocket::getProtocolError()
{
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

const char *ProtocolSocket::getProtocolErrorStr()
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
	if (len) {
		if (errStr && (strlen(errStr) < len)) {
			delete [] errStr;
			errStr = NULL;
		}
		if (errStr == NULL)
			errStr = new char[len + 1];

		sprintf(errStr, "%s", reinterpret_cast < TCHAR * >(lpMsgBuf));
		LocalFree(lpMsgBuf);

		return errStr;
	} 

	return unknownErrStr;
}
#endif /* OS_LINUX OS_MACOSX */

