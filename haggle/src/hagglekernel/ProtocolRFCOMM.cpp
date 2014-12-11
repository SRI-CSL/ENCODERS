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

#if defined(ENABLE_BLUETOOTH) && !defined(OS_MACOSX) && !defined(WIDCOMM_BLUETOOTH)

#include <string.h>
#include <haggleutils.h>

#include "ProtocolManager.h"
#include "ProtocolRFCOMM.h"
#include "HaggleKernel.h"

ProtocolRFCOMMClient::ProtocolRFCOMMClient(const InterfaceRef& _localIface, 
					   const InterfaceRef& _peerIface, 
					   const unsigned short channel, 
					   ProtocolManager *m) :
		ProtocolRFCOMM(_localIface, _peerIface, channel, PROT_FLAG_CLIENT, m)
{
	if (peerIface)
		memcpy(mac, peerIface->getIdentifier(), peerIface->getIdentifierLen());
}

ProtocolRFCOMM::ProtocolRFCOMM(SOCKET _sock, const char *_mac, 
			       const unsigned short _channel, 
			       const InterfaceRef& _localIface, 
			       const short flags, ProtocolManager *m) :
	ProtocolSocket(Protocol::TYPE_RFCOMM, "ProtocolRFCOMM", _localIface, NULL, flags, m, _sock), channel(_channel)
{
	memcpy(mac, _mac, BT_ALEN);
	
	BluetoothAddress addr((unsigned char *) mac);

	peerIface = Interface::create<BluetoothInterface>(mac, "Peer Bluetooth", addr, IFFLAG_UP);
}

ProtocolRFCOMM::ProtocolRFCOMM(const InterfaceRef& _localIface, 
			       const InterfaceRef& _peerIface, 
			       const unsigned short _channel, 
			       const short flags, ProtocolManager * m) : 
	ProtocolSocket(Protocol::TYPE_RFCOMM, "ProtocolRFCOMM", _localIface, _peerIface, flags, m), channel(_channel)
{	
}

ProtocolRFCOMM::~ProtocolRFCOMM()
{
}

bool ProtocolRFCOMM::initbase()
{
	struct sockaddr_bt localAddr;

        if (isConnected()) {
                // Nothing to do
                return true;
        }

	if (!openSocket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM, isServer())) {
		HAGGLE_ERR("Could not create RFCOMM socket\n");
                return false;
        }

	memset(&localAddr, 0, sizeof(struct sockaddr_bt));
	localAddr.bt_family = AF_BLUETOOTH;
	localAddr.bt_channel = channel & 0xff;

#ifndef OS_WINDOWS
	if (localIface) {
		BluetoothAddress *addr = localIface->getAddress<BluetoothAddress>();
		if (addr) {
                        // Binding RFCOMM sockets to a hardware address does not
                        // seem to work in Windows
			BDADDR_swap(&localAddr.bt_bdaddr, addr->getRaw());
                }
	}
#endif

	if (isServer()) {
		localAddr.bt_channel = channel & 0xff;
   
             	/* If this is a server we bing to a specific channel to listen on */
                if (!bind((struct sockaddr *)&localAddr, sizeof(localAddr))) {
                        closeSocket();
#ifdef OS_WINDOWS
                        HAGGLE_ERR("Bind failed for local address WSA error=%s\n", StrError(WSAGetLastError()));
#endif
                        
                        HAGGLE_ERR("Could not bind local address for RFCOMM socket\n");
                        
                        return false;
                }

                HAGGLE_DBG("Bound RFCOMM server to channel=%d\n", channel);
   
	} else {
		HAGGLE_DBG("Created RFCOMM client on channel=%d\n", channel);
	}

	return true;
}

ProtocolEvent ProtocolRFCOMMClient::connectToPeer()
{
	struct sockaddr_bt peerAddr;
	BluetoothAddress *addr;
	
	if (!peerIface)
		return PROT_EVENT_ERROR;

	addr = peerIface->getAddress<BluetoothAddress>();

	if(!addr)
		return PROT_EVENT_ERROR;

	memset(&peerAddr, 0, sizeof(peerAddr));
	peerAddr.bt_family = AF_BLUETOOTH;

	BDADDR_swap(&peerAddr.bt_bdaddr, mac);
	peerAddr.bt_channel = channel & 0xff;

	HAGGLE_DBG("%s Trying to connect over RFCOMM to [%s] channel=%u\n", 
		   getName(), addr->getStr(), channel);

	ProtocolEvent ret = openConnection((struct sockaddr *) &peerAddr, 
					   sizeof(peerAddr));

	if (ret != PROT_EVENT_SUCCESS) {
		HAGGLE_DBG("%s Connection failed to [%s] channel=%u\n", 
			   getName(), addr->getStr(), channel);
		return ret;
	}

	HAGGLE_DBG("%s Connected to [%s] channel=%u\n", 
		   getName(), addr->getStr(), channel);

	return ret;
}


ProtocolRFCOMMClient::~ProtocolRFCOMMClient()
{
	HAGGLE_DBG("Destroying %s\n", getName());
}

bool ProtocolRFCOMMClient::init_derived()
{
	return initbase();
}

ProtocolRFCOMMServer::ProtocolRFCOMMServer(const InterfaceRef& localIface, ProtocolManager *m, unsigned short channel, int _backlog) :
	ProtocolRFCOMM(localIface, NULL, channel, PROT_FLAG_SERVER, m), backlog(_backlog) 
{
}

ProtocolRFCOMMServer::~ProtocolRFCOMMServer()
{
	HAGGLE_DBG("Destroying %s\n", getName());
}

bool ProtocolRFCOMMServer::init_derived()
{
	if (!initbase())
		return false;
	
	if (!setListen(backlog)) {
		closeSocket();
		HAGGLE_ERR("Could not set socket to listening mode\n");
		return false;
        }
	return true;
}

ProtocolEvent ProtocolRFCOMMServer::acceptClient()
{
	SOCKET clientSock;
	struct sockaddr_bt clientAddr;
	socklen_t len;
	char btMac[BT_ALEN];

	HAGGLE_DBG("In RFCOMMServer receive\n");

	if (getMode() != PROT_MODE_LISTENING) {
		HAGGLE_DBG("Error: RFCOMMServer not in LISTEN mode\n");
		return PROT_EVENT_ERROR;
	}

	len = sizeof(clientAddr);

	clientSock = accept((struct sockaddr *) &clientAddr, &len);

	if (clientSock == INVALID_SOCKET)
		return PROT_EVENT_ERROR;

	/* Save address and channel information in client handle. */
	ProtocolManager *pm = static_cast<ProtocolManager *>(getManager());

	if (!pm) {
		HAGGLE_DBG("Error: No manager for protocol!\n");
		CLOSE_SOCKET(clientSock);
		return PROT_EVENT_ERROR;
	}

	BDADDR_swap(btMac, &clientAddr.bt_bdaddr);

	ProtocolRFCOMMClient *p = new ProtocolRFCOMMReceiver(clientSock, btMac,
							   (unsigned short) clientAddr.bt_channel,
							   this->getLocalInterface(), pm);

	if (!p || !p->init()) {
		HAGGLE_ERR("Could not create new RFCOMM receiver\n");
		CLOSE_SOCKET(clientSock);
                
                if (p)
                        delete p;

		return PROT_EVENT_ERROR;
	}

	p->registerWithManager();

	HAGGLE_DBG("Accepted client with socket %d, starting client thread\n", clientSock);

	return p->startTxRx();
}

#endif /* ENABLE_BLUETOOTH && !OS_MACOSX */
