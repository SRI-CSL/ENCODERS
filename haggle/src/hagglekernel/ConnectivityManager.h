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
#ifndef _CONNECTIVITYMANAGER_H
#define _CONNECTIVITYMANAGER_H

#include <libcpphaggle/List.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/Timeval.h>

using namespace haggle;

#include "Manager.h"
#include "Trace.h"
#include "ConnectivityInterfacePolicy.h"
/*
	Return values for reporting and checking interfaces managed by
	the connectivity manager.
*/
typedef enum {
	INTERFACE_STATUS_ERROR = -1,
	INTERFACE_STATUS_NONE = 0,
	INTERFACE_STATUS_UNKNOWN, // An interface that was not known since before
	INTERFACE_STATUS_OTHER,  // A known non-Haggle interface
	INTERFACE_STATUS_HAGGLE, // A known Haggle interface
} InterfaceStatus_t;

class ConnectivityManager;

#include "Connectivity.h"

/*
	CM_DBG macro - debugging for the connectivity manager and connectivities.

	Turn this on to make the connectivity manager verbose about what it is
	doing.
*/

// MOS - added DBG2

#if 1
#define CM_DBG(f, ...) \
	HAGGLE_DBG(f, ## __VA_ARGS__)
#define CM_DBG2(f, ...) \
	HAGGLE_DBG2(f, ## __VA_ARGS__)
#else
#define CM_DBG(f, ...)
#define CM_DBG2(f, ...)
#endif

/** Connectivity manager.

	The purpose of the connectivity manager is to find "nearby" haggle interfaces
	that represent interfacing points to other Haggle nodes. The Connectivity manager,
	however, knows nothing about the nodes and which one of them an interface
	belongs to. Its only task is to report when interfaces are discovered and
	when they disappear.
	Whenever a new interface is discovered, the Connectivity manager reports the 
	presence of the interface through a public event.
	"Nearby" can, e.g., mean within bluetooth range, or within the same ethernet
	segment.

	The Connectivity manager achieves its task by adding, removing and maintaining
	interfaces in the "Interface store", which stores currently active interfaces
	in the system. The actual detection of interfaces is delegated to connectivity
	modules that run in separate threads. The Connectivity manager maintains the
	current active connectivities in a connectivity module registry. It uses a
	private event to communicate with the modules.
	*/
class ConnectivityManager : public Manager
{
	typedef List<Connectivity *> connectivity_registry_t;
	connectivity_registry_t conn_registry;
	
	class InterfaceStats {
	public:
		unsigned long numTimesSeen;
		Timeval lastSeen;
		bool isHaggle; // True if this interface belongs to a Haggle device
		InterfaceStats(bool _isHaggle = false) : 
			numTimesSeen(1), lastSeen(Timeval::now()), isHaggle(_isHaggle) {}
		void operator++(int) { numTimesSeen++; lastSeen.now(); }
	};
	/*
		The connectivity manager maintains a registry of known interfaces,
		both Haggle ones and others. This is useful for Bluetooth connectivities,
		because they can avoid doing costly and blocking service discoveries on
		interfaces which have already been scanned for services before.

		The registry also maintains statistics for each interface, which
		might come in handy at some point.

		TODO: This registry can grow indefinately. Should probably implement
		some garbage collection of interfaces that have not been seen for 
		some time.
	*/
	typedef Map<InterfaceRef, InterfaceStats> known_interface_registry_t;
	known_interface_registry_t known_interface_registry;

        Mutex blMutex; // Protects blacklist
        List<Interface *> blacklist; // Blacklist interfaces
        void addToBlacklist(Interface::Type_t type, const unsigned char *identifier);
        bool removeFromBlacklist(Interface::Type_t type, const unsigned char *identifier);
        bool isBlacklisted(Interface::Type_t type, const unsigned char *identifier);
        
	EventType blacklistFilterEvent;
        void onBlacklistDataObject(Event *e);

	// This mutex protects the connectivity registry
	Mutex connMutex;
	// This mutex protects the known interface registry
	Mutex ifMutex;

	void report_dead(InterfaceRef iface);

	// This will start a new connectivity on the given Interface (must be a local interface).
	void spawn_connectivity(const InterfaceRef& iface);
        bool init_derived();
// SW: START ARP CACHE INSERTION:
    bool manualArpInsertion;
    bool pingArpInsertion;
    string arpInsertionPathString;
// SW: END ARP CACHE INSERTION.
protected:
	void onConfig(Metadata *m);
// SW: START: custom ethernet beacon period:
    int eth_beacon_period_ms;
    int eth_beacon_jitter_ms;
    int eth_beacon_epsilon_ms;
    int eth_beacon_loss_max;
// SW: END: custom ethernet beacon period.
public:	
	EventType deleteConnectivityEType;

	/*
	There are two ways that interfaces can go down: silently and
	non-silently. Silently means it's simply not detected in a certain 
	time and is therefore considered down. Non-silently means some sort
	of event (haggle or otherwise) happens to show that the interface 
	went down.


	When detecting interfaces which go down non-silently, you should do
	the following to notify haggle about them:

	1. When the interface comes online, call report_interface.
	2. When the interface goes down, call delete_interface.


	To notify haggle about interfaces coming up and going down, when 
	they silently go down, do the following in the detect loop:

	1.	call age_interfaces
	2.	Detect interfaces. Call report_interface for each interface found. 
	

	When a local interface goes down, all child interfaces will automatically
	go down when the local parent interface is deleted.

	*/
	

	/**
		Calls the age() function for each interface found by the given Interface.
		ConnectivityLocal can give a null-interface, indicating that all interfaces
		with a null-interface as parent will be aged.
	
		@param whose the parent interface that wants to age its children.
		@param lifetime an optional pointer to a Timeval that when the function returns
		will hold the lifetime of the child interface which is closest to death.
		If there are no child interfaces, the lifetime Timeval will be invalid.
	*/
	void age_interfaces(const InterfaceRef &whose, Timeval *lifetime = NULL);
	/**
		Report and register an interface in the known_interface_registry, and
		increase stats if it is a "new contact".

		@param iface the interface to report
		@param isHaggle whether the interface belongs to a Haggle device or not.
		@returns INTERFACE_STATUS_UNKNOWN if the interface was not known since before,
		or INTERFACE_STATUS_OTHER / INTERFACE_STATUS_HAGGLE depending on @param
		isHaggle.
	*/
	InterfaceStatus_t report_known_interface(const Interface& iface, bool isHaggle = false);
	InterfaceStatus_t report_known_interface(const InterfaceRef& iface, bool isHaggle = false);
	InterfaceStatus_t report_known_interface(Interface::Type_t type, const unsigned char *identifier, bool isHaggle = false);

	/**
		report_interface functions.
		The given interfaces are owned by the caller.

		@param found the interface to report.
		@param found_by the parent interface that wants to age its children.
		@param policy a policy object for this interface. The policy will be 
		associated with the interface, and will be deleted with the interface.
		@returns INTERFACE_STATUS_NONE if the interface was not previously known,
		or INTERFACE_STATUS_HAGGLE if it was.
	*/
	InterfaceStatus_t report_interface(Interface *found, const InterfaceRef &found_by, 
			      ConnectivityInterfacePolicy *policy);
	InterfaceStatus_t report_interface(InterfaceRef &found, const InterfaceRef &found_by, 
			      ConnectivityInterfacePolicy *policy);
        /**
        	Utility function to check whether an interface already exists in the
		interface store or not.

		@param type the interface type to check for.
		@param identifier the interface identifier of the interface to check for.
		@returns INTERFACE_STATUS_NONE if the interface is not in the store, otherwise
		INTERFACE_STATUS_HAGGLE.
        */
	InterfaceStatus_t have_interface(Interface::Type_t type, const unsigned char *identifier);

	 /**
        	Utility function to check whether an interface already exists in the
		interface store or not.

		@param iface the interface to check for.
		@returns INTERFACE_STATUS_NONE if the interface is not in the store, otherwise
		INTERFACE_STATUS_HAGGLE.
        */
	InterfaceStatus_t have_interface(const Interface *iface);
	/**
        	Utility function to check whether an interface already exists in the
		interface store or not.

		@param iface the interface to check for.
		@returns INTERFACE_STATUS_NONE if the interface is not in the store, otherwise
		INTERFACE_STATUS_HAGGLE.
        */
	InterfaceStatus_t have_interface(const InterfaceRef& iface);
	/*
		Check if an interface is known from before.

		@param type the interface type to check for.
		@param identifier the interface identifier of the interface to check for.
		@returns INTERFACE_STATUS_UNKNOWN if the interface is not known, 
		INTERFACE_STATUS_HAGGLE if the interface is known to belong to a haggle device,
		or INTERFACE_STATUS_OTHER if it is known to be a non-Haggle device.
	*/
	InterfaceStatus_t is_known_interface(Interface::Type_t type, const unsigned char *identifier);
	/*
		Check if an interface is known from before.

		@param iface the interface to check for.
		@returns INTERFACE_STATUS_UNKNOWN if the interface is not known, 
		INTERFACE_STATUS_HAGGLE if the interface is known to belong to a haggle device,
		or INTERFACE_STATUS_OTHER if it is known to be a non-Haggle device.
	*/
	InterfaceStatus_t is_known_interface(const InterfaceRef& iface);

	/**
        	Utility function to delete an interface
        */
        void delete_interface(Interface *iface);
	/**
		Utility function to delete an interface by reference.
	*/
	void delete_interface(InterfaceRef &iface);
	/**
        	Utility function to delete an interface by type and identifier.
        */
        void delete_interface(Interface::Type_t type, const unsigned char *identifier);
	/**
        	Utility function to delete an interface by its name.
        */
        void delete_interface(const string name);
#ifdef DEBUG
	void onDebugCmdEvent(Event *e);
#endif
        void onInsertConnectivity(Event *e);
        void onDeleteConnectivity(Event *e);
        void onIncomingDataObject(Event *e);
        void onFailedToSendDataObject(Event *e);
        void onNewPolicy(Event *e);

	void onStartup();
        void onPrepareShutdown();
	ConnectivityManager(HaggleKernel *_kernel = haggleKernel);
        ~ConnectivityManager();
// SW: START ARP CACHE INSERTION:
    bool useManualArpInsertion();
    bool usePingArpInsertion();
    string getArpInsertionPathString();
// SW: END ARP CACHE INSERTION.
};

#endif /* _CONNECTIVITYMANAGER_H */
