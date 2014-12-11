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
#include <libcpphaggle/PlatformDetect.h>

#if !defined(OS_WINDOWS)

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libcpphaggle/Exception.h>
#include <haggleutils.h>

#include "ProtocolLOCAL.h"

ProtocolLOCAL::ProtocolLOCAL(SOCKET _sock, const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
			 const char *_path, const short flags, ProtocolManager * m) :
	ProtocolSocket(Protocol::TYPE_LOCAL, "ProtocolLOCAL", _localIface, _peerIface, flags, m, _sock), uaddr(_path)
{
}

ProtocolLOCAL::ProtocolLOCAL(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
			 const char *_path, const short flags, ProtocolManager * m) : 
	ProtocolSocket(Protocol::TYPE_LOCAL, "ProtocolLOCAL", _localIface, _peerIface, flags, m), uaddr(_path)
{
}

ProtocolLOCAL::~ProtocolLOCAL()
{
	unlink(uaddr.getStr());
}

bool ProtocolLOCAL::initbase()
{
	int optval = 1;
	struct sockaddr_un local_addr;
        socklen_t addrlen = 0;
        
	uaddr.fillInSockaddr(&local_addr);

	if (!localIface) {
		HAGGLE_ERR("Local interface is NULL\n");
                return false;
        }
	
	// Check if we are already connected, i.e., we are a client
	// that was created from acceptClient()
	if (isConnected()) {
		// Nothing to initialize
		return true;
	}
	
	if (localIface->getType() != Interface::TYPE_APPLICATION_LOCAL) {
		HAGGLE_ERR("Local interface type is not LOCAL\n");
		return false;
	}
		
	if (!openSocket(local_addr.sun_family, SOCK_STREAM, 0, isServer())) {
		HAGGLE_ERR("Could not create LOCAL socket\n");
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
	
	if (!bind((struct sockaddr *)&local_addr, addrlen)) {
		closeSocket();
		HAGGLE_ERR("Could not bind LOCAL socket\n");
                return false;
        }
	
   
	HAGGLE_DBG("%s Created LOCAL socket - %s\n", getName(), uaddr.getStr());
   	
	return true;
}

bool ProtocolLOCALClient::init_derived()
{
	if (!peerIface) {
		HAGGLE_ERR("Client has no peer interface\n");
		return false;
	}
	return initbase();
}

ProtocolEvent ProtocolLOCALClient::connectToPeer()
{
	socklen_t addrlen;
	struct sockaddr_un peer_addr;
	InterfaceRef pIface; 
	UnixAddress *addr;
	
	if (!peerIface || peerIface->getType() != Interface::TYPE_APPLICATION_LOCAL)
		return PROT_EVENT_ERROR;
	    
	addr = peerIface->getAddress<UnixAddress>();
		
	if (!addr) {
		HAGGLE_DBG("No LOCAL address to connect to\n");
		return PROT_EVENT_ERROR;
	}
	
        addrlen = addr->fillInSockaddr(&peer_addr);
	
	ProtocolEvent ret = openConnection((struct sockaddr *)&peer_addr, addrlen);
	
	if (ret != PROT_EVENT_SUCCESS) {
		HAGGLE_DBG("%s Connection failed to LOCAL [%s]\n", 
			   getName(), addr->getStr());
		return ret;
	}
	
	HAGGLE_DBG("%s Connected to LOCAL [%s]\n", getName(), addr->getStr());
	
	return ret;
}

ProtocolLOCALServer::ProtocolLOCALServer(const InterfaceRef& _localIface, ProtocolManager *m, const char *_path, int _backlog) :
	ProtocolLOCAL(_localIface, NULL, _path, PROT_FLAG_SERVER, m), backlog(_backlog) 
{
}

ProtocolLOCALServer::~ProtocolLOCALServer()
{
}


bool ProtocolLOCALServer::init_derived()
{
	if (!initbase())
		return false;
	
	if (!setListen(backlog))  {
		HAGGLE_ERR("Could not set listen mode on socket\n");
        }
	return true;
}

ProtocolEvent ProtocolLOCALServer::acceptClient()
{
	struct sockaddr_un peer_addr;
	socklen_t len;
	SOCKET clientsock;
	ProtocolLOCALClient *p = NULL;
	
	HAGGLE_DBG("In LOCALServer receive\n");
	
	if (getMode() != PROT_MODE_LISTENING) {
		HAGGLE_DBG("Error: LOCALServer not in LISTEN mode\n");
		return PROT_EVENT_ERROR;
	}
	
	len = sizeof(struct sockaddr_un);
	
	clientsock = accept((struct sockaddr *)&peer_addr, &len);
	
	if (clientsock == SOCKET_ERROR)
		return PROT_EVENT_ERROR;
	
	if (!getManager()) {
		HAGGLE_DBG("Error: No manager for protocol!\n");
		CLOSE_SOCKET(clientsock);
		return PROT_EVENT_ERROR;
	}
		
	UnixAddress addr(peer_addr);

        p = new ProtocolLOCALReceiver(clientsock, localIface, resolvePeerInterface(addr), peer_addr.sun_path, getManager());
	
	if (!p || !p->init()) {
		HAGGLE_DBG("Unable to create new LOCAL client on socket %d\n", clientsock);
		CLOSE_SOCKET(clientsock);
		
		if (p)
			delete p;
		
		return PROT_EVENT_ERROR;
	}
	
	p->registerWithManager();
	
	HAGGLE_DBG("Accepted client with socket %d, starting client thread\n", clientsock);
	
	return p->startTxRx();
}

#endif
