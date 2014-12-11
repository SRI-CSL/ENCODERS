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
 *   Yu-Ting Yu (yty)
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

#include "ConnectivityLocal.h"
#if defined(ENABLE_ETHERNET)
#include "ConnectivityEthernet.h"
#endif
#if defined(ENABLE_BLUETOOTH)
#include "ConnectivityBluetooth.h"
#endif
#if defined(ENABLE_MEDIA)
#include "ConnectivityMedia.h"
#endif
#include "ConnectivityManager.h"

#include "Utility.h"
#include <utils.h>

// SW: START ARP CACHE INSERTION:
#include "SocketWrapperUDP.h"
// SW: END ARP CACHE INSERTION.

/*
  CM_IFACE_DBG macro - debugging for the connectivity manager.

  Turn this on to make the connectivity manager verbose about what it is
  doing with interfaces (adding, tagging as up, deleting, etc.).

  WARNING: turning this on makes the connectivity manager EXTREMELY VERBOSE.
*/
#if 1
#define CM_IFACE_DBG(f, ...)					\
	HAGGLE_DBG(f, ## __VA_ARGS__)
#else
#define CM_IFACE_DBG(f, ...)
#endif

ConnectivityManager::ConnectivityManager(HaggleKernel * _haggle) :
                Manager("ConnectivityManager", _haggle),
// SW: START ARP CACHE INSERTION:
    manualArpInsertion(false),
    pingArpInsertion(false),
    arpInsertionPathString(ARPHELPER_COMMAND_PATH),
// SW: END ARP CACHE INSERTION.
// SW: START: custom ethernet beacon period:
                eth_beacon_period_ms(0),
                eth_beacon_jitter_ms(0),
                eth_beacon_epsilon_ms(0),
                eth_beacon_loss_max(0)
// SW: END: custom ethernet beacon period.
{
}

bool ConnectivityManager::init_derived()
{
	int ret;

#define __CLASS__ ConnectivityManager
	CM_DBG("Initializing\n");
	CM_DBG("INIT CMDBG\n");
	HAGGLE_DBG("INIT HAGGLE_DBG\n");
	
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_INCOMING, onIncomingDataObject);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event interest\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_RESOURCE_POLICY_NEW, onNewPolicy);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event interest\n");
		return false;
	}
#ifdef DEBUG
	ret = setEventHandler(EVENT_TYPE_DEBUG_CMD, onDebugCmdEvent);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event interest\n");
		return false;
	}
#endif
	
	deleteConnectivityEType = registerEventType("ConnectivitManager Delete Connectivity Event", onDeleteConnectivity);

	if (deleteConnectivityEType < 0) {
		HAGGLE_ERR("Could not register connectivity event type...\n");
		return false;
	}

	registerEventTypeForFilter(blacklistFilterEvent, "Blacklist filter", onBlacklistDataObject, "Connectivity=*");

	return true;
}

/*
	Defer the startup of local connectivity until every other manager is
	prepared for startup. This will avoid any discovery of other nodes
	that might trigger events before other managers are prepared.
 */
void ConnectivityManager::onStartup()
{

	HAGGLE_DBG("START DBG\n");
	// Create and start local connectivity module
	Connectivity *conn = ConnectivityLocal::create(this);
	
	if (conn->startDiscovery()) {
		// Success! Store the connectivity:
                Mutex::AutoLocker l(connMutex);
		conn_registry.push_back(conn);
	} else {
		// Failure: delete the connectivity:
		delete conn;

		HAGGLE_ERR("Unable to start local connectivity module.\n");
	}
}

void ConnectivityManager::onPrepareShutdown()
{
	int n = 0;

	unregisterEventTypeForFilter(blacklistFilterEvent);
	
	CM_DBG("Telling all connectivities to cancel.\n");

	// If some connectivities are still running, then stop them.
	// Stopping a connectivity will wait for it to join... which
	// means that we will block until all connectivities are done.
        Mutex::AutoLocker l(connMutex);

	if (conn_registry.empty()) {
		signalIsReadyForShutdown();
	} else {
		// Go through all the connectivities:
		for (connectivity_registry_t::iterator it = conn_registry.begin(); it != conn_registry.end(); it++) {
			// Tell connectivity to cancel all activity:
			CM_DBG("Telling %s to cancel\n", (*it)->getName());
			(*it)->cancelDiscovery();
			CM_DBG("%s cancelled\n", (*it)->getName());
			n++;
		}
	}

	CM_DBG("Cancelled %d connectivity conn_registry\n", n);
}

ConnectivityManager::~ConnectivityManager()
{
        while (!blacklist.empty()) {
                Interface *iface = blacklist.front();
                blacklist.pop_front();
                delete iface;
        }
#if defined(ENABLE_BLUETOOTH)
        ConnectivityBluetoothBase::clearSDPLists();
#endif
	Event::unregisterType(deleteConnectivityEType);
}

void ConnectivityManager::onConfig(Metadata *m)
{
#if defined(ENABLE_BLUETOOTH)
	/*
	 Check for bluetooth module blacklisting/whitelisting data. For formatting
	 information, see ConnectivityBluetoothBase::updateSDPList().
	 */
	Metadata *bt = m->getMetadata("Bluetooth");
	
	if (bt) {
		ConnectivityBluetoothBase::updateSDPLists(bt);
	} 
#endif
// SW: START ARP CACHE INSERTION:
    const char *param = NULL;
    param = m->getParameter("use_arp_ping_insertion");
    if (NULL != param) {
        if (0 == strcmp(param, "true")) {
            pingArpInsertion = true;
        }
        else if (0 == strcmp(param, "false")) {
            pingArpInsertion = false;
        }
        else {
            HAGGLE_ERR("use_arp_ping_insertion parameter must be true or false\n");
        }
    }

    param = m->getParameter("use_arp_manual_insertion");
    if (NULL != param) {
        if (0 == strcmp(param, "true")) {
            manualArpInsertion = true;
        }
        else if (0 == strcmp(param, "false")) {
            manualArpInsertion = false;
        }
        else {
            HAGGLE_ERR("use_arp_manual_insertion parameter must be true or false\n");
        }
    }

    param = m->getParameter("arp_manual_insertion_path");
    if (NULL != param) {
        if (!manualArpInsertion) {
            HAGGLE_ERR("manualArpInsertion is set to false, but specified arp helper path\n");
        }
        arpInsertionPathString = string(param);
        if (0 != access(param, X_OK)) {
            HAGGLE_ERR("No execute permissions for arp helper file: %s\n", param);
            manualArpInsertion = false;
        }
    }

    if (pingArpInsertion || manualArpInsertion) {
        HAGGLE_DBG("ARP settings: use_arp_ping_insertion: %s, use_arp_manual_insertion: %s, arp_manual_insertion_path: %s\n",
            pingArpInsertion ? "true" : "false",
            manualArpInsertion ? "true" : "false", 
            arpInsertionPathString.c_str());
    }
// SW: END ARP CACHE INSERTION.

// SW: START: custom ethernet beacon period:
    Metadata *et = m->getMetadata("Ethernet");
    if (et) {
        const char *et_param; 
        et_param = et->getParameter("beacon_period_ms");
        if (et_param) {
            eth_beacon_period_ms = atoi(et_param);
        }
        et_param = et->getParameter("beacon_jitter_ms");
        if (et_param) {
            eth_beacon_jitter_ms = atoi(et_param);
        }
        et_param = et->getParameter("beacon_epsilon_ms");
        if (et_param) {
            eth_beacon_epsilon_ms = atoi(et_param);
        }
        et_param = et->getParameter("beacon_loss_max");
        if (et_param) {
            eth_beacon_loss_max = atoi(et_param);
        }
    }
// SW: END: custom ethernet beacon period.
}

void ConnectivityManager::onBlacklistDataObject(Event *e)
{
	DataObjectRefList& dObjs = e->getDataObjectList();

	while (dObjs.size()) {

		DataObjectRef dObj = dObjs.pop();

		if (!isValidConfigDataObject(dObj)) {
			HAGGLE_DBG("Received INVALID config data object\n");
			return;
		}
		HAGGLE_DBG("Received blacklist data object\n");

		Metadata *mc = dObj->getMetadata()->getMetadata(getName());

		if (!mc) {
			HAGGLE_ERR("No connectivity metadata in data object\n");
			return;
		}

		Metadata *blm = mc->getMetadata("Blacklist");

		while (blm) {
			const char *type = blm->getParameter("type");
			const char *action = blm->getParameter("action");
			string mac = blm->getContent();
			Interface::Type_t iftype = Interface::strToType(type);
			int act = 3;

			/*
			"action=add" means add interface to blacklist if not 
			present.

			"action=remove" means remove interface from blacklist if 
			present.

			All other values, including not having an "action" parameter
			means add interface if it is not in the blacklist, remove 
			interface if it is in the blacklist (effectively a toggle).
			*/

			if (action != NULL) {
				if (strcmp(action, "add") == 0)
					act = 1;
				else if (strcmp(action, "remove") == 0)
					act = 2;
			}

			if (iftype == Interface::TYPE_ETHERNET || 
				iftype == Interface::TYPE_BLUETOOTH ||
				iftype == Interface::TYPE_WIFI) {
					struct ether_addr etha;

					if (ether_aton_r(mac.c_str(), &etha)) {
						if (isBlacklisted(iftype, (unsigned char *)&etha)) {
							if (act != 1) {
								HAGGLE_DBG("Removing interface [%s - %s] from blacklist\n", type, mac.c_str());
								removeFromBlacklist(iftype, (unsigned char *)&etha);
							} else {
								HAGGLE_DBG("NOT removing interface [%s - %s] from blacklist (it's not there)\n", type, mac.c_str());
							}
						} else {
							if (act != 2) {
								HAGGLE_DBG("Blacklisting interface [%s - %s]\n", type, mac.c_str());
								addToBlacklist(iftype, (unsigned char *)&etha);
							} else {
								HAGGLE_DBG("NOT blacklisting interface [%s - %s] - already blacklisted\n", type, mac.c_str());
							}
						}
					}
			}

			blm = mc->getNextMetadata();
		}
	}
}

void ConnectivityManager::addToBlacklist(Interface::Type_t type, const unsigned char *identifier)
{
        if (isBlacklisted(type, identifier))
                return;

        blMutex.lock();

        Interface *iface = Interface::create(type, identifier);

        blacklist.push_back(iface);
        
        delete_interface(iface);
        blMutex.unlock();
}

bool ConnectivityManager::isBlacklisted(Interface::Type_t type, const unsigned char *identifier)
{
	Mutex::AutoLocker l(blMutex);

        for (List<Interface *>::iterator it = blacklist.begin(); it != blacklist.end(); it++) {
                if ((*it)->getType() == type && 
                    memcmp((*it)->getIdentifier(), identifier, (*it)->getIdentifierLen()) == 0)
                        return true;
        }
        return false;
}

bool ConnectivityManager::removeFromBlacklist(Interface::Type_t type, const unsigned char *identifier)
{
	Mutex::AutoLocker l(blMutex);

        for (List<Interface *>::iterator it = blacklist.begin(); it != blacklist.end(); it++) {
                if ((*it)->getType() == type && 
                    memcmp((*it)->getIdentifier(), identifier, (*it)->getIdentifierLen()) == 0) {
                        Interface *iface = *it;
                        blacklist.erase(it);
                        delete iface;
                        return true;
                }
        }
        return false;
}

#ifdef DEBUG
void ConnectivityManager::onDebugCmdEvent(Event *e)
{
	
	if (e->getDebugCmd()->getType() != DBG_CMD_PRINT_INTERNAL_STATE)
		return;
	
	kernel->getInterfaceStore()->print();
}
#endif /* DEBUG */

void ConnectivityManager::onDeleteConnectivity(Event *e)
{
	Mutex::AutoLocker l(connMutex);
	Connectivity *conn = (static_cast <Connectivity *>(e->getData()));

	if (!conn) {
		HAGGLE_ERR("Connectivity was NULL\n");	
		return;
	}

	CM_DBG("Deleting connectivity %s\n", conn->getName());

	// Take the connectivity out of the connectivity list
	conn_registry.remove(conn);

	conn->join();

	// Delete the connectivity:
	delete conn;

	// Are we preparing for shutdown?
	if (getState() == MANAGER_STATE_PREPARE_SHUTDOWN) {
		// Are there any connectivities left?
		if (conn_registry.empty()) {
			signalIsReadyForShutdown();
			CM_DBG("ConnectivityManager is prepared for shutdown!\n");
		} else {
			CM_DBG("ConnectivityManager preparing for shutdown: %ld connectivities left\n", 
				conn_registry.size());
		}
	}
}

void ConnectivityManager::spawn_connectivity(const InterfaceRef& iface)
{
        Mutex::AutoLocker l(connMutex);
	Connectivity *conn = NULL;
	
	// Is the shutdown procedure going?
	if (kernel->isShuttingDown())
		return;

	// Does the interface exist and is it a local interface?
	if (!iface || !iface->isLocal()) {
		HAGGLE_ERR("Trying to spawn connectivity on non-local interface\n");
		return;
	}

	connectivity_registry_t::iterator it = conn_registry.begin();

	for (; it != conn_registry.end() && conn == NULL; it++) {
		// Does this connectivity want the interface?
		if ((*it)->handleInterfaceUp(iface)) {
			// Yep. Don't start another one:
			conn = (*it);
		}
	}
	// Connectivity found?
	if (conn) {
		// Don't start another one:
		return;
	}
	
	// Create new connectivity module:
	switch (iface->getType()) {
#if defined(ENABLE_ETHERNET)
		case Interface::TYPE_ETHERNET:
		case Interface::TYPE_WIFI:
        {
            ConnectivityEthernet *conne = new ConnectivityEthernet(this, iface);
            conn = conne;
// SW: START: custom ethernet beacon period:
            if (eth_beacon_period_ms > 0) {
                conne->setBeaconPeriodMs(eth_beacon_period_ms);
            }
            if (eth_beacon_jitter_ms > 0) {
                conne->setBeaconJitterMs(eth_beacon_jitter_ms);
            }
            if (eth_beacon_epsilon_ms > 0) {
                conne->setBeaconEpsilonMs(eth_beacon_epsilon_ms);
            }
            if (eth_beacon_loss_max > 0) {
                conne->setBeaconLossMax(eth_beacon_loss_max);
            }
// SW: END: custom ethernet beacon period.
        }
        break;
#endif			
#if defined(ENABLE_BLUETOOTH)
		case Interface::TYPE_BLUETOOTH:
                        conn = new ConnectivityBluetooth(this, iface);
			break;
#endif
#if defined(ENABLE_MEDIA)
			// Nothing here yet.
		case Interface::TYPE_MEDIA:
                        conn = new ConnectivityMedia(this, iface);
			break;
#endif
		default:
			break;
	}
	
	if (conn) {
		// Call init function to initialize the connectivity
		// and then start it:
		if (conn->init() && conn->startDiscovery()) {
			// Tell the connectivity what the current resource policy is:
			conn->setPolicy(kernel->getCurrentPolicy());
			// Success! Store the connectivity:
			conn_registry.push_back(conn);
		} else {
			// Failure: delete the connectivity:
			delete conn;
		}
	}
}

InterfaceStatus_t ConnectivityManager::report_known_interface(const InterfaceRef& iface, bool isHaggle)
{
	Mutex::AutoLocker l(ifMutex);
	InterfaceStats stats(isHaggle);

	Pair<known_interface_registry_t::iterator, bool> p = known_interface_registry.insert(make_pair(iface, stats));

	// The interface was not known since before.
	if (p.second)
		return INTERFACE_STATUS_UNKNOWN;

	(*p.first).second++;

	if (isHaggle)
		(*p.first).second.isHaggle = true;

	return (*p.first).second.isHaggle ? INTERFACE_STATUS_HAGGLE : INTERFACE_STATUS_OTHER;
}

InterfaceStatus_t ConnectivityManager::report_known_interface(Interface::Type_t type, 
							      const unsigned char *identifier, 
							      bool isHaggle)
{
	InterfaceStatus_t ret = INTERFACE_STATUS_ERROR;

	InterfaceRef iface = Interface::create(type, identifier);

	if (iface) {
		ret = report_known_interface(iface, isHaggle);
	}

	return ret;
}

InterfaceStatus_t ConnectivityManager::report_known_interface(const Interface& report_iface, bool isHaggle)
{
	InterfaceStatus_t ret = INTERFACE_STATUS_ERROR;
	InterfaceRef iface = report_iface.copy();
	
	if (iface) {
		ret = report_known_interface(iface, isHaggle);
	}

	return ret;
}

/*
  This function makes sure an interface is in the table.
*/
InterfaceStatus_t ConnectivityManager::report_interface(Interface *found, const InterfaceRef& found_by, ConnectivityInterfacePolicy *policy)
{
	bool was_added;
	
        if (!found || isBlacklisted(found->getType(), found->getIdentifier()))
                return INTERFACE_STATUS_NONE;

	InterfaceRef iface = kernel->getInterfaceStore()->addupdate(found, found_by, policy, &was_added);

        if(iface && !iface->isLocal()){
             //YTY: report interface to loss estimate manager
             kernel->addEvent(new Event(EVENT_TYPE_RECEIVE_BEACON, iface, 0));
        }

	if (!iface || !was_added )
		return INTERFACE_STATUS_NONE;

	// Make sure the interface is up
	iface->up();

	CM_IFACE_DBG("%s interface [%s/%s] added/updated\n", 
		iface->isLocal() ? "Local" : "Neighbor", 
		iface->getIdentifierStr(), iface->getName());

	// Tell everyone about this new interface
	if (iface->isLocal()) {
		spawn_connectivity(iface);
		kernel->addEvent(new Event(EVENT_TYPE_LOCAL_INTERFACE_UP, iface));
	} else {
		kernel->addEvent(new Event(EVENT_TYPE_NEIGHBOR_INTERFACE_UP, iface));
	}
	return INTERFACE_STATUS_HAGGLE;
}

InterfaceStatus_t ConnectivityManager::report_interface(InterfaceRef& found, const InterfaceRef& found_by, ConnectivityInterfacePolicy *policy)
{
	bool was_added;
	
        if (!found) {
		HAGGLE_ERR("Invalid interface reported\n");
		return INTERFACE_STATUS_NONE;
	}

        if (isBlacklisted(found->getType(), found->getIdentifier())) {
		HAGGLE_ERR("Interface [%s] is blacklisted\n", found->getIdentifierStr());
		return INTERFACE_STATUS_NONE;
	}

	InterfaceRef iface = kernel->getInterfaceStore()->addupdate(found, found_by, policy, &was_added);

	if (!iface || !was_added) {
		//HAGGLE_ERR("Interface [%s] was not added to interface store\n", found->getIdentifierStr());
		return INTERFACE_STATUS_NONE;
	}
	// Make sure the interface is up
	iface->up();

	CM_IFACE_DBG("%s interface [%s/%s] added/updated\n", 
		iface->isLocal() ? "Local" : "Neighbor", 
		iface->getIdentifierStr(), iface->getName());

	// Tell everyone about this new interface
	if (iface->isLocal()) {
		spawn_connectivity(iface);
		kernel->addEvent(new Event(EVENT_TYPE_LOCAL_INTERFACE_UP, iface));
	} else {
		kernel->addEvent(new Event(EVENT_TYPE_NEIGHBOR_INTERFACE_UP, iface));
	}
	return INTERFACE_STATUS_HAGGLE;
}

/*
 The connectivity manager watches all incoming data objects to see
 if the source of the received data object is not a registered
 neighbor. This may happen in case a data object is received from
 a neighbor that has not yet been discovered (this happens typically
 with Bluetooth, but can also happen with WiFi due to periodic neighbor 
 discovery).
*/
void ConnectivityManager::onIncomingDataObject(Event *e)
{
	if (!e || !e->hasData())
		return;

	DataObjectRef dObj = e->getDataObject();
    // JJOY:
    if(!dObj) {
        HAGGLE_DBG("dataobject is null\n");
        return;
    }

    if(dObj->isNodeDescription()){
    	HAGGLE_DBG("Received node description\n");
    }

	InterfaceRef localIface = dObj->getLocalInterface();
	InterfaceRef remoteIface = dObj->getRemoteInterface();

	if (!localIface) {
		HAGGLE_DBG("No local interface set on snooped data object, IGNORING\n");
		return;
	}
	if (!remoteIface) {
		HAGGLE_DBG("No remote interface set on snooped data object, IGNORING\n");
		return;
	}
	
	// Make sure the interface is marked as up.
	remoteIface->up();

	/* Check that there is a receive interface set and that it is
	 * not from an application socket */
	if (!remoteIface->isApplication()) {
		CM_IFACE_DBG("DataObject [%s] incoming on interface [%s] from neighbor with interface [%s]\n", 
			   DataObject::idString(dObj).c_str(), localIface->getIdentifierStr(), 
			   remoteIface->getIdentifierStr());

		// Check whether this interface is already registered or not
		if (!have_interface(remoteIface)) {
			remoteIface->setFlag(IFFLAG_SNOOPED);
			if (remoteIface->getType() == Interface::TYPE_BLUETOOTH) {
				CM_IFACE_DBG("snooped Bluetooth interface [%s] %s\n", 
					     remoteIface->getIdentifierStr(), remoteIface->isUp() ? "UP" : "DOWN");
				report_interface(remoteIface, localIface, new ConnectivityInterfacePolicyTTL(2));
			} else if (remoteIface->getType() == Interface::TYPE_ETHERNET ||
				   remoteIface->getType() == Interface::TYPE_WIFI) {
				CM_IFACE_DBG("snooped Ethernet/WiFi interface [%s] %s\n", 
					     remoteIface->getIdentifierStr(), remoteIface->isUp() ? "UP" : "DOWN");
				report_interface(remoteIface, localIface, new ConnectivityInterfacePolicyTTL(3));
			} else {
				// hmm... this shouldn't happen. If it does we've added an 
				// interface type and forgotten to add it above.
				HAGGLE_DBG("Snooped unknown interface type.");
				report_interface(remoteIface, localIface, new ConnectivityInterfacePolicyTTL(3));
			}
			report_known_interface(remoteIface, true);
		}
	}
}

/*
	The connectivity manager watches all data objects that failed to send to
	detect interfaces that have gone down.
*/
void ConnectivityManager::onFailedToSendDataObject(Event *e)
{
	if (!e || !e->hasData())
		return;

	DataObjectRef dObj = e->getDataObject();
	InterfaceRef localIface = dObj->getLocalInterface();
	InterfaceRef remoteIface = dObj->getRemoteInterface();

	if (!localIface || !remoteIface)
		return;

	/* Check that there is a receive interface set and that it is
	 * not from an application socket */
	if (!remoteIface->isApplication()) {
		CM_IFACE_DBG("DataObject failed to send on interface [%s] to neighbor with interface [%s]\n", 
			     Interface::idString(localIface).c_str(), 
			     Interface::idString(remoteIface).c_str());

		delete_interface(remoteIface);
	}
}

void ConnectivityManager::report_dead(InterfaceRef iface)
{
	iface->down();

	if (iface->isLocal()) {
		// This is a local interface - there should be a Connectivity associated
		// with it.
		
		synchronized(connMutex) {
                        connectivity_registry_t::iterator it = conn_registry.begin();
                        // Go through the entire registry:
                        for (; it != conn_registry.end(); it++) {
                                // Tell this interface to handle the interface going down:
                                (*it)->handleInterfaceDown(iface);
                        }
                }
		
		CM_IFACE_DBG("Local interface [%s/%s] deleted.\n", 
			     iface->getIdentifierStr(), iface->getName());
		
		// Tell the rest of haggle that this interface has gone down:
		kernel->addEvent(new Event(EVENT_TYPE_LOCAL_INTERFACE_DOWN, iface));
	} else {
		CM_IFACE_DBG("Neighbour interface [%s/%s] deleted.\n", 
			     iface->getIdentifierStr(), iface->getName());
		
		// Tell the rest of haggle that this interface has gone down:
		kernel->addEvent(new Event(EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN, iface));	
	}
}

void ConnectivityManager::delete_interface(InterfaceRef &iface)
{
	InterfaceRefList dead;
        
        kernel->getInterfaceStore()->remove(iface, &dead);

	while (!dead.empty()) {
		report_dead(dead.pop());
	}
}

void ConnectivityManager::delete_interface(Interface *iface)
{
	InterfaceRefList dead;

        kernel->getInterfaceStore()->remove(iface, &dead);

	while (!dead.empty()) {
		report_dead(dead.pop());
	}
}

void ConnectivityManager::delete_interface(Interface::Type_t type, const unsigned char *identifier)
{
	InterfaceRefList dead;
        
        kernel->getInterfaceStore()->remove(type, identifier, &dead);

	while (!dead.empty()) {
		report_dead(dead.pop());
	}
}

void ConnectivityManager::delete_interface(const string name)
{
	InterfaceRefList dead;
        
        kernel->getInterfaceStore()->remove(name, &dead);

	while (!dead.empty()) {
		report_dead(dead.pop());
	}
}

InterfaceStatus_t ConnectivityManager::have_interface(Interface::Type_t type, const unsigned char *identifier)
{
	return kernel->getInterfaceStore()->stored(type, identifier) ? INTERFACE_STATUS_HAGGLE : INTERFACE_STATUS_NONE;
}

InterfaceStatus_t ConnectivityManager::have_interface(const Interface *iface)
{
	return kernel->getInterfaceStore()->stored(*iface) ? INTERFACE_STATUS_HAGGLE : INTERFACE_STATUS_NONE;
}

InterfaceStatus_t ConnectivityManager::have_interface(const InterfaceRef& iface)
{
	return kernel->getInterfaceStore()->stored(iface) ? INTERFACE_STATUS_HAGGLE : INTERFACE_STATUS_NONE;
}


/*
	The known interface cache is used to, e.g., avoid excessive SDP lookups with Bluetooth
	that take a lot of time and cause unnecessary interference. Ideally, a lookup
	should only have to be done once for any particular device, after which it is either 
	classified as a Haggle device or as a 'other' type of device. However, if a lookup fails 
	in an undetectable way, the cache might be 'wrong' and cause a device to never be 
	discovered as a Haggle device. Therefore, it is possible to set the parameter
	KNOWN_INTERFACE_TIMES_CACHED_MAX, which is the maximum number of times the cache
	can be queried for an interface before the cache entry expires. After entry expiry, the 
	device has to be queried again using SDP, or by some other means, in order to once more 
	be classified as either a Haggle device or 'other' device.
*/
#define KNOWN_INTERFACE_TIMES_CACHED_MAX 6

InterfaceStatus_t ConnectivityManager::is_known_interface(const InterfaceRef& iface)
{
	Mutex::AutoLocker l(ifMutex);
	known_interface_registry_t::iterator it = known_interface_registry.find(iface);

	if (it == known_interface_registry.end())
		return INTERFACE_STATUS_UNKNOWN;

	// Only cache the interface for a specified number of lookups.
	// This hopefully protects against mistakenly classifying devices as non-Haggle ones,
	// although they really are Haggle devices.
	if ((*it).second.numTimesSeen > KNOWN_INTERFACE_TIMES_CACHED_MAX) {
		HAGGLE_DBG("Interface %s removed from interface cache\n", (*it).first->getIdentifierStr());
		known_interface_registry.erase(it);
		return INTERFACE_STATUS_UNKNOWN;
	}

	// Increase the stats
	(*it).second++;

	return (*it).second.isHaggle ? INTERFACE_STATUS_HAGGLE : INTERFACE_STATUS_OTHER;
}

InterfaceStatus_t ConnectivityManager::is_known_interface(Interface::Type_t type, const unsigned char *identifier)
{
        InterfaceStatus_t ret = INTERFACE_STATUS_ERROR;

	InterfaceRef iface = Interface::create(type, identifier);
	
	if (iface) {
		ret = is_known_interface(iface);
	} else {
		HAGGLE_ERR("Could not create interface of type %s\n", Interface::typeToStr(type));
	}

	return ret;
}

void ConnectivityManager::age_interfaces(const InterfaceRef &whose, Timeval *lifetime)
{
	InterfaceRefList dead;
	
	kernel->getInterfaceStore()->age(whose, &dead, lifetime);

	while (!dead.empty()) {
		report_dead(dead.pop());
	}
}

void ConnectivityManager::onNewPolicy(Event *e)
{
	Mutex::AutoLocker l(connMutex);
	PolicyRef pr;
	
	pr = e->getPolicy();
	
	for (connectivity_registry_t::iterator it = conn_registry.begin(); it != conn_registry.end(); it++) {
		(*it)->setPolicy(pr);
	}
}

// SW: START ARP CACHE INSERTION:
bool ConnectivityManager::useManualArpInsertion()
{
    return manualArpInsertion;
}

bool ConnectivityManager::usePingArpInsertion()
{
    return pingArpInsertion;
}

string ConnectivityManager::getArpInsertionPathString()
{
    return arpInsertionPathString;
}
// SW: END ARP CACHE INSERTION.
