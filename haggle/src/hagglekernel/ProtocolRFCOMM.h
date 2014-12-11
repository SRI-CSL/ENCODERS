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
#ifndef _PROTOCOLRFCOMM_H
#define _PROTOCOLRFCOMM_H

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_BLUETOOTH)

// Pick something high and hope it is free. In the future, we should probably use
// a dynamic channel which is discovered through the SDP service record
#define RFCOMM_DEFAULT_CHANNEL 25

/*
Forward declarations of all data types declared in this file. This is to
avoid circular dependencies. If/when a data type is added to this file,
remember to add it here.
*/
class ProtocolRFCOMM;
class ProtocolRFCOMMClient;
class ProtocolRFCOMMServer;

#if defined(OS_MACOSX)
// On Mac OS X, use this instead:
#include "ProtocolRFCOMMMacOSX.h"
#elif defined(WIDCOMM_BLUETOOTH)
#include "ProtocolRFCOMMWIDCOMM.h"
#else

#include <haggleutils.h>

#if defined(OS_LINUX)
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#elif defined(OS_WINDOWS)
#include <ws2bth.h>
#if defined(ENABLE_BLUETOOTH)
#if defined(OS_WINDOWS_MOBILE)
#include <bthapi.h>
#include <bthutil.h>
#include <bt_api.h>
#include <bt_sdp.h>
#elif defined(OS_WINDOWS_XP)
#endif
#endif
#elif defined(OS_MACOSX)
// #error "Bluetooth work progress!"
#else
#error "Bluetooth not supported on this OS!"
#endif

#include "ProtocolSocket.h"
#include "Interface.h"
#include "DataObject.h"


/*
Configurable parameters. Remember to keep these in sync with
ProtocolRFCOMMMac.h
*/
#define RFCOMM_BACKLOG_SIZE 5

#if defined(OS_LINUX)
#define BDADDR_swap(d,s) baswap((bdaddr_t *)d,(const bdaddr_t *)s)
#elif defined(OS_WINDOWS)
#define BDADDR_swap(d,s) swap_6bytes(d,s)
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
#endif


/** */
class ProtocolRFCOMM : public ProtocolSocket
{
protected:
        char mac[BT_MAC_LEN];  // The Bluetooth byteswapped mac address
        unsigned short channel;
	bool initbase();
        ProtocolRFCOMM(SOCKET _sock, const char *_mac, const unsigned short _channel, const InterfaceRef& _localIface, const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);
public:
        ProtocolRFCOMM(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, const unsigned short channel = RFCOMM_DEFAULT_CHANNEL, const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);

        virtual ~ProtocolRFCOMM();

        unsigned short getMode() {
                return mode;
        }
};

/** */
class ProtocolRFCOMMClient : public ProtocolRFCOMM
{
	friend class ProtocolRFCOMMServer;
	bool init_derived();
public:
	ProtocolRFCOMMClient(SOCKET _sock, const char *_mac, const unsigned short _channel, const InterfaceRef& _localIface, ProtocolManager *m = NULL) :
		ProtocolRFCOMM(_sock, _mac, _channel, _localIface, PROT_FLAG_CLIENT | PROT_FLAG_CONNECTED, m) {}
	ProtocolRFCOMMClient(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, const unsigned short channel = RFCOMM_DEFAULT_CHANNEL, ProtocolManager *m = NULL);
	~ProtocolRFCOMMClient();
	ProtocolEvent connectToPeer();
};

/** */
class ProtocolRFCOMMSender : public ProtocolRFCOMMClient
{
public:
	ProtocolRFCOMMSender(const InterfaceRef& _localIface, 
		const InterfaceRef& _peerIface, 
		const unsigned short channel = RFCOMM_DEFAULT_CHANNEL, 
		ProtocolManager *m = NULL) :
	ProtocolRFCOMMClient(_localIface, _peerIface, channel, m) {}
	virtual bool isSender() { return true; }
};

/** */
class ProtocolRFCOMMReceiver : public ProtocolRFCOMMClient
{
public:
	ProtocolRFCOMMReceiver(SOCKET _sock, 
		const char *_mac, 
		const unsigned short _channel, 
		const InterfaceRef& _localIface, 
		ProtocolManager *m = NULL) : 
	ProtocolRFCOMMClient(_sock, _mac, _channel, _localIface, m) {}
	virtual bool isReceiver() { return true; }
};

/** */
class ProtocolRFCOMMServer : public ProtocolRFCOMM
{
	friend class ProtocolRFCOMM;
        int backlog;
	bool init_derived();
public:
        ProtocolRFCOMMServer(const InterfaceRef& localIface = NULL, ProtocolManager *m = NULL,
                             unsigned short channel = RFCOMM_DEFAULT_CHANNEL, 
			     int _backlog = RFCOMM_BACKLOG_SIZE);
        ~ProtocolRFCOMMServer();
        ProtocolEvent acceptClient();
};

#endif
#endif
#endif /* _PROTOCOLRFCOMM_H */
