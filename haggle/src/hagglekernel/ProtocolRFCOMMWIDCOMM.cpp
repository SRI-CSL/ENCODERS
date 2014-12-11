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

#if defined(ENABLE_BLUETOOTH) && !defined(OS_MACOSX) && defined(WIDCOMM_BLUETOOTH)

#include <string.h>
#include <haggleutils.h>

#include "ConnectivityBluetooth.h" // For HAGGLE_BLUETOOTH_SDP_UUID
#include "ProtocolManager.h"
#include "ProtocolRFCOMMWIDCOMM.h"
#include "HaggleKernel.h"

enum {
	QMSG_TYPE_DATA,
	QMSG_TYPE_CONNECTION_SUCCESS,
	QMSG_TYPE_CONNECTION_ERROR,
};

struct QMsg {
	UINT8 type;
	char *data;
	size_t len;
};

// There should be only one instance of rfCommIf per Bluetooth interface
CRfCommIf ProtocolRFCOMM::rfCommIf;

ProtocolError ProtocolRFCOMM::getProtocolError()
{
	return protocol_error;
}
	
const char *ProtocolRFCOMM::getProtocolErrorStr()
{
	return protocolErrorToStr(protocol_error);
}

RFCOMMConnection::RFCOMMConnection(ProtocolRFCOMM *_p) : p(_p), connected(false), assigned(false), has_remote_addr(false)
{
	HAGGLE_DBG("New RFCOMMConnection for protocol %s\n", p ? p->getName() : "Unknown");
}

RFCOMMConnection::~RFCOMMConnection()
{
	HAGGLE_DBG("RFCOMMConnection destroyed\n");
}

void RFCOMMConnection::OnDataReceived(void *p_data, UINT16 len)
{
	if (p) {
		if (p->isServer()) {
			HAGGLE_ERR("Protocol is server and is receiving data\n");
		}
		p->OnDataReceived(p_data, len);
	} else {
		HAGGLE_ERR("Receiving data and no protocol set for connection\n");
	}
}
void RFCOMMConnection::OnEventReceived(UINT32 event_code)
{
	if (p) {
		//HAGGLE_DBG("Event %lu received for %s\n", event_code, p->isServer() ? "server" : "client");
		p->OnEventReceived(event_code);
	} else {
		HAGGLE_ERR("Event %lu received and no protocol set for connection\n", event_code);
	}
}

bool RFCOMMConnection::connect(unsigned short channel, const unsigned char *addr)
{
	if (!addr)
		return false;

	memcpy(remote_addr, addr, sizeof(BD_ADDR));
	has_remote_addr = true;

	CRfCommPort::PORT_RETURN_CODE ret = OpenClient((UINT8)channel&0xff, remote_addr);

	if (ret != CRfCommPort::SUCCESS) {
		connected = false;
	} else {
		connected = true;
	}
	return connected;
}

bool RFCOMMConnection::isConnected()
{
	if (!has_remote_addr)
		return false;

	// Call the WIDCOMM API, and save the result for internal use in this class
	if (IsConnected(&remote_addr) == TRUE)
		connected = true;
	else
		connected = false;

	return connected;
}


bool RFCOMMConnection::getRemoteAddr(BD_ADDR *addr) const
{
	return const_cast<RFCOMMConnection *>(this)->IsConnected(addr) == TRUE;
}

List<const RFCOMMConnection *> ProtocolRFCOMM::connectionList;


bool ProtocolRFCOMM::initbase()		
{
	unsigned char uuid[] = HAGGLE_BLUETOOTH_SDP_UUID;
	GUID guid;
	UINT8 scn = autoAssignScn ? 0 : channel & 0xff;

	if (!rfCommConn) {
		HAGGLE_ERR("No valid RFCOMM connection\n");
		return false;
	}
	convertUUIDBytesToGUID((char *)uuid, &guid);

	if (!rfCommIf.AssignScnValue(&guid, scn)) {
		HAGGLE_ERR("Could not assign Bluetooth channel number %d\n", channel);
		return false;
	} 
		
	channel = rfCommIf.GetScn();

	if (isServer()) {
		if (!rfCommIf.SetSecurityLevel(L"Haggle", BTM_SEC_NONE, TRUE)) {
			HAGGLE_ERR("Could not set Bluetooth security level for RFCOMM server\n");
			return false;
		}
		HAGGLE_DBG("Created RFCOMM server on channel=%d\n", channel);
	} else {
		if (!rfCommIf.SetSecurityLevel(L"Haggle", BTM_SEC_NONE, FALSE)) {
			HAGGLE_ERR("Could not set Bluetooth security level for RFCOMM server\n");
			return false;
		}
		HAGGLE_DBG("Created RFCOMM client on channel=%d\n", channel);
	}
	return true;
}

ProtocolRFCOMM::ProtocolRFCOMM(RFCOMMConnection *_rfCommConn, const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
			       const unsigned short _channel, const short flags, ProtocolManager *m) :
	Protocol(Protocol::TYPE_RFCOMM, "ProtocolRFCOMM", _localIface, _peerIface, flags, m), 
		protocol_error(PROT_ERROR_UNKNOWN), rfCommConn(_rfCommConn), channel(_channel), autoAssignScn(true)
{
	rfCommConn->setProtocol(this);
}

ProtocolRFCOMM::ProtocolRFCOMM(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
			       const unsigned short _channel, const short flags, ProtocolManager * m) : 
	Protocol(Protocol::TYPE_RFCOMM, "ProtocolRFCOMM", _localIface, _peerIface, flags, m), 
		protocol_error(PROT_ERROR_UNKNOWN), rfCommConn(NULL), channel(_channel), autoAssignScn(false)
{
	rfCommConn = new RFCOMMConnection(this);
}

ProtocolRFCOMM::~ProtocolRFCOMM()
{
	if (rfCommConn){
		removeConnection(rfCommConn);
		delete rfCommConn;
	}
}

const RFCOMMConnection *ProtocolRFCOMM::_getConnection(const BD_ADDR addr)
{
	BD_ADDR addr2;

	for (List<const RFCOMMConnection *>::const_iterator it = connectionList.begin(); it != connectionList.end(); it++) {
		if ((*it)->getRemoteAddr(&addr2) && memcmp(addr2, addr, sizeof(BD_ADDR)) == 0)
			return *it;
	}
	return NULL;
}

bool ProtocolRFCOMM::hasConnection(const RFCOMMConnection *c)
{
	BD_ADDR addr;
	Mutex::AutoLocker l(mutex);
	
	if (!c->getRemoteAddr(&addr))
		return false;

	return _getConnection(addr) != NULL;
}


bool ProtocolRFCOMM::hasConnection(const BD_ADDR addr)
{
	Mutex::AutoLocker l(mutex);
	return _getConnection(addr) != NULL;
}

bool ProtocolRFCOMM::addConnection(const RFCOMMConnection *c)
{
	BD_ADDR addr;
	Mutex::AutoLocker l(mutex);

	if (!c->getRemoteAddr(&addr))
		return false;

	if (_getConnection(addr))
		return false;

	connectionList.push_back(c);

	return true;
}

RFCOMMConnection *ProtocolRFCOMM::getFirstUnassignedConnection()
{
	Mutex::AutoLocker l(mutex);
	
	for (List<const RFCOMMConnection *>::const_iterator it = connectionList.begin(); it != connectionList.end(); it++) {
		if (!(*it)->isAssigned())
			return const_cast<RFCOMMConnection *>(*it);
	}
	return NULL;
}

void ProtocolRFCOMM::removeConnection(const RFCOMMConnection *c)
{
	Mutex::AutoLocker l(mutex);

	connectionList.remove(c);
}

#define MSG_QUEUE_MAX_DATALEN 2000 // What is a good value here? The Bluetooth mtu?

#define DATA_BUFFER_START(p) ((p)->data_buffer)
#define DATA_BUFFER_END(p) ((p)->data_buffer + RFCOMM_DATA_BUFFER_SIZE)
#define DATA_BUFFER_EMPTY(p) ((p)->db_head == (p)->db_tail)
#define DATA_BUFFER_SPACE(p) (((p)->db_head >= (p)->db_tail) ? (RFCOMM_DATA_BUFFER_SIZE - ((p)->db_head - (p)->db_tail)) : ((p)->db_tail  - (p)->db_head))

ProtocolRFCOMMClient::ProtocolRFCOMMClient(RFCOMMConnection *rfCommConn, const InterfaceRef& _localIface, 
		const InterfaceRef& _peerIface, const unsigned short _channel, ProtocolManager *m) :
	ProtocolRFCOMM(rfCommConn, _localIface, _peerIface, _channel, PROT_FLAG_CLIENT | PROT_FLAG_CONNECTED, m), 
			hReadQ(NULL), hWriteQ(NULL), blockingTimeout(INFINITE), db_head(0), db_tail(0)
{
}

ProtocolRFCOMMClient::ProtocolRFCOMMClient(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, 
					   const unsigned short channel, ProtocolManager *m) :
	ProtocolRFCOMM(_localIface, _peerIface, channel, PROT_FLAG_CLIENT, m), db_head(0), db_tail(0)
{
}

ProtocolRFCOMMClient::~ProtocolRFCOMMClient()
{
	HAGGLE_DBG("Destroying %s\n", getName());

	if (hReadQ)
		CloseMsgQueue(hReadQ);
	if (hWriteQ)
		CloseMsgQueue(hWriteQ);
}

bool ProtocolRFCOMMClient::init_derived()
{
	MSGQUEUEOPTIONS mqOpts = { sizeof(MSGQUEUEOPTIONS), MSGQUEUE_NOPRECOMMIT, 0, 
			sizeof(struct QMsg), FALSE };

	if (!peerIface) {
		HAGGLE_ERR("Client has no peer interface\n");
		return false;
	}

	if (!initbase()) {
		HAGGLE_ERR("Initialization of base protocol class failed\n");
		return false;
	}
	hWriteQ = CreateMsgQueue(NULL, &mqOpts);

	if (!hWriteQ) {
		HAGGLE_ERR("Could not create message queue\n");	
		return false;
	}

	mqOpts.bReadAccess = TRUE;

	hReadQ = OpenMsgQueue(GetCurrentProcess(), hWriteQ, &mqOpts);

	if (!hReadQ) {
		HAGGLE_ERR("Could not open read handle for message queue\n");	
		CloseMsgQueue(hWriteQ);
		hWriteQ = NULL;
		return false;
	}
	
	return true;
}

size_t ProtocolRFCOMMClient::dataBufferWrite(const void *data, size_t len)
{	
	Mutex::AutoLocker l(db_mutex);

	if (DATA_BUFFER_SPACE(this) == 0)
		return 0;

	size_t n = 0;
	char *data_ptr = (char *)data;

	while (DATA_BUFFER_SPACE(this) > 0 && n < len) {
		data_buffer[db_head++ % RFCOMM_DATA_BUFFER_SIZE] = data_ptr[n++];
	}

	return n;
}

size_t ProtocolRFCOMMClient::dataBufferRead(void *data, size_t len)
{
	Mutex::AutoLocker l(db_mutex);

	if (DATA_BUFFER_EMPTY(this))
		return 0;

	size_t n = 0;
	char *data_ptr = (char *)data;

	while ((RFCOMM_DATA_BUFFER_SIZE - DATA_BUFFER_SPACE(this)) > 0 && n < len) {
		data_ptr[n++] = data_buffer[db_tail++ % RFCOMM_DATA_BUFFER_SIZE];
	}

	return n;
}

size_t ProtocolRFCOMMClient::dataBufferBytesToRead()
{
	Mutex::AutoLocker l(db_mutex);

	return RFCOMM_DATA_BUFFER_SIZE - DATA_BUFFER_SPACE(this);
}

bool ProtocolRFCOMMClient::dataBufferIsEmpty()
{
	Mutex::AutoLocker l(db_mutex);

	return DATA_BUFFER_EMPTY(this);
}

bool ProtocolRFCOMMClient::setNonblock(bool block)
{
	if (block)
		blockingTimeout = INFINITE;
	else
		blockingTimeout = 0;

	return true;
}

void ProtocolRFCOMMClient::OnEventReceived(UINT32 event_code)
{
	struct QMsg msg;

	// Ignore connection errors if we are not yet connected.
	if (event_code & PORT_EV_CONNECT_ERR) {
		//HAGGLE_ERR("Connection error event\n");

		msg.type = QMSG_TYPE_CONNECTION_ERROR;

		if (!WriteMsgQueue(hWriteQ, &msg, sizeof(struct QMsg), 500, 0)) {
			HAGGLE_ERR("Could not write connection error event to msg queue\n");
		} else {
			HAGGLE_DBG("Wrote connection ERROR event to msg queue\n");
		}
	} 
	if (event_code & PORT_EV_CONNECTED) {
		//HAGGLE_ERR("Client connection event\n");

		/*
			Only generate connection message once. It seems as
			if the stack generates this event multiple times for 
			each successful connection.
		*/
		if (!isConnected()) {
			setFlag(PROT_FLAG_CONNECTED);
			msg.type = QMSG_TYPE_CONNECTION_SUCCESS;

			if (!WriteMsgQueue(hWriteQ, &msg, sizeof(struct QMsg), 500, 0)) {
				HAGGLE_ERR("Could not write connection error event to msg queue\n");
			} else {
				HAGGLE_DBG("Wrote connection SUCCESS event to msg queue\n");
			}
		}
		
		//HAGGLE_DBG("Successfully connected\n");
	} 
	if (event_code & PORT_EV_OVERRUN) {
		HAGGLE_DBG("PORT_EV_OVERRUN\n");
	}
	if (event_code & PORT_EV_ERR) {
		HAGGLE_ERR("Error event\n");
	} 

	//HAGGLE_DBG("Received event_code %u\n", event_code);
}

void ProtocolRFCOMMClient::OnDataReceived(void *p_data, UINT16 len)
{
	struct QMsg msg;

	msg.type = QMSG_TYPE_DATA;

	msg.len = dataBufferWrite(p_data, len);

	if (msg.len == 0) {
		HAGGLE_ERR("Could not write any data to circular buffer\n");
		return;
	} else if (msg.len < len) {
		HAGGLE_ERR("Could not write all the data to the circular data buffer\n");
	}
	BOOL res = WriteMsgQueue(hWriteQ, &msg, sizeof(struct QMsg), 500, 0);

	if (res == FALSE) {
		HAGGLE_ERR("Could not write %u bytes data to message queue\n", len);
	} else {
		//HAGGLE_DBG("Wrote %u bytes out of %u bytes data to message queue\n", msg.len, len);
	}
}

ProtocolEvent ProtocolRFCOMMClient::connectToPeer()
{
	BluetoothAddress *addr;
	
	if (!peerIface) {
		protocol_error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}

	addr = peerIface->getAddress<BluetoothAddress>();

	if (!addr) {
		protocol_error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}

	if (!rfCommConn->connect(channel, addr->getRaw())) {
		HAGGLE_DBG("%s Connection failed to [%s] channel=%u\n", 
			   getName(), addr->getStr(), channel);
	}
	/*
		Check for a connection msg on the message queue.
	*/
	DWORD bytes_read, flags = 0;
	struct QMsg msg;

	if (!ReadMsgQueue(hReadQ, &msg, sizeof(struct QMsg), &bytes_read, 5000, &flags)) {
		if (GetLastError() == ERROR_TIMEOUT) {
			HAGGLE_DBG("%s Connection attempt to [%s] channel=%u timed out\n", 
				getName(), addr->getStr(), channel);
			return PROT_EVENT_TIMEOUT;
		}

		HAGGLE_ERR("Could not read msg queue for connection result\n");
		protocol_error = PROT_ERROR_BAD_HANDLE;
		return PROT_EVENT_ERROR;
	}

	if (msg.type != QMSG_TYPE_CONNECTION_SUCCESS) {
		HAGGLE_ERR("Expected event %u (CONNECTION_SUCCESS) on msg queue, but got %u\n", 
			QMSG_TYPE_CONNECTION_SUCCESS, msg.type);
		protocol_error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}

	HAGGLE_DBG("%s Connected to [%s] channel=%u\n", 
		   getName(), addr->getStr(), channel);

	// Do not set PROT_FLAG_CONNECTED here, wait until we receive a
	// connection successful event.

	return PROT_EVENT_SUCCESS;
}

void ProtocolRFCOMMClient::closeConnection()
{
	CRfCommPort::PORT_RETURN_CODE retcode;

	unSetFlag(PROT_FLAG_CONNECTED);

	setMode(PROT_MODE_DONE);

	removeConnection(rfCommConn);
	retcode = rfCommConn->Close();

	if (retcode != CRfCommPort::SUCCESS && retcode != CRfCommPort::NOT_OPENED) {
		HAGGLE_ERR("Could not close connection to [%s]\n", 
			peerIface->getAddress<BluetoothAddress>()->getStr());
	} 
}

ProtocolEvent ProtocolRFCOMMClient::receiveData(void *buf, size_t len, const int flags, size_t *bytes)
{
	DWORD bytes_read, qflags = 0;
	struct QMsg msg;
	*bytes = 0;
	
	BOOL ret = ReadMsgQueue(hReadQ, &msg, sizeof(struct QMsg), &bytes_read, blockingTimeout, &qflags);

	if (ret == FALSE) {
		HAGGLE_ERR("Could not read data from message queue, err=%d\n", GetLastError());
		if (GetLastError() == ERROR_PIPE_NOT_CONNECTED) {
			
			HAGGLE_ERR("message queue ERROR_PIPE_NOT_CONNECTED\n");
			return PROT_EVENT_PEER_CLOSED;
		}

		protocol_error = PROT_ERROR_BAD_HANDLE;

		return PROT_EVENT_ERROR;
	} 

	if (msg.type == QMSG_TYPE_CONNECTION_SUCCESS) {
		// We should not really get this message type here since
		// it is already handled in connectToPeer(). However, lets
		// handle the type here anyway just in case and return
		// success with zero bytes read.
		HAGGLE_DBG("Read connection SUCCESS event from msg queue\n");
		return PROT_EVENT_SUCCESS;
	} else if (msg.type == QMSG_TYPE_CONNECTION_ERROR) {
		HAGGLE_ERR("Got CONNECTION_ERROR event - peer closed?\n");
		return PROT_EVENT_PEER_CLOSED;
	} else if (msg.type != QMSG_TYPE_DATA) {
		HAGGLE_ERR("WARNING: Unknown Queue message type\n");
		protocol_error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}

	//HAGGLE_DBG("Got message saying that there is %lu bytes data to read\n", msg.len);

	if (msg.len > len) {
		*bytes = len;
	} else {
		*bytes = msg.len;
	}

	*bytes = dataBufferRead(buf, *bytes);

	// If there is still data to read, signal the message queue again
	if (*bytes < msg.len) {
		msg.type = QMSG_TYPE_DATA;
		msg.len = msg.len - *bytes;

		BOOL res = WriteMsgQueue(hWriteQ, &msg, sizeof(struct QMsg), 500, 0);

		if (res == FALSE) {
			HAGGLE_ERR("Could not write data msg to message queue\n");
		} else {
			//HAGGLE_DBG("Signaled to queue that there is still %lu bytes data to read\n", msg.len);
		}
	}

	if (*bytes == 0) {
		HAGGLE_ERR("Error: received 0 bytes\n");
		protocol_error = PROT_ERROR_WOULD_BLOCK;
		return PROT_EVENT_ERROR;
	}
	//HAGGLE_DBG("Successfully read %lu bytes data\n", *bytes);

	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolRFCOMMClient::sendData(const void *buf, size_t len, const int flags, size_t *bytes)
{
	UINT16 bytes_written;
	UINT16 to_write = len;

	*bytes = 0;

	CRfCommPort::PORT_RETURN_CODE ret = rfCommConn->Write((void *)buf, to_write, &bytes_written);

	if (ret == CRfCommPort::PEER_CONNECTION_FAILED)
		return PROT_EVENT_PEER_CLOSED;
	else if (ret != CRfCommPort::SUCCESS) {
		// Set an error that would make some sense
		protocol_error = PROT_ERROR_BAD_HANDLE;
		return PROT_EVENT_ERROR;
	}

	//HAGGLE_DBG("Wrote %u bytes data out of %lu requested\n", bytes_written, len);

	*bytes = (size_t)bytes_written;
	
	if (*bytes == 0) {
		protocol_error = PROT_ERROR_WOULD_BLOCK;
		return PROT_EVENT_ERROR;
	}

	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolRFCOMMClient::waitForEvent(Timeval *timeout, bool writeevent)
{
	Watch w;
	int ret, index;

	/* 
		Haven't found a way to check writable status. Therefore,
		if we are checking for writeability, we assume that
		we can write.
	*/

	if (writeevent)
		return PROT_EVENT_WRITEABLE;

	index = w.add(hReadQ, WATCH_STATE_READ);
	
	ret = w.wait(timeout);

	if (ret == Watch::TIMEOUT) {
		return PROT_EVENT_TIMEOUT;
	} else if (ret == Watch::FAILED) {
		protocol_error = PROT_ERROR_BAD_HANDLE;
		return PROT_EVENT_ERROR;
	} else if (ret == Watch::ABANDONED)
		return PROT_EVENT_SHOULD_EXIT;

	if (w.isReadable(index))
		return PROT_EVENT_INCOMING_DATA;
	
	protocol_error = PROT_ERROR_UNKNOWN;

	return PROT_EVENT_ERROR;
}

ProtocolEvent ProtocolRFCOMMClient::waitForEvent(DataObjectRef &dObj, Timeval *timeout, bool writeevent)
{
	QueueElement *qe = NULL;
	Queue *q = getQueue();
	
	if (!q) {
		protocol_error = PROT_ERROR_INVALID_ARGUMENT;
		return PROT_EVENT_ERROR;
	}

	if (writeevent)
		timeout = NULL;

	QueueEvent_t qev = q->retrieve(&qe, hReadQ, timeout, false);

	switch (qev) {
	case QUEUE_TIMEOUT:
		if (writeevent)
			return PROT_EVENT_WRITEABLE;
		return  PROT_EVENT_TIMEOUT;
	case QUEUE_WATCH_ABANDONED:
		return PROT_EVENT_SHOULD_EXIT;
	case QUEUE_WATCH_READ:
		return PROT_EVENT_INCOMING_DATA;
	case QUEUE_WATCH_WRITE:
		return PROT_EVENT_WRITEABLE;
	case QUEUE_ELEMENT:
		dObj = qe->getDataObject();
		delete qe;
		return PROT_EVENT_TXQ_NEW_DATAOBJECT;
	case QUEUE_EMPTY:
		return PROT_EVENT_TXQ_EMPTY;
	default:
		break;
	}

	protocol_error = PROT_ERROR_UNKNOWN;

	return PROT_EVENT_ERROR;
}

void ProtocolRFCOMMClient::hookShutdown()
{
	closeConnection();
}

ProtocolRFCOMMServer::ProtocolRFCOMMServer(const InterfaceRef& localIface, ProtocolManager *m, unsigned short channel) :
	ProtocolRFCOMM(localIface, NULL, channel, PROT_FLAG_SERVER, m)
{
}

ProtocolRFCOMMServer::~ProtocolRFCOMMServer()
{
	HAGGLE_DBG("Destroying %s\n", getName());
	
	getKernel()->unregisterWatchable(connectionEvent);
	
	CloseHandle(connectionEvent);

	if (rfCommConn)
		rfCommConn->Close();
}


bool ProtocolRFCOMMServer::init_derived()
{
	if (!initbase())
		return false;

	connectionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!connectionEvent) {
		HAGGLE_ERR("Could not create connection event\n");
		return false;
	}
	if (!setListen()) {
		CloseHandle(connectionEvent);
		connectionEvent = NULL;
		HAGGLE_ERR("Could not listen on channel %u\n", channel);
		return false;
	}
	
	if (getKernel()->registerWatchable(connectionEvent, getManager()) < 0) {
		CloseHandle(connectionEvent);
		connectionEvent = NULL;
		HAGGLE_ERR("Could not register watchable\n");
		return false;
	}

	return true;
}

bool ProtocolRFCOMMServer::hasWatchable(const Watchable &wbl)
{
	return wbl == connectionEvent;
}

void ProtocolRFCOMMServer::handleWatchableEvent(const Watchable &wbl)
{
	if (wbl != connectionEvent) {
		HAGGLE_ERR("ERROR! : %s does not belong to Protocol %s\n", wbl.getStr(), getName());
		return;
	}

	HAGGLE_DBG("Connection attempt\n");
	acceptClient();
}

bool ProtocolRFCOMMServer::setListen(int dummy)
{
	CRfCommPort::PORT_RETURN_CODE retcode;

	retcode = rfCommConn->OpenServer(channel & 0xff);

	if (retcode != CRfCommPort::SUCCESS)  {
		HAGGLE_ERR("Could not open server port %u, err=%d\n", channel, retcode);

		switch (retcode) {
		case CRfCommPort::ALREADY_OPENED:
			HAGGLE_ERR("port already opened\n");
			break;
		case CRfCommPort::INVALID_PARAMETER:
			HAGGLE_ERR("invalid parameter\n");
			break;
		case CRfCommPort::UNKNOWN_ERROR:
			HAGGLE_ERR("unknown error\n");
			break;
		case CRfCommPort::NOT_OPENED:
			HAGGLE_ERR("not opened\n");
			break;
		}
		return false;
	}

	setMode(PROT_MODE_LISTENING);

	HAGGLE_DBG("Listening for connections on channel %u\n", channel);

	return true;
}

ProtocolEvent ProtocolRFCOMMServer::acceptClient()
{
	if (getMode() != PROT_MODE_LISTENING) {
		HAGGLE_ERR("%s: cannot accept connection on non-listening protocol\n", getName());
		protocol_error = PROT_ERROR_CONNECTION_REFUSED;
		return PROT_EVENT_ERROR;
	}

	RFCOMMConnection *c = getFirstUnassignedConnection();
	
	if (!c || !c->getProtocol() || !c->getProtocol()->isClient()) {
		HAGGLE_ERR("No valid unassigned connection\n");
		protocol_error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}

	c->setAssigned();

	ProtocolRFCOMMClient *p = static_cast<ProtocolRFCOMMClient *>(c->getProtocol());

	if (!p) {
		HAGGLE_ERR("Bad protocol\n");
		protocol_error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}

	HAGGLE_DBG("Accepted client %s, starting client thread\n", 
		p->getPeerInterface()->getIdentifierStr());
	
	p->registerWithManager();

	ProtocolEvent pev = p->startTxRx();
	
	HAGGLE_DBG("Started new RFCOMM server\n");

	return pev;
}

void ProtocolRFCOMMServer::OnEventReceived(UINT32 event_code)
{
	static BD_ADDR remote_addr;

	if (event_code & PORT_EV_CONNECT_ERR) {
		HAGGLE_ERR("Connection Error - close...\n");
	} 
	if (event_code & PORT_EV_CONNECTED) {
		HAGGLE_DBG("Connection Event\n");

		// Check that we are actually connected, and that we do not already
		// have a connection for this address.
		if (rfCommConn->IsConnected(&remote_addr) && !hasConnection(remote_addr)) {
			Watch w;

			HAGGLE_DBG("Peer %02x:%02x:%02x:%02x:%02x:%02x connected. Adding to connection list...\n",
				remote_addr[0], remote_addr[1], remote_addr[2], 
				remote_addr[3], remote_addr[4], remote_addr[5]); 

			/*
			Lock Protocol mutex to protect the connection pointer and list,
			since this callback is called from another thread.
			*/
			mutex.lock();

			RFCOMMConnection *client_rfCommConn = rfCommConn;

			rfCommConn = new RFCOMMConnection(this);

			if (!setListen()) {
				setMode(PROT_MODE_IDLE);
				HAGGLE_ERR("Could not set listen\n");
			}

			mutex.unlock();

			BluetoothAddress addr(remote_addr);

			InterfaceRef iface = Interface::create<BluetoothInterface>(remote_addr, "Peer Bluetooth", addr, IFFLAG_UP);
	
			if (!iface) {
				HAGGLE_ERR("Could not create new peer interface!!!\n");
				return;
			}

			// Create a client immediately and associate with connection, but
			// do not start it until acceptClient() is called.
			ProtocolRFCOMMReceiver *p = new ProtocolRFCOMMReceiver(client_rfCommConn, 
				this->getLocalInterface(), iface, (unsigned short)channel, getManager());

			if (!p || !p->init()) {
				HAGGLE_ERR("Could not create new protocol receiver!!!\n");

				if (p)
					delete p;

				return;
			}	

			addConnection(client_rfCommConn);
			
			/*
			Notify the main thread that there is a new connection.
			*/
			w.add(connectionEvent);
			Timeval timeout = 0;

			if (w.wait(&timeout) == Watch::TIMEOUT) {
				HAGGLE_DBG("Connection event set\n");
				SetEvent(connectionEvent);
			}
		}
	} 
	if (event_code & PORT_EV_OVERRUN) {
		HAGGLE_DBG("PORT_EV_OVERRUN\n");
	}
	if (event_code & PORT_EV_ERR) {
		HAGGLE_ERR("Error event\n");
	} 
	HAGGLE_DBG("Received event_code %u\n", event_code);
}

#endif /* ENABLE_BLUETOOTH && !OS_MACOSX */
