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
 *   Ashish Gehani (AG)
 *   Hasnain Lakhani (HL)
 *   Tim McCarthy (TTM)
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
#include <libcpphaggle/Platform.h>
#include "SecurityManager.h"
#include "DataObject.h"
#include "ProtocolManager.h"
#include "Protocol.h"
#include "ProtocolRFCOMM.h"
#include <haggleutils.h>
// SW: START: SendPriorityManager
#include "SendPriorityManager.h"
// SW: END: SendPriorityManager
#include "XMLMetadata.h"

ProtocolManager::ProtocolManager(HaggleKernel * _kernel) :
    Manager("ProtocolManager", _kernel),
    forcedShutdownAfterTimeout(15), // MOS - replacement for Haggle's original protocol killer
    killer(NULL),
    protocolFactory(NULL)
{
    networkCodingProtocolHelper = new NetworkCodingProtocolHelper();
    fragmentationProtocolHelper = new FragmentationProtocolHelper();
}

ProtocolManager::~ProtocolManager() {

    // SW: fixing memory leaks, must unregister events
    Event::unregisterType(delete_protocol_event);
    Event::unregisterType(add_protocol_event);
    Event::unregisterType(send_data_object_actual_event);
    Event::unregisterType(protocol_prepare_shutdown_timeout_event);
    Event::unregisterType(protocol_shutdown_timeout_event);

    while (!protocol_registry.empty()) {
        Protocol *p = (*protocol_registry.begin()).second;
        protocol_registry.erase(p->getId());
        if(!p->isDetached()) // MOS - detached protocols are responsible for their own deletion
          delete p;
    }

    /* MOS - not used anymore

    if (killer) {
        killer->stop();
        HAGGLE_DBG("Joined with protocol killer thread\n");
        delete killer;
    }

    */

    if (NULL != protocolFactory) {
        delete protocolFactory;
    }

    if (this->networkCodingProtocolHelper) {
        delete this->networkCodingProtocolHelper;
        this->networkCodingProtocolHelper = NULL;
    }
    if(this->fragmentationProtocolHelper) {
        delete this->fragmentationProtocolHelper;
        this->fragmentationProtocolHelper = NULL;
    }
}

bool ProtocolManager::init_derived() {
    int ret;
#define __CLASS__ ProtocolManager

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND, onSendDataObject);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_LOCAL_INTERFACE_UP, onLocalInterfaceUp);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_LOCAL_INTERFACE_DOWN, onLocalInterfaceDown);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN, onNeighborInterfaceDown);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_NODE_UPDATED, onNodeUpdated);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    delete_protocol_event =
            registerEventType("ProtocolManager protocol deletion event", onDeleteProtocolEvent);

    if (delete_protocol_event < 0) {
        HAGGLE_ERR("Could not register protocol deletion event\n");
        return false;
    }

    add_protocol_event =
            registerEventType("ProtocolManager protocol addition event", onAddProtocolEvent);

    if (add_protocol_event < 0) {
        HAGGLE_ERR("Could not register protocol addition event\n");
        return false;
    }

    send_data_object_actual_event =
            registerEventType("ProtocolManager send data object actual event", onSendDataObjectActual);

    if (send_data_object_actual_event < 0) {
        HAGGLE_ERR("Could not register protocol send data object actual event\n");
        return false;
    }

    protocol_prepare_shutdown_timeout_event =
            registerEventType("ProtocolManager Protocol Prepare Shutdown Timeout Event", onProtocolPrepareShutdownTimeout);

    if (protocol_prepare_shutdown_timeout_event < 0) {
        HAGGLE_ERR("Could not register protocol prepare shutdown timeout event\n");
        return false;
    }

    protocol_shutdown_timeout_event =
            registerEventType("ProtocolManager Protocol Shutdown Timeout Event", onProtocolShutdownTimeout);

    if (protocol_shutdown_timeout_event < 0) {
        HAGGLE_ERR("Could not register protocol shutdown timeout event\n");
    return false;
    }

    ret = setEventHandler(EVENT_TYPE_DEBUG_CMD, onDebugCmdEvent);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }
    return true;
}

void ProtocolManager::onDebugCmdEvent(Event *e)
{
    if (e->getDebugCmd()->getType() == DBG_CMD_PRINT_PROTOCOLS) {
        protocol_registry_t::iterator it;
        for (it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
            Protocol *p = (*it).second;

            printf("Protocol \'%s\':\n", p->getName());

            printf("\tcommunication interfaces: %s <-> %s\n",
                    p->getLocalInterface() ? p->getLocalInterface()->getIdentifierStr() : "None",
                    p->getPeerInterface() ? p->getPeerInterface()->getIdentifierStr() : "None");
            if (p->getQueue() && p->getQueue()->size()) {
                           // printf("\tQueue:\n");
                   // p->getQueue()->print();
                       printf("\tQueue: %d objects\n", p->getQueue()->size()); // MOS - size is more interesting
        }
            else {
                printf("\tQueue: empty\n");
            }
        }
    // CBMEN, HL, Begin
    } else if (e->getDebugCmd()->getType() == DBG_CMD_OBSERVE_PROTOCOLS) {
        Metadata *m = new XMLMetadata(e->getDebugCmd()->getMsg());

        protocol_registry_t::iterator it;
        for (it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
            Protocol *p = (*it).second;
            Metadata *dm = m->addMetadata("Protocol");

            if (!dm) {
                continue;
            }

            dm->setParameter("name", p->getName());
            dm->setParameter("local_interface", p->getLocalInterface() ? p->getLocalInterface()->getIdentifierStr() : "None");
            dm->setParameter("peer_interface", p->getPeerInterface() ? p->getPeerInterface()->getIdentifierStr() : "None");
            dm->setParameter("queue_size", p->getQueue() ? p->getQueue()->size() : 0);

        }

        kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
    }
    // CBMEN, HL, End
}

void ProtocolManager::onAddProtocolEvent(Event *e) {
    registerProtocol(static_cast<Protocol *>(e->getData()));
}

bool ProtocolManager::registerProtocol(Protocol *p) {
    protocol_registry_t::iterator it;

    if (!p)
        return false;

    // MOS
    if(p->isDone() || p->isGarbage()) {
        HAGGLE_ERR("Protocol %s already terminated!\n", p->getName());
    return false;
    }

    // We are in shutdown, so do not accept this protocol to keep running.
    if (getState() > MANAGER_STATE_RUNNING) {
        p->shutdown();
        if (!p->isDetached()) {
            HAGGLE_DBG("Waiting for protocol %s to terminate\n", p->getName());
            p->join();
            HAGGLE_DBG("Joined with protocol %s\n", p->getName());
        }
        return false;
    }

    if (protocol_registry.insert(make_pair(p->getId(), p)).second == false) {
        HAGGLE_ERR("Protocol %s already registered!\n", p->getName());
        return false;
    }

    p->setRegistered();

    HAGGLE_DBG("Protocol %s registered\n", p->getName());

    return true;
}

void ProtocolManager::onNodeUpdated(Event *e)
{
    if (!e)
        return;

    // MOS - the follwong has been disabled due to possible race condition caused by peer node update (receiver side).
        // Seems to show up only in connection with UDP broadcast, normally after receiveDataObject() in Protocol.cpp, when the peerDescription is accessed.
        // It is better to update the peer node based on the interface in the protocol-specific receive function.
    // It should also be noted that for a receiver the peer node is not essential (but useful for debugging).
#ifdef ENABLE_PROTOCOL_NODE_UPDATE
    NodeRef& node = e->getNode();
    NodeRefList& nl = e->getNodeList();

    if (!node)
        return;

    // Check if there are any protocols that are associated with the updated nodes.
    for (NodeRefList::iterator it = nl.begin(); it != nl.end(); it++) {
        NodeRef& old_node = *it;
        old_node.lock();

        for (InterfaceRefList::const_iterator it2 = old_node->getInterfaces()->begin();
            it2 != old_node->getInterfaces()->end(); it2++) {
                const InterfaceRef& iface = *it2;

                for (protocol_registry_t::iterator it3 = protocol_registry.begin(); it3 != protocol_registry.end(); it3++) {
                    Protocol *p = (*it3).second;

                    if (p->isForInterface(iface)) {
                        HAGGLE_DBG("Setting peer node %s on protocol %s\n", node->getName().c_str(), p->getName());
                        p->setPeerNode(node);
                    }
                }
        }
        old_node.unlock();
    }
#endif
}

// MOS - shut down clients and then servers and check if done

bool ProtocolManager::shutdownNonApplicationProtocols()
{
  bool ready = true;

  // Go through the registered protocols
  for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
    // Tell the protocol to shutdown, with the exception of the application IPC protocol
    if (!(*it).second->isServer() && !((*it).second->isApplication())) { // MOS - clients only
    if(!(*it).second->isDone() && !(*it).second->isGarbage()) {
      (*it).second->closeQueue(); // MOS - no closeAndClearQueue to avoid concurrent readers
      (*it).second->shutdown();
    }
    }
  }

  for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
    if (!(*it).second->isServer() && !((*it).second->isApplication())) { // MOS - clients only
      HAGGLE_DBG("Not ready for shutdown - Client protocol %s still registered.\n", (*it).second->getName());
      ready = false;
    }
  }

  if(ready) {
    HAGGLE_DBG("All client protocols deleted - now shutting down server protocols.\n");
    for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
      // Tell the server protocol to shutdown, with the exception of the
      // application IPC protocol
      if (!((*it).second->isApplication())) {
    if(!(*it).second->isDone() && !(*it).second->isGarbage()) {
      (*it).second->closeQueue(); // MOS - no closeAndClearQueue to avoid concurrent readers
      (*it).second->shutdown();
    }
      }
    }
  }

  for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
    if ((*it).second->isServer() && !((*it).second->isApplication())) { // MOS - servers only
      HAGGLE_DBG("Not ready for shutdown - Server protocol %s still registered.\n", (*it).second->getName());
      ready = false;
    }
  }

  return ready;
}


// MOS - shut down application protocols and check if done

bool ProtocolManager::shutdownApplicationProtocols()
{
  bool ready = true;

  HAGGLE_DBG("Shutting down application protocols.\n");
  for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
    if ((*it).second->isApplication()) {
    if(!(*it).second->isDone() && !(*it).second->isGarbage()) {
      (*it).second->shutdown();
    }
    }
  }

  for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
    if ((*it).second->isApplication()) {
      HAGGLE_DBG("Application protocol %s still registered.\n", (*it).second->getName());
      ready = false;
    }
  }

  return ready;
}

void ProtocolManager::onDeleteProtocolEvent(Event *e) {
    Protocol *p;

    if (!e)
        return;

    p = static_cast<Protocol *>(e->getData());

    if (!p)
        return;
    
    if (protocol_registry.find(p->getId()) == protocol_registry.end()) {
        // CBMEN, HL - This seems to segfault otherwise
        if (getState() <= MANAGER_STATE_RUNNING) {
            HAGGLE_ERR(
                    "Trying to unregister protocol %s, which is not registered\n",
                    p->getName());
        }
    } else {
        // MOS - still need to delete (but not at shutdown)
        protocol_registry.erase(p->getId());
    }


    HAGGLE_DBG("Removing protocol %s\n", p->getName());

    /*
     Either we join the thread here, or we detach it when we start it.
     */
    if (!p->isDetached()) {
        HAGGLE_DBG("Waiting for protocol %s to terminate\n", p->getName());
        p->join();
        HAGGLE_DBG("Joined with protocol %s\n", p->getName());
        HAGGLE_DBG("Deleting protocol %s\n", p->getName());
        delete p;
    }

    // MOS - deferred signal to shutdown
    if(getState() == MANAGER_STATE_PREPARE_SHUTDOWN) {
          bool ready = shutdownNonApplicationProtocols();
      if(ready) signalIsReadyForShutdown();
    }
    if(getState() == MANAGER_STATE_SHUTDOWN) {
          bool ready = shutdownApplicationProtocols();
      if(ready) unregisterWithKernel();
    }

/* MOS - we unregister with kernel above if we cannot shut down all protocols

    if (getState() == MANAGER_STATE_SHUTDOWN) {
        if (protocol_registry.empty()) {
            unregisterWithKernel();
        }
#if defined(DEBUG)
        else {
            for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
                Protocol *p = (*it).second;
                HAGGLE_DBG("Protocol \'%s\' still registered\n", p->getName());
            }

        }
#endif
    }
*/
}

// MOS - protocols did not shut down in a timely manner

void ProtocolManager::onProtocolPrepareShutdownTimeout(Event *e) {
  if(getState() == MANAGER_STATE_PREPARE_SHUTDOWN) {
    HAGGLE_ERR("Initiating shutdown after timeout during preparation\n");
    signalIsReadyForShutdown();
  }
}

void ProtocolManager::onProtocolShutdownTimeout(Event *e) {
    HAGGLE_DBG("Checking for still registered protocols: num registered=%lu\n",
            protocol_registry.size());

    if (!protocol_registry.empty()) {
      while (!protocol_registry.empty()) {
            Protocol *p = NULL;

        // MOS - look for potentially stuck protocols
        for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
          if (!(*it).second->isApplication()) p = (*it).second;
        }

        if(!p) break; // MOS - app protocols need to shut down normally

            protocol_registry.erase(p->getId());

            // We are not going to join with these threads, so we detach.
            if (p->isRunning() ||
        (p->isCancelled() && !p->isGarbage())) { // MOS - need to cover threads stuck in connect

            HAGGLE_DBG("Protocol \'%s\' still registered after shutdown. Detaching it!\n", p->getName());

                // In detached state, the protocol will delete itself
                // once it stops running.
                p->detach();
                p->cancel();

                // We have to clear the protocol's send queue as it may contain
                // data objects whose send verdict managersa are
                // waiting for. Clearing the queue will evict all data objects
                // from its queue with a FAILURE verdict.

        p->closeQueue(); // MOS - no closeAndClearQueue to avoid concurrent readers
            }
            else {
            HAGGLE_DBG("Protocol \'%s\' still registered after shutdown. Deleting it!\n", p->getName());
            delete p;
            }
        }
        // unregisterWithKernel(); // MOS - happens when all application protocols terminate
    }
}

/* Close all server protocols so that we cannot create new clients. */
void ProtocolManager::onPrepareShutdown()
{
    HAGGLE_DBG("%lu protocols are registered, shutting down clients first.\n",
            protocol_registry.size());

    bool ready = shutdownNonApplicationProtocols();
    if(ready) signalIsReadyForShutdown(); // MOS - deferred if not ready
    else if(forcedShutdownAfterTimeout) // MOS - for testing this should be off to catch all anomalies
      kernel->addEvent(new Event(protocol_prepare_shutdown_timeout_event, (void*)NULL, forcedShutdownAfterTimeout));
}

/*
 Do not stop protocols until we are in shutdown, because other managers
 may rely on protocol services while they are preparing for shutdown.
 */
void ProtocolManager::onShutdown() {
    HAGGLE_DBG("%lu protocols are registered.\n", protocol_registry.size());

    if (protocol_registry.empty()) {
        unregisterWithKernel();
    }
    else {

        for (protocol_registry_t::iterator it = protocol_registry.begin(); it != protocol_registry.end(); it++) {
      if (!(*it).second->isApplication()) {
        HAGGLE_ERR("Protocol %s still registered at shutdown\n", (*it).second->getName());
      }
        }

    // MOS - we only shut down application protocols
    bool ready = shutdownApplicationProtocols();
    if(ready) unregisterWithKernel(); // MOS - allows Haggle to continue shutdown without waiting for termination of all protocols

    onProtocolShutdownTimeout(NULL); // MOS - detach remaining protocol threads

    /* MOS - no need for protocol killer with the previous line

        // Go through the registered protocols
        protocol_registry_t::iterator it = protocol_registry.begin();

        for (; it != protocol_registry.end(); it++) {
      if(!(*it).second->isDone() && !(*it).second->isGarbage()) { // MOS - only if not shutdown already
            // Tell this protocol we're shutting down!
            (*it).second->shutdown();
      }
        }

        // In case some protocols refuse to shutdown, launch a thread that sleeps for a while
        // before it schedules an event that forcefully kills the protocols.
        // Note that we need to use this thread to implement a timeout because in shutdown
        // the event queue executes as fast as it can, hence delayed events do not work.

        killer = new ProtocolKiller(protocol_shutdown_timeout_event, 15000,
                kernel);

        if (killer) {
            killer->start();
        }

    */

    }
}

#if defined(OS_WINDOWS_XP) && !defined(DEBUG)
// This is here to avoid a warning with catching the exception in the functions
// below.
#pragma warning( push )
#pragma warning( disable: 4101 )
#endif

class LocalByTypeCriteria: public InterfaceStore::Criteria {
    Interface::Type_t type;
public:
    LocalByTypeCriteria(Interface::Type_t _type) :
            type(_type) {
    }
    virtual bool operator()(const InterfaceRecord& ir) const {
        if (ir.iface->getType() == type && ir.iface->isLocal()
                && ir.iface->isUp())
            return true;
        return false;
    }
};

Protocol *ProtocolManager::getCachedSenderProtocol(
    const ProtType_t type,
    const InterfaceRef& peerIface)
{
    protocol_registry_t::iterator it = protocol_registry.begin();
    Protocol *p = NULL;
    for (; it != protocol_registry.end(); it++) {
        p = (*it).second;
        // Is this protocol the one we're interested in?
        if (p->isSender() && type == p->getType() &&
            p->isForInterface(peerIface) &&
            !p->isGarbage() && !p->isDone()) {
            break;
        }
        p = NULL;
    }

    return p;
}

// SW: START PROTOCOL CLASSIFICATION:
Protocol *ProtocolManager::getClassifiedSenderProtocol(
    const InterfaceRef& peerIface,
    DataObjectRef& dObj)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, not sending to peers\n");
        return NULL;
    }

    InterfaceRef parent = kernel->getInterfaceStore()->retrieveParent(peerIface);

    if (!parent) {
        return NULL;
    }

    Protocol *p = getProtocolFactory()->getSenderProtocol(parent, peerIface, dObj);

    if (NULL == p) {
        HAGGLE_ERR("Could not get sender protocol\n");
        return NULL;
    }

    return p;
}
// SW: END PROTOCOL CLASSIFICATION.

Protocol *ProtocolManager::getSenderProtocol(
    const ProtType_t type,
    const InterfaceRef& peerIface)
{
    Protocol *p = getCachedSenderProtocol(type, peerIface);
    if (NULL != p) {
        return p;
    }

    InterfaceRef parent = kernel->getInterfaceStore()->retrieveParent(peerIface);

    if (!parent) {
        return NULL;
    }

// SW: NOTE: This is the legacy code path that does not use classification

#if defined(ENABLE_BLUETOOTH)
    if (Protocol::TYPE_RFCOMM == type) {
        p = new ProtocolRFCOMMSender(parent, peerIface, RFCOMM_DEFAULT_CHANNEL, this);
    }
#endif

#if defined(ENABLE_MEDIA)
    if (Protocol::TYPE_MEDIA == type) {
        p = new ProtocolMedia(NULL, peerIface, this);
    }
#endif

    if (NULL == p) {
        HAGGLE_ERR("Could not get sender protocol\n");
        return NULL;
    }

    if (!p->init() || !registerProtocol(p)) {
        HAGGLE_ERR("Could not initialize protocol %s\n", p->getName());
        delete p;
        return NULL;
    }
    return p;
}

Protocol *ProtocolManager::getCachedServerProtocol(
    const ProtType_t type,
    const InterfaceRef& iface)
{
    protocol_registry_t::iterator it = protocol_registry.begin();

    Protocol *p = NULL;
    for (; it != protocol_registry.end(); it++) {
        p = (*it).second;
        // Assume that we are only looking for a specific type of server
        // protocol, i.e., there is only one server for multiple interfaces
        // of the same type
        if (p->isServer() && type == p->getType()) {
            break;
        }
        p = NULL;
    }
    return p;
}

Protocol *ProtocolManager::getServerProtocol(
    const ProtType_t type,
    const InterfaceRef& iface)
{
    Protocol *p = getCachedServerProtocol(type, iface);

    if (NULL != p) {
        return p;
    }

#if defined(ENABLE_BLUETOOTH)
    if (Protocol::TYPE_RFCOMM == type) {
        p = new ProtocolRFCOMMServer(iface, this);
    }
#endif

    if (NULL == p) {
        HAGGLE_ERR("Could not get server protocol\n");
        return NULL;
    }

    if (!p->init() || !registerProtocol(p)) {
        HAGGLE_ERR("Could not initialize or register protocol %s\n", p->getName());
        delete p;
        return NULL;
    }

    return p;
}

#if defined(OS_WINDOWS_XP) && !defined(DEBUG)
#pragma warning( pop )
#endif

void ProtocolManager::onWatchableEvent(const Watchable& wbl) {
    protocol_registry_t::iterator it = protocol_registry.begin();

    HAGGLE_DBG2("Receive on %s\n", wbl.getStr());

    // Go through each protocol in turn:
    for (; it != protocol_registry.end(); it++) {
        Protocol *p = (*it).second;

        // Did the Watchable belong to this protocol
        if (p->hasWatchable(wbl)) {
            // Let the protocol handle whatever happened.
            p->handleWatchableEvent(wbl);
            return;
        }
    }

    HAGGLE_DBG("Was asked to handle a socket no protocol knows about!\n");
    // Should not happen, but needs to be dealt with because if it isn't,
    // the kernel will call us again in an endless loop!

    kernel->unregisterWatchable(wbl);

    CLOSE_SOCKET(wbl.getSocket());
}

void ProtocolManager::onLocalInterfaceUp(Event *e) {
    if (!e)
        return;

    InterfaceRef iface = e->getInterface();

    if (!iface)
        return;

    const Addresses *adds = iface->getAddresses();

    for (Addresses::const_iterator it = adds->begin(); it != adds->end();
            it++) {
        switch ((*it)->getType()) {
        case Address::TYPE_IPV4:
#if defined(ENABLE_IPv6)
            case Address::TYPE_IPV6:
#endif
            getProtocolFactory()->instantiateServers(iface);
            return;
#if defined(ENABLE_BLUETOOTH)
            case Address::TYPE_BLUETOOTH:
            getServerProtocol(Protocol::TYPE_RFCOMM, iface);
            return;
#endif
#if defined(ENABLE_MEDIA)
            // FIXME: should probably separate loop interfaces from media interfaces somehow...
            case Address::TYPE_FILEPATH:
            getServerProtocol(Protocol::TYPE_MEDIA, iface);
            return;
#endif

        default:
            break;
        }
    }

    HAGGLE_DBG("Interface with no known address type - no server started\n");
}

void ProtocolManager::onLocalInterfaceDown(Event *e) {
    InterfaceRef& iface = e->getInterface();

    if (!iface)
        return;

    HAGGLE_DBG(
            "Local interface [%s] went down, checking for associated protocols\n",
            iface->getIdentifierStr());

    // Go through the protocol list
    protocol_registry_t::iterator it = protocol_registry.begin();

    for (; it != protocol_registry.end(); it++) {
        Protocol *p = (*it).second;

        /*
         Never bring down our application IPC protocol when
         application interfaces go down (i.e., applications deregister).
         */
        if (p->getLocalInterface()->getType()
                == Interface::TYPE_APPLICATION_PORT) {
            continue;
        }
        // Is the associated with this protocol?
        if (p->isForInterface(iface)) {
            /*
             NOTE: I am unsure about how this should be done. Either:

             p->handleInterfaceDown();

             or:

             protocol_list_mutex->unlock();
             p->handleInterfaceDown();
             return;

             or:

             protocol_list_mutex->unlock();
             p->handleInterfaceDown();
             protocol_list_mutex->lock();
             (*it) = protocol_registry.begin();

             The first has the benefit that it doesn't assume that there is
             only one protocol per interface, but causes the deletion of the
             interface to happen some time in the future (as part of event
             queue processing), meaning the protocol will still be around,
             and may be given instructions before being deleted, even when
             it is incapable of handling instructions, because it should
             have been deleted.

             The second has the benefit that if the protocol tries to have
             itself deleted, it happens immediately (because there is no
             deadlock), but also assumes that there is only one protocol per
             interface, which may or may not be true.

             The third is a mix, and has the pros of both the first and the
             second, but has the drawback that it repeatedly locks and
             unlocks the mutex, and also needs additional handling so it
             won't go into an infinite loop (a set of protocols that have
             already handled the interface going down, for instance).

             For now, I've chosen the first solution.
             */
            // Tell the protocol to handle this:
            HAGGLE_DBG(
                    "Shutting down protocol %s because local interface [%s] went down\n",
                    p->getName(), Interface::idString(iface).c_str());
            p->handleInterfaceDown(iface);
        }
    }
}

void ProtocolManager::onNeighborInterfaceDown(Event *e) {
    InterfaceRef& iface = e->getInterface();

    if (!iface)
        return;

    HAGGLE_DBG(
            "Neighbor interface [%s] went away, checking for associated protocols\n",
            iface->getIdentifierStr());

    // Go through the protocol list
    protocol_registry_t::iterator it = protocol_registry.begin();

    for (; it != protocol_registry.end(); it++) {
        Protocol *p = (*it).second;

        /*
         Never bring down our application IPC protocol when
         application interfaces go down (i.e., applications deregister).
         */
        if (p->getLocalInterface()->getType()
                == Interface::TYPE_APPLICATION_PORT) {
            continue;
        }
        // Is the associated with this protocol?
        if (p->isClient() && p->isForInterface(iface)) {
            HAGGLE_DBG(
                    "Shutting down protocol %s because neighbor interface [%s] went away\n",
            p->getName(), Interface::idString(iface).c_str());
            p->handleInterfaceDown(iface);
        }
    }
}

void ProtocolManager::onSendDataObject(Event *e)
{
    /*
        Since other managers might want to modify the data object before it is
        actually sent, we delay the actual send processing by sending a private
        event to ourselves, effectively rescheduling our own processing of this
        event to occur just after this event.
    */

    // MOS
    if (getState() > MANAGER_STATE_RUNNING && !e->getDataObject()->isControlMessage()) {
        HAGGLE_DBG("In shutdown, only sending control messages, not sending to peers\n");
        return;
    }

#ifdef DEBUG
  if(Trace::trace.getTraceType() <= TRACE_TYPE_DEBUG1) {
    e->getDataObject()->print(NULL, true); // MOS - NULL means print to debug trace
  }
#endif

    // CBMEN, AG, Begin

    // Send all control messages.
  if (e->getDataObject()->isControlMessage()) {
        HAGGLE_DBG("Sending control message %s\n", e->getDataObject()->getIdStr());
        kernel->addEvent(new Event(send_data_object_actual_event, e->getDataObject(), e->getNodeList()));
        return;
    }

  if (e->getDataObject()->getIsForMonitorApp()) { // MOS
        HAGGLE_DBG("Sending data object %s to monitoring application\n", e->getDataObject()->getIdStr());
        kernel->addEvent(new Event(send_data_object_actual_event, e->getDataObject(), e->getNodeList()));
        return;
    }

    // CBMEN, AG, End

    // CBMEN, HL, Begin
    // Send only encrypted objects if the security level is high enough.
    // We have to check for the presence of the attribute as the Security Manager
    // hasn't had a chance to set the ABE status yet
    if (kernel->getManager((char *)"SecurityManager") &&
        ((SecurityManager *)(kernel->getManager((char *)"SecurityManager")))->getEncryptFilePayload() &&
        e->getDataObject()->getAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY) &&
        !e->getDataObject()->getIsForLocalApp() &&
        !e->getDataObject()->getAttribute(DATAOBJECT_ATTRIBUTE_ABE)) {

        HAGGLE_DBG("Not sending unencrypted data object %s\n", e->getDataObject()->getIdStr());
        return;
    }

    if (kernel->getManager((char*)"SecurityManager") &&
        ((SecurityManager *)(kernel->getManager((char *)"SecurityManager")))->getEncryptFilePayload() &&
        e->getDataObject()->getIsForLocalApp() &&
        (e->getDataObject()->getABEStatus() == DataObject::ABE_DECRYPTION_NEEDED || e->getDataObject()->getABEStatus() == DataObject::ABE_ENCRYPTION_DONE)) {

        HAGGLE_DBG("Not sending data object %s as it needs decryption.\n", e->getDataObject()->getIdStr());
        return;
    }

    // CBMEN, HL, End

    // CBMEN, AG, Begin
    // Only send signed data objects.
    if (e->getDataObject()->isSigned()) {
      DataObjectRef dObj = e->getDataObject();
      String idStr = kernel->getThisNode()->getIdStr();
      if(dObj->getSignature() == NULL || dObj->getIsForLocalApp() || !dObj->getMetadata()->getMetadata("SignatureChain") || dObj->getSignee() == idStr) {
        HAGGLE_DBG("Sending checked/signed data object %s\n", dObj->getIdStr());
        //kernel->addEvent(new Event(send_data_object_actual_event, e->getDataObject(), e->getNodeList())); CBMEN, HL - we're doing this later
      } else {
        HAGGLE_DBG("Not sending unchecked/unsigned data object %s (not signed my myself)\n", dObj->getIdStr());
        return; // CBMEN, HL - we don't want to send this
      }
    } else {
        HAGGLE_DBG("Not sending unchecked/unsigned data object %s\n", e->getDataObject()->getIdStr());
        return; // CBMEN, HL - we don't want to send this
    }

    // CBMEN, AG, End

    HAGGLE_DBG("protocol manager event rescheduling for sending data object %s\n",
            e->getDataObject()->getIdStr());
    //Event* newEvent = new Event(send_data_object_actual_event, e->getDataObject(), e->getNodeList());

    Event* newEvent = NULL;

    DataObjectRef dataObjectRef = e->getDataObject();
    NodeRefList nodeRefList = e->getNodeList();

    for (NodeRefList::iterator it = nodeRefList.begin(); it != nodeRefList.end(); it++) {
        NodeRef node = kernel->getNodeStore()->retrieve(*it);

    if(!node) {
      kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dataObjectRef, *it));
      continue;
    }

    // CBMEN, HL, Begin
    if (dataObjectRef->getIsForLocalApp() &&
        dataObjectRef->getAttribute(DATAOBJECT_ATTRIBUTE_ABE)) {
        HAGGLE_DBG("Not sending data object %s with ABE attribute to local application.\n", dataObjectRef->getIdStr());
        continue;
    }
    // CBMEN, HL, End

	if(node->hasThisOrParentDataObject(dataObjectRef)) {
	  kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dataObjectRef, *it));
	  continue;
	}

	newEvent = this->fragmentationProtocolHelper->onSendDataObject(dataObjectRef,node,send_data_object_actual_event);
	if(newEvent) {
	  HAGGLE_DBG("fragmentation send event translation for data object %s\n", newEvent->getDataObject()->getIdStr());
	  kernel->addEvent(newEvent);
	  continue;
	}

	newEvent = this->networkCodingProtocolHelper->onSendDataObject(dataObjectRef,node,send_data_object_actual_event);
	if(newEvent) {
	  HAGGLE_DBG("networkcoding send event translation for data object %s\n", newEvent->getDataObject()->getIdStr());
	  kernel->addEvent(newEvent);
	  continue;
	}

	//NodeRefList nodeRefSingleton;
	//nodeRefSingleton.push_back(node);
	//newEvent = new Event(send_data_object_actual_event, e->getDataObject(), nodeRefSingleton);
	//kernel->addEvent(newEvent);
// SW: START: SendPriorityManager
        kernel->addEvent(SendPriorityManager::getSendPriorityEvent(e->getDataObject(), node, send_data_object_actual_event));
// SW: END: SendPriorityManager

    }
}

void ProtocolManager::onSendDataObjectActual(Event *e) {
    //int numTx = 0;

    if (!e || !e->hasData()) {
        if (!e) {
            HAGGLE_DBG("Event is null\n");
        }
        else if (!e->hasData()) {
            HAGGLE_DBG("Data object missing in event\n");
        }
        return;
    }

    // Get a copy to work with
    DataObjectRef dObj = e->getDataObject();
    HAGGLE_DBG("Really send data object %s\n", dObj->getIdStr());

    // Get target list:
    NodeRefList *targets = (e->getNodeList()).copy();

    if (!targets) {
        HAGGLE_ERR("No targets in data object when sending\n");
        return;
    }

    unsigned int numTargets = targets->size();

    // Go through all targets:
    while (!targets->empty()) {

        // A current target reference
        NodeRef targ = targets->pop();

        if (!targ) {
            HAGGLE_ERR("Target num %u is NULL!\n", numTargets);
            numTargets--;
            continue;
        }

    HAGGLE_DBG("Sending data object %s to target %s \n",
           DataObject::idString(dObj).c_str(), targ->getName().c_str());

        // If we are going to loop through the node's interfaces, we need to lock the node.
        targ.lock();

        const InterfaceRefList *interfaces = targ->getInterfaces();

        // Are there any interfaces here?
        if (interfaces == NULL || interfaces->size() == 0) {
            // No interfaces for target, so we generate a
            // send failure event and skip the target

            HAGGLE_DBG("Target %s has no interfaces\n",
                    targ->getName().c_str());

            targ.unlock();

            kernel->addEvent(
                    new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, targ));
            numTargets--;
            continue;
        }

        /*
         Find the target interface that suits us best
         (we assume that for any remote target
         interface we have a corresponding local interface).
         */
        InterfaceRef peerIface = NULL;
        bool done = false;

        InterfaceRefList::const_iterator it = interfaces->begin();

        //HAGGLE_DBG("Target node %s has %lu interfaces\n", targ->getName().c_str(), interfaces->size());

        for (; it != interfaces->end() && done == false; it++) {
            InterfaceRef iface = *it;

            // If this interface is up:
            if (iface->isUp()) {

                if (iface->getAddresses()->empty()) {
                    HAGGLE_DBG("Interface %s:%s has no addresses - IGNORING.\n",
                            iface->getTypeStr(), iface->getIdentifierStr());
                    continue;
                }

                switch (iface->getType()) {
#if defined(ENABLE_BLUETOOTH)
                case Interface::TYPE_BLUETOOTH:
                /*
                 Select Bluetooth only if there are no Ethernet or WiFi
                 interfaces.
                 */
                if (!iface->getAddress<BluetoothAddress>()) {
                    HAGGLE_DBG("Interface %s:%s has no Bluetooth address - IGNORING.\n",
                            iface->getTypeStr(), iface->getIdentifierStr());
                    break;
                }

                if (!peerIface)
                peerIface = iface;
                else if (peerIface->getType() != Interface::TYPE_ETHERNET &&
                        peerIface->getType() != Interface::TYPE_WIFI)
                peerIface = iface;
                break;
#endif
#if defined(ENABLE_ETHERNET)
                case Interface::TYPE_ETHERNET:
                    /*
                     Let Ethernet take priority over the other types.
                     */
                    if (!iface->getAddress<IPv4Address>()
#if defined(ENABLE_IPv6)
                    && !iface->getAddress<IPv6Address>()
#endif
                    ) {
                        HAGGLE_DBG(
                                "Interface %s:%s has no IPv4 or IPv6 addresses - IGNORING.\n",
                                iface->getTypeStr(), iface->getIdentifierStr());
                        break;
                    }
                    if (!peerIface)
                        peerIface = iface;
                    else if (peerIface->getType() == Interface::TYPE_BLUETOOTH
                            || peerIface->getType() == Interface::TYPE_WIFI)
                        peerIface = iface;
                    break;
                case Interface::TYPE_WIFI:
                    if (!iface->getAddress<IPv4Address>()
#if defined(ENABLE_IPv6)
                    && !iface->getAddress<IPv6Address>()
#endif
                    ) {
                        HAGGLE_DBG(
                                "Interface %s:%s has no IPv4 or IPv6 addresses - IGNORING.\n",
                                iface->getTypeStr(), iface->getIdentifierStr());
                        break;
                    }
                    if (!peerIface)
                        peerIface = iface;
                    else if (peerIface->getType() == Interface::TYPE_BLUETOOTH
                            && peerIface->getType() != Interface::TYPE_ETHERNET)
                        peerIface = iface;
                    break;
#endif // ENABLE_ETHERNET
                case Interface::TYPE_APPLICATION_PORT:
                case Interface::TYPE_APPLICATION_LOCAL:

                    if (!iface->getAddress<IPv4Address>()
#if defined(ENABLE_IPv6)
                    && !iface->getAddress<IPv6Address>()
#endif
                    ) {
                        HAGGLE_DBG(
                                "Interface %s:%s has no IPv4 or IPv6 addresses - IGNORING.\n",
                                iface->getTypeStr(), iface->getIdentifierStr());
                        break;
                    }
                    // Not much choise here.
                    if (targ->getType() == Node::TYPE_APPLICATION) {
                        peerIface = iface;
                        done = true;
                    }
                    else {
                        HAGGLE_DBG(
                                "Node %s is not application, but its interface is\n",
                                targ->getName().c_str());
                    }

                    break;
#if defined(ENABLE_MEDIA)
                    case Interface::TYPE_MEDIA:
                    break;
#endif
                case Interface::TYPE_UNDEFINED:
                default:
                    break;
                }
            }
            else {
                //HAGGLE_DBG("Send interface %s was down, ignoring...\n", iface->getIdentifierStr());
            }
        }

        // We are done looking for a suitable send interface
        // among the node's interface list, so now we unlock
        // the node.
        targ.unlock();

        if (!peerIface) {
            HAGGLE_DBG(
                    "No send interface found for target %s. Aborting send of data object!!!\n",
                    targ->getName().c_str());
            // Failed to send to this target, send failure event:
            kernel->addEvent(
                    new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, targ));
            numTargets--;
            continue;
        }

        // Ok, we now have a target and a suitable interface,
        // now we must figure out a protocol to use when we
        // transmit to that interface
        Protocol *p = NULL;

        // We make a copy of the addresses list here so that we do not
        // have to lock the peer interface while we call getSenderProtocol().
        // getSenderProtocol() might do a lookup in the interface store in order
        // to find the local interface which is parent of the peer interface.
        // This might cause a deadlock in case another thread also does a lookup
        // in the interface store while we hold the interface lock.
        const Addresses *adds = peerIface->getAddresses()->copy();

        // Figure out a suitable protocol given the addresses associated
        // with the selected interface
        for (Addresses::const_iterator it = adds->begin();
                p == NULL && it != adds->end(); it++) {

            switch ((*it)->getType()) {
#if defined(ENABLE_BLUETOOTH)
            case Address::TYPE_BLUETOOTH:
            p = getSenderProtocol(Protocol::TYPE_RFCOMM, peerIface);
            break;
#endif
            case Address::TYPE_IPV4:
#if defined(ENABLE_IPv6)
                case Address::TYPE_IPV6:
#endif
                if (peerIface->isApplication()) {
#ifdef USE_UNIX_APPLICATION_SOCKET
                    p = getSenderProtocol(Protocol::TYPE_LOCAL, peerIface);
#else
                    p = getSenderProtocol(Protocol::TYPE_UDP, peerIface);
#endif
                }
                else {
// SW: START PROTOCOL CLASSIFICATION:
                    p = getClassifiedSenderProtocol(peerIface, dObj);
// SW: END PROTOCOL CLASSIFICATION.
                }
                break;
#if defined(ENABLE_MEDIA)
                case Address::TYPE_FILEPATH:
                p = getSenderProtocol(Protocol::TYPE_MEDIA, peerIface);
                break;
#endif
            default:
                break;
            }
        }

        delete adds;

        // Send data object to the found protocol:

        if (p) {
            if (!p->sendDataObject(dObj, targ, peerIface)) {
                kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, targ));
            }
        }
        else {
            HAGGLE_DBG("No suitable protocol found for interface %s:%s!\n",
                    peerIface->getTypeStr(), peerIface->getIdentifierStr());
            // Failed to send to this target, send failure event:
            kernel->addEvent(
                    new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, targ));
        }

        numTargets--;
    }

    /* HAGGLE_DBG("Scheduled %d data objects\n", numTx); */

    delete targets;
}

// SW: START PROTOCOL FACTORY INSTANTIATION:
void ProtocolManager::onConfig(Metadata *m)
{
        const char*param = m->getParameter("forced_shutdown_after_timeout");

    if (param) {
        char *endptr = NULL;
        unsigned long value = strtoul(param, &endptr, 10);

        if (endptr && endptr != param) {
                forcedShutdownAfterTimeout = value;
            HAGGLE_DBG("forced_shutdown_after_timeout=%lu\n",
                   forcedShutdownAfterTimeout);
            LOG_ADD("# %s: forced_shutdown_after_timeout=%lu\n",
                getName(), forcedShutdownAfterTimeout);
        }
    }

    getProtocolFactory()->initialize(*m);
}


ProtocolFactory *ProtocolManager::getProtocolFactory()
{
    if (NULL == protocolFactory) {
        protocolFactory = new ProtocolFactory(this);
    }

    return protocolFactory;
}
// SW: END PROTOCOL FACTORY INSTANTIATION.

