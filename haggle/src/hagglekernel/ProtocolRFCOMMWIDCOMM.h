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
#ifndef _PROTOCOLRFCOMMWIDCOMM_H
#define _PROTOCOLRFCOMMWIDCOMM_H

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/List.h>

#if defined(ENABLE_BLUETOOTH) && defined(WIDCOMM_BLUETOOTH)

using namespace haggle;

#include <BtSdkCE.h>
#include "Interface.h"
#include "DataObject.h"
#include "Protocol.h"
#include "ProtocolRFCOMM.h"

class ProtocolRFCOMM;

/**
	RFCOMMConnection is the handle for a rfcomm connection.
	One instance of this class is associated with each
	instance of ProtocolRFCOMM.

	The only thing it does is to relay any incoming events
	to the currently associated ProtocolRFCOMM instance.
*/
class RFCOMMConnection : public CRfCommPort {
	ProtocolRFCOMM *p;
	bool connected, assigned, has_remote_addr;
	BD_ADDR remote_addr;
public:
	void setProtocol(ProtocolRFCOMM *_p) { p = _p; }
	ProtocolRFCOMM *getProtocol() { return p; }
	RFCOMMConnection(ProtocolRFCOMM *_p = NULL);
	~RFCOMMConnection();
	bool connect(unsigned short channel, const unsigned char *addr);
	bool isConnected();
	bool isAssigned() const { return assigned; }
	void setAssigned(bool a = true) { assigned = a; }
	bool getRemoteAddr(BD_ADDR *addr) const;
	void OnDataReceived(void *p_data, UINT16 len);
	void OnEventReceived(UINT32 event_code);
};

/** */
class ProtocolRFCOMM : public Protocol
{
	friend class RFCOMMConnection;
	static List<const RFCOMMConnection *> connectionList;
protected:
	ProtocolError protocol_error;
	static CRfCommIf rfCommIf;
	RFCOMMConnection *rfCommConn;
        unsigned short channel;
	bool autoAssignScn;

	bool initbase();
	// Functions manipulating the connection list
	const RFCOMMConnection *_getConnection(const BD_ADDR addr);
	bool hasConnection(const RFCOMMConnection *c);
	bool hasConnection(const BD_ADDR addr);
	bool addConnection(const RFCOMMConnection *c);
	void removeConnection(const RFCOMMConnection *c);
	RFCOMMConnection *getFirstUnassignedConnection();

	virtual void OnDataReceived(void *p_data, UINT16 len) { printf("OnDataReceived() %u bytes for base class\n", len); }
	virtual void OnEventReceived(UINT32 event_code) { printf("OnEventReceived() for base class\n"); }

	ProtocolError getProtocolError();
	const char *getProtocolErrorStr();

        ProtocolRFCOMM(RFCOMMConnection *rfCommConn, const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
		const unsigned short _channel, const short flags = PROT_FLAG_CLIENT, 
		ProtocolManager *m = NULL);

        ProtocolRFCOMM(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
		const unsigned short channel = RFCOMM_DEFAULT_CHANNEL, 
		const short flags = PROT_FLAG_CLIENT, ProtocolManager *m = NULL);

        virtual ~ProtocolRFCOMM();
};

// Not sure what a good size is here
#define RFCOMM_DATA_BUFFER_SIZE 20000 

/** */
class ProtocolRFCOMMClient : public ProtocolRFCOMM
{
        friend class ProtocolRFCOMMServer;
	HANDLE hReadQ, hWriteQ;
	DWORD blockingTimeout;
	void OnDataReceived(void *p_data, UINT16 len);
	void OnEventReceived(UINT32 event_code);
	/*
		This is a circular buffer to temporarily store data that is read from
		the OnDataReceived() callbacks before it can be read by the protocol
		thread's runloop.
	*/
	char data_buffer[RFCOMM_DATA_BUFFER_SIZE];
	unsigned long db_head, db_tail;
	size_t dataBufferWrite(const void *data, size_t len);
	size_t dataBufferRead(void *data, size_t len);
	size_t dataBufferBytesToRead();
	bool dataBufferIsEmpty();
	Mutex db_mutex;
public:
	ProtocolRFCOMMClient(RFCOMMConnection *rfcommConn, const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
		const unsigned short _channel, ProtocolManager *m = NULL);
	ProtocolRFCOMMClient(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
		const unsigned short channel = RFCOMM_DEFAULT_CHANNEL, ProtocolManager *m = NULL);
	~ProtocolRFCOMMClient();
	bool init_derived();
	bool setNonblock(bool block = false);
        ProtocolEvent connectToPeer();
	void closeConnection();
	void hookShutdown();
   
	/**
           Functions that are overridden from class Protocol.
	*/
	ProtocolEvent waitForEvent(Timeval *timeout = NULL, bool writeevent = false);
	ProtocolEvent waitForEvent(DataObjectRef &dObj, Timeval *timeout = NULL, bool writeevent = false);
	
	ProtocolEvent receiveData(void *buf, size_t len, const int flags, size_t *bytes);
	ProtocolEvent sendData(const void *buf, size_t len, const int flags, size_t *bytes);
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
	bool isSender() { return true; }
};

/** */
class ProtocolRFCOMMReceiver : public ProtocolRFCOMMClient
{
	friend class ProtocolRFCOMMServer;
	ProtocolRFCOMMReceiver(RFCOMMConnection *rfCommConn, const InterfaceRef& _localIface, 
		const InterfaceRef& _peerIface, const unsigned short _channel, ProtocolManager *m = NULL) : 
		ProtocolRFCOMMClient(rfCommConn, _localIface, _peerIface, _channel, m) {}	
public:
	bool isReceiver() { return true; }
};

/** */
class ProtocolRFCOMMServer : public ProtocolRFCOMM
{
	friend class ProtocolRFCOMM;
	friend class RFCOMMConnection;
	HANDLE connectionEvent;

	void OnEventReceived(UINT32 event_code);

	bool setListen(int dummy = 0);
public:
        ProtocolRFCOMMServer(const InterfaceRef& localIface = NULL, ProtocolManager *m = NULL,
                             unsigned short channel = RFCOMM_DEFAULT_CHANNEL);
        ~ProtocolRFCOMMServer();
	bool init_derived();
	bool hasWatchable(const Watchable &wbl);
	void handleWatchableEvent(const Watchable &wbl);

        ProtocolEvent acceptClient();
};

#endif
#endif /* _PROTOCOLRFCOMMWIDCOMMM_H */
