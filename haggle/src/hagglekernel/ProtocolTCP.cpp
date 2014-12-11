/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Tim McCarthy (TTM)
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
#include <string.h>

#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "ProtocolTCP.h"
#include "ProtocolManager.h"
#include "Interface.h"

#if defined(ENABLE_IPv6)
#define SOCKADDR_SIZE sizeof(struct sockaddr_in6)
#else
#define SOCKADDR_SIZE sizeof(struct sockaddr_in)
#endif

ProtocolTCP::ProtocolTCP(SOCKET _sock, const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
			 const short flags, ProtocolManager * m) :
	ProtocolSocket(Protocol::TYPE_TCP, "ProtocolTCP", _localIface, _peerIface, flags, m, _sock)
{
}

ProtocolTCP::ProtocolTCP(const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
			 const short flags, ProtocolManager * m) :
	ProtocolSocket(Protocol::TYPE_TCP, "ProtocolTCP", _localIface, _peerIface, flags, m)
{
}

ProtocolTCP::~ProtocolTCP()
{
}

bool ProtocolTCP::initbase()
{
	int optval = 1;
	char buf[SOCKADDR_SIZE];
	struct sockaddr *local_addr = (struct sockaddr *)buf;
	socklen_t addrlen = 0;
	int af = AF_INET;
	unsigned short port = isClient() ? 0 : getTCPConfiguration()->getPort();
	char ip_str[50];

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
	// Figure out the address type based on the local interface
#if defined(ENABLE_IPv6)
	if (localIface->getAddress<IPv6Address>() && peerIface && peerIface->getAddress<IPv6Address>())
		af = AF_INET6;
#endif

	/* Configure a sockaddr for binding to the given port. */
	if (af == AF_INET) {
		IPv4Address *localAddress = localIface->getAddress<IPv4Address>();
		struct sockaddr_in *sa = (struct sockaddr_in *)local_addr;
		localAddress->fillInSockaddr(sa, port);
		addrlen = sizeof(struct sockaddr_in);
	}
#if defined(ENABLE_IPv6)
	else if (af == AF_INET6) {
		IPv6Address *localAddress = localIface->getAddress<IPv6Address>();
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)local_addr;
		localAddress->fillInSockaddr(sa, port);
		sa->sin6_len = SOCKADDR_SIZE;
		addrlen = sizeof(struct sockaddr_in6);
	}
#endif

	if (!openSocket(local_addr->sa_family, SOCK_STREAM, IPPROTO_TCP, isServer())) {
		HAGGLE_ERR("Could not create TCP socket\n");
		HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n"); // MOS
		getKernel()->setFatalError();
		getKernel()->shutdown();
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
		// MOS - there is a Cyanogen bug in REUSEADDR so we better shutdown here
		HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n"); // MOS
		getKernel()->setFatalError();
		getKernel()->shutdown();
		return false;
	}
	if (inet_ntop(af, &((struct sockaddr_in *)local_addr)->sin_addr, ip_str, sizeof(ip_str))) {
		HAGGLE_DBG2("%s Created %s TCP socket - %s:%d\n",
				getName(), af == AF_INET ? "AF_INET" : "AF_INET6", ip_str, port);
	}
	return true;
}

bool ProtocolTCPClient::init_derived()
{
	if (!peerIface) {
		HAGGLE_ERR("Client has no peer interface\n");
		return false;
	}
	return initbase();
}

ProtocolEvent ProtocolTCPClient::connectToPeer()
{
	socklen_t addrlen;
	char buf[SOCKADDR_SIZE];
	struct sockaddr *peer_addr = (struct sockaddr *)buf;
	unsigned short peerPort;
	SocketAddress *addr = NULL;
	InterfaceRef pIface;

	if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, not opening new connections\n");
		return PROT_EVENT_ERROR;
	}

	// FIXME: use other port than the default one?
	// peerPort = TCP_DEFAULT_PORT;
	peerPort = getTCPConfiguration()->getPort(); // MOS

	if (!peerIface ||
		!(peerIface->getAddress<IPv4Address>() ||
#if defined(ENABLE_IPv6)
		peerIface->getAddress<IPv6Address>()
#else
		0
#endif
		))
		return PROT_EVENT_ERROR;

#if defined(ENABLE_IPv6)
	IPv6Address *addr6 = peerIface->getAddress<IPv6Address>();
	if (addr6) {
		addrlen = addr6->fillInSockaddr((struct sockaddr_in6 *)peer_addr, peerPort);
		addr = addr6;
		HAGGLE_DBG("Using IPv6 address %s to connect to peer\n", addr6->getStr());
	}
#endif

	if (!addr) {
		// Since the check above passed, we know there has to be an IPv4 or an
		// IPv6 address associated with the interface, and since there was no IPv6...
		IPv4Address *addr4 = peerIface->getAddress<IPv4Address>();
		if (addr4) {
			addrlen = addr4->fillInSockaddr((struct sockaddr_in *)peer_addr, peerPort);
			addr = addr4;
		}
	}

	if (!addr) {
		HAGGLE_DBG("No IP address to connect to\n");
		return PROT_EVENT_ERROR;
	}

	ProtocolEvent ret = openConnection(peer_addr, addrlen);

	if (ret != PROT_EVENT_SUCCESS) {
		HAGGLE_DBG("%s Connection failed to [%s] tcp port=%u\n",
			getName(), addr->getStr(), peerPort);
		return ret;
	}

	HAGGLE_DBG2("%s Connected to [%s] tcp port=%u\n", getName(), addr->getStr(), peerPort);

	return ret;
}

ProtocolTCPServer::ProtocolTCPServer(const InterfaceRef& _localIface, ProtocolManager *m)
	: ProtocolTCP(_localIface, NULL, PROT_FLAG_SERVER, m),
	deleteSenderEventType(-1)
{
#define __CLASS__ ProtocolTCPServer
}

ProtocolTCPServer::~ProtocolTCPServer()
{
	if (deleteSenderEventType > 0) {
		Event::unregisterType(deleteSenderEventType);
	}
}

bool ProtocolTCPServer::init_derived()
{
	if (!initbase())
		return false;

	if (!setListen(getTCPConfiguration()->getBacklog())) {
		HAGGLE_ERR("Could not set listen mode on socket\n");
	}
	return true;
}

ProtocolEvent ProtocolTCPServer::acceptClient()
{
	char buf[SOCKADDR_SIZE];
	struct sockaddr *peer_addr = (struct sockaddr *)buf;
	socklen_t len;
	SOCKET clientsock;
	ProtocolTCPClient *p = NULL;
	SocketAddress *addr = NULL;
	unsigned short port;

	if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, not accepting new connections\n");
		return PROT_EVENT_ERROR;
	}

	if (getMode() != PROT_MODE_LISTENING) {
		HAGGLE_DBG("Error: TCPServer not in LISTEN mode\n");
		return PROT_EVENT_ERROR;
	}

	len = SOCKADDR_SIZE;

	clientsock = accept((struct sockaddr *) peer_addr, &len);

	if (!getManager()) {
		HAGGLE_DBG("Error: No manager for protocol!\n");
		CLOSE_SOCKET(clientsock);
		return PROT_EVENT_ERROR;
	}

#if defined(ENABLE_IPv6)
	if (peer_addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *saddr_in6 = (struct sockaddr_in6 *)peer_addr;
		port = ntohs(saddr_in6->sin6_port);
		addr = new IPv6Address(*saddr_in6, TransportTCP(port));
	} else
#endif
	if (peer_addr->sa_family == AF_INET) {
		struct sockaddr_in *saddr_in = (struct sockaddr_in *)peer_addr;
		port = ntohs(saddr_in->sin_port);
		addr = new IPv4Address(*saddr_in, TransportTCP(port));
	} else {
		HAGGLE_ERR("no matching address for incoming client\n");
		return PROT_EVENT_ERROR;
	}

	if (!addr)
		return PROT_EVENT_ERROR;

	p = new ProtocolTCPReceiver(this, clientsock, localIface, resolvePeerInterface(*addr), getManager());

	if (p) {
		p->setConfiguration(getConfiguration());
	}

	delete addr;

	if (!p || !p->init()) {
		HAGGLE_DBG("Unable to create new TCP client on socket %d\n", clientsock);
		CLOSE_SOCKET(clientsock);

		if (p)
			delete p;

		return PROT_EVENT_ERROR;
	}

	p->registerWithManager();

	HAGGLE_DBG("Accepted client with socket %d, starting client thread\n", clientsock);

	return p->startTxRx();
}

unsigned long ProtocolTCPServer::interfaceToIP(
	const InterfaceRef peerIface)
{
	if (!peerIface) {
		HAGGLE_ERR("Null interface\n");
		return 0;
	}

	const IPv4Address *addr = peerIface->getAddress<IPv4Address>();
	if (NULL == addr) {
		HAGGLE_ERR("Could not get IP address\n");
		return 0;
	}

	char buf[SOCKADDR_SIZE];
	bzero(buf, SOCKADDR_SIZE);
	struct sockaddr *peer_addr = (struct sockaddr *)buf;
	addr->fillInSockaddr((struct sockaddr_in *)peer_addr, 0);

	// check cache
	unsigned long remote_ip =
	((struct sockaddr_in *)peer_addr)->sin_addr.s_addr;

	return remote_ip;
}

Protocol *ProtocolTCPServer::getSenderProtocol(const InterfaceRef peerIface)
{
	if (!peerIface) {
		HAGGLE_ERR("Null interface\n");
		return NULL;
	}

	unsigned long remote_ip = interfaceToIP(peerIface);

	if (0 == remote_ip) {
		HAGGLE_ERR("Could not get remote IP\n");
		return NULL;
	}

	tcp_sender_registry_t::iterator it = sender_registry.find(remote_ip);

	// MOS
	int activeProtocols = 0;
	while (it != sender_registry.end() && (*it).first == remote_ip) {
		ProtocolTCPSender *cachedProto = (*it).second;
		if (NULL != cachedProto) {
			if(!cachedProto->isGarbage() && !cachedProto->isDone()) return cachedProto;
			activeProtocols++;
		}
		it++;
	}

	HAGGLE_DBG2("Number of active TCP sender protocols: %d (%d for this link)\n", sender_registry.size(), activeProtocols);

	if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, not instantiating new protocols\n");
		return NULL;
	}

	// MOS - without the following there is a danger of file descriptor overflow
	if(activeProtocols >= getConfiguration()->getMaxInstancesPerLink()) {
		HAGGLE_DBG("Not instantiating new sender protocol - %d matching protocols still running or shutting down\n", activeProtocols);
		return NULL;
	}
	if(sender_registry.size() >= getConfiguration()->getMaxInstances()) {
		HAGGLE_DBG("Not instantiating new sender protocol - %d protocols still running or shutting down\n", sender_registry.size());
		return NULL;
	}

	// not in cache
	ProtocolTCPSender *p = new ProtocolTCPSender(this,
		localIface,
		peerIface,
		getManager(),
		deleteSenderEventType);

	if (!p->init()) {
		HAGGLE_ERR("Could not initialize protocol %s\n", p->getName());
		delete p;
		return NULL;
	}

	// MOS
	sender_registry.insert(make_pair(remote_ip, p));

	p->registerWithManager(); // MOS - do not use registerProtocol directly

	return p;
}

// MOS - this is now a function called by the destructor of the sender

void ProtocolTCPServer::removeCachedSender(ProtocolTCPSender *sender)
{
	for (tcp_sender_registry_t::iterator it = sender_registry.begin(); it != sender_registry.end(); it++) {
		ProtocolTCP *i = (*it).second;
		if (i == sender) {
			sender_registry.erase(it);
			return;
		}
	}

	HAGGLE_ERR("Could not uncache (not in registry)\n");
}

ProtocolConfigurationTCP *ProtocolTCP::getTCPConfiguration()
{
	ProtocolConfiguration *genCfg = getConfiguration();
	// SW: down cast to proper subclass, this looks dangerous but OK since
	// we overrode the other setters and getters
	return static_cast<ProtocolConfigurationTCP *>(genCfg);
}

ProtocolConfiguration *ProtocolTCP::getConfiguration()
{
	if (NULL == cfg) {
		ProtocolConfigurationTCP *tcfg = new ProtocolConfigurationTCP();
		setConfiguration(tcfg);
		delete tcfg;
	}
	return cfg;
}

void ProtocolTCPSender::hookCleanup()
{
}

// MOS - instead of using an event to remove the sender we need to do
//       it synchronously because the protocol is deleted by the
//       protocol manager and the event may be delayed leaving
//       behind a bad registry entry

ProtocolTCPSender::~ProtocolTCPSender()
{
	if(!isDetached()) // MOS - no assumptions about other protocols in detached state
		server->removeCachedSender(this);
}

