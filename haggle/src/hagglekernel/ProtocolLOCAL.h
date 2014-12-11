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
#ifndef _PROTOCOLLOCAL_H
#define _PROTOCOLLOCAL_H

class ProtocolLOCAL;
class ProtocolLOCALClient;
class ProtocolLOCALServer;


#include <libcpphaggle/Platform.h>

#include "ProtocolSocket.h"
#include <sys/types.h>
#include <sys/un.h>

/* Configurable parameters */
#define LOCAL_DEFAULT_BACKLOG 10
#define LOCAL_DEFAULT_PATH "/tmp/haggle.sock"

/** */
class ProtocolLOCAL : public ProtocolSocket
{
        friend class ProtocolLOCALServer;
        friend class ProtocolLOCALClient;
	UnixAddress uaddr;
	bool initbase();
        ProtocolLOCAL(SOCKET sock, const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
		    const char *_path, const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);
public:
        ProtocolLOCAL(const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
                    const char *_path = LOCAL_DEFAULT_PATH,
                    const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);
        virtual ~ProtocolLOCAL() = 0;
};

/** */
class ProtocolLOCALClient : public ProtocolLOCAL
{
        friend class ProtocolLOCALServer;
	bool init_derived();
public:
        ProtocolLOCALClient(SOCKET sock, const InterfaceRef& _localIface, const InterfaceRef& _peerIface, const char *_path, ProtocolManager *m = NULL) : 
	ProtocolLOCAL(sock, _localIface, _peerIface, _path, PROT_FLAG_CLIENT | PROT_FLAG_CONNECTED, m) {}
        ProtocolLOCALClient(const InterfaceRef& _localIface, const InterfaceRef& _peerIface,
                          const char *_path = LOCAL_DEFAULT_PATH, ProtocolManager *m = NULL) :
	ProtocolLOCAL(_localIface, _peerIface, _path, PROT_FLAG_CLIENT, m) {}
        ProtocolEvent connectToPeer();};


/** */
class ProtocolLOCALSender : public ProtocolLOCALClient
{
public:
	ProtocolLOCALSender(const InterfaceRef& _localIface, 
			  const InterfaceRef& _peerIface,
			  const char *_path = LOCAL_DEFAULT_PATH, 
			  ProtocolManager *m = NULL) :
	ProtocolLOCALClient(_localIface, _peerIface, _path, m) {}
	bool isSender() { return true; }
};

/** */
class ProtocolLOCALReceiver : public ProtocolLOCALClient
{
public:
	ProtocolLOCALReceiver(SOCKET sock, 
			    const InterfaceRef& _localIface,
			    const InterfaceRef& _peerIface,
			    const char  *_path,
			    ProtocolManager *m = NULL) : 
	ProtocolLOCALClient(sock, _localIface, _peerIface, _path, m) {}
	bool isReceiver() { return true; }
};

/** */
class ProtocolLOCALServer : public ProtocolLOCAL
{
        friend class ProtocolLOCAL;
        int backlog;
	bool init_derived();
public:
        ProtocolLOCALServer(const InterfaceRef& _localIface = NULL, ProtocolManager *m = NULL,
                          const char *_path = LOCAL_DEFAULT_PATH, int _backlog = LOCAL_DEFAULT_BACKLOG);
        ~ProtocolLOCALServer();
        ProtocolEvent acceptClient();
};

#endif /* PROTOCOLLOCAL_H */
