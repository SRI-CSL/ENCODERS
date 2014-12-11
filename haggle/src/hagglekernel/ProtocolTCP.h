/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
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
#ifndef _PROTOCOLTCP_H
#define _PROTOCOLTCP_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

#define TCP_BACKLOG_SIZE 30
#define TCP_DEFAULT_PORT 9697

class ProtocolTCP;
class ProtocolTCPClient;
class ProtocolTCPServer;


#include <libcpphaggle/Platform.h>

#include "ProtocolSocket.h"
#include "ProtocolConfigurationTCP.h"


/** */
class ProtocolTCP : public ProtocolSocket
{
        friend class ProtocolTCPServer;
        friend class ProtocolTCPClient;
	bool initbase();
        ProtocolTCP(SOCKET sock, const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
		const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);
    ProtocolConfiguration *getConfiguration();
    ProtocolConfigurationTCP *getTCPConfiguration();
public:
        ProtocolTCP(const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
                    const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);
        virtual ~ProtocolTCP() = 0;
};

/** */
class ProtocolTCPClient : public ProtocolTCP
{
        friend class ProtocolTCPServer;
	bool init_derived();
protected:
	ProtocolTCPServer *server; // MOS
public:
        ProtocolTCPClient(ProtocolTCPServer *_server, SOCKET sock, const InterfaceRef& _localIface, const InterfaceRef& _peerIface, ProtocolManager *m = NULL) : 
	ProtocolTCP(sock, _localIface, _peerIface, PROT_FLAG_CLIENT | PROT_FLAG_CONNECTED, m), server(_server)  {}

        ProtocolTCPClient(ProtocolTCPServer *_server, const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
                          ProtocolManager *m = NULL) :
	ProtocolTCP(_localIface, _peerIface, PROT_FLAG_CLIENT, m), server(_server) {}
        ProtocolEvent connectToPeer();};

/** */
class ProtocolTCPSender : public ProtocolTCPClient
{
private:
    EventType deleteSenderEventType;
public:
        ProtocolTCPSender(ProtocolTCPServer *_server, const InterfaceRef& _localIface, 
		const InterfaceRef& _peerIface,
		ProtocolManager *m = NULL,
        EventType _deleteSenderEventType = -1) :
        ProtocolTCPClient(_server,_localIface, _peerIface, m),
        deleteSenderEventType(_deleteSenderEventType) {}
	virtual ~ProtocolTCPSender();
	bool isSender() { return true; }
    void hookCleanup();
};

/** */
class ProtocolTCPReceiver : public ProtocolTCPClient
{
public:
	ProtocolTCPReceiver(ProtocolTCPServer *_server, SOCKET sock, 
		const InterfaceRef& _localIface,
		const InterfaceRef& _peerIface,
		ProtocolManager *m = NULL) : 
        ProtocolTCPClient(_server, sock, _localIface, _peerIface, m) {}
	bool isReceiver() { return true; }
};

/** */
// MOS - use BasicMap instead of Map to allow multiple entries per ip
typedef BasicMap< unsigned long, ProtocolTCPSender *> tcp_sender_registry_t;
class ProtocolTCPServer : public ProtocolTCP
{
    tcp_sender_registry_t sender_registry;
    EventType deleteSenderEventType;
        friend class ProtocolTCP;
	bool init_derived();
public:
        ProtocolTCPServer(const InterfaceRef& _localIface = NULL, ProtocolManager *m = NULL);
        virtual ~ProtocolTCPServer();
        ProtocolEvent acceptClient();
// SW: START: instantiate senders from server:
    void removeCachedSender(ProtocolTCPSender *sender); // MOS
    Protocol *getSenderProtocol(const InterfaceRef peerIface);
    static unsigned long interfaceToIP(const InterfaceRef peerIface);
// SW: END: instantiate senders from server.
};

#endif /* _PROTOCOLTCP_H */
