/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
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
#include "ForwardingManager.h"

// SW: START FORWARDER FACTORY:
#include "ForwarderFactory.h"
// SW: END FORWARDER FACTORY.
#include "ForwarderAsynchronous.h"
#include "ForwarderProphet.h"
#include "ForwarderAlphaDirect.h"
#include "XMLMetadata.h"

// #define ENABLE_FORWARDING_METADATA 1 // MOS - now a config parameter

// SW: START: punts to avoid init and onConfig race condition
#define PUNT_DELAY_INIT_S 1
#define PUNT_DELAY_CONFIG_S 1
// SW: END: punts to avoid init and onConfig race condition

#define DEFAULT_DATAOBJECT_RETRIES (1) // MOS
#define DEFAULT_DATAOBJECT_RETRIES_SHORTCIRCUIT (1) // MOS
#define DEFAULT_NODE_DESCRIPTION_RETRIES (1) // MOS

ForwardingManager::ForwardingManager(HaggleKernel * _kernel) :
	Manager("ForwardingManager", _kernel), 
	dataObjectQueryCallback(NULL), 
	delayedDataObjectQueryCallback(NULL), // SW
	nodeQueryCallback(NULL), 
	forwardDobjCallback(NULL), // SW
	repositoryCallback(NULL),
	periodicDataObjectQueryCallback(NULL), // SW
	periodicRoutingUpdateCallback(NULL), // SW
	moduleEventType(-1),
	routingInfoEventType(-1),
	periodicDataObjectQueryEventType(-1),
	periodicRoutingUpdateEventType(-1), // MOS
	periodicDataObjectQueryEvent(NULL),
	periodicRoutingUpdateEvent(NULL), // MOS
	periodicDataObjectQueryInterval(300),
	periodicRoutingUpdateInterval(300), // MOS
	maxNodesToFindForNewDataObjects(MAX_NODES_TO_FIND_FOR_NEW_DATAOBJECTS), // MOS
	dataObjectRetries(DEFAULT_DATAOBJECT_RETRIES),
	nodeDescriptionRetries(DEFAULT_NODE_DESCRIPTION_RETRIES),
	forwardingModule(NULL),
	enableForwardingMetadata(false), // MOS
	recursiveRoutingUpdates(false),
	doQueryOnNewDataObject(true),
// SW: START NEW NBR NODE DESCRIPTION PUSH:
    doPushNodeDescriptions(false),
// SW: END NEW NBR NODE DESCRIPTION PUSH.
// MOS: START ENABLE GENERATE TARGETS:
    enableTargetGeneration(true),
// MOS: END ENABLE GENERATE TARGETS
// MOS: START RANDOMIZED FORWARDING DELAY
    maxNodeDescForwardingDelayLinearMs(0),
    maxNodeDescForwardingDelayBaseMs(20), // MOS - using small delay of 20ms to enable randomization
    maxForwardingDelayLinearMs(0),
    maxForwardingDelayBaseMs(20), // MOS - using small delay of 20ms to enable randomization
// MOS: END RANDOMIZED FORWARDING DELAY
    neighborForwardingShortcut(true),
    loadReductionMinQueueSize(ULONG_MAX),
    loadReductionMaxQueueSize(ULONG_MAX),
    noForwarder(false),  // SW: properly handle case where forwarder disabled
    enableMultihopBloomfilters(true)
{
    this->networkCodingConfiguration = new NetworkCodingConfiguration();
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
    this->fragmentationConfiguration = new FragmentationConfiguration();
    this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
    this->forwardingManagerHelper = new ForwardingManagerHelper(this->getKernel());
    this->fragmentationForwardingManagerHelper = new FragmentationForwardingManagerHelper(this->getKernel());
}

bool ForwardingManager::init_derived()
{
	int ret;
#define __CLASS__ ForwardingManager

	ret = setEventHandler(EVENT_TYPE_NODE_UPDATED, onNodeUpdated);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_NEW, onNewDataObject);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_FORWARD, onDataObjectForward);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onSendDataObjectResult);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, onSendDataObjectResult);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_NEW, onNewNeighbor);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_END, onEndNeighbor);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_TARGET_NODES, onTargetNodes);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DELEGATE_NODES, onDelegateNodes);
	
	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_DELETED, onDeletedDataObject); // MOS

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL, onSendDataObjectResult);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL, onSendDataObjectResult);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL\n");
        return false;
    }

	ret = setEventHandler(EVENT_TYPE_DEBUG_CMD, onDebugCmd);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler EVENT_TYPE_DEBUG_CMD\n");
        return false;
    }

    moduleEventType = registerEventType(getName(), onForwardingTaskComplete);

    if (moduleEventType < 0) {
        HAGGLE_ERR("Could not register module event type...");
        return false;
    }
    // Register filter for forwarding objects
    registerEventTypeForFilter(routingInfoEventType, "Forwarding",
            onRoutingInformation, "Forwarding=*");

    dataObjectQueryCallback = newEventCallback(onDataObjectQueryResult);
    delayedDataObjectQueryCallback = newEventCallback(onDelayedDataObjectQuery);
    nodeQueryCallback = newEventCallback(onNodeQueryResult);
    repositoryCallback = newEventCallback(onRepositoryData);

    periodicDataObjectQueryCallback = newEventCallback(onPeriodicDataObjectQuery); // MOS
    periodicRoutingUpdateCallback = newEventCallback(onPeriodicRoutingUpdate); // MOS

    periodicDataObjectQueryEventType =
            registerEventType("Periodic DataObject Query Event",
                    onPeriodicDataObjectQuery);

	periodicDataObjectQueryEvent = new Event(periodicDataObjectQueryEventType);

	if (!periodicDataObjectQueryEvent)
		return false;

	periodicDataObjectQueryEvent->setAutoDelete(false);

    periodicRoutingUpdateEventType = registerEventType("Periodic Routing Update Event", 
						       onPeriodicRoutingUpdate);

    if (periodicRoutingUpdateEventType < 0) {
      HAGGLE_ERR("Could not register periodic routing update event type...\n");
      return false;
    }

    periodicRoutingUpdateEvent = new Event(periodicRoutingUpdateEventType);

    if (!periodicRoutingUpdateEvent)
      return false;

    periodicRoutingUpdateEvent->setAutoDelete(false);

    // SW: START: RACE CONDITION DURING INIT PUNTS
	on_punt_init_event = registerEventType("ForwardingManager Initialization Punt Event", onInitPunt);
	on_config_punt_event = registerEventType("ForwardingManager onConfig Punt Event", onConfigPunt);
    // SW: END: RACE CONDITION DURING INIT PUNTS

    return true;
}

ForwardingManager::~ForwardingManager()
{
    if(this->networkCodingConfiguration) {
      delete this->networkCodingConfiguration;
      this->networkCodingConfiguration = NULL;
    }
    
    if (this->networkCodingDataObjectUtility) {
      delete this->networkCodingDataObjectUtility;
      this->networkCodingDataObjectUtility = NULL;
    }

    if(this->fragmentationConfiguration) {
      delete this->fragmentationConfiguration;
      this->fragmentationConfiguration = NULL;
    }
    
    if (this->fragmentationDataObjectUtility) {
      delete this->fragmentationDataObjectUtility;
      this->fragmentationDataObjectUtility = NULL;
    }

    if(this->forwardingManagerHelper) {
        delete this->forwardingManagerHelper;
        this->forwardingManagerHelper = NULL;
    }
    if(this->fragmentationForwardingManagerHelper) {
    	delete this->fragmentationForwardingManagerHelper;
    	this->fragmentationForwardingManagerHelper = NULL;
    }

    // Destroy the forwarding module:
    if (forwardingModule) {
        HAGGLE_ERR("Forwarding module detected in forwarding manager "
        "destructor. This shouldn't happen.\n");
        forwardingModule->quit();
        delete forwardingModule;
    }

    if (dataObjectQueryCallback)
        delete dataObjectQueryCallback;

    // Empty the list of forwarded data objects.
    while (!forwardedObjects.empty()) {
      // SW: NOTE: this is a normal condition, occurs when we are shutting down
      // and DOs are still in queues.
        HAGGLE_DBG("Clearing unsent data object.\n");
        forwardedObjects.pop_front();
    }
    if (nodeQueryCallback)
        delete nodeQueryCallback;

    if (repositoryCallback)
        delete repositoryCallback;

    // SW: memleak 
    if (periodicDataObjectQueryCallback)
        delete periodicDataObjectQueryCallback;

    if (periodicRoutingUpdateCallback) 
        delete periodicRoutingUpdateCallback;
    // SW: end memleak

    if (delayedDataObjectQueryCallback)
        delete delayedDataObjectQueryCallback;

    Event::unregisterType(moduleEventType);

    if (periodicDataObjectQueryEvent) {
        if (periodicDataObjectQueryEvent->isScheduled())
            periodicDataObjectQueryEvent->setAutoDelete(true);
        else
            delete periodicDataObjectQueryEvent;
    }

    Event::unregisterType(periodicDataObjectQueryEventType);

    if (periodicRoutingUpdateEvent) {
        if (periodicRoutingUpdateEvent->isScheduled()) {
            periodicRoutingUpdateEvent->setAutoDelete(true);
        }
        else {
            delete periodicRoutingUpdateEvent;
        }
    }

    Event::unregisterType(periodicRoutingUpdateEventType);

// SW: START: RACE CONDITION DURING INIT PUNTS
    Event::unregisterType(on_punt_init_event);
    Event::unregisterType(on_config_punt_event);
// SW: END: RACE CONDITION DURING INIT PUNTS
}

void ForwardingManager::onPrepareStartup() {
    // SW: wait until startup event has fired before loading singleton
    //     and reading from DB.
  /* MOS - removed default forwarder to avoid race condition with valgrind
    setForwardingModule(ForwarderFactory::getSingletonForwarder(this,
            moduleEventType));
  */
}

void ForwardingManager::onPrepareShutdown()
{	
	unregisterEventTypeForFilter(routingInfoEventType);

	// Save the forwarding module's state
	if (forwardingModule) {
		if (forwardingModule->isRunning())
			// Delay signaling we are ready for shutdown until the running
			// module tells us it is done
			setForwardingModule(NULL);
		else {
			// The forwarding module exists, but is not running,
			// delete it and signal that we are ready
		        // delete forwardingModule; // MOS - avoid deletion because callbacks might be still queued up
			signalIsReadyForShutdown();
		}
	} else {
		signalIsReadyForShutdown();
	}
}

void ForwardingManager::setForwardingModule(Forwarder *f,
        bool deRegisterEvents) {
    // Is there a current forwarding module?
    if (forwardingModule) {
        if (f && strcmp(forwardingModule->getName(), f->getName()) == 0) {
            HAGGLE_DBG("Forwarder %s already set!\n", f->getName());
            delete f;
            f = NULL;
            return;
        }
        // SW: NOTE: _VERY_ nasty bug here, we have to start() and then quit() 
        // in order to trigger the proper shutdown mechanisms in asynchronous
        // forwarders.
        forwardingModule->start();
        // Tell the module to quit
        forwardingModule->quit();

        // delete it:
        // delete forwardingModule; // MOS - avoid deletion because callbacks might be still queued up
        forwardingModule = NULL;
    }
    // Change forwarding module:
    forwardingModule = f;

    // Is there a new forwarding module?
    if (forwardingModule) {
        /*
         Yes, request its state from the repository.
         The state will be read in the callback, and the module's thread will be started
         after the state has been saved in the module.
         */
        // SW: load state for newly assigned forwarding module
        forwardingModule->getRepositoryData(repositoryCallback);

        HAGGLE_DBG("Set new forwarding module to \'%s'\n",
                forwardingModule->getName());
        LOG_ADD("# %s: forwarding module is \'%s'\n",
                getName(), forwardingModule->getName());

        /* Register callbacks */
        if (!getEventInterest(EVENT_TYPE_DATAOBJECT_NEW)) {
            setEventHandler(EVENT_TYPE_DATAOBJECT_NEW, onNewDataObject);
        }
        if (!getEventInterest(EVENT_TYPE_NODE_UPDATED)) {
            setEventHandler(EVENT_TYPE_NODE_UPDATED, onNodeUpdated);
        }
        if (!getEventInterest(EVENT_TYPE_NODE_CONTACT_NEW)) {
            setEventHandler(EVENT_TYPE_NODE_CONTACT_NEW, onNewNeighbor);
        }
    }
    else {
        HAGGLE_DBG("Set new forwarding module to \'NULL'\n");
        LOG_ADD("# %s: forwarding module is \'NULL'\n", getName());

        if (deRegisterEvents) {

            HAGGLE_DBG(
                    "Deregistering events EVENT_TYPE_DATAOBJECT_NEW EVENT_TYPE_NODE_UPDATED EVENT_TYPE_NODE_CONTACT_NEW\n");
            // remove interest in new data objects to avoid resolution
            removeEventHandler(EVENT_TYPE_DATAOBJECT_NEW);
            removeEventHandler(EVENT_TYPE_NODE_UPDATED);
            removeEventHandler(EVENT_TYPE_NODE_CONTACT_NEW);
        }
    }
}

void ForwardingManager::onShutdown()
{
	// Set the current forwarding module to none. See setForwardingModule().
	unregisterWithKernel();
}

void ForwardingManager::onForwardingTaskComplete(Event *e) {
    if (!e || !e->hasData())
        return;

    ForwardingTask *task = static_cast<ForwardingTask *>(e->getData());

    if (!task)
        return;

    //HAGGLE_DBG("Got event type %u\n", task->getType());

    switch (task->getType()) {
    /*
     QUIT: save the modules state which is returned in the task.
     */
    case FWD_TASK_QUIT: {
        RepositoryEntryList *rel = task->getRepositoryEntryList();

        if (rel) {
            HAGGLE_DBG(
                    "Forwarding module QUIT, saving its state... (%lu entries)\n",
                    rel->size());

            for (RepositoryEntryList::iterator it = rel->begin();
                    it != rel->end(); it++) {
                kernel->getDataStore()->insertRepository(*it);
            }

        }
        /*
         If we are preparing for shutdown, we should also signal
         that we are ready.
         */
        if (getState() == MANAGER_STATE_PREPARE_SHUTDOWN) {
            signalIsReadyForShutdown();
        }
    }
        break;
    case FWD_TASK_GENERATE_ROUTING_INFO_DATA_OBJECT:
        if (isNeighbor(task->getNode())) {
            if (forwardingModule) {
                // Send the neighbor our forwarding metric if we have one
                if (task->getDataObject()) {
                    HAGGLE_DBG("Sending routing information to %s\n",
                            task->getNode()->getName().c_str());
		    NodeRef actual_target; // MOS
                    if (shouldForward(task->getDataObject(), task->getNode(), actual_target)) { // MOS
                        if (addToSendList(task->getDataObject(),
                                actual_target)) {
                            // Check if this is a recursive update, and we have a list
                            // of nodes that have received it already.
                            if (task->getNodeList()) {
                                // Append the list to the data object
                                recurseListToMetadata(
                                        task->getDataObject()->getMetadata()->getMetadata(
                                                getName()),
                                        *task->getNodeList());
                            }
                            kernel->addEvent(
                                    new Event(EVENT_TYPE_DATAOBJECT_SEND,
                                            task->getDataObject(),
                                            actual_target));
                        }
                    }
                    else {
                        HAGGLE_ERR(
                                "Could not send routing information to neighbor\n");
                    }
                }
            }
        }
    default:
        break;
    }
    delete task;
}

// SW: init punt to avoid race condition due to singleton forwarder
void ForwardingManager::onInitPunt(Event *e) {
    // Only send ready signal if this callback was made while still in startup
    if (!isReadyForStartup() && 
        (NULL != forwardingModule) &&  
        forwardingModule->isInitialized()) {
        signalIsReadyForStartup();
    }
    else if (!isReadyForStartup() && noForwarder) {
        signalIsReadyForStartup();
    }
}

void ForwardingManager::onRepositoryData(Event *e) {
    DataStoreQueryResult *qr = NULL;

    if (!e || !e->getData())
        goto out;

    HAGGLE_DBG("Got repository callback\n");

    // This event either reports the forwarding module's state
    qr = static_cast<DataStoreQueryResult *>(e->getData());

    if (forwardingModule) {
        if (qr->countRepositoryEntries() > 0) {
            unsigned int n = 0;
            // Then this is most likely the forwarding module's state:
            RepositoryEntryRef re;

            while ((re = qr->detachFirstRepositoryEntry())) {
                // Was there a repository entry? => was this really what we expected?
                // SW: check if the loaded data is relevant to the forwarder
                if (forwardingModule->isReleventRepositoryData(re)) {
                    if (!forwardingModule->setSaveState(re)) {
                        HAGGLE_ERR(
                                "Error loading saved state for forwarder.\n");
                    }
                    n++;
                }
            }
            HAGGLE_DBG("Restored %u entries of saved state for module \'%s\'\n",
                    n, forwardingModule->getName());
        }
        // SW: NOTE: we notify the forwarding module that there was a repository callback,
        // indicating that data was loaded. This is used to determine if the forwarder
        // is fully initilaized later.
	HAGGLE_DBG("Received repository callback for module \'%s\'\n",
		   forwardingModule->getName());
        forwardingModule->incrementRepositoryCallbackCount();
    }
    else {
        HAGGLE_DBG(
                "No forwarding module set for when retreiving saved state\n");
    }

    delete qr;
    out: if (forwardingModule) {
        // It is now safe to start the module thread
        HAGGLE_DBG("Starting forwarding module \'%s\'\n",
                forwardingModule->getName());
        forwardingModule->start();
    }

    // SW: NOTE: there's a race condition here where the default forwarder 
    // triggers a signalIsReadyForStartup before the onConfig has fired
    // and the other forwarder modules have loaded. Thus, we postpone
    // the initilazation time until a later date, when we have more
    // information.
	getKernel()->addEvent(new Event(on_punt_init_event, NULL, PUNT_DELAY_INIT_S));
    HAGGLE_DBG("Punting initialzation check until %d seconds\n", PUNT_DELAY_INIT_S);
}

void ForwardingManager::onDebugCmd(Event *e)
{
	if (e) {
		if (e->getDebugCmd()->getType() == DBG_CMD_PRINT_ROUTING_TABLE) {
			if (forwardingModule)
				forwardingModule->printRoutingTable();
			else
				printf("No forwarding module");
		} else if (e->getDebugCmd()->getType() == DBG_CMD_OBSERVE_ROUTING_TABLE) {
            if (forwardingModule) {
                Metadata *m = new XMLMetadata(e->getDebugCmd()->getMsg());
                if (m) {
                    forwardingModule->getRoutingTableAsMetadata(m);
                    kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
                }
            }
        }
	}
}

/*
  A wrapper around node->isNeighbor() that handles application nodes
  and checks that a node retrieved from the data store is really a
  neighbor (i.e., a node from the data store may have inaccurate
  interface information and therefore has to be checked against a
  potential neighbor node in the node store).
 */
bool ForwardingManager::isNeighbor(const NodeRef& node)
{
	if (node->isNeighbor())
		return true;

	if (node && node->getType() == Node::TYPE_PEER) {
		/*
		 WARNING! Previously, kernel->getNodeStore()->retrieve(node,...) was
		 called on the same line as node->getType(), i.e.,
		 
		 if (node && node->getType() == Node::TYPE_PEER && kernel->getNodeStore()->retrieve(node, true))
			...
		 
		 but this could cause a potential deadlock in the NodeStore(). The first
		 call to getType() will lock the node (through the use of the reference auto
		 locking) and that lock will remain until the next line is executed. If
		 getNodeStore()->retrieve() is called on the same line, the node will hence
		 be in a locked state when accessing the node store, which is forbidden
		 due to the risk of deadlock (see separate note in NodeStore.{h,cpp}).
		 */
		if (kernel->getNodeStore()->retrieve(node, true))
			return true;
	}
        return false;
}
/*
   This function verifies that a node is a valid receiver for a
   specific data object, e.g., that the node has not already received
   the data object.
 */

bool ForwardingManager::addToSendList(DataObjectRef& dObj, const NodeRef& node, int repeatCount)
{
 	// Check if the data object/node pair is already in our send list:
        for (forwardingList::iterator it = forwardedObjects.begin();
             it != forwardedObjects.end(); it++) {
                // Is this it?
                if ((*it).first.first == dObj &&
                    (*it).first.second == node) {
                        // Yep. Do not forward this.
                        HAGGLE_DBG("Data object already in send list for node '%s'\n",
                                   node->getName().c_str());
                        return false;
                }
        }
	
        // Remember that we tried to send this:
        forwardedObjects.push_front(Pair< Pair<const DataObjectRef, const NodeRef>,int>(Pair<const DataObjectRef, const NodeRef>(dObj,node), repeatCount));
        
        return true;
}

// MOS - The following function hass been extended by a feature to replace node by the actual node to forward to (the proxy in case of application nodes).
//       This function also checks the proxy bloomfilter in case of application node targets to enable.
//       In this way application BF do not have to be disseminated, because they are aggregated in the proxy BF
//       (instead of Haggle's aggregation of interests and BFs we only keep aggregation of BFs for all applications on a node).

bool ForwardingManager::shouldForward(const DataObjectRef& dObj, const NodeRef& node, NodeRef& actual_node)
{
  NodeRef peer;
        
    if (!node) {
        HAGGLE_ERR("node is NULL\n");
        return false;
    }

    const char *idStr = dObj->getIdStr(); // CBMEN, HL - prevent deadlocks
    if (dObj->isNodeDescription()) {
        NodeRef descNode = Node::create(dObj); // MOS - removed Node::TYPE_PEER to allow for application nodes

        if (descNode) {
            if (descNode == node) {
                // Do not send the peer its own node description
                HAGGLE_DBG(
                        "Data object [%s] is peer %s's node description. - not sending!\n",
                        idStr, node->getName().c_str());
                return false;
            }
        }
    }

    HAGGLE_DBG( "Checking if data object %s should be forwarded to node %s \n", idStr, node->getName().c_str());

    // Make sure we use the node in the node store
    peer = kernel->getNodeStore()->retrieve(node, false);
	if (!peer) {
		peer = node;
	} 

	if (peer->hasThisOrParentDataObject(dObj)) {
		HAGGLE_DBG("node %s [%s] already has data object [%s]\n", 
			peer->getName().c_str(), peer->getIdStr(), idStr);
		return false;
	}

	// MOS   
	if(kernel->firstClassApplications() && 
           node->getType() == Node::TYPE_APPLICATION && !node->hasProxyId(kernel->getThisNode()->getId())) {
	  Node::Id_t proxyId;
	  memcpy(proxyId, node->getProxyId(), NODE_ID_LEN); // careful to avoid locking due to reference lock proxies
	  NodeRef proxy = kernel->getNodeStore()->retrieve(proxyId);
	  if (proxy) {
	    if (proxy->hasThisOrParentDataObject(dObj)) {
	      HAGGLE_DBG("%s proxy node %s [%s] already has data object [%s]\n", 
			 getName(), proxy->getName().c_str(), proxy->getIdStr(), idStr);
	      return false;
	    } else {
	      actual_node = proxy;
	    }
	  } else {
        if (enableMultihopBloomfilters) {
	        HAGGLE_DBG("%s proxy node of %s [%s] missing from local node store - not sending\n", 
                getName(), node->getName().c_str(), node->getIdStr());
            return false;
        }
        else {
            actual_node = node;
            return true;	
        }
	  }
	} else {
	  actual_node = node; //JM should be peer
	}
	
	return true; // MOS - ok to send
}

void ForwardingManager::forwardByDelegate(DataObjectRef &dObj, const NodeRef &target, 
					  const NodeRefList *other_targets)
{
	if (forwardingModule)
		forwardingModule->generateDelegatesFor(dObj, target, other_targets);
}

void ForwardingManager::onDataObjectForward(Event *e)
{
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring send data object forarding\n");
          return;
        }

	// Get the data object:
	DataObjectRef dObj = e->getDataObject();

	if (!dObj) {
		HAGGLE_ERR("EVENT_TYPE_DATAOBJECT_FORWARD without a data object.\n");
		return;
	}

	// Get the node:
	NodeRef& target = e->getNode();

	if (!target) {
		HAGGLE_ERR("EVENT_TYPE_DATAOBJECT_FORWARD without a node.\n");
		return;
	}
	
	if (!dObj)
		return;

	// Start forwarding the object:
	NodeRef actual_target;

// SW: START CONSUME INTEREST
        if (target && (target->getType() == Node::TYPE_APPLICATION)) {
                kernel->addEvent(new Event(EVENT_TYPE_ROUTE_TO_APP, dObj, target)); // SW
        }
// SW: END CONSUME INTEREST

	if (shouldForward(dObj, target, actual_target)) {
		// Ok, the target wants the data object. Now check if
		// the target is a neighbor, in which case the data
		// object is sent directly to the target, otherwise
		// ask the forwarding module to generate delegates.
		if (neighborForwardingShortcut && isNeighbor(actual_target)) {
			if (addToSendList(dObj, actual_target)) {
			        addForwardingMetadata(dObj); // MOS - was missing here
				HAGGLE_DBG("Sending data object %s directly to target neighbor %s\n", 
					   DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
				kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, actual_target));
			}
		} else {
			HAGGLE_DBG("Trying to find delegates for data object %s bound for target %s\n", 
				   DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
			forwardByDelegate(dObj, actual_target);
		}
	}
}

// MOS - deleted objects should be removed from forwarded objects list and not sent again

void ForwardingManager::onDeletedDataObject(Event * e)
{
	if (!e || !e->hasData())
		return;
	
	DataObjectRefList dObjs = e->getDataObjectList();

	for (DataObjectRefList::iterator it1 = dObjs.begin(); it1 != dObjs.end(); it1++) {
	  for (forwardingList::iterator it2 = forwardedObjects.begin(); it2 != forwardedObjects.end(); it2++) {
	        DataObjectRef dObj = (*it2).first.first;
		if (dObj == *it1) {
		  HAGGLE_DBG("Removing deleted data object %s from forwarded objects\n", (*it1)->getIdStr());
		  dObj->setObsolete(); // mark as obsolete to prempt processing by protocol
		  forwardedObjects.erase(it2);
		}
	  }
	}
}

unsigned long ForwardingManager::getDataObjectRetries(DataObjectRef &dObj)
{
  if(dObj->isNodeDescription()) return nodeDescriptionRetries;
  if (forwardingModule && forwardingModule->useNewDataObjectShortCircuit(dObj)) {
    return dataObjectRetriesShortCircuit;
  }
  else {
    return dataObjectRetries;
  }
}

void ForwardingManager::onSendDataObjectResult(Event *e)
{
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring send data object results\n");
          return;
        }

	DataObjectRef& dObj = e->getDataObject();
	NodeRef& node = e->getNode();

        HAGGLE_DBG("Checking data object %s send status - forwarded objects pending: %d\n", 
		   dObj->getIdStr(), forwardedObjects.size());

	// Find the data object in our send list:
	for (forwardingList::iterator it = forwardedObjects.begin();
		it != forwardedObjects.end(); it++) {
		
		if ((*it).first.first == dObj && (*it).first.second == node) {
            if (e->getType()
                    == EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL) {
            	HAGGLE_DBG("Handling EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL\n");
                //get repeat count before erase
                int repeatCount = (*it).second;

                // Remove the data object - it has been forwarded.
                forwardedObjects.erase(it);

                // Resending data object to trigger next block
		NodeRef actual_target;
		if (shouldForward(dObj, node, actual_target) && isNeighbor(actual_target) && 
		    addToSendList(dObj, actual_target, repeatCount)) {
		      // MOS - isNeighbor now after shouldForward to check actual target
                    HAGGLE_DBG("Trigger sending next block of data object %s\n",
                            dObj->getIdStr());
		  this->forwardingManagerHelper->addDataObjectEventWithDelay(
			     dObj, actual_target.getObj()); // MOS
		}
            }
            else if(e->getType()
                    == EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL) {
            	HAGGLE_DBG("Handling EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL\n");
                //get repeat count before erase
                int repeatCount = (*it).second;

                // Remove the data object - it has been forwarded.
                forwardedObjects.erase(it);

                // Resending data object to trigger next fragment
		NodeRef actual_target;
		if (shouldForward(dObj, node, actual_target) && isNeighbor(actual_target) && 
		    addToSendList(dObj, actual_target, repeatCount)) {
		      // MOS - isNeighbor now after shouldForward to check actual target
                    HAGGLE_DBG("Trigger sending next fragment of data object %s\n",
                            dObj->getIdStr());
                    this->fragmentationForwardingManagerHelper->addDataObjectEventWithDelay(
			    dObj, actual_target.getObj());
                }
            }
            else if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL) {
				// Remove the data object - it has been forwarded.
				forwardedObjects.erase(it);
			} else if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_FAILURE) {
				int repeatCount;
				NodeRef actual_target;
				repeatCount = (*it).second + 1;
				// Remove this from the list. It may be reinserted later.
				forwardedObjects.erase(it);
				if(repeatCount <= (int) getDataObjectRetries(dObj)) {
				  // Try resending the data object:
					        if (shouldForward(dObj, node, actual_target) && isNeighbor(actual_target) && 
						    addToSendList(dObj, actual_target, repeatCount)) {
						  // MOS - isNeighbor now after shouldForward to check actual target
						  kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, actual_target)); // MOS
						}
				}
			}
			// No need to search this list further (the "it" variable is 
			// garbage anyway...)
			goto found_it;
		}
	}

found_it:;
	// Done.
}

void ForwardingManager::addForwardingMetadata(DataObjectRef &dObj)
{
	  if(enableForwardingMetadata) {
		Metadata *m = dObj->getMetadata()->getMetadata(getName());
		unsigned long count = 0;

		dObj.lock(); // MOS

		if (!m) {
			m = dObj->getMetadata()->addMetadata(getName());
			
			if (!m) {
			        dObj.unlock();
				return;
			}
		} else {
			char *endptr = NULL;
			const char *param = m->getParameter("hop_count");
			
			count = strtoul(param, &endptr, 10);
			
			if (!endptr || endptr == param)
				count = 0;
		}
		
		char countstr[30];
		snprintf(countstr, 29, "%lu", ++count);
		m->setParameter("hop_count", countstr);
		
		m = m->addMetadata("Hop", kernel->getThisNode()->getIdStr());
		
		if (m) {
			m->setParameter("time", Timeval::now().getAsString().c_str());
			m->setParameter("hop_num", countstr);
		}

		dObj.unlock();
	  }
}


void ForwardingManager::addForwardingMetadataDelegated(DataObjectRef &dObj)
{
	  if(enableForwardingMetadata) {

		dObj.lock(); // MOS

		Metadata *m = dObj->getMetadata()->getMetadata(getName());

		if (m) {
			Metadata *hop = m->getMetadata("Hop");

			while (hop) {
				if (hop->getContent() == kernel->getThisNode()->getIdStr()) {
					hop->setParameter("delegated", "true");
				}
				hop = m->getNextMetadata();
			}
		}

		dObj.unlock(); // MOS
	  }
}

// MOS - This callback is used to two different queries (doDataObjectQuery and doDataObjectForNodesQuery)
//       which have subtle differences in the results. It might be cleaner to have two different functions.
 
void ForwardingManager::onDataObjectQueryResult(Event *e) {
    if (!e || !e->hasData()) {
        HAGGLE_ERR("Error: No data in data store query result!\n");
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring data object query results\n");
      return;
    }

    DataStoreQueryResult *qr = static_cast<DataStoreQueryResult*>(e->getData());
    NodeRef target = qr->detachFirstNode();

    if (!target) {
        HAGGLE_DBG("No peer node in query result\n");
        delete qr;
        return;
    }

    NodeRef delegate = target; // MOS

    HAGGLE_DBG("Got dataobject query result for target node %s\n",
            target->getIdStr());

    DataObjectRef dObj;

    while (dObj = qr->detachFirstDataObject()) {
      NodeRef optional_target = qr->detachFirstNode(); // MOS
      if(optional_target) target = optional_target; // MOS

      HAGGLE_DBG("Considering data object %s for delegate %s and target %s\n", 
		 DataObject::idString(dObj).c_str(), Node::nameString(delegate).c_str(), Node::nameString(target).c_str());

	// MOS - this is the normal case after generateTargets for a potential delegate
	if (neighborForwardingShortcut && isNeighbor(delegate)) {
	  NodeRef actual_delegate;
	  if (shouldForward(dObj, delegate, actual_delegate)) {
	    if (addToSendList(dObj, actual_delegate)) {
	      kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, actual_delegate));
	      HAGGLE_DBG("Sending data object %s directly to delegate neighbor %s\n", 
			 DataObject::idString(dObj).c_str(), actual_delegate->getName().c_str());
	    }
	  }
	  continue;	    
	}

		// Does this target already have this data object, or
		// is the data object its node description? shouldForward() tells us.
		NodeRef actual_target;
		if (shouldForward(dObj, target, actual_target)) {
			// Is this node a currently available neighbor node?
			if (neighborForwardingShortcut && isNeighbor(actual_target)) {
				// Yes: it is it's own best delegate,
				// so start "forwarding" the object:
				
				if (addToSendList(dObj, actual_target)) {
				        addForwardingMetadata(dObj); // MOS
					kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, actual_target));
					HAGGLE_DBG("Sending data object %s directly to target neighbor %s\n", 
						   DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
				}
                        } else if (kernel->firstClassApplications() && actual_target->isLocalApplication()) { // MOS
					HAGGLE_DBG("Sending data object %s directly to local application %s\n", 
						   DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
			  kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_TO_APP, dObj, actual_target));
			} else { 
				HAGGLE_DBG("Trying to find delegates for data object %s bound for target %s\n", 
					   DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
				forwardByDelegate(dObj, actual_target);
			}
		}
	}
        
	delete qr;
}

// MOS - compute delay to randomize order and timing of send events

double ForwardingManager::randomizedForwardingDelay(NodeRefList& ns, DataObjectRef& dObj)
{
  if(dObj->isNodeDescription()) {
    double base = maxNodeDescForwardingDelayBaseMs ? ((rand() % maxNodeDescForwardingDelayBaseMs) / (double) 1000) : 0;
    double delay = base + (maxNodeDescForwardingDelayLinearMs ? (((rand() % maxNodeDescForwardingDelayLinearMs) / (double) 1000) * (double) (ns.size() - 1)) : 0);
    return delay;
  } else {
    double base = maxForwardingDelayBaseMs ? ((rand() % maxForwardingDelayBaseMs) / (double) 1000) : 0;
    double delay = base + (maxForwardingDelayLinearMs ? (((rand() % maxForwardingDelayLinearMs) / (double) 1000) * (double) (ns.size() - 1)) : 0);
    return delay;
  }
}

void ForwardingManager::onNodeQueryResult(Event *e)
{
	if (!e || !e->hasData())
		return;

        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring node query results\n");
          return;
        }
	
	DataStoreQueryResult *qr = static_cast < DataStoreQueryResult * >(e->getData());
	DataObjectRef dObj = qr->detachFirstDataObject();
	const NodeRefList *targets = qr->getNodeList();

	if (!dObj) {
		HAGGLE_DBG("No dataobject in query result\n");
		delete qr;
		return;
	}
	if (!targets || targets->empty()) {
#if defined(BENCHMARK)
		HAGGLE_DBG2("No nodes in query result\n");
#else
		HAGGLE_ERR("No nodes in query result\n");
#endif
		delete qr;
		return;
	}

	HAGGLE_DBG("Got node query result for dataobject %s with %lu nodes\n", 
		   DataObject::idString(dObj).c_str(), targets->size());

	// MOS - we first translate the targets using shouldForward
	NodeRefList actual_targets;
	for (NodeRefList::const_iterator it = targets->begin(); it != targets->end(); it++) {
		const NodeRef& target = *it;
                 // Is this node a currently available neighbor node?

// SW: START CONSUME INTEREST
                if (target && (target->getType() == Node::TYPE_APPLICATION)) {
                     kernel->addEvent(new Event(EVENT_TYPE_ROUTE_TO_APP, dObj, target));
                }
// SW: END CONSUME INTEREST

		NodeRef actual_target;
                if (shouldForward(dObj, target, actual_target)) {
		  actual_targets.push_back(actual_target);
		}
	}
	
	NodeRefList target_neighbors;

	for (NodeRefList::const_iterator it = actual_targets.begin(); it != actual_targets.end(); it++) {
		const NodeRef& target = *it;
                 // Is this node a currently available neighbor node?
		NodeRef actual_target = target;
                // if (shouldForward(dObj, target, actual_target)) { // MOS - now redundant
                        if (neighborForwardingShortcut && isNeighbor(actual_target)) {
                                // Yes: it is it's own best delegate,
                                // so start "forwarding" the object:
                                if (addToSendList(dObj, actual_target)) {
                                        HAGGLE_DBG("Sending data object %s directly to target neighbor %s\n", 
						DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
                                        target_neighbors.push_front(actual_target);
                                }
                        } else if (kernel->firstClassApplications() && actual_target->isLocalApplication()) { // MOS
			  kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_TO_APP, dObj, actual_target));
                        } else if (actual_target->getType() == Node::TYPE_PEER || 
				   actual_target->getType() == Node::TYPE_GATEWAY ||
				   (kernel->firstClassApplications() && actual_target->getType() == Node::TYPE_APPLICATION)) { // MOS - added remote app 
                                HAGGLE_DBG("Trying to find delegates for data object %s bound for target %s\n", 
					DataObject::idString(dObj).c_str(), actual_target->getName().c_str());
                                // forwardByDelegate(dObj, actual_target, targets); // MOS - old version
                                forwardByDelegate(dObj, actual_target, &actual_targets);
                        }
		//}
	}

    if (!target_neighbors.empty()) {
        addForwardingMetadata(dObj); // MOS - was missing here

	// kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, target_neighbors));
	// MOS - adding randomized forwarding delay for desynchronization
	for (NodeRefList::iterator it = target_neighbors.begin(); it != target_neighbors.end(); it++) {
	  NodeRefList nl;
	  nl.push_back(*it);
	  double delay = randomizedForwardingDelay(target_neighbors,dObj);
	  kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, nl, delay));
	}
    }
	
	delete qr;
}

// SW: START NEW NBR NODE DESCRIPTION PUSH:
void ForwardingManager::forwardNodeDescriptions(const NodeRef& node) {

    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not forwarding node descriptions\n");
      return;
    }

    if (!doPushNodeDescriptions) {
        return;
    }

    NodeRefList allNodesRefList;
    kernel->getNodeStore()->retrieveAllNodes(allNodesRefList);
    
    for (NodeRefList::iterator it = allNodesRefList.begin(); it != allNodesRefList.end(); it++) {
        NodeRef& currentNode = *it;
	if(currentNode == node) continue; // MOS - do not forward its own description
	if(currentNode->isLocalApplication()) continue; // MOS - do not forward its own local app node descriptions
        if (currentNode->getType() != Node::TYPE_PEER && currentNode->getType() != Node::TYPE_APPLICATION) { // MOS
            continue;
        }

        string currentNodeName = currentNode->getName();
        string nodeName = node->getName();
        HAGGLE_DBG("Node description push of: %s -> %s\n", currentNodeName.c_str(), nodeName.c_str());

        DataObjectRef currentNodeDataObj = currentNode->getDataObject(true);
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_FORWARD, currentNodeDataObj, node));
    }
}
// SW: END NEW NBR NODE DESCRIPTION PUSH.

void ForwardingManager::onDelayedDataObjectQuery(Event *e) {
    if (!e || !e->hasData())
        return;

    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not processing delayed queries\n");
      return;
    }

    NodeRef node = e->getNode();

    HAGGLE_DBG("Delayed data object query for neighbor %s\n",
	       node->getName().c_str());

    // If the node is still in the node query list, we know that
    // there has been no update for it since we generated this
    // delayed call.
    for (List<NodeRef>::iterator it = pendingQueryList.begin();
            it != pendingQueryList.end(); it++) {
        if (node == *it) {
            pendingQueryList.erase(it);
            if (enableMultihopBloomfilters) {
                forwardNodeDescriptions(node); // MOS - node descriptions first
            }
            findMatchingDataObjectsAndTargets(node);
            break;
        }
    }

    // SW: NOTE we delay this so that the forwarders only operate on the
    // most up-to-date neighbor information:
    // Tell the forwarding module that we've got a new neighbor:
    if (forwardingModule) {
        forwardingModule->newNeighbor(node);		
    }
}

void ForwardingManager::onPeriodicDataObjectQuery(Event *e) {
    NodeRefList neighbors;
    Timeval now = Timeval::now();
    double nextTimeout = periodicDataObjectQueryInterval;

    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    kernel->getNodeStore()->retrieveNeighbors(neighbors);

    for (NodeRefList::iterator it = neighbors.begin(); it != neighbors.end();
            it++) {
        NodeRef& neigh = *it;

        if ((now - neigh->getLastDataObjectQueryTime())
                > periodicDataObjectQueryInterval) {
            HAGGLE_DBG("Periodic data object query for neighbor %s\n",
                    neigh->getName().c_str());
            findMatchingDataObjectsAndTargets(neigh);
        }
        else if ((now - neigh->getLastDataObjectQueryTime()) < nextTimeout) {
            nextTimeout =
                    (now - neigh->getLastDataObjectQueryTime()).getSeconds();
        }

// SW: START NEW NBR NODE DESCRIPTION PUSH:
/* // TODO: not clear that this is needed, due to previous flooding mechanism
        if (!doPushNodeDescriptions) {
            continue;
        }

        NodeRefList allNodesRefList;
        kernel->getNodeStore()->retrieveAllNodes(allNodesRefList);
        
        for (NodeRefList::iterator it = allNodesRefList.begin(); it != allNodesRefList.end(); it++) {
            NodeRef& currentNode = *it;
            if (currentNode->getType() != Node::TYPE_PEER) {
                continue;
            }
            DataObjectRef currentNodeDataObj = currentNode->getDataObject(true);
            kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_FORWARD, currentNodeDataObj, neigh));
        }
*/
// SW: END NEW NBR NODE DESCRIPTION PUSH.
    }

	if (kernel->getNodeStore()->numNeighbors() > 0 &&
	    !periodicDataObjectQueryEvent->isScheduled() &&
	    periodicDataObjectQueryInterval > 0) {
		periodicDataObjectQueryEvent->setTimeout(nextTimeout);
		kernel->addEvent(periodicDataObjectQueryEvent);
	}
}

// MOS - added to generate routing info periodically and especially to include app nodes
//       this should be optimized to avoid redundant updates if nothing changes - FIXME

void ForwardingManager::onPeriodicRoutingUpdate(Event *e)
{
	NodeRefList neighbors;
	Timeval now = Timeval::now();
	double nextTimeout = periodicRoutingUpdateInterval;

        if (getState() > MANAGER_STATE_RUNNING) {
	  HAGGLE_DBG("In shutdown, ignoring event\n");
	  return;
	}
	
	kernel->getNodeStore()->retrieveNeighbors(neighbors);

	for (NodeRefList::iterator it = neighbors.begin(); it != neighbors.end(); it++) {
		NodeRef& neigh = *it;

		if(neigh->isNeighbor()) {
		  if (forwardingModule) {
		    if ((now - neigh->getLastRoutingUpdateTime()) > 
			periodicRoutingUpdateInterval) {
		      HAGGLE_DBG("Periodic routing update for neighbor %s\n",
				 neigh->getName().c_str());
		      forwardingModule->generateRoutingInformationDataObject(neigh);
		    } else if ((now - neigh->getLastRoutingUpdateTime()) < nextTimeout) {
		      nextTimeout = (now - neigh->getLastRoutingUpdateTime()).getSeconds();
		    }
		  }
		}
	}
	
	if (!periodicRoutingUpdateEvent->isScheduled() &&
	    periodicRoutingUpdateInterval > 0) {
		periodicRoutingUpdateEvent->setTimeout(nextTimeout);
		kernel->addEvent(periodicRoutingUpdateEvent);
	}
}

void ForwardingManager::onNewNeighbor(Event *e)
{
	if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, ignoring new neighbor\n");
		return;
	}
	
	NodeRef node = e->getNode();

	if (node->getType() == Node::TYPE_UNDEFINED)
		return;

    if (forwardingModule) {
        // SW: note we removed newNeighbor here to wait until we have the
        // most up-to-date node description for the forwarders
		forwardingModule->generateRoutingInformationDataObject(node);
    }
	
	// Find matching data objects for the node and figure out whether it is a good
	// delegate. But delay these operations in case we get a node update event for the
	// same node due to receiving a new node description for it. If we get the
	// the update, we should only do the query once using the updated information.
	pendingQueryList.push_back(node);
	kernel->addEvent(new Event(delayedDataObjectQueryCallback, node, 5));
	
	HAGGLE_DBG("%s - new node contact with %s [id=%s]."
		   " Delaying data object query in case there is"
		   " an incoming node description for the node\n", 
		   getName(), node->getName().c_str(), node->getIdStr());


	/* Start periodic data object query if configuration allows. */
	if ( // kernel->getNodeStore()->numNeighbors() == 1 && // MOS - this is too restrictive,
             // it may happen that multiple neighbors appear simulataneously (only important if periodic queries are enabled)
	    !periodicDataObjectQueryEvent->isScheduled() &&
	    periodicDataObjectQueryInterval > 0) {
		periodicDataObjectQueryEvent->setTimeout(periodicDataObjectQueryInterval);
		kernel->addEvent(periodicDataObjectQueryEvent);
	}

	if (!periodicRoutingUpdateEvent->isScheduled() &&
	    periodicRoutingUpdateInterval > 0) {
		periodicRoutingUpdateEvent->setTimeout(periodicRoutingUpdateInterval);
		kernel->addEvent(periodicRoutingUpdateEvent);
	}
}

void ForwardingManager::onEndNeighbor(Event *e)
{	
	if (e->getNode()->getType() == Node::TYPE_UNDEFINED)
		return;
	
	NodeRef node = e->getNode();

	// Tell the forwarding module that the neighbor went away
	if (forwardingModule)
		forwardingModule->endNeighbor(node);

	// Cancel any queries for this node in the data store since they are no longer
	// needed
	kernel->getDataStore()->cancelDataObjectQueries(node);

    // Also remove from pending query list so that
    // onDelayedDataObjectQuery won't generate a delayed query
    // after the node went away
    for (List<NodeRef>::iterator it = pendingQueryList.begin();
            it != pendingQueryList.end(); it++) {
        if (node == *it) {
            pendingQueryList.erase(it);
            break;
        }
    }
    if (recursiveRoutingUpdates) {
        // Trigger a new routing update to inform our other
        // neighbors about our new metrics
        recursiveRoutingUpdate(node, NULL);
    }
}

// MOS - probabilistic load reduction for kernel thread

bool ForwardingManager::probabilisticLoadReduction()
{
  unsigned long qs = kernel->size(); 
  if(qs < loadReductionMinQueueSize) return false;
  if(qs > loadReductionMaxQueueSize) return true;
  double p = (double)(qs - loadReductionMinQueueSize) / (double)(loadReductionMaxQueueSize - loadReductionMinQueueSize);
  double r = (double)rand()/(double)RAND_MAX;
  return r <= p;
}

void ForwardingManager::onNodeUpdated(Event *e)
{
       if (getState() > MANAGER_STATE_RUNNING) {
               HAGGLE_DBG("In shutdown, ignoring node update\n");
               return;
       }

	if (!e || !e->hasData())
		return;

	NodeRef node = e->getNode();
	NodeRefList &replaced = e->getNodeList();
	
	if (node->getType() == Node::TYPE_UNDEFINED) {
		HAGGLE_DBG("%s Node is undefined, deferring dataObjectQuery\n", 
			   getName());
		return;
	} 
	
	HAGGLE_DBG("%s - got node update for %s [id=%s]\n", 
		   getName(), node->getName().c_str(), node->getIdStr());

	// MOS - the following periodic events should also happen when a new neighbor 
        //       is discovered but onNewNeighbor is not called

	if (kernel->getNodeStore()->numNeighbors() > 0) {
 
	  if (!periodicDataObjectQueryEvent->isScheduled() &&
	      periodicDataObjectQueryInterval > 0) {
	    periodicDataObjectQueryEvent->setTimeout(periodicDataObjectQueryInterval);
	    kernel->addEvent(periodicDataObjectQueryEvent);
	  }
	  
	  if (!periodicRoutingUpdateEvent->isScheduled() &&
	      periodicRoutingUpdateInterval > 0) {
	    periodicRoutingUpdateEvent->setTimeout(periodicRoutingUpdateInterval);
	    kernel->addEvent(periodicRoutingUpdateEvent);
	  }
	}

	bool forwardNodeDesc = false;
	// Did this updated node replace an undefined node?
	// Go through the replaced nodes to find out...
	NodeRefList::iterator it = replaced.begin();
	
	while (it != replaced.end()) {
		// Was this undefined?
	        if ((*it)->getType() == Node::TYPE_UNDEFINED && node->isNeighbor()) {
			// Yep. Tell the forwarding module that we've got a new neighbor:
			if (forwardingModule) {
				forwardingModule->newNeighbor(node);
				forwardingModule->generateRoutingInformationDataObject(node);
			}			
			forwardNodeDesc = true;
			break;
		} else if (node->isNeighbor()) {
			if (forwardingModule) {
				forwardingModule->newNeighborNodeDescription(node);
			}			
			forwardNodeDesc = true;
			break;
		}
		it++;
	}

	// Check if there are any pending node queries that have been
	// initiated by a previous new node contact event (in
	// onNewNeighbor). In that case, remove the node from the
	// pendingQueryList so that we do not generate the query
	// twice.
	for (List<NodeRef>::iterator it = pendingQueryList.begin(); 
	     it != pendingQueryList.end(); it++) {
		if (node == *it) {
			pendingQueryList.erase(it);
			forwardNodeDesc = true;
			break;
		}
	}

	if(enableMultihopBloomfilters && forwardNodeDesc) forwardNodeDescriptions(node); // MOS - node descriptions first

	if(!probabilisticLoadReduction()) {
	  findMatchingDataObjectsAndTargets(node);
	}
	else {
	  HAGGLE_DBG("skip matching on node update to reduce load\n");
	}
}

void ForwardingManager::findMatchingDataObjectsAndTargets(NodeRef& node) {
    if (!node || node->getType() == Node::TYPE_UNDEFINED)
        return;
    
    if(enableTargetGeneration) {
      // Check that this is an active neighbor node we can send to:
      if (node->isNeighbor()) {
        // Ask the forwarding module for additional target nodes for which
        // this neighbor can act as delegate.

        if (forwardingModule) {
            HAGGLE_DBG(
                    "%s trying to find targets for which neighbor %s [id=%s] is a good delegate\n",
                    getName(), node->getName().c_str(), node->getIdStr());
            forwardingModule->generateTargetsFor(node);
        }
      }
    }

    HAGGLE_DBG("%s doing data object query for node %s [id=%s]\n",
            getName(), node->getName().c_str(), node->getIdStr());

    // Ask the data store for data objects bound for the node.
    // The node can be a valid target, even if it is not a current
    // neighbor -- we might find a delegate for it.

    node->setLastDataObjectQueryTime(Timeval::now());
    kernel->getDataStore()->doDataObjectQuery(node, 1, dataObjectQueryCallback);
}

/*
 
 The recurse list is a list of node that a "triggered" update has traversed.
 
 The general idea is that nodes, in the event of a change in a neighbor's status, should
 be able to tell all their other neighbors about this change.
 
 However, this leads to a risk of circular and never ending routing updates. The idea of the 
 recurse list is to mitigate such updates by appending a list to the update that indicates 
 the nodes that have already processed an update. Hence, when a node receives a recursive
 routing update, it appends all its  neighbors not already in the list and sends forth its
 updated metrics with this list attached. If a node finds that all its neighbors have
 processed this update, then the recursive update ends.
 */
size_t ForwardingManager::metadataToRecurseList(Metadata *m, NodeRefList& recurse_list)
{	
	if (!m || m->getName() != "RecurseList")
		return 0;
		
	Metadata *tm = m->getMetadata("Node");
	
	while (tm) {
		const char *id = tm->getParameter("node_id");
		
		if (id) {
			NodeRef n = Node::create_with_id(Node::TYPE_PEER, id);
			
			if (n) {
				recurse_list.push_back(n);
			}
		}
		tm = m->getNextMetadata();
	}
	
	return recurse_list.size();
}

// MOS - make sure the data object is locked when calling this

Metadata *ForwardingManager::recurseListToMetadata(Metadata *m, 
						   const NodeRefList& recurse_list)
{
	if (!m)
		return NULL;
	
	Metadata *tm = m->addMetadata("RecurseList");
	
	if (!tm)
		return NULL;
	
	for (NodeRefList::const_iterator it = recurse_list.begin(); 
	     it != recurse_list.end(); it++) {
		Metadata *nm = tm->addMetadata("Node");
		
		if (nm) {
			nm->setParameter("node_id", (*it)->getIdStr());
		}
	}
	
	return tm;
}

/* 
 
 This function triggers the node to send its current metrics to all its neighbors
 modulo the ones already in a recurse list, which is given as a metadata object 
 from a just received routing update.
 
 If the recursive routing update is triggered by the loss of a neighbor, then
 the metadata is NULL.
 */
void ForwardingManager::recursiveRoutingUpdate(NodeRef peer, Metadata *m)
{
        if(forwardingModule == NULL) return; // MOS - to cover noForward

	// Send out our updated routing information to all neighbors
	NodeRefList neighbors;
	NodeRefList notify_list;
	NodeRefList recurse_list;
	size_t n = 0;
	
	// Fill in any existing nodes that have been notified by this recursive update
	if (m) {
		n = metadataToRecurseList(m->getMetadata("RecurseList"), recurse_list);
	}
	
	if (n == 0)
		recurse_list.add(peer);
	
	kernel->getNodeStore()->retrieveNeighbors(neighbors);
	
	// Figure out which peers have not already received this recursive update
	for (NodeRefList::iterator it = neighbors.begin(); it != neighbors.end(); it++) {
		bool should_notify = true;
		NodeRef neighbor = *it;

		if(neighbor->getType() != Node::TYPE_PEER) continue; // MOS - exclude unnamed neighbors

		// Do not notify nodes that are already in the list
		for (NodeRefList::iterator jt = recurse_list.begin(); 
		     jt != recurse_list.end(); jt++) {
			if (neighbor == *jt) {
				/*
				 HAGGLE_DBG("Neighbor %s [%s] has already received the update\n", 
					   neighbor->getName().c_str(), neighbor->getIdStr());
				 */
				should_notify = false;
				break;
			}
		}
		// Generate a routing update for this neighbor
		if (should_notify) {
			notify_list.push_back(neighbor);
			recurse_list.push_back(neighbor);
		}
	}
	// We have the complete list of neighbors that haven't received the update.
	// Now send our recursive update to them, append the current nodes that have
	// been part of this recursive routing update
	while (notify_list.size()) {
		NodeRef neighbor = notify_list.pop();
		forwardingModule->generateRoutingInformationDataObject(neighbor, 
								       &recurse_list);
	}	
}

void ForwardingManager::onRoutingInformation(Event *e) {
    if (!e || !e->hasData())
        return;

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring routing information\n");
        return;
    }

	DataObjectRefList& dObjs = e->getDataObjectList();

	while (dObjs.size()) {
		DataObjectRef dObj = dObjs.pop();
		
		InterfaceRef iface = dObj->getRemoteInterface();
		NodeRef peer = kernel->getNodeStore()->retrieve(iface);
		
		if (!peer || peer == kernel->getThisNode()) {
			HAGGLE_DBG("Routing information is from ourselves -- ignoring\n");
			return;
		}
		
		// Check if there is a module, and that the
		// information received data object makes sense to it.
		if (forwardingModule && forwardingModule->hasRoutingInformation(dObj)) {
			
			// Tell our module that it has new routing information
			forwardingModule->newRoutingInformation(dObj);
		}

            if (recursiveRoutingUpdates && peer) {
                recursiveRoutingUpdate(peer,
                        dObj->getMetadata()->getMetadata(getName()));
            }
        }
}

void ForwardingManager::onNewDataObject(Event *e) {
    if (!e || !e->hasData())
        return;

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring data object\n");
        return;
    }

    DataObjectRef& dObj = e->getDataObject();

	if(networkCodingConfiguration->isNetworkCodingEnabled(dObj,NULL) &&
	   !networkCodingConfiguration->isForwardingEnabled()) {
	  if(networkCodingDataObjectUtility->isNetworkCodedDataObject(dObj)) {
	    return;
	  }
	}

	if(fragmentationConfiguration->isFragmentationEnabled(dObj,NULL) &&
	   !fragmentationConfiguration->isForwardingEnabled()) {
	  if(fragmentationDataObjectUtility->isFragmentationDataObject(dObj)) {
	    return;
	  }
	}

// SW: START FORWARDING MODULE SHORT CIRCUIT:
    if (forwardingModule
            && forwardingModule->useNewDataObjectShortCircuit(dObj)) {

        forwardingModule->doNewDataObjectShortCircuit(dObj);

	// MOS - the following takes care of forwarding to local applications
	// (if using first class applications, otherwise it is done by data store filters)
	if (kernel->firstClassApplications() && dObj->isPersistent() && !dObj->isNodeDescription()) {
	  if (kernel->getNodeStore()->numLocalApplications() > 0) { // MOS - application nodes are first-class citizens
	    HAGGLE_DBG("%s - new data object %s, doing app node query\n", 
		       getName(), dObj->getIdStr());
	    kernel->getDataStore()->doNodeQuery(dObj, maxNodesToFindForNewDataObjects, 1, nodeQueryCallback, true); // MOS - localAppOnly = true
	  } else {
	    HAGGLE_DBG("%s data object %s, but deferring app node query due to 0 applications\n", 
		       getName(), dObj->getIdStr());
	  }
	} 

        return;
    }
// SW: END FORWARDING MODULE SHORT CIRCUIT.
	

    if (!doQueryOnNewDataObject)
        return;

	// No point in doing node queries if we have no neighbors to
	// forward the data object to
	if (dObj->isPersistent()) {
	        // if (kernel->getNodeStore()->numNeighbors() > 0) {
	  if (kernel->getNodeStore()->numNeighbors() > 0 || 
	      (kernel->firstClassApplications() && kernel->getNodeStore()->numLocalApplications() > 0)) { // MOS - application nodes are first-class citizens
			HAGGLE_DBG("%s - new data object %s, doing node query\n", 
				   getName(), dObj->getIdStr());
			kernel->getDataStore()->doNodeQuery(dObj, 
							    maxNodesToFindForNewDataObjects, 1, 
							    nodeQueryCallback);
		} else {
			HAGGLE_DBG("%s data object %s, but deferring node query due to 0 neighbors\n", 
				   getName(), dObj->getIdStr());
		}
	} 
}

void ForwardingManager::onTargetNodes(Event * e)
{
	const NodeRef &delegate_node = e->getNode();
	const NodeRefList &target_nodes = e->getNodeList();

	// Ask the data store for data objects bound for the nodes for which the 
	// node is a good delegate forwarder:
	if (isNeighbor(delegate_node)) {
	    // No point in asking for data objects if the delegate is not a current neighbor
	  if(kernel->firstClassApplications()) {
	    NodeRefList all_app_target_nodes;
	    // MOS - we first translate the targets to app nodes (because interests are not aggregated anymore)
	    for (NodeRefList::const_iterator it = target_nodes.begin(); it != target_nodes.end(); it++) {
	      const NodeRef& target = *it;
	      NodeRefList app_target_nodes;
	      HAGGLE_DBG("target node: %s\n",target->getName().c_str());
	      kernel->getNodeStore()->retrieveApplications(target,app_target_nodes);
	      for (NodeRefList::const_iterator it2 = app_target_nodes.begin(); it2 != app_target_nodes.end(); it2++) {
		all_app_target_nodes.push_back(*it2);
		HAGGLE_DBG("app target node: %s\n",(*it2)->getName().c_str()); 
	      }
	    }
	    kernel->getDataStore()->
	      doDataObjectForNodesQuery(delegate_node, 
					all_app_target_nodes, 1, // MOS - here we use the translation
					dataObjectQueryCallback);
	  } else {
	    for (NodeRefList::const_iterator it = target_nodes.begin(); it != target_nodes.end(); it++) {
	      const NodeRef& target = *it;
	      HAGGLE_DBG("target node: %s\n",target->getName().c_str());
	    }
	    kernel->getDataStore()->
	      doDataObjectForNodesQuery(delegate_node, 
					target_nodes, 1,
					dataObjectQueryCallback);
	  }	
	}
}

void ForwardingManager::onDelegateNodes(Event * e)
{
	NodeRefList *delegates = e->getNodeList().copy();

	if (!delegates)
		return;

	DataObjectRef dObj = e->getDataObject();
	
	// Go through the list of delegates:
	NodeRef delegate;
	NodeRefList ns;
	do {
		delegate = delegates->pop();
		if (delegate) {
			NodeRef actual_delegate;
			// Find the actual node reference to the node (if it is a 
			// neighbor):
			actual_delegate = kernel->getNodeStore()->retrieve(delegate, true);
                        
			if (actual_delegate) {
	                        HAGGLE_DBG("Node %s returned as potential delegate for data object %s\n",
					   actual_delegate->getName().c_str(), DataObject::idString(dObj).c_str());
				// Forward the message via this neighbor:
			        NodeRef actual_target;
				if (shouldForward(dObj, actual_delegate, actual_target) && isNeighbor(actual_target)) {
				  // MOS - checking neighbor now after possible translation in shouldForward
					if (addToSendList(dObj, actual_target))
                                                ns.push_front(actual_target);
                                }
				/*
					There is no checking for if this delegate has the data
					object or the data object is being sent. This is because
					shouldForward() does that check.
				*/
			}
		}
	} while (delegate);

    if (!ns.empty()) {
        addForwardingMetadata(dObj); // MOS - was missing here
        addForwardingMetadataDelegated(dObj); // MOS

	// kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, ns));
	// MOS - adding randomized forwarding delay for desynchronization
	for (NodeRefList::iterator it = ns.begin(); it != ns.end(); it++) {
	    NodeRefList nl;
	    nl.push_back(*it);
	    double delay = randomizedForwardingDelay(ns,dObj);
// SW: START CONSUME INTEREST
            { 
                NodeRef intendedTarget = e->getNode();
                if (intendedTarget && (intendedTarget->getType() == Node::TYPE_APPLICATION)) {
                    kernel->addEvent(new Event(EVENT_TYPE_ROUTE_TO_APP, dObj, intendedTarget));
                }
            }
// SW: END CONSUME INTEREST
	    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, nl, delay));
	}
    }

    delete delegates;
}

#if 0
void ForwardingManager::onSendRoutingInformation(Event * e)
{
	// Do we have a forwarding module?
	if (!forwardingModule) 
		return;
	
	// Do we have a metric DO?
	DataObjectRef dObj = forwardingModule->createForwardingDataObject();
	
	if (dObj) {
		// Yes. Send it to all neighbors that don't already have it:
		NodeRefList nodes;
		
		// Get all neighbors:
		kernel->getNodeStore()->retrieveNeighbors(nodes);
		
		NodeRefList ns;
		// For each neighbor:
		for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
			// Send to that neighbor:
			NodeRef actual_target;
			if (shouldForward(dObj, *it, actual_target) && isNeighbor(actual_target)) {
			  // MOS - checking neighbor now after possible translation in shouldForward
				if (addToSendList(dObj, actual_target))
					ns.push_front(actual_target);
			}
		}
		if (!ns.empty()) {
			/*
			 Here, we don't send a _WILL_SEND event, because these data 
			 objects are not meant to be modified, and should eventually 
			 be removed entirely, in favor of using the node description 
			 to transmit the forwarding metric.
			 */
			kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, ns));
		}
	}
}
#endif

// SW: START: punts to avoid init and onConfig race condition
void ForwardingManager::onConfigPunt(Event *e) {

    if ((NULL == e) || !e->hasData()) {
        HAGGLE_ERR("Invalid event\n");
        return;
    }

    Metadata *m = static_cast<Metadata*>(e->getData());
    if (NULL == m) {
        HAGGLE_ERR("Could not get metadata\n");
        return;
    }

    onConfigImplementation(m);
}
// SW: END: punts to avoid init and onConfig race condition

void ForwardingManager::onConfig(Metadata *m) {
    // we clone the metadata to avoid a memleak 
    m = m->copy();
    // SW: check if it's really time to fire onConfig, or should we wait?
    if ((NULL != forwardingModule) && !forwardingModule->isInitialized())  {
        // SW: we are not done reading from the repository, 
        // punt this event to a later date to avoid a race condition
        // where we switch forwarders before the first forwarder has
        // fired the repository callback
	    getKernel()->addEvent(new Event(on_config_punt_event, m, PUNT_DELAY_CONFIG_S));
        HAGGLE_DBG("Punting onConfig event for %d seconds\n", PUNT_DELAY_CONFIG_S);
        return;
    } 
    onConfigImplementation(m);
}

/*
 handled configurations:
 - <ForwardingModule>noResolution</ForwardingModule>	(no resolution, nothing!)
 - <ForwardingModule>Prophet</ForwardingModule>		(Prophet)
 - <ForwardingModule>noForward</ForwardingModule>	(no forwarding)
 
 default forwarding is defined in ForwardingManager::init_derived(), at the moment Prophet 
 */
// SW: changed name (onConfig => onConfigImplementation) to enable punting on onConfig
void ForwardingManager::onConfigImplementation(Metadata *m) {
    const char *param;

    if (NULL == m) {
        HAGGLE_ERR("Null metadata to onConfig\n");
        return;
    }

	param = m->getParameter("enable_forwarding_metadata");
	
	if (param) {
		if (strcmp(param, "true") == 0) {
			HAGGLE_DBG("Enabling forwarding metadata\n");
			enableForwardingMetadata = true;
		} else if (strcmp(param, "false") == 0) {
			HAGGLE_DBG("Disabling forwarding metadata\n");
			enableForwardingMetadata = false;
		}
	}

    param = m->getParameter("recursive_routing_updates");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling recursive routing updates\n");
            recursiveRoutingUpdates = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling recursive routing updates\n");
            recursiveRoutingUpdates = false;
        }
    }

	param = m->getParameter("query_on_new_dataobject");

	if (param) {
		if (strcmp(param, "true") == 0) {
			HAGGLE_DBG("Enabling query on new data object\n");
			doQueryOnNewDataObject = true;
		} else if (strcmp(param, "false") == 0) {
			HAGGLE_DBG("Disabling query on new data object\n");
			doQueryOnNewDataObject = false;
		}
		
		LOG_ADD("# %s: query_on_new_dataobject=%s\n", 
			getName(), doQueryOnNewDataObject ? "true" : "false");
	}

	param = m->getParameter("periodic_dataobject_query_interval");
	if (param) {		
		char *endptr = NULL;
		unsigned long period = strtoul(param, &endptr, 10);
		
		if (endptr && endptr != param) {
			if (periodicDataObjectQueryInterval == 0 && 
			    period > 0 && 
			    kernel->getNodeStore()->numNeighbors() > 0 &&
			    !periodicDataObjectQueryEvent->isScheduled()) {
				kernel->addEvent(new Event(periodicDataObjectQueryCallback, 
							   NULL, 
							   periodicDataObjectQueryInterval));
			} 
			periodicDataObjectQueryInterval = period;
			HAGGLE_DBG("periodic_dataobject_query_interval=%lu\n", 
				   periodicDataObjectQueryInterval);
			LOG_ADD("# %s: periodic_dataobject_query_interval=%lu\n", 
				getName(), periodicDataObjectQueryInterval);
		}
	}

	param = m->getParameter("periodic_routing_update_interval");

	if (param) {		
		char *endptr = NULL;
		unsigned long period = strtoul(param, &endptr, 10);
		
		if (endptr && endptr != param) {
			if (periodicRoutingUpdateInterval == 0 && 
			    period > 0 && 
			    kernel->getNodeStore()->numNeighbors() > 0 &&
			    !periodicRoutingUpdateEvent->isScheduled()) {
				kernel->addEvent(new Event(periodicRoutingUpdateCallback, 
							   NULL, 
							   periodicRoutingUpdateInterval));
			} 
			periodicRoutingUpdateInterval = period;
			HAGGLE_DBG("periodic_routing_update_interval=%lu\n", 
				   periodicRoutingUpdateInterval);
			LOG_ADD("# %s: periodic_routing_update_interval=%lu\n", 
				getName(), periodicRoutingUpdateInterval);
		}
	}

	param = m->getParameter("max_nodes_to_find_for_new_dataobjects"); // MOS - added parameter

	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);
		
		if (endptr && endptr != param) {
		  maxNodesToFindForNewDataObjects = value;
		  HAGGLE_DBG("max_nodes_to_find_for_new_dataobjects=%lu\n", 
			     maxNodesToFindForNewDataObjects);
		  LOG_ADD("# %s: max_nodes_to_find_for_new_dataobjects=%lu\n", 
			  getName(), maxNodesToFindForNewDataObjects);
		}
	}

// SW: START NEW NBR NODE DESCRIPTION PUSH:
    param = m->getParameter("push_node_descriptions_on_contact");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling push of node descriptions\n");
            doPushNodeDescriptions = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling push of node descriptions\n");
            doPushNodeDescriptions = false;
        }

        LOG_ADD("# %s: push_node_descriptions_on_contact=%s\n",
                getName(), doPushNodeDescriptions ? "true" : "false");
    }
// SW: END NEW NBR NODE DESCRIPTION PUSH.
// MOS: START ENABLE GENERATE TARGETS:
    param = m->getParameter("enable_target_generation");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling target generation\n");
            enableTargetGeneration = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling target generation\n");
            enableTargetGeneration = false;
        }

        LOG_ADD("# %s: enable_target_generation=%s\n",
                getName(), enableTargetGeneration ? "true" : "false");
    }
// MOS: END ENABLE GENERATE TARGETS

// MOS: START RANDOMIZED FORWARDING DELAY
	param = m->getParameter("max_node_desc_forwarding_delay_linear");
	if(!param) param = m->getParameter("max_node_desc_forwarding_delay"); // MOS - synonymous for backward compatibility

	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
			maxNodeDescForwardingDelayLinearMs = value;
			HAGGLE_DBG("max_node_desc_forwarding_delay_linear=%lu\n", 
				   maxForwardingDelayLinearMs);
			LOG_ADD("# %s: max_node_desc_forwarding_delay_linear=%lu\n", 
				getName(), maxNodeDescForwardingDelayLinearMs);
		}
	}

	param = m->getParameter("max_node_desc_forwarding_delay_base");

	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
			maxNodeDescForwardingDelayBaseMs = value;
			HAGGLE_DBG("max_node_desc_forwarding_delay_base=%lu\n", 
				   maxNodeDescForwardingDelayBaseMs);
			LOG_ADD("# %s: max_node_desc_forwarding_delay_base=%lu\n", 
				getName(), maxNodeDescForwardingDelayBaseMs);
		}
	}

	param = m->getParameter("max_forwarding_delay_linear");
	if(!param) param = m->getParameter("max_forwarding_delay"); // MOS - synonymous for backward compatibility

	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
			maxForwardingDelayLinearMs = value;
			HAGGLE_DBG("max_forwarding_delay_linear=%lu\n", 
				   maxForwardingDelayLinearMs);
			LOG_ADD("# %s: max_forwarding_delay_linear=%lu\n", 
				getName(), maxForwardingDelayLinearMs);
		}
	}

	param = m->getParameter("max_forwarding_delay_base");

	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
			maxForwardingDelayBaseMs = value;
			HAGGLE_DBG("max_forwarding_delay_base=%lu\n", 
				   maxForwardingDelayBaseMs);
			LOG_ADD("# %s: max_forwarding_delay_base=%lu\n", 
				getName(), maxForwardingDelayBaseMs);
		}
	}
// MOS: END RANDOMIZED FORWARDING DELAY

	param = m->getParameter("dataobject_retries"); // MOS - added parameter
		
	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
		  dataObjectRetries = value;
		  HAGGLE_DBG("dataobject_retries=%lu\n", 
			     dataObjectRetries);
		  LOG_ADD("# %s: dataobject_retries=%lu\n", 
			  getName(), dataObjectRetries);
		}
	}

	param = m->getParameter("dataobject_retries_shortcircuit"); // MOS - added parameter
		
	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
		  dataObjectRetriesShortCircuit = value;
		  HAGGLE_DBG("dataobject_retries_shortcircuit=%lu\n", 
			     dataObjectRetriesShortCircuit);
		  LOG_ADD("# %s: dataobject_retries_shortcircuit=%lu\n", 
			  getName(), dataObjectRetriesShortCircuit);
		}
	}

	param = m->getParameter("node_description_retries"); // MOS - added parameter

	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);
		
		if (endptr && endptr != param) {
		  nodeDescriptionRetries = value;
		  HAGGLE_DBG("node_description_retries=%lu\n", 
			     nodeDescriptionRetries);
		  LOG_ADD("# %s: node_description_retries=%lu\n", 
			  getName(), nodeDescriptionRetries);
		}
	}

       param = m->getParameter("load_reduction_min_queue_size"); // MOS - added parameter
               
        if (param) {            
                char *endptr = NULL;
                unsigned long value = strtoul(param, &endptr, 10);

                if (endptr && endptr != param) {
                  loadReductionMinQueueSize = value;
		  HAGGLE_DBG("load_reduction_min_queue_size=%lu\n", 
			     loadReductionMinQueueSize);
		}
	}

	param = m->getParameter("load_reduction_max_queue_size"); // MOS - added parameter
		
	if (param) {		
		char *endptr = NULL;
		unsigned long value = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
		  loadReductionMaxQueueSize = value;
		  HAGGLE_DBG("load_reduction_max_queue_size=%lu\n", 
			     loadReductionMaxQueueSize);
		}
	}

    param = m->getParameter("neighbor_forwarding_shortcut");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling neighbor forwarding shortcut\n");
            neighborForwardingShortcut = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling neighbor forwarding shortcut\n");
            neighborForwardingShortcut = false;
        }

        LOG_ADD("# %s: neighbor_forwarding_shortcut=%s\n",
                getName(), neighborForwardingShortcut ? "true" : "false");
    }

	param = m->getParameter("enable_multihop_bloomfilters");
	
// SW: START: 1-hop bloomfilter extension
	if (param) {
		if (strcmp(param, "true") == 0) {
			HAGGLE_DBG("Enabling multihop bloomfilters\n");
			enableMultihopBloomfilters = true;
		} else if (strcmp(param, "false") == 0) {
			HAGGLE_DBG("Disabling multihop bloomfilters\n");
			enableMultihopBloomfilters = false;
		}
	}
// SW: END: 1-hop bloomfilter extension

// SW: START FORWARDER FACTORY:
    bool resolutionDisabled = false;
    Forwarder* forwarder = ForwarderFactory::getNewForwarder(this,
            moduleEventType, m, &resolutionDisabled);
    if (!forwarder) {
        noForwarder = true;
        // in this case there is no repository callback
	    getKernel()->addEvent(new Event(on_punt_init_event, NULL, PUNT_DELAY_INIT_S));
    }
    setForwardingModule(forwarder, resolutionDisabled);
// SW: END FORWARDER FACTORY.

    // SW: avoid mem leak on metadata
    delete m;
}
