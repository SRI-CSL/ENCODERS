/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
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
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

/*
  Forward declarations of all data types declared in this file. This is to
  avoid circular dependencies. If/when a data type is added to this file,
  remember to add it here.
*/
class Protocol;
typedef unsigned short ProtType_t;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/Watch.h>

#include "ProtocolManager.h"
#include "ManagerModule.h"
#include "Interface.h"
#include "DataObject.h"
#include "Metadata.h"

// SW: START CONFIG:
#include "ProtocolConfiguration.h"
// SW: END CONFIG.

#include "networkcoding/protocol/NetworkCodingProtocolHelper.h"

using namespace haggle;

/* Return errors */
typedef enum {
        _PROT_ERROR_MIN = -1,
        PROT_ERROR_WOULD_BLOCK = 0,
        PROT_ERROR_BAD_HANDLE,
        PROT_ERROR_CONNECTION_REFUSED,
        PROT_ERROR_INTERRUPTED,
        PROT_ERROR_INVALID_ARGUMENT,
        PROT_ERROR_NO_MEMORY,
        PROT_ERROR_NOT_CONNECTED,
        PROT_ERROR_CONNECTION_RESET,
        PROT_ERROR_NOT_A_SOCKET,
        PROT_ERROR_NO_STORAGE_SPACE,
        PROT_ERROR_SEQ_NUM, // MOS
        PROT_ERROR_UNKNOWN,
        _PROT_ERROR_MAX,
} ProtocolError;

#define NUM_PROT_ERRORS (_PROT_ERROR_MAX)

/* Return values/events */
typedef enum {
	PROT_EVENT_ERROR_FATAL = -2,
        PROT_EVENT_ERROR = -1,
        PROT_EVENT_SUCCESS = 0,
	PROT_EVENT_REJECT, // A data object was rejected
	PROT_EVENT_REJECT_PARENTDATAOBJECT,
	PROT_EVENT_TERMINATE, // The peer does not want any more data objects in queue
	PROT_EVENT_SEND_FAILED, // A non fatal send error.
	PROT_EVENT_RECV_FAILED, // A non fatal recv error.
        PROT_EVENT_TIMEOUT,
        PROT_EVENT_PEER_CLOSED,
        PROT_EVENT_SHOULD_EXIT,
        PROT_EVENT_NOT_CONNECTED,
        PROT_EVENT_TXQ_EMPTY,
        PROT_EVENT_TXQ_NEW_DATAOBJECT,
        PROT_EVENT_INCOMING_DATA,
        PROT_EVENT_WRITEABLE,
} ProtocolEvent;

typedef enum {
        PROT_MODE_LISTENING, // The protocol is listening for incoming connections (only servers)
        PROT_MODE_SENDING, // The protocol is sending data (only clients)
        PROT_MODE_RECEIVING, // The protocol is receiving data (only clients)
        PROT_MODE_IDLE, // The protocol is in its runloop but is neither receiving nor sending at the moment (clients), 
			// or listening for connections (servers)
        PROT_MODE_DONE, // The protocol has finished and will exit its runloop
        PROT_MODE_GARBAGE, // The protocol's thread has quit and is not running anymore. The protocol can be
			   // safely deleted, or it can be started again.
} ProtocolMode;


// Protocol flags
#define PROT_FLAG_CLIENT      0x1 // We can send stuff with this protocol
#define PROT_FLAG_SERVER      0x2 // We can only receive incoming connection requests
#define PROT_FLAG_CONNECTED   0x4 // Protocol has establised a connection with another end-point
#define PROT_FLAG_APPLICATION 0x8 // Protocol has establised a connection with another end-point

// TODO: What is a good buffer size here?
// If we have a large buffer, we will waste a lot of memory when there are
// many protocols running. A small buffer may be inefficient.
#define PROTOCOL_DEFAULT_BUFSIZE (4096) 

/**
	Protocol class

	A protocol can be/do one or more of the following:

	Sending protocol: Can send data objects to one or more other nodes.
	Receiving protocol: Can receive data objects from one or more other nodes.
	Server protocol: Can receive connections from other nodes, and will create
		a receiving protocol instance when it does.
*/
class Protocol : public ManagerModule<ProtocolManager>
{
	friend class ProtocolManager;
public:
	/*
	 When adding more protocol types,
	 make sure the list is synchronized
	 with the typestr[] array.
	 */
	enum Type {
		TYPE_LOCAL = 0,
		TYPE_UDP,
		TYPE_TCP,
		TYPE_RAW,
// SW: START: UDP UNICAST PROTOCOL
        TYPE_UDP_UNICAST,
// SW: END: UDP UNICAST PROTOCOL
// SW: START: UDP BROADCAST PROTOCOL
        TYPE_UDP_BROADCAST,
// SW: END: UDP BROADCAST PROTOCOL
#if OMNETPP
		TYPE_OMNETPP,
#endif
#if defined(ENABLE_BLUETOOTH)
		TYPE_RFCOMM,
#endif
#if defined(ENABLE_MEDIA)
		TYPE_MEDIA,
#endif
		NUM_PROTOCOL_TYPES,
	};
#define PROT_TYPE_MAX (NUM_PROTOCOL_TYPES-1)
	
private:
	static const char *protocol_error_str[];	
        /**
           This is used to track IDs.
        */
        static unsigned int num;
	static const char *typestr[];

        /*
          Control messages are used to implement a transaction
          protocol for data objects. This protocol is needed since it
          is otherwise (in case of sudden connection failure)
          impossible to know which data objects have been successfully
          received at the other end.  

          Therefore, an acknowledgement (ACK) is sent whenever a data
          object is received so that the sender can track which data
          objects have successfully reached the other node.

          Sometimes a receiver might not want a data object, but it
          cannot know until the metadata header has been received. In
          order to reject or accept a data object, a sender will wait
          for an ACCEPT or REJECT control message after the header has
          been sent. Hence, the sender will not send the potentially
          large payload unless the receiver accepts the data object.

         */
        typedef enum crtlmsg_type {
                CTRLMSG_TYPE_ACK = 5, // use something which is not zero
                CTRLMSG_TYPE_ACCEPT,
                CTRLMSG_TYPE_REJECT,
		CTRLMSG_TYPE_TERMINATE, /* Terminate the transmission of data objects.
					Currently not implemented. */
		CTRLMSG_TYPE_REJECT_PARENTDATAOBJECT
        } ctrlmsg_type_t;

        typedef struct ctrlmsg {
                u_int32_t type;
                DataObjectId_t dobj_id;
        } ctrlmsg_t;

        NetworkCodingProtocolHelper* networkCodingProtocolHelper;
protected:
	/**
	   True if the Protocol is registered with the Protocol manager.
	*/
	bool isRegistered;
        /**
           The type of protocol. See ProtType_t.
        */
        const ProtType_t type;
        /**
           ID. Unique to an instance of a protocol object. Used to identify
           protocols.
        */
        const unsigned long id;
        /**
           The interface this protocol is connected to. For a server, this means
           the local interface that it is "listening" on, for a sender or receiver
           this means the remote interface it is "sending" or "receiving" to.
        */
        /**
           The current error in case a function returns a PROT_EVENT_ERROR.
        */
        ProtocolError error;


        /**
           The type of the protocol (client, server).
        */
        int flags;


        /**
           The mode of the protocol, as indicated by the ProtocolMode type.
         */
        ProtocolMode mode;
        InterfaceRef localIface;  // Our local interface this protocol uses to communicate

	Mutex mutex; // MOS - mutex for the following two variables
        InterfaceRef peerIface; // Interface of the remote peer that we communicate with
	NodeRef peerNode; // The node matching the peer interface

        // Buffer for reading incoming data
        unsigned char *buffer;

	// The buffer size
	size_t bufferSize;
        // The amount of data read into the buffer
        size_t bufferDataLen;

	// MOS
	int dataObjectsOutgoing, dataObjectsIncoming;
	int dataObjectsOutgoingNonControl, dataObjectsIncomingNonControl;
	long dataObjectBytesOutgoing, dataObjectBytesIncoming;
	int dataObjectsNotReceived, dataObjectsRejected;
	int dataObjectFragmentsRejected, dataObjectBlocksRejected;
	int dataObjectsAccepted, dataObjectsAcknowledged;
	int dataObjectsNotSent, dataObjectsSent, dataObjectsSentAndAcked, dataObjectsReceived;
	double dataObjectSendExecutionTime, dataObjectReceiveExecutionTime;
	long dataObjectBytesSent, dataObjectBytesReceived;
	int nodeDescSent, nodeDescReceived;
	long nodeDescBytesSent, nodeDescBytesReceived;
	long appNodeDescReceived, appNodeDescBytesReceived, appNodeDescSent, appNodeDescBytesSent;

        /**
           Receive a data object from the connected peer.
         */
        virtual ProtocolEvent receiveDataObject();

	// SW: START: added hooks for children classes:
	virtual void receiveDataObjectSuccessHook(const DataObjectRef& dObj) {};

	virtual ProtocolEvent sendDataObjectNowSuccessHook(const DataObjectRef& dObj) {return PROT_EVENT_SUCCESS;};
	// SW: END: added hooks for children classes.

	// Generate a description of the peer node for this protocol.
	// This function handles the fact that a node may be undefined
	// before the node description is received. In that case, the
	// function tries to generate a description using the peer interface 
	// instead.
	string peerDescription();

        /**
           Return an platform independent error number whenever a file
           operation fails.
         */
        int getFileError();
        /**
           Get human readable error whenever a file operation fails.
         */
        const char *getFileErrorStr();

	/**
		Block and wait for either incoming data or a that a data object
		is available in the protocol's transmit queue. If the timeout is not given or set to NULL, the 
		function will block 

		The caller should pass a Data object reference which will reference a new data objects
		if it was available in the queue.
		Returns: PROT_EVENT_TIMEOUT when a timeout occurred.
		PROT_EVENT_ERROR on error, or
		PROT_EVENT_INCOMING_DATA if there is incoming data (the dObj will be non NULL), 
		PROT_EVENT_WRITEABLE if the writeevent argument was true and it is possible to write data, or
		PROT_EVENT_TXQ_NEW_DATAOBJECT if a data object is in the queue.
	*/
	virtual ProtocolEvent waitForEvent(DataObjectRef &dObjRef, Timeval *timeout = NULL, bool writeevent = false);

        /**
           Block and wait for incoming data given a timeout. If the
           timeout is not given or set to NULL, the function will block until there is
           incoming data. 
           
           Returns PROT_EVENT_ERROR, PROT_EVENT_TIMEOUT, PROT_EVENT_WRITEABLE or
           PROT_EVENT_INCOMING_DATA.
         */
        virtual ProtocolEvent waitForEvent(Timeval *timeout = NULL, bool writeevent = false);

        /**
        	Wrapper to send bytes. Should be implemented by derived class.
        	
        	Returns: Protocol event indicating success, or error.
        */
        virtual ProtocolEvent sendData(const void *buf, size_t len, const int flags, size_t *bytes) 
	{ 
                return PROT_EVENT_ERROR; 
        }
        /**
        	Wrapper to receive bytes. Should be implemented by derived class.
        	
        	Returns: Protocol event indicating success, or error.
        */
        virtual ProtocolEvent receiveData(void *buf, size_t len, const int flags, size_t *bytes) 
        {
                return PROT_EVENT_ERROR;
        }

	// MOS - Currently we use 0 to indicate these number are not used, as e.g. in TCP.
        //       Better replace the corresponding checks by clean inheritance in the future.

	virtual void setSessionNo(const DataObjectId_t session) { } // MOS
	virtual bool getSessionNo(DataObjectId_t session) { return false; } // MOS
	virtual void setSeqNo(int seq) { } // MOS
	virtual int getSeqNo() { return 0; } // MOS

        virtual ProtocolError getProtocolError();
        virtual const char *getProtocolErrorStr();
	const char *protocolErrorToStr(const ProtocolError e) const;
        virtual void closeConnection();
	
        // Thread entry and exit
        // SW: allow derived classes to overide run()
        virtual bool run();
        void cleanup();
		
	/** 
		A derived class can override this hook in order to
		implement protocol specific shutdown code. The hook is
		called automatically from shutdown(). 
	 */
	virtual void hookShutdown() {}
	
	/**
		A hook that is called at cleanup, unless the derived class itself
		implements cleanup().
	*/
	virtual void hookCleanup() {}

	// MOS - timeout can be subclass specific
	virtual int getWaitTimeBeforeDoneMillis();
	
	/**
		Calls receiveData to fill the empty parts of the buffer with data.
		
		Returns: 
			positive value: the number of bytes read.
			zero: no bytes were available.
			negative value: error.
		
		FIXME: better name, perhaps?
	*/
	ProtocolEvent getData(size_t *bytesRead);
	
	/**
		Removes the first n bytes of the buffer, making room for getData() to
		fill more data in.
	*/
	void removeData(size_t len);
	
	/**
		Simple shorthand for sending ack/continue/reject messages from a receiver
		to a sender.
	*/
	ProtocolEvent sendControlMessage(struct ctrlmsg *m);
	
	/**
		Simple shorthand for receiving ack/continue/reject messages from a 
		receiver to a sender.
	*/
	ProtocolEvent receiveControlMessage(struct ctrlmsg *m);

        /**
           Convert control message to human readable format.
         */
        const string ctrlmsgToStr(struct ctrlmsg *m) const;
	
	/**
	 Initialization function that may be overridden by derived class. It is 
	 automatically called by init().
	 */
	virtual bool init_derived() { return true; }
	virtual void setPeerNode(const NodeRef& node) { peerNode = node; }

	bool probabilisiticLoadReduction(); // MOS

// SW: START CONFIG:
    ProtocolConfiguration *cfg;
    virtual ProtocolConfiguration *getConfiguration();
// SW: END CONFIG.

public:	
	
        /**
           Constructor.
        */
        Protocol(const ProtType_t _type,
                 const string _name,
                 const InterfaceRef& _localIface,
                 const InterfaceRef& _peerIface,
                 const int _flags,
                 ProtocolManager *_m,
		 size_t _bufferSize = PROTOCOL_DEFAULT_BUFSIZE);
        /**
           Constructor.
        */
        Protocol(const Protocol &);
       
        const Protocol& operator=(const Protocol &); // Not defined
        /**
           Destructor.
        */
        virtual ~Protocol(); // MOS - destructor virtual now
	
	void closeQueue(); // MOS - close quickly
	unsigned long closeAndClearQueue();
	
	/**
	   Initialization
	 */
	bool init();
		
	/**
	Overridden from class Manager. Returns the Protocol name with its id appended.
	*/
	const char *getName() const { return name.c_str(); }
	const char *getTypeStr() const { return typestr[type]; }
	void setRegistered() { isRegistered = true; }

        /**
           This function is called by the protocol manager to notify a protocol
           that it should stop. This may be because it's interface is down or
           because haggle is shutting down.
        	
           If not overridden, this function calls m->deleteProtocol(this). Any
           subclass that wants to do some processing/shutdown before that will
           have to override this function and make sure to call
           m->deleteProtocol(this) at a later time.
        */
        void shutdown(void);

        /**
           This function answers the question "have you registered this socket"
           from the protocol manager. It should not do any other processing for
           the socket than what is needed to tell if it is. If this function
           returns true, the function handleEventSocket() will be called to deal
           with the socket.
        	
           This function should never return true for a socket not created by the
           protocol. If it does, the actual owner of the socket will not be
           notified that something happened to its socket. That might be bad.
		*/
        virtual bool hasWatchable(const Watchable &wbl);

        /**
           Called when the protocol manager gets a socket event. Only called for
           the protocol that returns true for hasSocket().
        */
        virtual void handleWatchableEvent(const Watchable &wbl);

        /**
           Returns the unique ID of this protocol.
        */
        unsigned long getId(void) const;

        /**
           Returns the type of protocol.
        */
        ProtType_t getType(void) const;

        /**
           Returns true iff this interface is associated with this protocol.
        */
        virtual bool isForInterface(const InterfaceRef &iface);

        /**
           Returns the associated local interface.
         */
        InterfaceRef getLocalInterface() const 
	{
                return localIface;
        }

        /**
           Returns the associated remote interface.
         */
        InterfaceRef getPeerInterface() const 
	{
                return peerIface;
        }
		
        /**
           Tries to figure out (and set) the peer interface.
         */
        virtual void setPeerInterface(const Address *addr = NULL) {}

        /**
           Tells the protocol that a "local interface down" event has occured
           regarding the interface associated with it.
        */
        virtual void handleInterfaceDown(const InterfaceRef &iface);

        /**
           Returns true iff this protocol is a sending protocol. This function
           has to be overridden by a child class to return true iff that child
           class is to be treated as a sender class.
        */
        virtual bool isSender();

	/**
	   Makes sure that a data object is (eventually) sent to the peer 
	   interface.
	   
	   This function is virtual, in case a subclass would want to override it.
	   
	   Returns: true if the protocol has assumed responsibility for sending 
	   the data object, false if it failed to do so.
	*/
	virtual bool sendDataObject(const DataObjectRef& dObj, const NodeRef& peer, const InterfaceRef& iface);
	
        virtual ProtocolEvent startTxRx();
        /*
          The following functions are to be overridden by receiver child classes.
          
          Receiver child classes are those that handle a single other node's
          incoming data objects.
          
          Receiver child classes tend to their deletion by using the is_garbage
          flag to signal that it is ready to be deleted.
        */

        /**
           Returns true iff this protocol is a receiving protocol. This function
           has to be overridden by a child class to return true iff that child
           class is to be treated as a receiver class.
        */
        virtual bool isReceiver();

        /*
          The following functions are to be overridden by server subclasses.
          
          Server child classes are those that wait for other nodes to contact it
          and spawns a receiver class to handle the
          
          Server child classes don't tend to their own deletion. They are deleted
          when the protocol manager gets a "local interface down" event regarding
          the interface associated with the protocol.
        */

        /**
           Returns true iff this protocol is a server protocol. This function
           has to be overridden by a child class to return true iff that child
           class is to be treated as a server class.
        */
        virtual bool isServer() const 
	{
                return (flags & PROT_FLAG_SERVER) ? true : false;
        }

        /**
           Returns true if the protocol acts as a client.
        */
        virtual bool isClient() const 
	{
                return (flags & PROT_FLAG_CLIENT) ? true : false;
        }
	/**
	 Returns true if this is an application protocol.
	 */
	bool isApplication() const;
        /**
           Returns true if there is pending data to be received.
        */
        virtual bool hasIncomingData();

        /**
           Returns true if the protocol is connected to another peer.
        */
        virtual bool isConnected() const 
	{
                return (flags & PROT_FLAG_CONNECTED) ? true : false;
        }

        // Flag functions
        void setFlags(const int _flags) 
	{
                flags = _flags;
        }
        void setFlag(const int flag) 
	{
                flags = flags | (flag);
        }
        void unSetFlag(const int flag) 
	{
                flags = flags & ~(flag);
        }
        int getFlags() const 
	{
                return flags;
        }
	virtual bool setNonblock(bool nonblock) { return false; }
        /**
           Returns true if the protocol has completed sending and
           reeciving all pending data objects.
        */
        virtual bool isDone() const 
	{
                return (mode == PROT_MODE_DONE) ? true : false;
        }
        virtual bool isGarbage() const 
	{
                return (mode == PROT_MODE_GARBAGE) ? true : false;
        }

        // Mode functions
        ProtocolMode getMode() const 
	{
                return mode;
        }
        void setMode(ProtocolMode _mode) 
	{
                mode = _mode;
        }

        // Quick send function. Adds data object to queue and starts thread
        // You need to make sure that no one else has a pointer to the dObj, i.e., in most cases
        // make a copy
        // Functions that CAN or SHOULD be overridden/implemented
        //virtual int recycle();
        virtual ProtocolEvent acceptClient() 
	{
                return PROT_EVENT_ERROR;
        }
        virtual ProtocolEvent connectToPeer() 
	{
                return PROT_EVENT_ERROR;
        }
        // Functions to send and receive data objects
        /*
        	This function takes a data object to send and sends it immediately.
        	
        	Returns: PROT_EVENT_SUCCESS on successful transmission, another
        	protocol event on failiure.
        	
        	This function does not delete the data object. The data object remains
        	the property of the caller. (This is the way it's supposed to be, so
        	that the caller can send a proper event regarding success/failiure of
        	sending a data object.)
        */
        virtual ProtocolEvent sendDataObjectNow(const DataObjectRef& dObj);

	// Unregister this protocol with the Protocol manager
	void registerWithManager();
	void unregisterWithManager();

// SW: START CONFIG:
    virtual void setConfiguration(ProtocolConfiguration *cfg);
// SW: END CONFIG.

    static string protTypeToStr(ProtType_t type) {
        if (type < TYPE_LOCAL || type >= NUM_PROTOCOL_TYPES) {
            return string("Invalid type");
        }
        return string(typestr[type]);
    }

	void logStatistics(); // MOS
};

#endif
