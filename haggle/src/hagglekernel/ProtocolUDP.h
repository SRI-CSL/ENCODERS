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
#ifndef _PROTOCOLUDP_H
#define _PROTOCOLUDP_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ProtocolUDP;

#include <libcpphaggle/Platform.h>
#include "ProtocolSocket.h"

#define HAGGLE_SERVICE_DEFAULT_PORT 8787

/** */
class ProtocolUDP : public ProtocolSocket
{
        unsigned short port;
        ProtocolEvent sendData(const void *buf, size_t buflen, const int flags, size_t *bytes);
	ProtocolEvent receiveData(void *buf, size_t buflen, struct sockaddr *peer_addr, const int flags, size_t *bytes);
	ProtocolEvent receiveDataObject();
	bool init_derived();
public:
	ProtocolUDP(const InterfaceRef& _localIface = NULL, unsigned short _port = HAGGLE_SERVICE_DEFAULT_PORT, ProtocolManager *m = NULL);
	ProtocolUDP(const char *localIP, unsigned short _port = HAGGLE_SERVICE_DEFAULT_PORT, ProtocolManager *m = NULL);
	virtual ~ProtocolUDP(); // MOS - virtual
	bool isForInterface(const InterfaceRef& iface);
	bool isSender();
	bool isReceiver();
	bool sendDataObject(const DataObjectRef& dObj, const NodeRef& peer, const InterfaceRef& _peerIface);

	unsigned short getPort() const {
		return port;
	}
};

#endif /* PROTOCOLUDP_H */
