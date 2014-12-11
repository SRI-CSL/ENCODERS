/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Joshua Joy (JJ, jjoy)
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 *   James Mathewson (JM, JLM)
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

#include "Protocol.h"

#if defined(OS_LINUX)
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#elif defined(OS_MACOSX)
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#else 
// OS_WINDOWS
#endif

// This include is for the HAGGLE_ATTR_CTRL_NAME attribute name. 
// TODO: remove dependency on libhaggle header
#include "../libhaggle/include/libhaggle/ipc.h"`

// SW: START SEND PRIORITY
#include "SendPriorityManager.h"
// SW: END SEND PRIORITY

// SW: START REPLICATION MANAGER
#include "ReplicationManagerFactory.h"
// SW: END REPLICATION MANAGER

/*
  -------------------------------------------------------------------------------
  Protocol code
*/
unsigned int Protocol::num = 0;

const char *Protocol::typestr[] = {
	"LOCAL",
	"UDP",
	"TCP",
	"RAW",
// SW: START: UDP UNICAST PROTOCOL
    "UDP_UNICAST",
// SW: END: UDP UNICAST PROTOCOL
// SW: START: UDP BROADCAST PROTOCOL
    "UDP_BROADCAST",
// SW: END: UDP BROADCAST PROTOCOL
#ifdef OMNETPP
	"OMNET++",
#endif
#if defined(ENABLE_BLUETOOTH)
	"RFCOMM",
#endif
#if defined(ENABLE_MEDIA)
	"MEDIA",
#endif
	NULL
};

const char *Protocol::protocol_error_str[] = {
        "PROT_ERROR_WOULD_BLOCK",
        "PROT_ERROR_BAD_HANDLE",
        "PROT_ERROR_CONNECTION_REFUSED",
        "PROT_ERROR_INTERRUPTED",
        "PROT_ERROR_INVALID_ARGUMENT",
        "PROT_ERROR_NO_MEMORY",
        "PROT_ERROR_NOT_CONNECTED",
        "PROT_ERROR_CONNECTION_RESET",
        "PROT_ERROR_NOT_A_SOCKET",
        "PROT_ERROR_NO_STORAGE_SPACE",
        "PROT_ERROR_SEQ_NUM", // MOS
        "PROT_ERROR_UNKNOWN",
	NULL
};

static inline char *create_name(const char *name, const unsigned long id, const int flags)
{
	static char tmpStr[50];
	
	snprintf(tmpStr, 50, "%s%s:%lu", name, flags & PROT_FLAG_SERVER ? "Server" : "Client", id);
	
	return tmpStr;
}

Protocol::Protocol(const ProtType_t _type, const string _name, const InterfaceRef& _localIface, 
		   const InterfaceRef& _peerIface, const int _flags, ProtocolManager *_m, size_t _bufferSize) : 
	ManagerModule<ProtocolManager>(_m, create_name(_name.c_str(), num + 1,  _flags)),
	isRegistered(false), type(_type), id(num++), error(PROT_ERROR_UNKNOWN), flags(_flags), 
	mode(PROT_MODE_IDLE), localIface(_localIface), peerIface(_peerIface), peerNode(NULL),
	buffer(NULL), bufferSize(_bufferSize), bufferDataLen(0), cfg(NULL),
	dataObjectsOutgoing(0), dataObjectsIncoming(0),
	dataObjectsOutgoingNonControl(0), dataObjectsIncomingNonControl(0),
	dataObjectBytesOutgoing(0), dataObjectBytesIncoming(0),
	dataObjectsNotReceived(0), dataObjectsRejected(0), 
	dataObjectFragmentsRejected(0), dataObjectBlocksRejected(0), 
	dataObjectsAccepted(0), dataObjectsAcknowledged(0),
	dataObjectsNotSent(0), dataObjectsSent(0), dataObjectsSentAndAcked(0), dataObjectsReceived(0),
	dataObjectSendExecutionTime(0.0), dataObjectReceiveExecutionTime(0.0),
	dataObjectBytesSent(0), dataObjectBytesReceived(0),
	nodeDescSent(0), nodeDescReceived(0),
	nodeDescBytesSent(0), nodeDescBytesReceived(0),
	appNodeDescReceived(0), appNodeDescBytesReceived(0), appNodeDescSent(0), appNodeDescBytesSent(0)
{
	HAGGLE_DBG("%s Buffer size is %lu\n", getName(), bufferSize);

	this->networkCodingProtocolHelper = new NetworkCodingProtocolHelper();
}

bool Protocol::init()
{
	if (buffer) {
		HAGGLE_ERR("%s Initializing already initialized protocol\n", getName());
		return false;
	}
	buffer = new unsigned char[bufferSize];

	if (!buffer) {
		HAGGLE_ERR("%s Could not allocate buffer of size %lu\n", getName(), bufferSize);
		return false;
	}
	
	// Cache the peer node here. If the node goes away, it may be taken out of the
	// node store before the protocol quits, and then we cannot retrieve it for the
	// send result event.
	if (!isServer() && isClient()) {
		if (!peerIface) {
			HAGGLE_ERR("%s No peer interface for protocol !\n", getName());
			return false;
		}

		// Try to figure out the peer node. This might fail in case this 
		// is an incoming connection that we didn't initiate ourselves,
		// i.e., there is no peer node in the node store yet.
		peerNode = getManager()->getKernel()->getNodeStore()->retrieve(peerIface);

		if (!peerNode) {
			// Set a temporary peer node. It will be updated by the protocol manager
			// when it received a node updated event once the node description
			// has been received.
			peerNode = Node::create(Node::TYPE_UNDEFINED, "Peer node");

			if (!peerNode)
				return false;

			peerNode->addInterface(peerIface);
		}
	}

	return init_derived();
}

Protocol::~Protocol()
{
	if (isRegistered) {
	        HAGGLE_ERR("%s deleting still registered protocol\n", getName()); // MOS - can happen during forced shutdown
	}
	
	HAGGLE_DBG("%s destroyed\n", getName());
	
	if (buffer) {
		delete[] buffer;
		buffer = NULL;
	}

	// If there is anything in the queue, these should be data objects, and if 
	// they are here, they have not been sent. So send an
	// EVENT_TYPE_DATAOBJECT_SEND_FAILURE for each of them:

	if(!isDetached()) // MOS
	  closeAndClearQueue();

	if( this->networkCodingProtocolHelper) {
	    delete this->networkCodingProtocolHelper;
	}

// SW: START CONFIG:
    if (cfg) {
        delete cfg;
    }
// SW: END CONFIG.
}

// MOS
void Protocol::closeQueue()
{
	Queue *q = getQueue();
	// Make sure that the queue is closed and no more inserts are allowed
	q->close();
}

unsigned long Protocol::closeAndClearQueue()
{
	unsigned long count = 0;
	
	// No need to do getQueue() more than once:
	Queue *q = getQueue();
	
	if (!q)
		return count;
	
	// Make sure that the queue is closed and no more inserts are allowed
	q->close();
	
	// With all the data objects in the queue:
	while (!q->empty()) {
		QueueElement *qe = NULL;
		DataObjectRef dObj;
		
		// Get the first element. Don't delay:
		switch (q->retrieveTry(&qe)) {
			default:
			        HAGGLE_ERR("%s Error when closing and clearing protocol queue\n", getName());
			        goto done; // MOS - exit loop on error
			case QUEUE_TIMEOUT:
			        HAGGLE_ERR("%s Timeout when closing and clearing protocol queue\n", getName());
			        goto done; // MOS - exit loop on error
			case QUEUE_EMPTY:
				break;
			case QUEUE_ELEMENT:
				// Get data object:
				dObj = qe->getDataObject();
				
				if (dObj) {
					// Tell the rest of haggle that this data object was not 
					// sent:
				        NodeRef currentPeer;
				        { Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS
				        getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, currentPeer));
// SW: START SEND PRIORITY
                        getKernel()->addEvent(SendPriorityManager::getSendPriorityFailureEvent(dObj, currentPeer));
// SW: END SEND PRIORITY
					count++;
				}
				break;
		}
		// Delete the queue element:
		if (qe)
			delete qe;
	}

 done:
	return count;
}

bool Protocol::hasIncomingData()
{
	return (waitForEvent() == PROT_EVENT_INCOMING_DATA);
}

ProtocolEvent Protocol::waitForEvent(Timeval *timeout, bool writeevent)
{
	Watch w;
	int ret;

	ret = w.wait(timeout);
	
	if (ret == Watch::TIMEOUT)
		return PROT_EVENT_TIMEOUT;
	else if (ret == Watch::FAILED)
		return PROT_EVENT_ERROR;

	return PROT_EVENT_INCOMING_DATA;
}

ProtocolEvent Protocol::waitForEvent(DataObjectRef& dObj, Timeval *timeout, bool writeevent)
{	
	QueueElement *qe = NULL;
	Queue *q = getQueue();

	if (!q)
		return PROT_EVENT_ERROR;

	QueueEvent_t qev = q->retrieve(&qe, timeout);

	switch (qev) {
	case QUEUE_TIMEOUT:
		return  PROT_EVENT_TIMEOUT;
	case QUEUE_ELEMENT:
		dObj = qe->getDataObject();
		delete qe;
		return PROT_EVENT_TXQ_NEW_DATAOBJECT;
	default:
		break;
	}
	return PROT_EVENT_ERROR;
}

void Protocol::closeConnection()
{
	return;
}

bool Protocol::isForInterface(const InterfaceRef& the_iface)
{
	if (!the_iface)
		return false;

	// if (localIface && the_iface == localIface) // MOS - this function is used to find protocol based on remote interface
	// return true;                               //       need to check if there are any other uses

	if (peerIface && the_iface == peerIface)
		return true;

	return false;
}

unsigned long Protocol::getId() const 
{
	return id;
} 

ProtType_t Protocol::getType() const 
{
	return type;
} 

void Protocol::handleInterfaceDown(const InterfaceRef& iface)
{
	if (localIface == iface)
		shutdown();
	else if (peerIface && peerIface == iface)
		shutdown();
}

bool Protocol::isSender() 
{
	return false;
}

bool Protocol::isReceiver()
{
	return false;
}

bool Protocol::isApplication() const
{
	if (localIface && localIface->isApplication())
		return true;
	return false;
}

bool Protocol::hasWatchable(const Watchable &wbl)
{
	return false;
}

void Protocol::handleWatchableEvent(const Watchable &wbl)
{
}

string Protocol::peerDescription()
{
	Mutex::AutoLocker l(mutex); // MOS
	string peerstr = "Unknown peer";
	NodeRef peer = peerNode;
	if (peer) {
		if (peer->getType() != Node::TYPE_UNDEFINED) {
		        peerstr = peer->getName();
		} else {
		  InterfaceRef iface = peerIface;
		  if (iface) peerstr = iface->getIdentifierStr();
		}
	}

	return peerstr;
}

ProtocolError Protocol::getProtocolError()
{
	return PROT_ERROR_UNKNOWN;
}

const char *Protocol::getProtocolErrorStr()
{
	static const char *unknownErrStr = "Unknown error";

	return unknownErrStr;
}

#if defined(OS_LINUX) || defined (OS_MACOSX)
// Function to translate platform specific errors into something more
// useful
int Protocol::getFileError()
{
	return getProtocolError();
}
const char *Protocol::getFileErrorStr()
{
	// We could append the system error string from strerror
	//return (error < _PROT_ERROR_MAX && error > _PROT_ERROR_MIN) ? errorStr[error] : "Bad error";
	return strerror(errno);
}
#else

int Protocol::getFileError()
{
	switch (GetLastError()) {
	case ERROR_INVALID_HANDLE:
		error = PROT_ERROR_BAD_HANDLE;
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
		error = PROT_ERROR_NO_MEMORY;
		break;
	case ERROR_OUTOFMEMORY:
		error = PROT_ERROR_NO_STORAGE_SPACE;
	default:
		error = PROT_ERROR_UNKNOWN;
		break;
	}

	return error;
}
const char *Protocol::getFileErrorStr()
{
	static char *errStr = NULL;
	LPVOID lpMsgBuf;

	if (errStr)
		delete [] errStr;

	DWORD len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL,
				  GetLastError(),
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPTSTR) & lpMsgBuf,
				  0,
				  NULL);
	if (len) {
		errStr = new char[len + 1];
		sprintf(errStr, "%s", reinterpret_cast < TCHAR * >(lpMsgBuf));
		LocalFree(lpMsgBuf);
	}
	return errStr;
	//return (error < _PROT_ERROR_MAX && error > _PROT_ERROR_MIN) ? errorStr[error] : "Bad error";
}
#endif

const char *Protocol::protocolErrorToStr(const ProtocolError e) const
{
	return protocol_error_str[e];
}

#if defined(OS_WINDOWS_XP) && !defined(DEBUG)
// This is here to avoid a warning with catching the exception in the function below.
#pragma warning( push )
#pragma warning( disable: 4101 )
#endif
ProtocolEvent Protocol::startTxRx()
{
	if (isClient()) {
		if (isRunning()) {
			return PROT_EVENT_SUCCESS;
		}
		return start() ? PROT_EVENT_SUCCESS : PROT_EVENT_ERROR;
	}
	HAGGLE_DBG("%s Client flag not set\n", getName());
	return PROT_EVENT_ERROR;
}

#if defined(OS_WINDOWS_XP) && !defined(DEBUG)
#pragma warning( pop )
#endif

/* Convenience function to send a single data object. Adds a data
   object to the queue and starts the thread.  This will only work if
   the protocol is in idle mode, i.e., it was just created. */
bool Protocol::sendDataObject(const DataObjectRef& dObj, 
			      const NodeRef& peer, 
			      const InterfaceRef& iface)
{
	if (mode == PROT_MODE_DONE || mode == PROT_MODE_GARBAGE) {
		HAGGLE_DBG("%s Protocol is no longer valid\n", getName());
		return false;
	}

	Queue *q = getQueue();

	if (!q) {
		HAGGLE_ERR("%s No valid Queue for protocol\n", getName());
		return false;
	}
	if (isGarbage()) {
		HAGGLE_DBG("%s Send DataObject on garbage protocol, trying to recycle?\n", getName());
		return false;
	}

	if (!peer) {
		HAGGLE_ERR("%s Target node is NULL\n", getName());
		return false;
	}
	
	NodeRef currentPeer;
	{ Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS

	if (!currentPeer || (currentPeer->getType() == Node::TYPE_UNDEFINED && 
			            peer->getType() != Node::TYPE_UNDEFINED)) {
		HAGGLE_DBG("%s Setting peer node to %s, which was previously undefined\n", getName(), 
			   peer->getName().c_str());
        setPeerNode(peer);
	} else if (currentPeer != peer) {
		HAGGLE_ERR("%s Target node %s does not match peer %s in protocol\n", getName(), 
			   peer->getName().c_str(), currentPeer->getName().c_str());
		return false;
	}

	// We do not have to care about the interface here since we assume the protocol
	// is already connected to a peer interface.
	QueueElement *qe = new QueueElement(dObj, peer, iface);

	if (!qe)
		return false;

	if (!q->insert(qe, true)) {
		delete qe;
		HAGGLE_DBG("%s Data object [%s] already in protocol send queue for node %s [%s]\n", getName(), 
			DataObject::idString(dObj).c_str(), peer->getName().c_str(), peer->getIdStr());
		return false;
	} else {
	  // MOS
	  int qsize = q->size();
	  HAGGLE_DBG("%s Data object [%s] inserted into protocol send queue for node %s [%s] - queue size: %u\n", 
		     getName(), DataObject::idString(dObj).c_str(), peer->getName().c_str(), peer->getIdStr(), qsize);
	}
	
	startTxRx();

	return true;
}

// MOS
int Protocol::getWaitTimeBeforeDoneMillis()
{
  return getConfiguration()->getWaitTimeBeforeDoneMillis();
}

ProtocolEvent Protocol::getData(size_t *bytesRead)
{
	long readLen = 0;
	ProtocolEvent pEvent = PROT_EVENT_SUCCESS;
	Timeval waitTimeout = (getConfiguration()->getConnectionWaitMillis() / (double) 1000);
	int blockCount = 0;
	
        while (pEvent == PROT_EVENT_SUCCESS) {
                pEvent = waitForEvent(&waitTimeout);
                
                if (pEvent == PROT_EVENT_INCOMING_DATA) {
                        
                        readLen = bufferSize - bufferDataLen;

			if (readLen <= 0) {
				HAGGLE_ERR("%s Read buffer is full!\n", getName());
				*bytesRead = 0;
				return PROT_EVENT_ERROR;
			}
                        
                        pEvent = receiveData(buffer, readLen, 0, bytesRead);
                        
                        if (pEvent == PROT_EVENT_ERROR) {
                                switch (getProtocolError()) {
				case PROT_ERROR_BAD_HANDLE:
				case PROT_ERROR_NOT_CONNECTED:
				case PROT_ERROR_NOT_A_SOCKET:
					HAGGLE_ERR("%s Fatal error! %s\n", getName(),
							   getProtocolErrorStr());
					return PROT_EVENT_ERROR_FATAL;
				case PROT_ERROR_CONNECTION_RESET:
					HAGGLE_DBG("%s %s\n", getName(), getProtocolErrorStr());
					return PROT_EVENT_PEER_CLOSED;
				case PROT_ERROR_SEQ_NUM: // MOS
				        HAGGLE_ERR("Protocol error : %s\n", getProtocolErrorStr());
					break;
				case PROT_ERROR_WOULD_BLOCK:
					if (blockCount++ < getConfiguration()->getMaxBlockingTries()) {
						// Set event to success so that the loop does not quit
						pEvent = PROT_EVENT_SUCCESS;
						
						/*
						  On Windows mobile, we repeatedly get a WSAEWOULDBLOCK error
						  here when runnig on over RFCOMM. The protocol probably reads 
						  faster than data arrives over the bluetooth channel.
						  Even if the socket is set to blocking mode by default,
						  we get WSAEWOULDBLOCK anyway. I guess, for now, we just
						  have to live with regular errors and just let the protocol
						  try again after a short sleep time. 
						  
                                                          In order to avoid repeated debug messages here as data is
                                                          received, we only print the debug message once we reach
                                                          max block count in the case of Windows mobile.
						*/
#ifdef OS_WINDOWS_MOBILE
						if (blockCount == getConfiguration()->getMaxBlockingTries()) {
							HAGGLE_DBG("%s Receive would block, try number %d/%d\n", getName(),
                                                                   blockCount, getConfiguration()->getMaxBlockingTries());
						}
#else
						HAGGLE_DBG("%s Receive would block, try number %d/%d in %.3lf seconds\n", getName(),
							   blockCount, getConfiguration()->getMaxBlockingTries(), 
							   (double)getConfiguration()->getBlockingSleepMillis() / 1000);
#endif
						
						cancelableSleep(getConfiguration()->getBlockingSleepMillis());
						
					} else {
						HAGGLE_DBG("%s Max tries reached,"
							   " giving up\n", getName());
						pEvent = PROT_EVENT_ERROR_FATAL;
					}
					break;
				default:
				  HAGGLE_ERR("%s Protocol error : %s\n", getName(), getProtocolErrorStr()); // MOS - fixed message
                                }
                        } else if (pEvent == PROT_EVENT_PEER_CLOSED) {
                                HAGGLE_DBG("%s Peer [%s] closed connection\n", 
					   getName(), peerDescription().c_str());
                        }

                        if (pEvent == PROT_EVENT_SUCCESS && // MOS - this is impoartant
			    *bytesRead > 0) {
                                // This indicates a successful read
                                bufferDataLen += *bytesRead;
                                break;
                        }
                } else if (pEvent == PROT_EVENT_TIMEOUT) {
                        HAGGLE_DBG("%s Protocol timed out while getting data\n", 
				   getName());
                } 
        }
	return pEvent;
}

void Protocol::removeData(size_t len)
{
	size_t i;
	
	// Make sure there is something to do:
	if (len <= 0)
		return;
	
	// Will any bytes be left after the move?
	if (len >= bufferDataLen) {
		// No? Then we don't need to move any bytes.
		bufferDataLen = 0;
		return;
	}
	
	// Move the bytes left over to where they should be:
	for (i = 0; i < bufferDataLen-len; i++)
		buffer[i] = buffer[i+len];
	// Adjust bufferDataLen:
	bufferDataLen -= len;
}

const string Protocol::ctrlmsgToStr(struct ctrlmsg *m) const
{
        if (!m)
                return "Bad control message";

        // Return the right string based on type:
        switch (m->type) {
                case CTRLMSG_TYPE_ACK:
			return "ACK";
                case CTRLMSG_TYPE_ACCEPT:
                        return "ACCEPT";
                case CTRLMSG_TYPE_REJECT:
			return "REJECT";
		case CTRLMSG_TYPE_TERMINATE:
			return "TERMINATE";
		case CTRLMSG_TYPE_REJECT_PARENTDATAOBJECT:
			return "REJECT2";
		default:
		{
			char buf[30];
                        snprintf(buf, 30, "Unknown message type=%u", m->type);
			return buf;
		}
        }
        // Shouldn't be able to get here, but still...
        return "Bad control message";
}


ProtocolEvent Protocol::sendControlMessage(struct ctrlmsg *m)
{
	ProtocolEvent pEvent;
	size_t bytesRead;
	Timeval waitTimeout = (getConfiguration()->getConnectionWaitMillis() / (double) 1000);

	HAGGLE_DBG2("%s Waiting for writable event\n", getName());
	pEvent = waitForEvent(&waitTimeout, true);
	
	if (pEvent == PROT_EVENT_WRITEABLE) {
		HAGGLE_DBG("%s sending control message %s\n", getName(), ctrlmsgToStr(m).c_str());

		pEvent = sendData(m, sizeof(struct ctrlmsg), 0, &bytesRead);
		
		if (pEvent == PROT_EVENT_SUCCESS) {
			HAGGLE_DBG2("%s Sent %u bytes control message '%s'\n", getName(), bytesRead, ctrlmsgToStr(m).c_str());
		} else if (pEvent == PROT_EVENT_PEER_CLOSED) {
			HAGGLE_ERR("%s Could not send control message '%s': peer closed\n", getName(), ctrlmsgToStr(m).c_str());
		} else {
			switch (getProtocolError()) {
				case PROT_ERROR_BAD_HANDLE:
				case PROT_ERROR_NOT_CONNECTED:
				case PROT_ERROR_NOT_A_SOCKET:
				case PROT_ERROR_CONNECTION_RESET:
					pEvent = PROT_EVENT_ERROR_FATAL;
					break;
				default:
					break;
			}
			HAGGLE_ERR("%s Could not send control message '%s': error %s\n", getName(), ctrlmsgToStr(m).c_str(), getProtocolErrorStr());
		}
	} else if (pEvent == PROT_EVENT_TIMEOUT) {
		HAGGLE_DBG("%s Protocol timed out while waiting to send control message '%s'\n", getName(), ctrlmsgToStr(m).c_str());
	} else {
		HAGGLE_ERR("%s Protocol was not writeable when sending control message '%s'\n", getName(), ctrlmsgToStr(m).c_str());
	}

        return pEvent;
}

ProtocolEvent Protocol::receiveControlMessage(struct ctrlmsg *m)
{
	int blockCount = 0;
	ProtocolEvent pEvent = PROT_EVENT_SUCCESS;
	Timeval waitTimeout;
	
	// Repeat until we get a byte/"message" or permanently fail:
	while (pEvent == PROT_EVENT_SUCCESS) {
		waitTimeout = (getConfiguration()->getConnectionWaitMillis() / (double) 1000);
		// Wait for there to be some readable data:
		pEvent = waitForEvent(&waitTimeout);
		
		// Check return value:
		if (pEvent == PROT_EVENT_TIMEOUT) {
                        HAGGLE_DBG("%s Got a timeout while waiting for control message\n", getName());
		} else if (pEvent == PROT_EVENT_INCOMING_DATA) {
			// Incoming data, this is what we've been waiting for:
			size_t bytesReceived;
			
			// Get the message:
			pEvent = receiveData(m, sizeof(struct ctrlmsg), 0, &bytesReceived);

			// Did we get it?
			if (pEvent == PROT_EVENT_SUCCESS) {
				if (bytesReceived != sizeof(struct ctrlmsg)) {
					pEvent = PROT_EVENT_ERROR;
					HAGGLE_ERR("%s Control message has bad size %lu, expected %lu\n", getName(), 
						bytesReceived, sizeof(struct ctrlmsg));
				} else {
					HAGGLE_DBG2("%s Got control message '%s', %lu bytes\n", getName(), 
						ctrlmsgToStr(m).c_str(), bytesReceived);
				}
                                break;
                        } else if (pEvent == PROT_EVENT_ERROR) {
                                switch (getProtocolError()) {
                                        case PROT_ERROR_BAD_HANDLE:
                                        case PROT_ERROR_NOT_CONNECTED:
                                        case PROT_ERROR_NOT_A_SOCKET:
                                        case PROT_ERROR_CONNECTION_RESET:
                                                // These are fatal errors:
                                                pEvent = PROT_EVENT_ERROR_FATAL;
                                                
                                                // We didn't receive the message
                                                // Unknown error, assume fatal:
                                                HAGGLE_ERR("%s Protocol error : %s\n", getName(), getProtocolErrorStr());
                                                break;
				        case PROT_ERROR_SEQ_NUM: // MOS
                                                HAGGLE_ERR("Protocol error : %s\n", getProtocolErrorStr());
                                                pEvent = PROT_EVENT_SUCCESS; // MOS - ignore control messages iwth wrong seq num
                                                break;
                                        case PROT_ERROR_WOULD_BLOCK:
                                                // This is a non-fatal error, but after a number of retries we
                                                // give up.
                                                if (blockCount++ > getConfiguration()->getMaxBlockingTries()) {
                                                        
#ifdef OS_WINDOWS_MOBILE
                                                        // Windows mobile: this appears to happen a lot, so we reduce
                                                        // the amount of output:
                                                        HAGGLE_DBG("%s Receive would block, try number %d/%d\n", getName(),
								   blockCount, getConfiguration()->getMaxBlockingTries());
#endif
                                                        break;
                                                }
                                                
#ifndef OS_WINDOWS_MOBILE
                                                HAGGLE_DBG("Receive would block, try number %d/%d in %.3lf seconds\n", getName(),
						   blockCount, getConfiguration()->getMaxBlockingTries(), 
							   (double)getConfiguration()->getBlockingSleepMillis() / 1000);

#endif
                                                
						cancelableSleep(getConfiguration()->getBlockingSleepMillis());
                                                pEvent = PROT_EVENT_SUCCESS;
                                                break;
                                        default:
                                                // Unknown error, assume fatal:
                                                HAGGLE_ERR("%s Protocol error : %s\n", getName(), getProtocolErrorStr());
                                                break;
                                }
			} else if (pEvent == PROT_EVENT_PEER_CLOSED) {
                                HAGGLE_DBG("%s Peer [%s] closed connection\n", getName(), peerDescription().c_str());
                        }
		} 	
	}
	return pEvent;
}

ProtocolEvent Protocol::receiveDataObject()
{
	size_t bytesRead = 0, totBytesRead = 0, totBytesPut = 0, bytesRemaining;
	Timeval t_start, t_end;
	// Metadata *md = NULL;
	bool md = false; // MOS - enough to know if metadata has been received
	ProtocolEvent pEvent;
	DataObjectRef dObj;
        struct ctrlmsg m;
	bool incoming = false; // MOS
	bool curSessionNoDefined = false;
	DataObjectId_t curSessionNo; // MOS
	int curSeqNo = 1; // MOS
	DataObjectId_t newSessionNo; // MOS

	HAGGLE_DBG("%s receiving data object\n", getName());

// MOS: START SETTING PEER NODE
	{
	Mutex::AutoLocker l(mutex); // MOS

	if (!peerIface) {
	  HAGGLE_ERR("TCP peer interface was null\n");          
	  return PROT_EVENT_ERROR;
	}

	// MOS - Protocol.cpp relies on peerNode for debugging
	NodeRef peer = getKernel()->getNodeStore()->retrieve(peerIface);
	if(peer) peerNode = peer;

	if (!peerNode) {
	  peerNode = Node::create(Node::TYPE_UNDEFINED, "Peer node");      
	  if (!peerNode) {      
	    HAGGLE_ERR("Could not create peer node\n");
	    return PROT_EVENT_ERROR;
	  }
	  peerNode->addInterface(peerIface);
	}
	}
// MOS: END SETTING PEER NODE

	dObj = DataObject::create_for_putting(localIface, 
					      peerIface, 
					      getKernel()->getStoragePath());

        if (!dObj) {
		HAGGLE_ERR("%s Could not create pending data object\n", getName());
		return PROT_EVENT_ERROR;
	}

	t_start.setNow();
	bytesRemaining = DATAOBJECT_METADATA_PENDING;
	bufferDataLen = 0; // MOS - this is important
	
	do {
		pEvent = getData(&bytesRead);

		if(pEvent == PROT_EVENT_PEER_CLOSED) {
			HAGGLE_DBG("%s Peer [%s] closed connection\n", getName(), 
				   peerDescription().c_str());
			// return pEvent;
			break; // MOS - allow cleanup code to be executed
		}
	        else if(pEvent == PROT_EVENT_ERROR_FATAL) {
		        // return pEvent;
			break; // MOS - allow cleanup code to be executed
		}
		else if(pEvent == PROT_EVENT_ERROR) {
		        // return pEvent;
			break; // MOS - allow cleanup code to be executed
		}
		else if(pEvent == PROT_EVENT_TIMEOUT) { // MOS - new
		        // return pEvent;
			break; // MOS - allow cleanup code to be executed
		}
		
		if(isSender()) {
		  HAGGLE_ERR("%s Sender received unexpected message - discarding\n", getName());
		  pEvent = PROT_EVENT_ERROR; // MOS
		  break;
		}

		// MOS - Room for optimization below by moving the protocol to the right state
                //       instead of declaring an error and discarding the message.

		// MOS
		if(getSessionNo(newSessionNo)) {
		  if(!curSessionNoDefined) memcpy(curSessionNo,newSessionNo,sizeof(DataObjectId_t));
		  else if(memcmp(curSessionNo,newSessionNo,sizeof(DataObjectId_t)) != 0) {
		    // MOS - there is room for improvement here by not dropping it
		    HAGGLE_ERR("%s Received message with unexpected session number %s != %s - resetting\n", getName(), 
			       DataObject::idString(newSessionNo).c_str(), DataObject::idString(curSessionNo).c_str());
		    pEvent = PROT_EVENT_ERROR;
		    break;
		  }
		}

		// MOS
		if(getSeqNo() != 0) {
		  if(getSeqNo() == curSeqNo) {
		    curSeqNo += 1;
		  }
		  else if (getSeqNo() <= curSeqNo - 1) {
		    HAGGLE_ERR("%s Received message with same or lower sequence number %d as previous one - potential duplicate - ignoring\n", getName(), getSeqNo());
		    continue; // MOS
		  }
		  else {
		    HAGGLE_ERR("%s Received message with higher than expected sequence number %d - potential gap - resetting\n", getName(), getSeqNo());
		    pEvent = PROT_EVENT_ERROR;
		    break;
		  }
		}
		
		if (bufferDataLen == 0) {
			HAGGLE_DBG("%s No data to put into data object!\n", getName());
		} else {
			ssize_t bytesPut = 0;
			
			totBytesRead += bytesRead;

			bytesPut = dObj->putData(buffer, bufferDataLen, &bytesRemaining);

			if (bytesPut < 0) {
				HAGGLE_ERR("%s Error on put data!" 
					   " [BytesPut=%lu totBytesPut=%lu"
					   " bytesRead=%lu totBytesRead=%lu bytesRemaining=%lu]\n", 
					   getName(), bytesPut, 
					   totBytesPut, bytesRead, totBytesRead, 
					   bytesRemaining);
				if(dObj->hasFatalError()) { // MOS
				  HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n");
				  getKernel()->setFatalError();
				  getKernel()->shutdown();
				}
				// return PROT_EVENT_ERROR;
			        pEvent = PROT_EVENT_ERROR;
				break; // MOS - allow cleanup code to be executed
			} else {
			  HAGGLE_DBG2("%s Sucess on put data!" // MOS - temporary debug message
				     " [BytesPut=%lu totBytesPut=%lu"
				     " bytesRead=%lu totBytesRead=%lu bytesRemaining=%lu]\n", 
				     getName(), bytesPut, 
				     totBytesPut, bytesRead, totBytesRead, 
				     bytesRemaining);
			}

			removeData(bytesPut);
			totBytesPut += bytesPut;

			/*
			Did the data object just create it's metadata and still has
			data to receive?
			*/
			if (bytesRemaining != DATAOBJECT_METADATA_PENDING && !md) {
				// Yep.
			        // md = dObj->getMetadata(); // MOS - not needed (potentially costly)
			        md = true; 
				// Send "Incoming data object event."
				if (md) 
                                        {
					
					HAGGLE_DBG("%s Metadata header received"
						   " [BytesPut=%lu totBytesPut=%lu"
						   " totBytesRead=%lu bytesRemaining=%lu]\n", 
						   getName(), bytesPut, totBytesPut, 
						   totBytesRead, bytesRemaining);
					
                                        // Save the data object ID in the control message header.
                                        memcpy(m.dobj_id, dObj->getId(), DATAOBJECT_ID_LEN);

					HAGGLE_DBG("%s Incoming data object [%s] from peer %s\n", 
						getName(), dObj->getIdStr(), 
						   peerDescription().c_str());

					// SW: bloomfilter add optimization: we now know that the neighbor has
					// this particular data object (TODO: consider the security implications
					// of this)
					NodeRef currentPeer;
					{ Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS
					if (currentPeer) {
					  currentPeer->getBloomfilter()->add(dObj);
					}

					dataObjectsIncoming += 1; // MOS
					dataObjectBytesIncoming += totBytesRead; // MOS
					if(!dObj->isControlMessage()) dataObjectsIncomingNonControl += 1; // MOS

					// Check if we already have this data object (FIXME: or are 
					// otherwise not willing to accept it).
					if (getKernel()->getThisNode()->getBloomfilter()->has(dObj)) {
						// Reject the data object:
                                                m.type = CTRLMSG_TYPE_REJECT;
						dataObjectsNotReceived += 1; // MOS
						dataObjectsRejected += 1; // MOS
                                                HAGGLE_DBG("%s Sending REJECT control message to peer %s\n", getName(), 
                                                           peerDescription().c_str());

						setSessionNo(dObj->getId()); // MOS
						setSeqNo(1); // MOS - not checked
						pEvent = sendControlMessage(&m);

                                                HAGGLE_DBG("%s receive DONE after rejecting data object\n", getName());

						if (pEvent == PROT_EVENT_SUCCESS) {
						        Mutex::AutoLocker l(mutex); // MOS
							LOG_ADD("%s: %s\t%s\t%s\n", 
								Timeval::now().getAsString().c_str(), ctrlmsgToStr(&m).c_str(), 
								DataObject::idString(dObj).c_str(), peerNode ? peerNode->getIdStr() : "unknown");
						}
						
                                                return pEvent;
					}
					else if(getKernel()->getThisNode()->getBloomfilter()->hasFragmentationParentDataObject(dObj)) {
						// Reject the data object:
						m.type = CTRLMSG_TYPE_REJECT_PARENTDATAOBJECT;
						dataObjectsNotReceived += 1; // MOS
						dataObjectFragmentsRejected += 1; // MOS
						HAGGLE_DBG("%s Sending REJECT2 control message to peer %s\n", getName(),
							   peerDescription().c_str());

						setSessionNo(dObj->getId()); // MOS
						setSeqNo(1); // MOS - not checked
						pEvent = sendControlMessage(&m);

						HAGGLE_DBG("%s receive DONE after rejecting data object\n", getName());

						if (pEvent == PROT_EVENT_SUCCESS) {
						        Mutex::AutoLocker l(mutex); // MOS
							LOG_ADD("%s: %s\t%s\t%s\n",
								Timeval::now().getAsString().c_str(), ctrlmsgToStr(&m).c_str(),
								DataObject::idString(dObj).c_str(), currentPeer ? currentPeer->getIdStr() : "unknown");
						}

						return pEvent;
					}
					else if(getKernel()->getThisNode()->getBloomfilter()->hasNetworkCodingParentDataObject(dObj)) {
						// Reject the data object:
						m.type = CTRLMSG_TYPE_REJECT_PARENTDATAOBJECT;
						dataObjectsNotReceived += 1; // MOS
						dataObjectBlocksRejected += 1; // MOS
						HAGGLE_DBG("%s Sending REJECT2 control message to peer %s\n", getName(),
							   peerDescription().c_str());

						setSessionNo(dObj->getId()); // MOS
						setSeqNo(1); // MOS - not checked
						pEvent = sendControlMessage(&m);

						HAGGLE_DBG("%s receive DONE after rejecting data object\n", getName());

						if (pEvent == PROT_EVENT_SUCCESS) {
						        Mutex::AutoLocker l(mutex); // MOS
							LOG_ADD("%s: %s\t%s\t%s\n",
								Timeval::now().getAsString().c_str(), ctrlmsgToStr(&m).c_str(),
								DataObject::idString(dObj).c_str(), currentPeer ? currentPeer->getIdStr() : "unknown");
						}

						return pEvent;
					}
					else {
                                                m.type = CTRLMSG_TYPE_ACCEPT;
						dataObjectsAccepted += 1; // MOS
						// Tell the other side to continue sending the data object:
                                                
						/*
						  We add the data object to the bloomfilter of this node here, although it is really
						  the data manager that maintains the bloomfilter of "this node" in the INCOMING event. 
						  However, if many nodes try to send us the same data object at the same time, we
						  cannot afford to wait for the data manager to add the object to the bloomfilter. If we wait,
						  we will ACCEPT many duplicates of the same data object in other protocol threads. 
						  
						  The reason for not adding the data objects to the bloomfilter only at this location is that
						  the data manager maintains a counting version of the bloomfilter for this node, and that
						  version will eventually be updated in the incoming event, and replace "this node"'s bloomfilter.
						*/
						getKernel()->getThisNode()->getBloomfilter()->add(dObj);

						HAGGLE_DBG("%s Sending ACCEPT control message to peer [%s]\n", getName(), peerDescription().c_str());

						setSessionNo(dObj->getId()); // MOS
						setSeqNo(1); // MOS - not checked
                                                pEvent = sendControlMessage(&m);

						if (pEvent == PROT_EVENT_SUCCESS) {
							LOG_ADD("%s: %s\t%s\t%s\n", 
								Timeval::now().getAsString().c_str(), ctrlmsgToStr(&m).c_str(), 
								DataObject::idString(dObj).c_str(), currentPeer ? currentPeer->getIdStr() : "unknown");
						}
					}
					
					getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_INCOMING, dObj, currentPeer));
					incoming = true; // MOS
				}
			}				
		} 
		//HAGGLE_DBG("bytesRead=%lu bytesRemaining=%lu\n", bytesRead, bytesRemaining);
	} while (bytesRemaining && pEvent == PROT_EVENT_SUCCESS);

	// MOS - added cleanup for failures after incoming event was raised
        if (pEvent != PROT_EVENT_SUCCESS) {
	  if(incoming) getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, dObj, false));
	  return pEvent;
	}

	HAGGLE_DBG2("%s totBytesPut=%lu totBytesRead=%lu bytesRemaining=%lu\n", getName(), 
				totBytesPut, totBytesRead, bytesRemaining);
	t_end.setNow();
	t_end -= t_start;

	dObj->setRxTime((long)t_end.getTimeAsMilliSeconds());

	HAGGLE_DBG("%s %ld bytes data received in %.3lf seconds, average speed %.2lf kB/s\n", getName(), 
		   totBytesRead, t_end.getTimeAsSecondsDouble(),
		   ((double) totBytesRead) / dObj->getRxTime());

	dataObjectsReceived += 1; // MOS
	dataObjectBytesReceived += totBytesRead; // MOS
	if (dObj->isNodeDescription()) { 
		nodeDescReceived += 1; 
		nodeDescBytesReceived += totBytesRead; 
		NodeRef newNode = Node::create(dObj);
		if (newNode->getType() == Node::TYPE_APPLICATION) {
			appNodeDescReceived++;
			appNodeDescBytesReceived += totBytesRead;
		}
	}

	// Send ACK message back:	
	HAGGLE_DBG("%s Sending ACK control message to peer %s\n", getName(), peerDescription().c_str());
        m.type = CTRLMSG_TYPE_ACK;
	dataObjectsAcknowledged += 1; // MOS

	setSeqNo(3); // MOS - not checked
	sendControlMessage(&m);

	dObj->setReceiveTime(Timeval::now());

	HAGGLE_DBG("%s Received data object [%s] from node %s\n", getName(), 
		   DataObject::idString(dObj).c_str(), peerDescription().c_str()); 
	// MOS - removed interface due to locking issue

	// SW - adding success hook for protocols to update before event
	// is fired
	receiveDataObjectSuccessHook(dObj);

	Mutex::AutoLocker l(mutex); // MOS
	getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_RECEIVED, dObj, peerNode));
       
	return pEvent;
}

// MOS - probabilistic load reduction 
//       to keep protocol queue size within reasonable bounds

bool Protocol::probabilisiticLoadReduction()
{
  Queue *q = getQueue();
  if(!q) return false;
  int qs = q->size(); 
  int loadReductionMinQueueSize = getConfiguration()->getLoadReductionMinQueueSize();
  if(qs < loadReductionMinQueueSize) return false;
  int loadReductionMaxQueueSize = getConfiguration()->getLoadReductionMaxQueueSize();
  if(qs > loadReductionMaxQueueSize) return true;
  double p = (double)(qs - loadReductionMinQueueSize) / (double)(loadReductionMaxQueueSize - loadReductionMinQueueSize);
  double r = (double)rand()/(double)RAND_MAX;
  return r <= p;
}

ProtocolEvent Protocol::sendDataObjectNow(const DataObjectRef& dObj)
{
	int blockCount = 0;
	unsigned long totBytesSent = 0;
	ProtocolEvent pEvent = PROT_EVENT_SUCCESS;
	Timeval t_start = Timeval::now();
	Timeval waitTimeout;
	bool hasSentHeader = false;
	ssize_t len;
        struct ctrlmsg m;
	DataObjectId_t curSessionNo; // MOS
	DataObjectId_t newSessionNo; // MOS
	memcpy(curSessionNo, dObj->getId(), sizeof(DataObjectId_t)); // MOS
	int curSeqNo = 1; // MOS

	HAGGLE_DBG("%s Sending data object [%s] to peer \'%s\'\n", 
			getName(), dObj->getIdStr(), peerDescription().c_str());

	dataObjectsOutgoing += 1; // MOS
	if(!dObj->isControlMessage()) dataObjectsOutgoingNonControl += 1; // MOS
	
	DataObjectDataRetrieverRef retriever = dObj->getDataObjectDataRetriever();

	if (!retriever || !retriever->isValid()) {
		HAGGLE_ERR("%s unable to start reading data\n", getName());
		if(dObj->hasFatalError()) { // MOS
		  HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n");
		  getKernel()->setFatalError();
		  getKernel()->shutdown();
		}
		return PROT_EVENT_ERROR;
	}
	// Repeat until the data object is completely sent:
	do {
		// Get the data:
		len = retriever->retrieve(buffer, bufferSize, !hasSentHeader);
		
		if (len < 0) {
			HAGGLE_ERR("%s Could not retrieve data from data object\n", getName());
			pEvent = PROT_EVENT_ERROR;
		} else if (len > 0) {
			size_t totBytes = 0;
                        
                        do {
				size_t bytesSent = 0;
				waitTimeout = (getConfiguration()->getConnectionWaitMillis() / 1000);

				pEvent = waitForEvent(&waitTimeout, true);
				
				if (pEvent == PROT_EVENT_TIMEOUT) {
				  HAGGLE_DBG("%s Protocol timed out while waiting to write data\n", getName());
					break;
								  
				} else if (pEvent != PROT_EVENT_WRITEABLE) {
					HAGGLE_ERR("%s Protocol was not writeable, event=%d\n", getName(), pEvent);
                                        break;
				}
				
				setSessionNo(curSessionNo); // MOS
				setSeqNo(curSeqNo); // MOS

				// MOS - adding optional redundancy
				for(int i = 0; i <= getConfiguration()->getRedundancy(); i++) {
				pEvent = sendData(buffer + totBytes, len - totBytes, 0, &bytesSent);
				if (pEvent != PROT_EVENT_SUCCESS) break;
				}

				if (pEvent == PROT_EVENT_ERROR) {
					switch (getProtocolError()) {
					case PROT_ERROR_BAD_HANDLE:
					case PROT_ERROR_NOT_CONNECTED:
					case PROT_ERROR_NOT_A_SOCKET:
					case PROT_ERROR_CONNECTION_RESET:
						pEvent = PROT_EVENT_ERROR_FATAL;
						break;
					case PROT_ERROR_WOULD_BLOCK:
                                                if (blockCount++ > getConfiguration()->getMaxBlockingTries()) 
                                                        break;

						// Set event to success so that the loop does not quit

						HAGGLE_DBG("%s Sending would block, try number %d/%d in %.3lf seconds\n", getName(),
							blockCount, getConfiguration()->getMaxBlockingTries(), 
							(double)getConfiguration()->getBlockingSleepMillis() / 1000);
						cancelableSleep(getConfiguration()->getBlockingSleepMillis());

						pEvent = PROT_EVENT_SUCCESS;
						break;
					default:
						HAGGLE_ERR("%s Protocol error : %s\n", getName(), getProtocolErrorStr());
						break;
					}
				} else {
					// Reset the block count since we successfully sent some data
					blockCount = 0;						
					totBytes += bytesSent;
					dataObjectBytesOutgoing += bytesSent; // MOS
					curSeqNo += 1; // MOS
					//HAGGLE_DBG("%s Sent %lu bytes data on channel\n", getName(), bytesSent);
				}
			} while ((len - totBytes) && pEvent == PROT_EVENT_SUCCESS);

			totBytesSent += totBytes;
		}

		// If we've just finished sending the header:
		if (len == 0 && !hasSentHeader && pEvent == PROT_EVENT_SUCCESS) {
			// We are sending to a local application: done after sending the 
			// header:
		        if (isApplication()) {
			        dataObjectsSent += 1; // MOS
			        dataObjectBytesSent += totBytesSent; // MOS
                                return pEvent;
			}

			hasSentHeader = true;
			
                        HAGGLE_DBG("%s Getting accept/reject control message\n", getName());
			// Get the accept/reject "message":
			pEvent = receiveControlMessage(&m);

			// Did we get it?                        
			if (pEvent == PROT_EVENT_SUCCESS) {
                                HAGGLE_DBG2("%s Received control message '%s'\n", getName(), ctrlmsgToStr(&m).c_str());

				if(getSessionNo(newSessionNo) && memcmp(newSessionNo, dObj->getId(), sizeof(DataObjectId_t)) != 0) { // MOS
				  HAGGLE_ERR("%s Received message with unexpected session number %s != %s - error\n", getName(), 
					     DataObject::idString(newSessionNo).c_str(), DataObject::idString(dObj).c_str());
				  pEvent = PROT_EVENT_ERROR;
				} else if (m.type == CTRLMSG_TYPE_ACCEPT) {
					// ACCEPT message. Keep on going.
                                        // This is to make the while loop start over, in case there was
                                        // more of the data object to send.
                                        len = 1;
                                        HAGGLE_DBG("%s Got ACCEPT control message, continue sending\n", getName());
				} else if (m.type == CTRLMSG_TYPE_REJECT) {
					// Reject message. Stop sending this data object:
                                        HAGGLE_DBG("%s Got REJECT control message, stop sending\n", getName());
                                        return PROT_EVENT_REJECT;
				} else if (m.type == CTRLMSG_TYPE_TERMINATE) {
					// Terminate message. Stop sending this data object, and all queued ones:
                                        HAGGLE_DBG("%s Got TERMINATE control message, purging queue\n", getName());
                                        return PROT_EVENT_TERMINATE;
				} else if(m.type == CTRLMSG_TYPE_REJECT_PARENTDATAOBJECT) {
					// Reject message. Stop sending this data object:
                                        HAGGLE_DBG("%s Got REJECTPARENTDATAOBJECT control message, stop sending\n", getName());
                                        return PROT_EVENT_REJECT_PARENTDATAOBJECT;
				}
			} else {
                                HAGGLE_ERR("%s Did not receive accept/reject control message\n", getName());
                        }
		}
	} while (len > 0 && pEvent == PROT_EVENT_SUCCESS);
	
	if (pEvent != PROT_EVENT_SUCCESS) {
		HAGGLE_ERR("%s Send - %s\n", 
			   getName(), pEvent == PROT_EVENT_PEER_CLOSED ? "Peer closed" : "Error");
                return pEvent;
	}
#ifdef DEBUG
        Timeval tx_time = Timeval::now() - t_start;

        HAGGLE_DBG("%s Sent %lu bytes data in %.3lf seconds, average speed = %.2lf kB/s\n", 
                   getName(), totBytesSent, tx_time.getTimeAsSecondsDouble(), 
                   (double)totBytesSent / (1000*tx_time.getTimeAsSecondsDouble()));
#endif
	dataObjectsSent += 1; // MOS
	dataObjectBytesSent += totBytesSent; // MOS
	if (dObj->isNodeDescription()) { 
		nodeDescSent += 1; 
		nodeDescBytesSent += totBytesSent; 
		NodeRef newNode = Node::create(dObj);
		if (newNode->getType() == Node::TYPE_APPLICATION) {
			appNodeDescSent += 1;
			appNodeDescBytesSent += totBytesSent;
		}
	} // MOS

	HAGGLE_DBG("%s Waiting %f seconds for ACK from peer [%s]\n", getName(), 
                   (getConfiguration()->getConnectionWaitMillis() / (double) 1000), peerDescription().c_str());
        
        pEvent = receiveControlMessage(&m);

        if (pEvent == PROT_EVENT_SUCCESS) {
	        if(getSessionNo(newSessionNo) && memcmp(newSessionNo, dObj->getId(), sizeof(DataObjectId_t)) != 0) { // MOS
		  HAGGLE_ERR("%s Received message with unexpected session number %s != %s - error\n", getName(),
			     DataObject::idString(newSessionNo).c_str(), DataObject::idString(dObj).c_str());
		  pEvent = PROT_EVENT_ERROR;
		} else if (m.type == CTRLMSG_TYPE_ACK) {
                        HAGGLE_DBG("%s Received '%s'\n", getName(), ctrlmsgToStr(&m).c_str());
			dataObjectsSentAndAcked += 1; // MOS
                        sendDataObjectNowSuccessHook(dObj); // SW: added hooks for children classes.
                } else {
                        HAGGLE_ERR("%s Control message malformed: expected 'ACK', got '%s'\n", getName(), ctrlmsgToStr(&m).c_str());
                        pEvent = PROT_EVENT_ERROR;
                }
        } else {
                HAGGLE_ERR("%s Did not receive ACK control message... Assuming data object was not received.\n", getName());
        }
	return pEvent;
}

bool Protocol::run()
{
	ProtocolEvent pEvent;
	int numConnectTry = 0;
	setMode(PROT_MODE_IDLE);
	int numerr = 0;
// SW: START: SEND TIMEOUT RETRY:
    int numSendTimeouts = 0;
// SW: END: SEND TIMEOUT RETRY.
	Queue *q = getQueue();
	Timeval lastSendTime(-1); // MOS

	if (!q) {
		HAGGLE_ERR("%s Could not get a Queue for protocol\n", getName());
		setMode(PROT_MODE_DONE);
		return false;
	}

	HAGGLE_DBG("%s Running protocol\n", getName());

	DataObjectRef dObjToRetransmit; // MOS

	while (!isDone() && !shouldExit()) {
		while (!isConnected() && !shouldExit() && !isDone()) {
                        
			HAGGLE_DBG("%s Protocol connecting to %s\n", 
				   getName(), peerDescription().c_str());

			pEvent = connectToPeer();
			
			if (pEvent == PROT_EVENT_SUCCESS) {
				// The connected flag should probably
				// be set in connectToPeer, but set it
				// here for safety
				HAGGLE_DBG("%s successfully connected to %s\n", 
					   getName(), 
					   peerDescription().c_str());
			} else if (pEvent == PROT_EVENT_ERROR_FATAL) {
				setMode(PROT_MODE_DONE);
				HAGGLE_ERR("%s Fatal error, protocol done!\n", getName());
			} else {
				numConnectTry++;
				HAGGLE_DBG("%s connect failure %d/%d to %s\n", 
					   getName(), numConnectTry, 
					   getConfiguration()->getConnectionAttempts(), 
					   peerDescription().c_str());

				if (numConnectTry == getConfiguration()->getConnectionAttempts()) {
					HAGGLE_DBG("%s connect failed to %s\n", 
						   getName(), 
						   peerDescription().c_str());
					q->close();
					setMode(PROT_MODE_DONE);
				} else {
					unsigned int sleep_ms = 
						RANDOM_INT(getConfiguration()->getConnectionPauseJitterMillis())
                        + getConfiguration()->getConnectionPauseMillis();

					HAGGLE_DBG("%s sleeping %u secs\n", 
						   getName(), sleep_ms / 1000);

					cancelableSleep(sleep_ms);
				}
			}
                        // Check to make sure we were not cancelled
                        // before we start doing work
                        if (isDone() || shouldExit())
                                goto done;
		}

		Timeval t_start = Timeval::now();
		Timeval timeout(getWaitTimeBeforeDoneMillis() / (double) 1000);
		DataObjectRef dObj;
		HAGGLE_DBG("%s Waiting for data object or timeout (%f seconds)...\n", 
			   getName(), (getWaitTimeBeforeDoneMillis() / (double) 1000));

		if(dObjToRetransmit) { // MOS - retransmission
		  dObj = dObjToRetransmit; 
		  dObjToRetransmit = NULL; 
		  pEvent = PROT_EVENT_TXQ_NEW_DATAOBJECT;
		}
		else pEvent = waitForEvent(dObj, &timeout);
		
		timeout = Timeval::now() - t_start;

		HAGGLE_DBG("%s Got event %d, checking what to do...\n", 
			   getName(), pEvent);

		switch (pEvent) {
			case PROT_EVENT_TIMEOUT:
				// Timeout expired:
				setMode(PROT_MODE_DONE);
			break;
			case PROT_EVENT_TXQ_NEW_DATAOBJECT:
            {
				// Data object to send:
				if (!dObj) {
					// Something is wrong here. TODO: better error handling than continue?
					HAGGLE_ERR("%s No data object in queue. ERROR when sending to [%s]!\n", 
						   getName(), peerDescription().c_str());
					break;
				}
				HAGGLE_DBG("%s Data object retrieved from queue, sending to [%s]\n", 
					   getName(), peerDescription().c_str());
				
// SW: START: late bloomfilter check
                NodeRef currentPeer;
                { Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS

                NodeRef actualPeer = getManager()->getKernel()->getNodeStore()->retrieve(currentPeer, true);
                if (!actualPeer) {
                    HAGGLE_ERR("%s Peer not in node store\n", getName());
		    dataObjectsNotSent += 1; // MOS
                    return PROT_EVENT_ERROR;
                }

                // SW: START REPLICATION MANAGER
                long beforeBytesSent = dataObjectBytesSent;
                Timeval beforeSendTime = Timeval::now();
                // SW: END REPLICATION MANAGER
                // check if already in peers bloomfilter
                if (dObj->isObsolete()) {
		    HAGGLE_DBG("%s Data object %s is obsolete - not sending.\n", getName(), DataObject::idString(dObj).c_str());
		    dataObjectsNotSent += 1; // MOS
                    pEvent = PROT_EVENT_SUCCESS;
                } 
                else if (probabilisiticLoadReduction()) {
                    HAGGLE_DBG("Data object discarded to reduce load.\n");
		    dataObjectsNotSent += 1; // MOS
                    pEvent = PROT_EVENT_ERROR;
                } 
                else if (actualPeer->hasThisOrParentDataObject(dObj)) { // MOS - added check for parent
                    HAGGLE_DBG("%s Peer already had data object.\n", getName());
		    dataObjectsNotSent += 1; // MOS
                    pEvent = PROT_EVENT_SUCCESS;
                } 
                else {
		    // MOS - some basic rate limiting (especially important for node descriptions)
		    int minDelay = getConfiguration()->getMinSendDelayBaseMillis();
		    int minDelayPerNeighbor = getConfiguration()->getMinSendDelayLinearMillis();
		    int minDelayPerNeighborSq = getConfiguration()->getMinSendDelaySquareMillis();
		    int numNeighbors = getKernel()->getNodeStore()->numNeighbors();
		    minDelay += minDelayPerNeighbor * numNeighbors;
		    minDelay += minDelayPerNeighborSq * numNeighbors * numNeighbors;

		    Timeval now = Timeval::now();
		    if(lastSendTime.isValid()) {
		      int actualDelay = (now - lastSendTime).getTimeAsMilliSeconds();
		      if(actualDelay < minDelay) {
			HAGGLE_DBG2("%s Sleeping for %d ms to limit sending rate\n", getName(), minDelay - actualDelay);
			cancelableSleep(minDelay - actualDelay);
		      }
		    }

		    // MOS - this is important to randomize broadcast
		    int maxRandomDelay = getConfiguration()->getMaxRandomSendDelayMillis();
		    if(maxRandomDelay) cancelableSleep(rand() % maxRandomDelay);

		    // MOS
		    Timeval startTimeSend = Timeval::now();
		    lastSendTime = startTimeSend;

                    pEvent = sendDataObjectNow(dObj);
		    // MOS
		    if(pEvent == PROT_EVENT_SUCCESS) {
		      Timeval endTimeSend = Timeval::now();
		      dataObjectSendExecutionTime += (endTimeSend - startTimeSend).getTimeAsSecondsDouble();
		    }
                }
// SW: END: late bloomfilter add
                //pEvent = sendDataObjectNow(dObj);
				
				if (pEvent == PROT_EVENT_SUCCESS || pEvent == PROT_EVENT_REJECT || pEvent == PROT_EVENT_REJECT_PARENTDATAOBJECT) {
					// Treat reject as SUCCESS, since it probably means the peer already has the
					// data object and we should therefore not try to send it again.

					int flag = 0;
					switch(pEvent) {
					case PROT_EVENT_REJECT:
						flag = 1;
						break;
					case PROT_EVENT_REJECT_PARENTDATAOBJECT:
						flag = 2;
						break;
					}
					HAGGLE_DBG2("creating event EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL with flag=%d\n",flag);
					NodeRef currentPeer;
					{ Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS
					getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dObj, currentPeer, flag));
// SW: START SEND PRIORITY
                    getKernel()->addEvent(SendPriorityManager::getSendPrioritySuccessEvent(dObj, currentPeer));
// SW: END SEND PRIORITY
					// SW: START REPLICATION MANAGER
					getKernel()->addEvent(ReplicationManagerFactory::getSendStatsEvent(currentPeer, dObj, (long)(Timeval::now() - beforeSendTime).getTimeAsMilliSeconds(), dataObjectBytesSent - beforeBytesSent));
					// SW: END REPLICATION MANAGER
				} else {
					// Send success/fail event with this data object
					switch (pEvent) {
						case PROT_EVENT_TERMINATE:
							// TODO: What to do here?
							// We should stop sending completely, but if we just
							// close the connection we might just connect and start
							// sending again. We need a way to signal that we should 
							// not try to send to this peer again -- at least not
							// until next time he is our neighbor. For now, treat
							// the same way as if the peer closed the connection.
						case PROT_EVENT_PEER_CLOSED:
							HAGGLE_DBG("%s Peer [%s] closed connection.\n", 
								getName(), peerDescription().c_str());
							q->close();
							setMode(PROT_MODE_DONE);
							closeConnection();
							break;
						case PROT_EVENT_ERROR:
							HAGGLE_ERR("%s Data object send to [%s] failed...\n", 
								getName(), peerDescription().c_str());
							if (numerr++ >= getConfiguration()->getMaxProtocolErrors() || shouldExit()) {
							  q->close();
							  closeConnection();
							  setMode(PROT_MODE_DONE);
							  HAGGLE_DBG("%s Reached max errors=%d. Cancelling.\n", 
								     getName(), numerr);
							}
							else {
							  HAGGLE_DBG("%s Send error, retrying (try: %d)\n", getName(), numerr);
							  dObjToRetransmit = dObj; // MOS - make sure to retransmit
							  continue;
							}
							break;
						case PROT_EVENT_ERROR_FATAL:
							HAGGLE_ERR("%s Fatal error when sending to %s!\n", 
								getName(), peerDescription().c_str());
							q->close();
							setMode(PROT_MODE_DONE);
							break;
// SW: START: SEND TIMEOUT RETRY:
                        case PROT_EVENT_TIMEOUT:
			    HAGGLE_ERR("%s Data object send to [%s] failed...\n", 
				     getName(), peerDescription().c_str());
                            if (numSendTimeouts++ >= getConfiguration()->getMaxSendTimeouts() || shouldExit()) {
                                q->close();
				closeConnection();
                                setMode(PROT_MODE_DONE);   
                                HAGGLE_DBG("%s Reached max timeouts=%d. Cancelling.\n", getName(), numSendTimeouts);
				break;
                            }
                            else {
			      HAGGLE_DBG("%s Send timeout, retrying (try: %d)\n", getName(), numSendTimeouts);
				dObjToRetransmit = dObj; // MOS - make sure to retransmit
                                continue;
                            }
// SW: END: SEND TIMEOUT RETRY.
						default:
							q->close();
							setMode(PROT_MODE_DONE);
					}
					NodeRef currentPeer;
					{ Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS
					getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, currentPeer));
// SW: START SEND PRIORITY
                    getKernel()->addEvent(SendPriorityManager::getSendPriorityFailureEvent(dObj, currentPeer));
// SW: END SEND PRIORITY
				}
				break;
            }
			case PROT_EVENT_INCOMING_DATA:
				// Data object to receive:
				HAGGLE_DBG("%s Incoming data object from [%s]\n", 
					   getName(), peerDescription().c_str());

				{
				  // MOS
				  Timeval startTimeRecv = Timeval::now();
				  
				  pEvent = receiveDataObject();	
				  
				  // MOS
				  if(pEvent == PROT_EVENT_SUCCESS) {
				    Timeval endTimeRecv = Timeval::now();
				    dataObjectReceiveExecutionTime += (endTimeRecv - startTimeRecv).getTimeAsSecondsDouble();
				  }
				}

				switch (pEvent) {
					case PROT_EVENT_SUCCESS:
						HAGGLE_DBG2("%s Data object successfully received from [%s]\n", 
							getName(), peerDescription().c_str());
						break;
					case PROT_EVENT_PEER_CLOSED:
						q->close();
						setMode(PROT_MODE_DONE);
						closeConnection();
						break;
					case PROT_EVENT_ERROR:
				        case PROT_EVENT_TIMEOUT: // MOS - treat recv timeout as error (better define separate category in the future)
						HAGGLE_ERR("%s Data object receive failed... error num %d\n", 
							   getName(), numerr);
						if (numerr++ >= getConfiguration()->getMaxProtocolErrors() || shouldExit()) {
							q->close();
							closeConnection();
							setMode(PROT_MODE_DONE);
							HAGGLE_DBG("%s Reached max errors=%d. Cancelling.\n", 
								getName(), numerr);
						}
						else {
						  HAGGLE_DBG("%s Receive error, retrying (try: %d)\n", getName(), numerr);
						}
						continue;
					case PROT_EVENT_ERROR_FATAL:
						HAGGLE_ERR("%s Data object receive fatal error!\n",
							   getName());
						q->close();
						setMode(PROT_MODE_DONE);
						break;
					default:
						q->close();
						setMode(PROT_MODE_DONE);
				}
				break;
			case PROT_EVENT_TXQ_EMPTY:
				HAGGLE_ERR("%s - Queue was empty\n", 
					   getName());
				break;
			case PROT_EVENT_ERROR:
				HAGGLE_ERR("%s Error num %d in protocol\n", getName(), 
					   numerr);

				if (numerr++ >= getConfiguration()->getMaxProtocolErrors() || shouldExit()) {
					q->close();
					closeConnection();
					setMode(PROT_MODE_DONE);
					HAGGLE_DBG("%s Reached max errors=%d - Cancelling protocol!\n", 
						getName(), numerr);
				}
				continue;
                        case PROT_EVENT_SHOULD_EXIT:
                                setMode(PROT_MODE_DONE);
                                break;
			default:
				HAGGLE_ERR("%s Unknown protocol event!\n", getName());
				break;
		}
                // Reset error
		numerr = 0;
// SW: START: SEND TIMEOUT RETRY:
        numSendTimeouts = 0;
// SW: START: SEND TIMEOUT RETRY.
	}
      done:

	HAGGLE_DBG("%s DONE!\n", getName());

	if (isConnected())
		closeConnection();

        setMode(PROT_MODE_DONE);

	HAGGLE_DBG2("%s exits runloop!\n", getName());

	return false;
}

void Protocol::registerWithManager()
{
	if (isRegistered) {
		HAGGLE_ERR("%s Trying to register protocol more than once\n", getName());
		return;
	}
	
	isRegistered = true;
	getKernel()->addEvent(new Event(getManager()->add_protocol_event, this));
}


void Protocol::unregisterWithManager()
{
  if(!isDetached()) { // MOS - detached protocols sadly don't have a manager

	if (!isRegistered) {
		HAGGLE_ERR("%s Trying to unregister protocol twice\n", getName());
		return;
	}
	isRegistered = false;

	HAGGLE_DBG("%s Protocol sends unregister event\n", getName()); 

	getKernel()->addEvent(new Event(getManager()->delete_protocol_event, this));
  }
}

// MOS
void Protocol::logStatistics() 
{

	if(dataObjectsOutgoingNonControl) {
	  HAGGLE_STAT("%s Summary Statistics - Non-Control Data Objects Outgoing: %d\n", getName(), dataObjectsOutgoingNonControl); // MOS
	}

	if(dataObjectsIncomingNonControl) {
	  HAGGLE_STAT("%s Summary Statistics - Non-Control Data Objects Incoming: %d\n", getName(), dataObjectsIncomingNonControl); // MOS
	}

	if(dataObjectsOutgoing) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Outgoing: %d\n", getName(), dataObjectsOutgoing); // MOS
	  HAGGLE_STAT("%s Summary Statistics - Data Object Bytes Outgoing: %u - Avg. Bytes Outgoing: %.0f\n", getName(), dataObjectBytesOutgoing, 
		      (double)dataObjectBytesOutgoing/(double)dataObjectsOutgoing); // MOS
	}

	if(dataObjectsIncoming) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Incoming: %d\n", getName(), dataObjectsIncoming); // MOS
	  HAGGLE_STAT("%s Summary Statistics - Data Object Bytes Incoming: %u - Avg. Bytes Incoming: %.0f\n", getName(), dataObjectBytesIncoming, 
		      (double)dataObjectBytesIncoming/(double)dataObjectsIncoming); // MOS
	}

	if(dataObjectsSent) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Fully Sent: %d - Avg. Execution Time: %.6f\n", getName(), dataObjectsSent, 
		      dataObjectSendExecutionTime/(double)dataObjectsSent); // MOS
	  HAGGLE_STAT("%s Summary Statistics - Data Object Bytes Fully Sent: %u - Avg. Bytes Sent: %.0f - Avg. Speed: %.0f B/s\n", getName(), dataObjectBytesSent, 
		      (double)dataObjectBytesSent/(double)dataObjectsSent, 
		      (double)dataObjectBytesSent/(double)dataObjectSendExecutionTime); // MOS
	}

	if(dataObjectsReceived) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Fully Received: %d - Avg. Execution Time: %.6f\n", getName(), dataObjectsReceived, 
		      dataObjectReceiveExecutionTime/(double)dataObjectsReceived); // MOS
	  HAGGLE_STAT("%s Summary Statistics - Data Object Bytes Fully Received: %u - Avg. Bytes Received: %.0f - Avg. Speed: %.0f B/s\n", getName(), dataObjectBytesReceived, 
		      (double)dataObjectBytesReceived/(double)dataObjectsReceived, 
		      (double)dataObjectBytesReceived/(double)dataObjectReceiveExecutionTime); // MOS
	}

	if(nodeDescSent) {
	  HAGGLE_STAT("%s Summary Statistics - Node Descriptions Sent: %d\n", getName(), nodeDescSent); // MOS
	  HAGGLE_STAT("%s Summary Statistics - Node Description Bytes Sent: %u - Avg. Bytes Sent: %.0f\n", getName(), nodeDescBytesSent, 
		      (double)nodeDescBytesSent/(double)nodeDescSent); // MOS
	}

	if(nodeDescReceived) {
	  HAGGLE_STAT("%s Summary Statistics - Node Descriptions Received: %d\n", getName(), nodeDescReceived); // MOS
	  HAGGLE_STAT("%s Summary Statistics - Node Description Bytes Received: %u - Avg. Bytes Received: %.0f\n", getName(), nodeDescBytesReceived, 
		      (double)nodeDescBytesReceived/(double)nodeDescReceived); // MOS
	}

	if(appNodeDescSent) {
	  HAGGLE_STAT("%s Summary Statistics - Application Node Descriptions Sent: %d\n", getName(), appNodeDescSent); 
	  HAGGLE_STAT("%s Summary Statistics - Application Node Description Bytes Sent: %u - Avg. Bytes Sent: %.0f\n", getName(), appNodeDescBytesSent, 
		      (double)appNodeDescBytesSent/(double)appNodeDescSent); // MOS
	}

	if(appNodeDescReceived) {
	  HAGGLE_STAT("%s Summary Statistics - Application Node Descriptions Received: %d\n", getName(), appNodeDescReceived); 
	  HAGGLE_STAT("%s Summary Statistics - Application Node Description Bytes Received: %u - Avg. Bytes Received: %.0f\n", getName(), appNodeDescBytesReceived, 
		      (double)appNodeDescBytesReceived/(double)appNodeDescReceived); 
	}

	if(dataObjectsSentAndAcked) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Fully Sent and Acked: %d\n", getName(), dataObjectsSentAndAcked); // MOS
	}

	if(dataObjectsNotSent) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Not Sent: %d\n", getName(), dataObjectsNotSent); // MOS
	}

	if(dataObjectsNotReceived) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Not Received: %d\n", getName(), dataObjectsNotReceived); // MOS
	}

	if(dataObjectsAccepted) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Accepted: %d\n", getName(), dataObjectsAccepted); // MOS
	}

	if(dataObjectsRejected) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Rejected: %d\n", getName(), dataObjectsRejected); // MOS
	}

	if(dataObjectFragmentsRejected) {
	  HAGGLE_STAT("%s Summary Statistics - Data Object Fragments Rejected: %d\n", getName(), dataObjectFragmentsRejected); // MOS
	}

	if(dataObjectBlocksRejected) {
	  HAGGLE_STAT("%s Summary Statistics - Data Object Blocks Rejected: %d\n", getName(), dataObjectBlocksRejected); // MOS
	}

	if(dataObjectsAcknowledged) {
	  HAGGLE_STAT("%s Summary Statistics - Data Objects Acknowledged: %d\n", getName(), dataObjectsAcknowledged); // MOS
	}
}

void Protocol::shutdown()
{
    if(!isDone()) {
        setMode(PROT_MODE_DONE);

	HAGGLE_DBG("%s Shutting down protocol\n", getName());

	hookShutdown();
	
	if (isRunning())
		cancel();
	else if (isRegistered)
		unregisterWithManager();
    } else {
	HAGGLE_DBG("%s Protocol already shutting down\n", getName());
    }
}

void Protocol::cleanup()
{
	HAGGLE_DBG("%s Cleaning up protocol\n", getName());
	
	hookCleanup();
	
	if (isConnected())
		closeConnection();
	HAGGLE_DBG("%s protocol is now garbage!\n", getName());
	setMode(PROT_MODE_GARBAGE);

	logStatistics(); // MOS
	
	if (isDetached()) {
		delete this;
	} else if (isRegistered) { // MOS
		unregisterWithManager();
	}
}


// SW: START CONFIG:
void Protocol::setConfiguration(ProtocolConfiguration *_cfg)
{
    if (NULL == _cfg) {
        HAGGLE_ERR("Trying to set NULL config\n");
        return;
    }

    if (type != _cfg->getType()) {
        HAGGLE_ERR("Setting wrong configuration type\n");
        return;
    }

    ProtocolConfiguration *oldCfg = cfg;
    cfg = _cfg->clone();

    if (NULL != oldCfg) {
        delete oldCfg;
    }
}

ProtocolConfiguration *Protocol::getConfiguration()
{
    if (NULL == cfg) {
        cfg = new ProtocolConfiguration(type);
    }
    return cfg;
}
// SW: END CONFIG.

