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
#ifndef _PROTOCOLMANAGER_H
#define _PROTOCOLMANAGER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

class ProtocolManager;

#include <libcpphaggle/Map.h>

#include "Protocol.h"
#include "Interface.h"
#include "Manager.h"
#include "networkcoding/protocol/NetworkCodingProtocolHelper.h"
#include "fragmentation/protocol/FragmentationProtocolHelper.h"

#include <haggleutils.h>

#include "ProtocolFactory.h"

/*
	PM_DBG macro - debugging for the protocol manager and protocols.

	Turn this on to make the protocol manager verbose about what it is doing.
*/
#if 1
#define PM_DBG(f, ...) \
	HAGGLE_DBG(f, ## __VA_ARGS__)
#else
#define PM_DBG(f, ...)
#endif

class ProtocolUDPProxy;

using namespace haggle;

/** */
class ProtocolManager : public Manager
{
	friend class ProtocolFactory;
	friend class Protocol;
private:
	NetworkCodingProtocolHelper* networkCodingProtocolHelper;
	FragmentationProtocolHelper* fragmentationProtocolHelper;

	typedef Map<unsigned long, Protocol *> protocol_registry_t;
        protocol_registry_t protocol_registry;
        EventType delete_protocol_event;
        EventType add_protocol_event;
        EventType send_data_object_actual_event;
	EventType protocol_prepare_shutdown_timeout_event; // MOS
	EventType protocol_shutdown_timeout_event;
        // Event processing
        void onSendDataObject(Event *e);
        void onSendDataObjectActual(Event *e);
        void onLocalInterfaceUp(Event *e);
        void onLocalInterfaceDown(Event *e);
        void onNeighborInterfaceDown(Event *e);
        void onAddProtocolEvent(Event *e);
	bool shutdownNonApplicationProtocols(); // MOS
	bool shutdownApplicationProtocols(); // MOS
        void onDeleteProtocolEvent(Event *e);
	unsigned long forcedShutdownAfterTimeout; // MOS
	void onProtocolPrepareShutdownTimeout(Event *e); // MOS
	void onProtocolShutdownTimeout(Event *e);
	void onDebugCmdEvent(Event *e);
    Protocol *getCachedSenderProtocol(const ProtType_t type, const InterfaceRef& peerIface);

    Protocol *getClassifiedSenderProtocol(
        const InterfaceRef& peerIface,
        DataObjectRef& dObj);

	void onNodeUpdated(Event *e);
        /**
        	Returns the client sender protocol for the given remote interface.
        	
        	If no protocol is found, one will be started.
        	
        	Will only return NULL if one could not be found or started.
        */
        Protocol *getSenderProtocol(const ProtType_t type, const InterfaceRef& iface);

    Protocol *getCachedServerProtocol(const ProtType_t type, const InterfaceRef& iface);
        /**
        	Returns the server protocol for the given local interface.
        	
        	If no protocol is found, one will be started.
        	
        	Will only return NULL if one could not be found or started.
        */
        Protocol *getServerProtocol(const ProtType_t type, const InterfaceRef& iface);
	
	void onPrepareShutdown();
        void onShutdown();
	bool init_derived();
	void onConfig(Metadata *m);
	
	// This Runnable is used to schedule a timeout event after a certain
	// period of time.
	// When the timeout occurs, any hung protocol threads are forcefully
	// killed.
	class ProtocolKiller : public Runnable {
		unsigned long eventid, sleep_msec;
		HaggleKernel *kernel;
		
		bool run()
		{
			cancelableSleep(sleep_msec);
			kernel->addEvent(new Event(eventid, NULL, 0));
			return false;
		}
		void cleanup()
		{
		}
	public:
		ProtocolKiller(unsigned long _eventid,
			       unsigned long _sleep_msec,
			       HaggleKernel *_kernel) :
		eventid(_eventid),
		sleep_msec(_sleep_msec),
		kernel(_kernel) {}
		~ProtocolKiller() {}
	};
	ProtocolKiller *killer;

    ProtocolFactory *getProtocolFactory();
    ProtocolFactory *protocolFactory;

public:
        ProtocolManager(HaggleKernel *_kernel = haggleKernel);
        ~ProtocolManager();
        void onWatchableEvent(const Watchable& wbl);
	    bool registerProtocol(Protocol *p);
};

#endif /* _PROTOCOLMANAGER_H */
