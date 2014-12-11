/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Joshua Joy (JJ, jjoy)
 *   James Mathewson (JM, JLM)
 *   Yu-Ting Yu (yty)
 *   Hasnain Lakhani (HL)
 *   Jihwa Lee (JL)
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
#include <stdlib.h>
#include <time.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <haggleutils.h>

#include "EventQueue.h"
#include "NodeManager.h"
#include "DataObject.h"
#include "Node.h"
#include "Event.h"
#include "Attribute.h"
#include "Interface.h"
#include "Filter.h"

// SW: START REPLICATION MANAGER
#include "ReplicationManagerFactory.h"
// SW: END REPLICATION MANAGER

using namespace haggle;

// #define MERGE_BLOOMFILTERS // MOS - disable merging
// MOS - Merging current peer bloomfilters with bloomfilters from incoming
// node descriptions is problematic because we rely on periodic refresh to
// get peer bloomfilters in sync with reality (Haggle is only adding, never
// removing). Full periodic flooing of BFs is not be needed for this
// but helps to suppress redundant transmisions in certain situation
// (need more experimentation to understand the cost/benefit).

#define FILTER_KEYWORD NODE_DESC_ATTR
#define FILTER_NODEDESCRIPTION FILTER_KEYWORD "=" ATTR_WILDCARD

#define DEFAULT_NODE_DESCRIPTION_RETRY_WAIT (10.0) // Seconds
#define DEFAULT_NODE_DESCRIPTION_RETRIES (3)

NodeManager::NodeManager(HaggleKernel * _haggle) : 
	Manager("NodeManager", _haggle), 
// SW: START: include 1-hop neighborhood  in ND metadata
    send_neighborhood(false),
// SW: END: include 1-hop neighborhood  in ND metadata.
    sendAppNodeDOs(true),
	thumbnail_size(0), thumbnail(NULL),
	nodeDescriptionRetries(DEFAULT_NODE_DESCRIPTION_RETRIES),
	nodeDescriptionRetryWait(DEFAULT_NODE_DESCRIPTION_RETRY_WAIT),
// SW: START REFRESH:
    periodicNodeDescriptionRefreshEventType(-1),
    periodicNodeDescriptionRefreshEvent(NULL),
    nodeDescriptionRefreshPeriodMs(0),
    nodeDescriptionRefreshJitterMs(0),
// SW: END REFRESH.
// SW: START PURGE:
    periodicNodeDescriptionPurgeEventType(-1),
    periodicNodeDescriptionPurgeEvent(NULL),
    nodeDescriptionPurgeMaxAgeMs(0),
    nodeDescriptionPurgePollPeriodMs(0),
// SW: END PURGE.
// JM: START SOCIAL GROUP LABEL
    nodeSocialName(""),
    useSocialName(false),
// JM: END Social Group label
    acceptNeighborNodeDescriptionsFromThirdParty(false)
{
}

NodeManager::~NodeManager()
{
	if (onRetrieveNodeCallback)
		delete onRetrieveNodeCallback;
	
	if (onRetrieveThisNodeCallback)
		delete onRetrieveThisNodeCallback;

	if (onInsertedNodeCallback)
		delete onInsertedNodeCallback;

// SW: START REFRESH:
    Event::unregisterType(periodicNodeDescriptionRefreshEventType);
    if (periodicNodeDescriptionRefreshEvent) {
        if (periodicNodeDescriptionRefreshEvent->isScheduled()) {
            periodicNodeDescriptionRefreshEvent->setAutoDelete(true);
        } else {
            delete periodicNodeDescriptionRefreshEvent;
        }        
    }
// SW: END REFRESH.
// SW: START PURGE
    Event::unregisterType(periodicNodeDescriptionPurgeEventType);
    if (periodicNodeDescriptionPurgeEvent) {
        if (periodicNodeDescriptionPurgeEvent->isScheduled()) {
            periodicNodeDescriptionPurgeEvent->setAutoDelete(true);
        } else {
            delete periodicNodeDescriptionPurgeEvent;
        }
    }
// SW: END PURGE
}

bool NodeManager::init_derived()
{
#define __CLASS__ NodeManager
	int ret;

	// Register filter for node descriptions
	registerEventTypeForFilter(nodeDescriptionEType, "NodeDescription", onReceiveNodeDescription, FILTER_NODEDESCRIPTION);

	ret = setEventHandler(EVENT_TYPE_LOCAL_INTERFACE_UP, onLocalInterfaceUp);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_LOCAL_INTERFACE_DOWN, onLocalInterfaceDown);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NEIGHBOR_INTERFACE_UP, onNeighborInterfaceUp);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN, onNeighborInterfaceDown);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_NEW, onNewNodeContact);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NODE_DESCRIPTION_SEND, onSendNodeDescription);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onSendResult);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, onSendResult);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	onRetrieveNodeCallback = newEventCallback(onRetrieveNode);
	onRetrieveThisNodeCallback = newEventCallback(onRetrieveThisNode);
	onInsertedNodeCallback = newEventCallback(onInsertedNode);

	kernel->getDataStore()->retrieveNode(kernel->getThisNode(), onRetrieveThisNodeCallback);

// SW: START REFRESH:
    periodicNodeDescriptionRefreshEventType = registerEventType("Periodic Node Description Refresh Event", 
        onPeriodicNodeDescriptionRefresh);

    if (periodicNodeDescriptionRefreshEventType < 0) {
        HAGGLE_ERR("Could not register periodic node description refresh event type...\n");
        return false;
    }

	periodicNodeDescriptionRefreshEvent = new Event(periodicNodeDescriptionRefreshEventType);

    if (!periodicNodeDescriptionRefreshEvent) {
        return false;
    }

    periodicNodeDescriptionRefreshEvent->setAutoDelete(false);

// SW: END REFRESH.
// SW; START PURGE
     periodicNodeDescriptionPurgeEventType = registerEventType("Periodic Node Description Purge Event", 
        onPeriodicNodeDescriptionPurge);

    if (periodicNodeDescriptionPurgeEventType < 0) {
        HAGGLE_ERR("Could not register periodic node description purge event type...\n");
        return false;
    }

	periodicNodeDescriptionPurgeEvent = new Event(periodicNodeDescriptionPurgeEventType);

    if (!periodicNodeDescriptionPurgeEvent) {
        return false;
    }

    periodicNodeDescriptionPurgeEvent->setAutoDelete(false);
  
// SW: END PURGE
	
	/*
		We only search for a thumbnail at haggle startup time, to avoid 
		searching the disk every time we create a new node description.
	*/
	/*
		Search for and (if found) load the avatar image for this node.
	*/

	string str = HAGGLE_DEFAULT_STORAGE_PATH;
	str += PLATFORM_PATH_DELIMITER;
	str += "Avatar.jpg";
	FILE *fp = fopen(str.c_str(), "r");
        
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		thumbnail_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		thumbnail = (char *) malloc(thumbnail_size);
	
                if (thumbnail == NULL) {
			thumbnail_size = 0;
		} else {
			if (fread(thumbnail, 1, thumbnail_size, fp) != (size_t) thumbnail_size) {
                                free(thumbnail);
                                thumbnail = NULL;
                        }
		}
		fclose(fp);
	}

	if (thumbnail == NULL){
		HAGGLE_DBG("No avatar image found.\n");
	} else {
		HAGGLE_DBG("Found avatar image. Will attach to all node descriptions\n");
	}
	
	return true;
}

void NodeManager::onPrepareShutdown()
{
	// Remove the node description filter from the data store:
	unregisterEventTypeForFilter(nodeDescriptionEType);
	// Save the "this node" node in the data store so it can be retrieved when 
	// we next start up.
	kernel->getDataStore()->insertNode(kernel->getThisNode());
	
	// We're done:
	signalIsReadyForShutdown();
}

#if defined(ENABLE_METADAPARSER)
bool NodeManager::onParseMetadata(Metadata *md)
{         
        HAGGLE_DBG("NodeManager onParseMetadata()\n");

        // FIXME: should check 'Node' section of metadata
        return true;
}
#endif

void NodeManager::onRetrieveThisNode(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	NodeRef node = e->getNode();
	
	// If we found a "this node" in the data store:
	if (node) {
		// try to update the node store with that node:
		if (kernel->getNodeStore()->update(node)) {
			// Success! update the hagglekernel's reference, too.
			kernel->setThisNode(node);
		}
	}
	// Update create time to mark the freshness of the thisNode node description
	kernel->getThisNode()->setNodeDescriptionCreateTime();
}

bool NodeManager::isInSendList(const NodeRef& node, const DataObjectRef& dObj)
{
	for (SendList_t::iterator it = sendList.begin(); it != sendList.end(); it++) {
		if ((*it).first == node && (*it).second.dObj == dObj) {
			return true;
		}
	}
	return false;
}

// MOS - add node description to send list and mark previous ones as obsolete

void NodeManager::addToSendList(const NodeRef& node, const DataObjectRef& dObj)
{
	if(kernel->getNodeStore()->preemptLocalObsoleteNodeDescriptions()) {
		for (SendList_t::iterator it = sendList.begin(); it != sendList.end(); it++) {
			const char* nodeIdOld = (*it).second.dObj->getMetadata()->getMetadata(NODE_METADATA)->getParameter(NODE_METADATA_ID_PARAM);
			const char* nodeIdNew = dObj->getMetadata()->getMetadata(NODE_METADATA)->getParameter(NODE_METADATA_ID_PARAM);

			// To decide which one is obsolete, try to match target node and id's of node descriptions
			if ((*it).first == node && strcmp(nodeIdOld, nodeIdNew) == 0) {
			  (*it).second.dObj->setObsolete();
			  HAGGLE_DBG("Node description %s marked obsolete\n", (*it).second.dObj->getIdStr());
		  }
		}
	}

  SendEntry_t se = { dObj, 0 };
  // Remember that we tried to send our node description to this node:
  sendList.push_back(Pair<NodeRef, SendEntry_t>(node, se));
}

// MOS - generalized to take node argument

int NodeManager::sendIndividualNodeDescription(NodeRef node, NodeRefList& neighList)
{
	NodeRefList targetList;

	// DataObjectRef dObj = node->getDataObject()->copy(); // MOS - need to make a copy to allow for concurrent modifications
	DataObjectRef dObj = node->getDataObject(); // MOS - copy causes major inefficiency (because each copy would go into the send queue)

	HAGGLE_DBG("Pushing node description [%s] to %lu neighbors\n", dObj->getIdStr(), neighList.size());

	if (thumbnail != NULL)
		dObj->setThumbnail(thumbnail, thumbnail_size);
	
	for (NodeRefList::iterator it = neighList.begin(); it != neighList.end(); it++) {
		NodeRef& neigh = *it;
	
		if (neigh->getType() != Node::TYPE_UNDEFINED && // MOS - optimistic addition to BF prevents retransmission without this
		    neigh->has(dObj)) {
			HAGGLE_DBG("Neighbor %s already has our most recent node description\n", neigh->getName().c_str());

		} else if (!isInSendList(neigh, dObj)) {
			HAGGLE_DBG("Sending node description [%s] to \'%s\', bloomfilter #objs=%lu\n", 
				   DataObject::idString(dObj).c_str(), neigh->getName().c_str(), node->getBloomfilter()->numObjects());
			targetList.push_back(neigh);
			addToSendList(neigh,dObj);

			HAGGLE_DBG("akl: add neighbor to target list\n");

		} else {
			HAGGLE_DBG("Node description [%s] is already in send list for neighbor %s\n",
				DataObject::idString(dObj).c_str(), neigh->getName().c_str());
		}
	}

// SW: START: include 1-hop neighborhood  in ND metadata
    if (send_neighborhood && 
	node->getType() != Node::TYPE_APPLICATION) { // MOS

        NodeRefList nbrs;
        kernel->getNodeStore()->retrieveNeighbors(nbrs);
        bool addedMetadata = false;
        Metadata *m =  NULL;
        int numNbrsAdded = 0;
	dObj.lock(); // MOS - important
        for (NodeRefList::iterator it = nbrs.begin(); it != nbrs.end(); it++) {
            NodeRef& nbr = *it;
            if (Node::TYPE_PEER == nbr->getType()) {
                if (!addedMetadata) {
                    m = dObj->getMetadata()->addMetadata(getName());
                    m->removeMetadata("Neighbors");  // MOS
                    m = m->addMetadata("Neighbors");
                    addedMetadata = true;
                    if (!m) {
                        HAGGLE_ERR("Could not generate metadata\n");
                        break;
                    }
                }
                m->addMetadata("Neighbor", nbr->getIdStr());
                numNbrsAdded++;
            }
        }
	dObj.unlock();
        HAGGLE_DBG("Added %d neighbors to node description\n", numNbrsAdded);
    }
// SW: END: include 1-hop neighborhood  in ND metadata.

// JM:  Add social group ID, if defined
    Metadata *m=NULL;
    if (useSocialName && 
	node->getType() != Node::TYPE_APPLICATION) { 
	dObj.lock(); 
        m = dObj->getMetadata()->addMetadata(string("SocialGroups")); 
        m->removeMetadata("SocialId");  
        m->setParameter("SocialId", nodeSocialName);
	dObj.unlock();

        HAGGLE_DBG("Added social name %s to node description\n", nodeSocialName.c_str());
    }
// JM: END Add social group ID, if defined
	
	if (targetList.size()) {
		if (kernel->isRMEnabled()) {
		// SW: START REPLICATION MANAGER
			kernel->addEvent(ReplicationManagerFactory::getSendNodeDescriptionEvent(dObj, targetList));
		} else {
		// SW: END REPLICATION MANAGER
		// if RM not invoked, keep same functionality
			kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, targetList));
		}

		HAGGLE_DBG("akl: set send event\n");

	} else {
		HAGGLE_DBG("All neighbors already had our most recent node description\n");
	}

	
	return 1;
}

// MOS - one may think of app nodes as part of our own node description

int NodeManager::sendNodeDescription(NodeRefList& neighList)
{
    sendIndividualNodeDescription(kernel->getThisNode(), neighList); // MOS - ideally bfs go out first

    // SW: let InterestManager manage propagation of application node descriptions
    if(kernel->firstClassApplications() && sendAppNodeDOs) {
    //if(kernel->firstClassApplications()) {
      NodeRefList localApplications;
      kernel->getNodeStore()->retrieveLocalApplications(localApplications);
      for (NodeRefList::iterator it = localApplications.begin(); it != localApplications.end(); it++) {
        // SW: only include applications with interests
        NodeRef app = (*it);
        const Attributes *attrs = app->getAttributes();
        if (attrs && attrs->size() > 0) {
	      sendIndividualNodeDescription(*it, neighList);
        }
      }
    }
    return 1;
}

void NodeManager::onSendResult(Event *e)
{
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring send data object results\n");
          return;
        }

	NodeRef neigh = kernel->getNodeStore()->retrieve(e->getNode(), false);
	DataObjectRef &dObj = e->getDataObject();
	
	if (!neigh) {
		neigh = e->getNode();

		if (!neigh) {
			HAGGLE_ERR("No node in send result\n");
			return;
		}
	}
	// Go through all our data regarding current node exchanges:
	for (SendList_t::iterator it = sendList.begin(); it != sendList.end(); it++) {
		// Is this the one?
		if ((*it).first == neigh && (*it).second.dObj == dObj) {
			// Yes.
			
			// Was the exchange successful?
			if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL) {
				// Yes. Set the flag.
				neigh->setExchangedNodeDescription(true);
				
				if (!(*it).second.dObj->isObsolete()) {
					HAGGLE_DBG("Successfully sent node description [%s] to neighbor %s [%s], after %lu retries\n",
						   DataObject::idString(dObj).c_str(), neigh->getName().c_str(), neigh->getIdStr(), (*it).second.retries);
					//dObj->print();
				} else {
					// For more accurate log message.
					HAGGLE_DBG("Node description [%s] is not sent to neighbor %s [%s] - it is obsolete\n",
							DataObject::idString(dObj).c_str(), neigh->getName().c_str(), neigh->getIdStr());
				}
				
				sendList.erase(it);
			} else if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_FAILURE) {
				// No. Unset the flag.
				neigh->setExchangedNodeDescription(false);

				(*it).second.retries++;

				// Retry
				if ((*it).second.retries <= nodeDescriptionRetries && neigh->isNeighbor()) {
					HAGGLE_DBG("Sending node description [%s] to neighbor %s [%s], retry=%u\n", 
						DataObject::idString(dObj).c_str(), neigh->getName().c_str(), neigh->getIdStr(), (*it).second.retries);
					// Retry, but delay for some seconds.
					kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, neigh, 0, nodeDescriptionRetryWait));
				} else {
					// Remove this entry from the list:
					HAGGLE_DBG("FAILED to send node description to neighbor %s [%s] after %u retries...\n",
						neigh->getName().c_str(), neigh->getIdStr(), (*it).second.retries);
					sendList.erase(it);
				}
			}
			// Done, no need to look further.
			return;
		}
	}
}

void NodeManager::onLocalInterfaceUp(Event * e)
{
	kernel->getThisNode()->addInterface(e->getInterface());
}

void NodeManager::onLocalInterfaceDown(Event *e)
{
	kernel->getThisNode()->setInterfaceDown(e->getInterface());
}

void NodeManager::onNeighborInterfaceUp(Event *e)
{
	NodeRef neigh = kernel->getNodeStore()->retrieve(e->getInterface(), true);
	if (!neigh) {
		// No one had that interface?

		// merge if node exists in datastore (asynchronous call), we force it to return
		// as we only generate a node up event when we know we have the best information
		// for the node.
		HAGGLE_DBG("No active neighbor with interface [%s], retrieving node from data store\n", 
			e->getInterface()->getIdentifierStr());

		kernel->getDataStore()->retrieveNode(e->getInterface(), onRetrieveNodeCallback, true);
	} else {
		HAGGLE_DBG("Neighbor %s has new active interface [%s], setting to UP\n", 
			neigh->getName().c_str(), e->getInterface()->getIdentifierStr());

		neigh->setInterfaceUp(e->getInterface());
	}
}

/* 
	callback on retrieve node from Datastore
 
	called in NodeManager::onNeighborInterfaceUp 
	to retrieve a previously known node that matches an interface from an interface up event.
*/
void NodeManager::onRetrieveNode(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	NodeRef& node = e->getNode();

	if (!node) {
		InterfaceRef& iface = e->getInterface();
		
		if (!iface) {
			HAGGLE_ERR("Neither node nor interface in callback\n");
			return;
		}
		
		node = Node::create();
		
		if (!node) {
			HAGGLE_ERR("Could not create peer node\n");
			return;
		}
		node->addInterface(iface);
		
	} else {
		HAGGLE_DBG("Neighbor node %s has %lu objects in bloomfilter\n", 
			   node->getName().c_str(), node->getBloomfilter()->numObjects());
	}
	
	// See if this node is already an active neighbor but in an uninitialized state
	if (kernel->getNodeStore()->update(node)) {
		HAGGLE_DBG("Node %s [%s] was updated in node store\n", node->getName().c_str(), node->getIdStr());
	} else {
		HAGGLE_DBG("Node %s [%s] not previously neighbor... Adding to node store\n", node->getName().c_str(), node->getIdStr());
		kernel->getNodeStore()->add(node);
	}
	
	kernel->addEvent(new Event(EVENT_TYPE_NODE_CONTACT_NEW, node));
}

void NodeManager::onNeighborInterfaceDown(Event *e)
{
	// Let the NodeStore know:
	NodeRef node = kernel->getNodeStore()->retrieve(e->getInterface(), false);
	
	if (node) {
		node->setInterfaceDown(e->getInterface());
		
		if (!node->isAvailable()) {
		  // MOS - consistent with Sam's modification (see MOS below) we should not remove nodes 
		  //       (app nodes are an exception, see ApplicationManager)
		  // kernel->getNodeStore()->remove(node); 

			/*
				We need to update the node information in the data store 
				since the bloomfilter might have been updated during the 
				neighbor's co-location.
			*/
			kernel->getDataStore()->insertNode(node);

			// Report the node as down
			// SW: START REPLICATION MANAGER
			kernel->addEvent(ReplicationManagerFactory::getNodeStatsEvent(node, Timeval::now() - node->getCreateTime()));
			// SW: END REPLICATION MANAGER
			kernel->addEvent(new Event(EVENT_TYPE_NODE_CONTACT_END, node));
		}
	}
}

void NodeManager::onNewNodeContact(Event *e)
{
	NodeRefList neighList;
	
	if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, ignoring new node contact\n");
		return;
	}
	
	if (!e)
		return;

	NodeRef neigh = e->getNode();

	switch (neigh->getType()) {
	case Node::TYPE_UNDEFINED:
		HAGGLE_DBG("%s - New node contact. Have not yet received node description!\n", getName());
		break;
	case Node::TYPE_PEER:
		HAGGLE_DBG("%s - New node contact %s [id=%s]\n", getName(), neigh->getName().c_str(), neigh->getIdStr());
		break;
	case Node::TYPE_GATEWAY:
		HAGGLE_DBG("%s - New gateway contact %s\n", getName(), neigh->getIdStr());
		break;
	default:
		break;
	}

	neighList.push_back(neigh);

	// MOS - the following line is strictly not necessary, but not sending
        // the node description on contact leads to unnecessary
	// delaying of data object query in forwarding manager
	kernel->getThisNode()->setNodeDescriptionCreateTime();
	sendNodeDescription(neighList);

	// JM - add new nodes to social list
	kernel->getNodeStore()->insertNodeSocialGroup(neigh);
}

// SW: START REFRESH:

/*
 * Helper to send node description to neighbors.
 */
void NodeManager::pushNodeDescriptionToNeighbors() 
{
    NodeRefList neighList;

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, not sending node description\n");
        return;
    }

    unsigned long num = kernel->getNodeStore()->retrieveNeighbors(neighList);

    if (num <= 0) {
        HAGGLE_DBG("No neighbors - not sending node description\n");
        return;
    }

    sendNodeDescription(neighList);
}

// MOS - There is an issue below with setting the create time without
// adding the new node desc to my local BF (which is now reserved for
// data objects that are not node descriptions), because with some bad
// luck they can come back through a thirsh node (in which case they
// would be ignored). Need to see if this is a problem in practice.

void NodeManager::pushNewNodeDescriptionToNeighbors()
{
    if(kernel->firstClassApplications()) {
      NodeRefList localApplications;
      kernel->getNodeStore()->retrieveLocalApplications(localApplications);
      for (NodeRefList::iterator it = localApplications.begin(); it != localApplications.end(); it++)
	(*it)->setNodeDescriptionCreateTime();
    }
    kernel->getThisNode()->setNodeDescriptionCreateTime();
    pushNodeDescriptionToNeighbors();        
}

void NodeManager::onPeriodicNodeDescriptionRefresh(Event * e)
{
    if (!e) {
        HAGGLE_ERR("Null event.\n");
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring refresh.\n");
        return;
    }

    double jitter = rand() % nodeDescriptionRefreshJitterMs;

    // timeout uses seconds
    double nextTimeout = (nodeDescriptionRefreshPeriodMs + jitter) / (double) 1000;

     
    pushNewNodeDescriptionToNeighbors();

    if (!periodicNodeDescriptionRefreshEvent->isScheduled() 
        && (nextTimeout > 0)) {
        // schedule next refresh event 
        periodicNodeDescriptionRefreshEvent->setTimeout(nextTimeout);
        kernel->addEvent(periodicNodeDescriptionRefreshEvent);
    }
}

// SW: END REFRESH.

// SW: START PURGE:

void NodeManager::onPeriodicNodeDescriptionPurge(Event * e)
{
    if (!e) {
        HAGGLE_ERR("Null event.\n");
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring purge.\n");
        return;
    }

    NodeRefList allNodesRefList;
    kernel->getNodeStore()->retrieveAllNodes(allNodesRefList);

    // TODO: we may be able to push this into the criteria  (and the very least
    // retrieving the old node descriptions
    for (NodeRefList::iterator it = allNodesRefList.begin(); it != allNodesRefList.end(); it++) {
        NodeRef& node = *it;
        unsigned long nowMs = (unsigned long) Timeval::now().getTimeAsMilliSeconds();
        unsigned long ageMs =  nowMs - (unsigned long)node->getCreateTime().getTimeAsMilliSeconds();
        // purge stale nodes that are _not_ neighbors 
        if ((ageMs > 0) && (ageMs > nodeDescriptionPurgeMaxAgeMs)
            && (node->getType() == Node::TYPE_PEER || node->getType() == Node::TYPE_APPLICATION) 
	    && !node->isNeighbor() && !node->isLocalApplication()) {
	    HAGGLE_DBG("purging stale node description of %s [%s] (age: %d ms).\n", node->getName().c_str(), node->getIdStr(), ageMs);
	    kernel->getDataStore()->insertNode(node); // MOS - insert node to keep its state for next encounter
            kernel->getNodeStore()->remove(node);
    	    //JM if we purge, remove from node social list
    	    kernel->getNodeStore()->removeNodeSocialGroup(node);
        }
    }

    // timeout uses seconds
    double nextTimeout = (nodeDescriptionPurgePollPeriodMs / (double) 1000);

    if (!periodicNodeDescriptionPurgeEvent->isScheduled()
        && (nextTimeout > 0)) {
        periodicNodeDescriptionPurgeEvent->setTimeout(nextTimeout);
        kernel->addEvent(periodicNodeDescriptionPurgeEvent);
    }

}

// SW: END PURGE.

// SW: Push our node description to all neighbors
void NodeManager::onSendNodeDescription(Event *e)
{
	HAGGLE_DBG("akl: send node description\n");
    pushNodeDescriptionToNeighbors();        
}


void NodeManager::onReceiveNodeDescription(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, ignoring new node description\n");
		return;
	}
	

	DataObjectRefList& dObjs = e->getDataObjectList();

	while (dObjs.size()) {
		bool fromThirdParty = false;

		DataObjectRef dObj = dObjs.pop();

		NodeRef node = Node::create(dObj);

		if (!node) {
			HAGGLE_DBG("Could not create node from metadata!\n");
			continue;
		}

                HAGGLE_DBG("Node description [%s] of node %s [%s] received\n", 
			DataObject::idString(dObj).c_str(), node->getName().c_str(), node->getIdStr());

		if (node == kernel->getThisNode()) {
			HAGGLE_DBG("Node description is my own device. Ignoring and deleting from data store\n");
			// Remove the data object from the data store (keep in bloomfilter):
			kernel->getDataStore()->deleteDataObject(dObj, true);
			continue;
		}

		if (node->hasProxyId(kernel->getThisNode()->getId())) { // MOS
		        node->setLocalApplication(); // MOS
		        HAGGLE_DBG("Node description is my own application.\n");
			InterfaceRef iface = dObj->getRemoteInterface();
			if(iface && iface->getType() != Interface::TYPE_APPLICATION_PORT) {
			  HAGGLE_DBG("Node description is my own application. Ignoring and deleting from data store\n");
			  // Remove the data object from the data store (keep in bloomfilter):
			  kernel->getDataStore()->deleteDataObject(dObj, true);
			  continue;
			} 
		}

		// Retrieve any existing neighbor nodes that might match the newly received 
		// node description.
		// NodeRef neighbor = kernel->getNodeStore()->retrieve(node, true);
		NodeRef neighbor = kernel->getNodeStore()->retrieve(node); // MOS - replacement by create time needs to happen even if not neighbor

		// Make sure at least the interface of the remote node is set to up
		InterfaceRef remoteIface = dObj->getRemoteInterface();
		
		if (remoteIface) {
			if (node->hasInterface(remoteIface)) {
				// Node description was received directly from
				// the node it belongs to
				
				// Mark the interface as up in the node.
				node->setInterfaceUp(remoteIface);				
			} else {
				// Node description was received from third party.
				
				fromThirdParty = true;

				NodeRef peer = kernel->getNodeStore()->retrieve(remoteIface, true);
				
				if  (peer) {
					HAGGLE_DBG("Received %s's node description from third party node %s [%s]\n",
						   node->getName().c_str(), peer->getName().c_str(), peer->getIdStr());
				} else {
					HAGGLE_DBG("Received %s's node description from third party node with interface %s\n",
						   node->getName().c_str(), remoteIface->getIdentifierStr());
				}

				// Ignore the node description if the node it describes
				// is a current neighbor. We trust such a neighbor to give
				// us its latest node description when necessary.
				if(neighbor && neighbor->isNeighbor()) { // MOS
				  if(acceptNeighborNodeDescriptionsFromThirdParty) { // MOS
					HAGGLE_DBG("Node description of %s received from third party describes a neighbor -- not ignoring\n",
						   node->getName().c_str());
				  } else {
					HAGGLE_DBG("Node description of %s received from third party describes a neighbor -- ignoring!\n",
						   node->getName().c_str());
					continue;
				  }
				}
			}
		} else {
			HAGGLE_DBG("Node description of %s [%s] has no remote interface\n",
				   node->getName().c_str(), node->getIdStr());

                       if(neighbor) {
                         if(neighbor->isLocalApplication()) {
                           HAGGLE_DBG("Node description should represent local application, updating from node store\n");

                           node->setLocalApplication(); // MOS
                           node->setProxyId(neighbor->getProxyId()); // MOS

                           neighbor.lock();
                           const InterfaceRefList *lst = neighbor->getInterfaces();
                           
                           for (InterfaceRefList::const_iterator it = lst->begin(); 
                                it != lst->end(); it++) {
                             node->addInterface(*it);
                           }
                           neighbor.unlock();
                         }
                       }
                }
	
		// If the existing neighbor node was undefined, we merge the bloomfilters
		// of the undefined node and the node created from the node description
		// MOS - neighbor may not be a neighbor in our generalization
		if (neighbor) {
			if (neighbor->getType() == Node::TYPE_UNDEFINED) {
				// MOS - merge can not work because bloomfilter salts not equal (randomized)
				// Not a big problem but can lead to redundant transmissions
				// to previously unknown neighbors (e.g. at startup).
			        // HAGGLE_DBG("Merging bloomfilter of node %s with its previously undefined node\n", 
				//	node->getName().c_str());
				// node->getBloomfilter()->merge(*neighbor->getBloomfilter());
			} else {
                                if (node->getNodeDescriptionCreateTime() < neighbor->getNodeDescriptionCreateTime()) { // MOS - do not ignore if timestamp the same
					HAGGLE_DBG("Node description create time lower than on existing neighbor node. IGNORING node description\n");
					// Delete old node descriptions from data store (keep in bloomfilter)
					if (node->getNodeDescriptionCreateTime() < neighbor->getNodeDescriptionCreateTime())
						kernel->getDataStore()->deleteDataObject(dObj, true); 
					continue;
				}
			}
		} 

		HAGGLE_DBG("New node description from node %s -- createTime %s updateTime %s receiveTime %s, bloomfilter #objs=%lu\n", 
			node->getName().c_str(), 
			dObj->getCreateTime().getAsString().c_str(), 
			dObj->getUpdateTime().getAsString().c_str(), 
			dObj->getReceiveTime().getAsString().c_str(),
			node->getBloomfilter()->numObjects());

		if(kernel->discardObsoleteNodeDescriptions()) 
		  kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_NEW, dObj)); // MOS

		// Here we have a fast path and a slow path depending on whether the node description
		// was received directly from the node it describes or not. In the case the node description 
		// was received directly from the node it describes, then we trust it to contain the latest 
		// information, and an updated bloomfilter. In this case we take the fast path. In the case we received
		// the node description from a third party node, we cannot trust the bloomfilter to be up-to-date
		// with all the data objects we have previously sent the node. Therefore, we should merge
		// the received bloomfilter with the one we have in the data store. This requires first a 
		// call to the data store to retrieve the 
		if (fromThirdParty) {
#if defined(MERGE_BLOOMFILTERS)
			// Slow path, wait for callback from data store before proceeding
			kernel->getDataStore()->insertNode(node, onInsertedNodeCallback, true);
#else
			// Do fast path also here if we do not want to merge bloomfilters
			if(!neighbor || node->getNodeDescriptionUpdateTime() > neighbor->getNodeDescriptionUpdateTime()) // MOS - optimization for unchanged node descriptions
			  kernel->getDataStore()->insertNode(node);
			nodeUpdate(node);
#endif
		} else {
			// Fast path, without callback
			if(!neighbor || node->getNodeDescriptionUpdateTime() > neighbor->getNodeDescriptionUpdateTime()) // MOS - optimization for unchanged node descriptions
			  kernel->getDataStore()->insertNode(node);
			nodeUpdate(node);
		}
	}
}

void NodeManager::nodeUpdate(NodeRef& node)
{
	NodeRefList nl;

    // SW: START REPLICATION MANAGER
    kernel->addEvent(ReplicationManagerFactory::getNewNodeDescriptionEvent(node));
    // SW: END REPLICATION MANAGER

	// See if this node is already an active neighbor but in an uninitialized state
	if (kernel->getNodeStore()->update(node, &nl)) {
		HAGGLE_DBG("Neighbor node %s [id=%s] was updated in node store\n", 
			   node->getName().c_str(), node->getIdStr());
	} else {
		// This is the path for node descriptions received via a third party, i.e.,
		// the node description does not belong to the neighbor node we received it
		// from.
		
		// NOTE: in reality, we should never get here for node descriptions belonging to neighbor nodes. 
		// This is because a node (probably marked as 'undefined') should have been 
		// added to the node store when we got an interface up event just before the node description
		// was received (the interface should have been discovered or 'snooped').
		// Thus, the getNodeStore()->update() above should have succeeded. However,
		// in case the interface discovery somehow failed, there might not be a
		// neighbor node in the node store that matches the received node description.
		// Therefore, we might get here for neighbor nodes as well, but in that case
		// there is not much more to do until we discover the node properly

		HAGGLE_DBG("Got a node description [%s] for node %s [id=%s], which is not a previously discovered neighbor.\n", 
			   DataObject::idString(node->getDataObject()).c_str(), node->getName().c_str(), node->getIdStr());

	        if(node->isLocalApplication()) { // MOS - do not reinsert deregistered applications
		  HAGGLE_DBG("Ignoring node update from local application %s that is not in node store.\n", node->getName().c_str()); 
		  return;		
		}

		// Sync the node's interfaces with those in the interface store. This
		// makes sure all active interfaces are marked as 'up'.
		node.lock();

		const InterfaceRefList *ifl = node->getInterfaces();
		
		for (InterfaceRefList::const_iterator it = ifl->begin(); it != ifl->end(); it++) {
			if (kernel->getInterfaceStore()->stored(*it)) {
				node->setInterfaceUp(*it);
			}
		}
		node.unlock();
		
		if (node->isAvailable()) {
			// Add node to node store
			HAGGLE_DBG("Node %s [id=%s] was a neighbor -- adding to node store\n", 
				   node->getName().c_str(), node->getIdStr());
			
			kernel->getNodeStore()->add(node);
		
			// We generate both a new contact event and an updated event because we
			// both 'discovered' the node and updated its node description at once.
			// In most cases, this should never really happen (see not above).
			kernel->addEvent(new Event(EVENT_TYPE_NODE_CONTACT_NEW, node));
		} else {
		        // HAGGLE_DBG("Node %s [id=%s] had no active interfaces, not adding to store\n", 
			//	   node->getName().c_str(), node->getIdStr());
		        HAGGLE_DBG("Node %s [id=%s] had no active interfaces, nevertheless adding to store\n", // MOS
				   node->getName().c_str(), node->getIdStr());
                        // MOS - Sam's solution to ensure that remote nodes are in the NodeStore
			//       (think of node store as a cache that is not limited to neighors)
			kernel->getNodeStore()->add(node);
		}
	}

	// MOS - the following notifies applications of transitions from unnamed to named neighbors
	NodeRefList::iterator it = nl.begin();	
	while (it != nl.end()) {
	        if ((*it)->getType() == Node::TYPE_UNDEFINED && node->isNeighbor()) {
		        kernel->addEvent(new Event(EVENT_TYPE_NODE_CONTACT_NEW, node));
			break;
		}
		it++;
	}
	
	// We send the update event for all nodes that we have received a new node description from, even
	// if they are not neighbors. This is because we want to match data objects against the node although
	// we might not have direct connectivity to it.
	kernel->addEvent(new Event(EVENT_TYPE_NODE_UPDATED, node, nl));
}

void NodeManager::onInsertedNode(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	NodeRef& node = e->getNode();

	if (!node) {
		HAGGLE_ERR("No node in callback\n");
		return;
	}

	HAGGLE_DBG("Node %s [%s] was successfully inserted into data store\n", 
		node->getName().c_str(), node->getIdStr());

	nodeUpdate(node);
        //JM
        kernel->getNodeStore()->insertNodeSocialGroup(node);
}

void NodeManager::onConfig(Metadata *m)
{

    const char *param;
//JM: change to #define names
   param = m->getParameter("social_group");
   if (param) {
       nodeSocialName=string(param);
       useSocialName=true;
   } else {
       useSocialName=false;
   }
   kernel->getNodeStore()->setMyNodeSocialGroupName(nodeSocialName);
    HAGGLE_DBG("social identifer: %s=%s\n", useSocialName ? "SocialGroup" : "false", useSocialName ? nodeSocialName.c_str() : "not used");

// JM: END CONFIG

	Metadata *nm = m->getMetadata("Node");

	if (nm) {
		char *endptr = NULL;
		const char *param = nm->getParameter("matching_threshold");
		
		if (param) {
			unsigned long matchingThreshold = strtoul(param, &endptr, 10);

			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting matching threshold to %lu\n", matchingThreshold);
				kernel->getThisNode()->setMatchingThreshold(matchingThreshold);
				LOG_ADD("# %s: matching threshold=%lu\n", getName(), matchingThreshold);
			}
		}
		
		param = nm->getParameter("max_dataobjects_in_match");
		
		if (param) {
			unsigned long maxDataObjectsInMatch = strtoul(param, &endptr, 10);
			
			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting max data objects in match to %lu\n", maxDataObjectsInMatch);
				kernel->getThisNode()->setMaxDataObjectsInMatch(maxDataObjectsInMatch);
				LOG_ADD("# %s: max data objects in match=%lu\n", getName(), maxDataObjectsInMatch);
			}
		}

		param = nm->getParameter("node_description_attribute");

		if (param) {		  
		  if (strcmp(param, "id") == 0) {
		    Node::setNodeDescriptionAttribute(Node::NDATTR_ID);
		    HAGGLE_DBG("Node description attribute set to ID\n");
		  } else if (strcmp(param, "type") == 0) {
		    Node::setNodeDescriptionAttribute(Node::NDATTR_TYPE);
		    HAGGLE_DBG("Node description attribute set to TYPE\n");
		  } else if (strcmp(param, "empty") == 0) {
		    Node::setNodeDescriptionAttribute(Node::NDATTR_EMPTY);
		    HAGGLE_DBG("Node description attribute set to EMPTY\n");
		  } else if (strcmp(param, "none") == 0) {
		    Node::setNodeDescriptionAttribute(Node::NDATTR_NONE);
		    HAGGLE_DBG("Node description attribute set to NONE\n");
		  } else {
		    HAGGLE_ERR("Unrecognized node description attribute setting\n");
		  }
		}
		
		param = nm->getParameter("node_description_attribute_weight");
		
		if (param) {
			unsigned long weight = strtoul(param, &endptr, 10);

			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting node description attribute weight to %lu\n", weight);
				Node::setNodeDescriptionAttributeWeight(weight);
			}
		}

// SW: START: include 1-hop neighborhood  in ND metadata
		param = nm->getParameter("send_neighborhood");
        if (param) {
		    if (strcmp(param, "true") == 0) {
                send_neighborhood = true;
            } else if (strcmp(param, "false") == 0) {
                send_neighborhood = false;
            } else {
                HAGGLE_ERR("send_neighborhood parameter must be 'true' or 'false'\n");
            }
        }
// SW: END: include 1-hop neighborhood  in ND metadata.

        param = nm->getParameter("send_app_node_descriptions");
        if (param) {
            if (strcmp(param, "true") == 0) {
                sendAppNodeDOs = true;
            } else if (strcmp(param, "false") == 0) {
                sendAppNodeDOs = false;
            } else {
                HAGGLE_ERR("send_app_node_descriptions parameter must be 'true' or 'false'\n");
            }
        }
	}

	nm = m->getMetadata("NodeDescriptionRetry");

	if (nm) {
		char *endptr = NULL;
		const char *param = nm->getParameter("retries");

		if (param) {
			unsigned long retries = strtoul(param, &endptr, 10);

			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting node description retries to %lu\n", retries);
				nodeDescriptionRetries = retries;
				LOG_ADD("# %s: node description retries=%lu\n", getName(), nodeDescriptionRetries);
			}
		}

		param = nm->getParameter("retry_wait");

		if (param) {
			char *endptr = NULL;
			double retry_wait = strtod(param, &endptr);

			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting node description retry wait to %lf\n", retry_wait);
				nodeDescriptionRetryWait = retry_wait;
				LOG_ADD("# %s: node description retry wait=%lf\n", getName(), nodeDescriptionRetryWait);
			}
		}
	}
    
    // SW: START REFRESH:
    nm = m->getMetadata("NodeDescriptionRefresh");

    if (nm) {

        unsigned long refresh_period_ms = 0;
        unsigned long refresh_jitter_ms = 0;

        const char *param = nm->getParameter("refresh_period_ms");
        
        if (param) {
            char *endptr = NULL;
            unsigned long period = strtoul(param, &endptr, 10);
            if (endptr && endptr != param) {
                refresh_period_ms = period;    
            }
        } else {
            HAGGLE_ERR("Must specify 'refresh_period_ms' for refresh.\n");
        }

        param = nm->getParameter("refresh_jitter_ms");

        if (param) {
            char *endptr = NULL;
            unsigned long jitter = strtoul(param, &endptr, 10);
            if (endptr && endptr != param) {
                refresh_jitter_ms = jitter;
            }
        } else {
            HAGGLE_ERR("Must specify 'refresh_jitter_ms' for refresh.\n");
        }
        
        if ((refresh_period_ms > 0) && (refresh_jitter_ms > 0)) {
            nodeDescriptionRefreshPeriodMs = refresh_period_ms; 
            nodeDescriptionRefreshJitterMs = refresh_jitter_ms;
            HAGGLE_DBG("Loaded NodeManager Refresh settings with period (ms): %d, jitter (ms): %d\n", 
                nodeDescriptionRefreshPeriodMs, nodeDescriptionRefreshJitterMs);
            // fire off the first event 
            if (!periodicNodeDescriptionRefreshEvent->isScheduled()) {
                kernel->addEvent(periodicNodeDescriptionRefreshEvent);
            }
        } else {
            HAGGLE_ERR("Could not load `NodeDescriptionRefresh', parameters missing.\n");
        }
    }
    // SW: END REFRESH.

    // SW: START PURGE:
    nm = m->getMetadata("NodeDescriptionPurge");

    if (nm) {

        unsigned long purge_max_age_ms = 0;
        unsigned long purge_poll_period_ms = 0;

        const char *param = nm->getParameter("purge_max_age_ms");
        
        if (param) {
            char *endptr = NULL;
            unsigned long max_age = strtoul(param, &endptr, 10);
            if (endptr && endptr != param) {
                purge_max_age_ms = max_age;    
            }
        } else {
            HAGGLE_ERR("Must specify 'purge_max_age_ms' for node description purge.\n");
        }

        param = nm->getParameter("purge_poll_period_ms");
        if (param) {
            char *endptr = NULL;
            unsigned long purge_period = strtoul(param, &endptr, 10);
            if (endptr && endptr != param) {
                purge_poll_period_ms = purge_period;    
            }
        } else {
            HAGGLE_ERR("Must specify 'purge_poll_period_ms' for node description purge.\n");
        }

        if ((purge_max_age_ms > 0) && (purge_poll_period_ms > 0)) {
            HAGGLE_DBG("Loaded NodeDescriptionPurge settings with max age (ms): %d, poll period (ms): %d.\n",
                purge_max_age_ms, purge_poll_period_ms);
            nodeDescriptionPurgeMaxAgeMs = purge_max_age_ms;
            nodeDescriptionPurgePollPeriodMs = purge_poll_period_ms;
            // fire off the first event 
            if (!periodicNodeDescriptionPurgeEvent->isScheduled()) {
                kernel->addEvent(periodicNodeDescriptionPurgeEvent);
            }
        } else {
            HAGGLE_ERR("Could not load `NodeDescriptionPurge', invalid parameters.\n");
        }
    }
    // SW: END PURGE.

    // MOS - discard obsolete incoming node descriptions (instead of forwarding them to other nodes)
    param = m->getParameter("discard_obsolete_node_descriptions");
	
    if (param) {
      if (strcmp(param, "true") == 0) {
	HAGGLE_DBG("Enabling discarding of obsolete node descriptions\n");
	kernel->setDiscardObsoleteNodeDescriptions(true);
      } else if (strcmp(param, "false") == 0) {
	HAGGLE_DBG("Disabling  discarding of obsolete node descriptions\n");
	kernel->setDiscardObsoleteNodeDescriptions(false);
      }
    }

    // MOS - obsolete node descriptions can be preempted even if already queued for sending
    param = m->getParameter("preempt_obsolete_node_descriptions");
	
    if (param) {
      if (strcmp(param, "true") == 0) {
	HAGGLE_DBG("Enabling preemption of obsolete node descriptions\n");
	kernel->getNodeStore()->setPreemptObsoleteNodeDescriptions(true);
      } else if (strcmp(param, "false") == 0) {
	HAGGLE_DBG("Disabling  preemption of obsolete node descriptions\n");
	kernel->getNodeStore()->setPreemptObsoleteNodeDescriptions(false);
      }
    }

    // MOS - obsolete node descriptions can be preempted even if already queued for sending
    param = m->getParameter("preempt_local_obsolete_node_descriptions");
	
    if (param) {
      if (strcmp(param, "true") == 0) {
	HAGGLE_DBG("Enabling preemption of local obsolete node descriptions\n");
	kernel->getNodeStore()->setPreemptLocalObsoleteNodeDescriptions(true);
      } else if (strcmp(param, "false") == 0) {
	HAGGLE_DBG("Disabling  preemption of local obsolete node descriptions\n");
	kernel->getNodeStore()->setPreemptLocalObsoleteNodeDescriptions(false);
      }
    }

    // MOS - Bloomfilter continuity
    param = m->getParameter("continuous_bloomfilters");
	
    if (param) {
      if (strcmp(param, "true") == 0) {
	HAGGLE_DBG("Enabling continuous bloomfilters\n");
	kernel->getNodeStore()->setContinuousBloomfilters(true);
      } else if (strcmp(param, "false") == 0) {
	HAGGLE_DBG("Disabling continuous bloomfilters\n");
	kernel->getNodeStore()->setContinuousBloomfilters(false);
      }
    }

    // MOS
    param = m->getParameter("continuous_bloomfilter_update_interval");	
    if (param) {		
      char *endptr = NULL;
      unsigned long period = strtoul(param, &endptr, 10);	  
      if (endptr && endptr != param) {
	kernel->getNodeStore()->setContinuousBloomfilterUpdateInterval(period);	
      }
    }

    param = m->getParameter("accept_neighbor_node_descriptions_from_third_party");	
    if (param) {
      if (strcmp(param, "true") == 0) {
	HAGGLE_DBG("Enabling acceptance of neighbor node descriptions from third party\n");
	acceptNeighborNodeDescriptionsFromThirdParty = true;
      } else if (strcmp(param, "false") == 0) {
	HAGGLE_DBG("Disabling acceptance of neighbor node descriptions from third party\n");
	acceptNeighborNodeDescriptionsFromThirdParty = false;
      }
    }
}
