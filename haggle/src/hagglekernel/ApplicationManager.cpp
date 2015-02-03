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
 *   Hasanat Kazmi (HK)
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
#include <libcpphaggle/Watch.h>
#include <libcpphaggle/String.h>

#include "EventQueue.h"
#include "ApplicationManager.h"
#include "DataObject.h"
#include "Event.h"
#include "Interface.h"
#include "Attribute.h"
#include "Filter.h"
#include "Node.h"
#include "Utility.h"
#include "XMLMetadata.h" // CBMEN, HL
#include "SecurityManager.h"	// IRD, HK

#include <malloc.h> // CBMEN, HL
#include <sys/time.h> // CBMEN, HL
#include <sys/resource.h> // CBMEN, HL
#include <base64.h>

// This include is for the making sure we use the same attribute names as those in libhaggle. 
// TODO: remove dependency on libhaggle header
#include "../libhaggle/include/libhaggle/ipc.h"

using namespace haggle;

#define DEBUG_APPLICATION_API

static const char *intToStr(int n)
{
	static char intStr[5];

	sprintf(intStr, "%d", n);
	return intStr;
}

/*
  Define various criterias that the application manager uses to fetch
  nodes from the node store.

 */
class EventCriteria : public NodeStore::Criteria
{
	EventType etype;
public:
	EventCriteria(EventType _etype) : etype(_etype) {}
	bool operator() (const NodeRef& n) const
	{
		if (n->getType() == Node::TYPE_APPLICATION)  {
			const ApplicationNode *a = static_cast<const ApplicationNode*>(n.getObj());
			if (a->hasEventInterest(etype))
				return true;
		}
		return false;
	}
};

class EventCriteria2 : public NodeStore::Criteria
{
	EventType etype1;
	EventType etype2;
public:
	EventCriteria2(EventType _etype1, EventType _etype2) : etype1(_etype1), etype2(_etype2) {}
	bool operator() (const NodeRef& n) const
	{
		if (n->getType() == Node::TYPE_APPLICATION) {
		 	const ApplicationNode *a = static_cast<const ApplicationNode*>(n.getObj());
			if (a->hasEventInterest(etype1) && 
			    a->hasEventInterest(etype2))
				return true;
		}
		return false;
	}
};

ApplicationManager::ApplicationManager(HaggleKernel * _kernel) :
	Manager("ApplicationManager", _kernel), numClients(0), sessionid(0),
	dataStoreFinishedProcessing(false), 
	resetBloomFilterAtRegistration(true), // MOS
	forcedShutdownAfterTimeout(15), // MOS
	deleteStateOnDeregister(false),
	// CBMEN, HL, Begin
	observerCallback(NULL), 
	observerCallbackDelay(-1), 
	waitingForObserverCallback(false), 
	dumpedDBs(0),
	baseTempFilePath(TEMP_OBSERVER_FILE_PATH),
	tempFilePath(""),
	tempFile(NULL),
	dumpedObservables(0),
	observerDOPublishedCount(0)
	// CBMEN, HL, End
{
    this->applicationManagerHelper = new ApplicationManagerHelper();
}

bool ApplicationManager::init_derived()
{
#define __CLASS__ ApplicationManager
	int ret;

	ret = setEventHandler(EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN, onNeighborStatusChange);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NEIGHBOR_INTERFACE_UP, onNeighborStatusChange);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onSendResult);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, onSendResult);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_NEW, onNeighborStatusChange);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	// IRD, HK Begin
	ret = setEventHandler(EVENT_TYPE_APP_NODE_INTERESTS_POLICIES_SEND, onSendInterestsPolicies);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}
	// IRD, HK End

	/* MOS - why should this be signaled to applications 
                 (too many unnecessary events with periodic refresh)

	ret = setEventHandler(EVENT_TYPE_NODE_UPDATED, onNeighborStatusChange);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}
	*/

	ret = setEventHandler(EVENT_TYPE_APP_NODE_UPDATED, onAppNodeUpdated);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_END, onNeighborStatusChange);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	// MOS - added event for forwarding manager
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_TO_APP, onSendDataObjectToApp);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	// CBMEN, HL - added event for observer applications
	ret = setEventHandler(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, onSendObserverDataObject);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

        
	onRetrieveNodeCallback = newEventCallback(onRetrieveNode);
	onDataStoreFinishedProcessingCallback = newEventCallback(onDataStoreFinishedProcessing);

	// MOS
	prepare_shutdown_timeout_event =
	  registerEventType("ApplicationManager Prepare Shutdown Timeout Event", onPrepareShutdownTimeout);

	if (prepare_shutdown_timeout_event < 0) {
	  HAGGLE_ERR("Could not register application prepare shutdown timeout event\n");
	  return false;
	}

	onRetrieveAppNodesCallback = newEventCallback(onRetrieveAppNodes);
	onInsertedDataObjectCallback = newEventCallback(onInsertedDataObject); // MOS
	observerCallback = newEventCallback(onObserverCallback); // CBMEN, HL
	
	kernel->getDataStore()->retrieveNode(Node::TYPE_APPLICATION, onRetrieveAppNodesCallback);
	
        /* 
         * Register a filter that makes sure we receive all data
         * objects from applications that contain control information.
         */
	registerEventTypeForFilter(ipcFilterEvent, "Application API filter", onReceiveFromApplication, "HaggleIPC=*");

	return true;
}

ApplicationManager::~ApplicationManager()
{
	if (onRetrieveNodeCallback)
		delete onRetrieveNodeCallback;

	// CBMEN, HL
	if (observerCallback)
		delete observerCallback;
	
	if (onDataStoreFinishedProcessingCallback)
		delete onDataStoreFinishedProcessingCallback;
	
	if (onRetrieveAppNodesCallback)
		delete onRetrieveAppNodesCallback;

	if (onInsertedDataObjectCallback)
		delete onInsertedDataObjectCallback;

	if( this->applicationManagerHelper) {
	    delete this->applicationManagerHelper;
	    this->applicationManagerHelper = NULL;
	}

	Event::unregisterType(prepare_shutdown_timeout_event); // MOS
}

void ApplicationManager::onStartup()
{
	/*
		This function is called when haggle has finished starting up. Here
		we wish to tell any application waiting for haggle to start up that
		we've finished. We do this by sending a data object to a specific UDP
		port. Any application that is waiting for haggle to start will be 
		listening on that port.
	*/

		HAGGLE_DBG("Added default interests\n");
        addDefaultInterests(kernel->getThisNode()); // MOS - add default interest to primary node
	
	// We need a fake application node, for the protocol to be selected 
	// properly
	NodeRef fakeAppNode = Node::create(Node::TYPE_APPLICATION, "Startup application node");

	if (!fakeAppNode)
		return;
	
	struct in_addr ip;
	inet_pton(AF_INET, "127.0.0.1", &ip);
	
	// Set up the address to the application:
	IPv4Address addr(ip, TransportUDP(50888));
	
	// Reuse whatever is written in the URL above, to minimize the number of 
	// hardcoded values:
	// Create an interface for the address:
	InterfaceRef iface = new ApplicationPortInterface(50888, "App startup", &addr, IFFLAG_UP);
	
	// Add the interface to the node:
	fakeAppNode->addInterface(iface);
	
	// Create data object:
	DataObjectRef fakeDO = DataObject::create();

	if (!fakeDO) {
		HAGGLE_ERR("Could not create data object\n");
		return;
	}
	
	fakeDO->addAttribute(HAGGLE_ATTR_CONTROL_NAME);
	fakeDO->setControlMessage(); // MOS
	
	// Send data object:
	kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, fakeDO, fakeAppNode));
}

// MOS
void ApplicationManager::onAppNodeUpdated(Event *e)
{
	if (!e || !e->hasData())
		return;

	NodeRef node = e->getNode();
	ApplicationNodeRef appNode = e->getNode();
	//SW: unused var: NodeRefList &replaced = e->getNodeList();
	
	HAGGLE_DBG("%s - got application node update for %s [id=%s]\n", 
		   getName(), node->getName().c_str(), node->getIdStr());

	updateApplicationInterests(appNode);

	kernel->getDataStore()->insertNode(node);
							
	// Update the node description and send it to all 
	// neighbors.
							
	if(kernel->firstClassApplications()) { // MOS - disable aggregration
	  DataObjectRef dObj = node->getDataObject(); // MOS - get app node description
	  node->addNodeDescriptionAttribute(dObj); // MOS - add attribute to enable propagation
	  kernel->getDataStore()->insertDataObject(dObj,onInsertedDataObjectCallback); // MOS - app nodes now first-class citizens
	  // Push the updated node description to all neighbors
	  kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));
	}
	else {
	  kernel->getDataStore()->retrieveNode(Node::TYPE_APPLICATION, onRetrieveAppNodesCallback);
	}	
}

// Updates the "this node" with all attributes that the application nodes have: 
void ApplicationManager::onRetrieveAppNodes(Event *e)
{
	if (!e)
		return;

	NodeRefList *nodes = (NodeRefList *) e->getData();
	
	/**
		FIXME: This function is currently not capable of determining if there
		is a difference in the interests of thisNode before and after, and 
		therefore assume a difference, and sends out the node description.
		
		This is perhaps unwanted behavior.
	*/

	HAGGLE_DBG("Recompiling this node interests\n");
	
	// Remove all the old attributes:
	Attributes *al = kernel->getThisNode()->getAttributes()->copy();

	for (Attributes::iterator jt = al->begin(); jt != al->end(); jt++) {
		kernel->getThisNode()->removeAttribute((*jt).second);
	}

	delete al;

	// MOS - keep default interests
        addDefaultInterests(kernel->getThisNode()); 
	
	if (nodes && !nodes->empty()) {
		// Insert all the attributes:
		for (NodeRefList::iterator it = nodes->begin(); it != nodes->end(); it++) {
			const Attributes *al;
			NodeRef	nr;

			// Try to get the most updated node from the node store:
			nr = kernel->getNodeStore()->retrieve(*it);
			// No node in the store? Default to the node in the data store:
			if (!nr)
				nr = (*it);

			if(nr->getName() == MONITOR_APP_NAME) continue; // MOS
			
			nr.lock();
			al = nr->getAttributes();
			for (Attributes::const_iterator jt = al->begin(); jt != al->end(); jt++) {
				kernel->getThisNode()->addAttribute((*jt).second);
				HAGGLE_DBG("Adding interest %s from application %s\n", (*jt).second.getName().c_str(), nr->getName().c_str());
			}
			nr.unlock();
		}
	}
	
	// The node list will not be deleted along with the event, so we have to do it:
	if (nodes)
		delete nodes;
		
	// Push the updated node description to all neighbors
	if (kernel->getNodeStore()->numNeighbors())
		kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));
}

void ApplicationManager::onDataStoreFinishedProcessing(Event *e)
{
        dataStoreFinishedProcessing = true;

	HAGGLE_DBG("Data store finished processing\n");

	// MOS - discard unimportant messages (currently everything except shutdown)
	SentToApplicationList::iterator it = pendingDOs.begin();
	while (it != pendingDOs.end()) {
	  DataObjectRef dObj =  (*it).second;
	  Metadata *m = dObj->getMetadata()->getMetadata(DATAOBJECT_METADATA_APPLICATION);
	  if(m) {
	    Metadata *ctrl_m = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL);
	    if(ctrl_m) {
	      Metadata *event_m = ctrl_m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);	
	      if (event_m) {
		const char *param = event_m->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM);
		if(param && strcmp(param, intToStr(LIBHAGGLE_EVENT_SHUTDOWN)) != 0) {
		  HAGGLE_DBG("Discarding pending data object [%s]\n", DataObject::idString(dObj).c_str());
		  it = pendingDOs.erase(it);
		  continue;
		}
	      }
	    }
	  }
	  it++;
	}

	if (pendingDOs.empty()) {
		HAGGLE_DBG("No more pending data objects -> ready for shutdown\n");
		signalIsReadyForShutdown();
	}
}

// MOS - application manager did not shut down in a timely manner

void ApplicationManager::onPrepareShutdownTimeout(Event *e) {
  if(getState() == MANAGER_STATE_PREPARE_SHUTDOWN) {
    HAGGLE_ERR("Initiating shutdown after timeout during preparation\n");

    if (pendingDOs.empty()) {
      HAGGLE_DBG("No more pending data objects -> ready for shutdown\n");
      signalIsReadyForShutdown();
    } else {
      HAGGLE_ERR("There are %lu pending data objects -> forcing shutdown\n", pendingDOs.size());
      SentToApplicationList::iterator it = pendingDOs.begin();
      while (it != pendingDOs.end()) {
	DataObjectRef dObj =  (*it).second;
	HAGGLE_DBG("Discarding pending data object [%s]\n", DataObject::idString(dObj).c_str());
	it = pendingDOs.erase(it);
      }
      signalIsReadyForShutdown(); // MOS - force shutdown for now
    }
  }
}

void ApplicationManager::onSendResult(Event *e)
{
	// This function responds to both EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL and
	// EVENT_TYPE_DATAOBJECT_SEND_FAILURE, but doesn't distinguish between them
	// at all.

	// Check that the event exists (just in case)
	if (e == NULL)
		return;
	
	NodeRef app = e->getNode();
	
	if (!app || app->getType() != Node::TYPE_APPLICATION)
		return;
	
	DataObjectRef dObj = e->getDataObject();
	
	// Check that the data object exists:
	if (!dObj)
		return;
	
	HAGGLE_DBG("Send result %s for application %s - data object id=%s\n", 
		e->getType() == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL ? "SUCCESS" : "FAILURE",
		app->getName().c_str(), DataObject::idString(dObj).c_str());

	// Go through the list and find which (if any) sends this was in reference 
	// to
        SentToApplicationList::iterator it = pendingDOs.begin();

	while (it != pendingDOs.end()) {
		// Only check against the data object here in case some application
		// has deregistered.
                if ((*it).second == dObj) {
                        it = pendingDOs.erase(it);
                } else {
                        it++;
                }
	}
	// If we didn't find any matches, ignore.
	
	// If we are preparing for shutdown and the data store
	// is done with processing deregistered application nodes,
	// then signal we are ready for shutdown
	if (getState() == MANAGER_STATE_PREPARE_SHUTDOWN) {
		if (pendingDOs.empty() && dataStoreFinishedProcessing) {
			HAGGLE_DBG("Ready for shutdown!\n");
			signalIsReadyForShutdown();
		} else {
			HAGGLE_DBG("Preparing shutdown, but %lu data objects are still pending\n", pendingDOs.size());
		}
	}
}

void ApplicationManager::onDeletedDataObject(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	DataObjectRefList dObjs = e->getDataObjectList();
	
	for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
		DataObjectRef dObj = *it;
		
		// This is a bit of a brute force approach, removing
		// the deleted data object from all application's
		// bloomfilter
	}
}
void ApplicationManager::sendToApplication(DataObjectRef& dObj, ApplicationNodeRef& app)
{
	NodeRef node = app;
	pendingDOs.push_back(make_pair(app, dObj));
	HAGGLE_DBG("Sending data object [%s] to application %s\n", DataObject::idString(dObj).c_str(), app->getName().c_str());
	kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, node));
}

void ApplicationManager::onPrepareShutdown()
{	
	HAGGLE_DBG("Prepare shutdown! Notifying applications\n");

	DataObjectRef dObj = DataObject::create();
	
	if (!dObj)
		return;
	
	// Tell all applications that we are shutting down.
	Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_EVENT, "All Applications", dObj->getMetadata());
	
	if (!ctrl_m)
		return;
		
	Metadata *event_m = ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
	
	if (!event_m)
		return;
	
	event_m->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM, intToStr(LIBHAGGLE_EVENT_SHUTDOWN));

	dObj->setControlMessage(); // MOS
	
	sendToAllApplications(dObj, LIBHAGGLE_EVENT_SHUTDOWN);

	// Retrieve all application nodes from the Node store.
	ApplicationNodeRefList lst;
	unsigned long num;
	
        num = kernel->getNodeStore()->retrieve(lst);
	
	if (num) {
		for (ApplicationNodeRefList::iterator it = lst.begin(); it != lst.end(); it++) {
			ApplicationNodeRef app = *it;
			deRegisterApplication(app);
		}
		
		/*
		 The point here is to delay deregistering until after the data store 
		 has finished processing what deRegisterApplication sent it, not to
		 get the actual data.
		 */
		NodeRef appnode = *lst.begin();
		HAGGLE_DBG("Retrieving node %s to determine when data store has finished\n", appnode->getName().c_str());
		kernel->getDataStore()->retrieveNode(appnode, onDataStoreFinishedProcessingCallback, true);

		if(forcedShutdownAfterTimeout) // MOS - for testing this should be off to catch all anomalies
		  kernel->addEvent(new Event(prepare_shutdown_timeout_event, (void*)NULL, forcedShutdownAfterTimeout));
		
	} else {
		// There where no registered applications, and therefore there
		// is no more processing to be done in the data store
		dataStoreFinishedProcessing = true;

		/*
		 No more data objects pending --> We are ready for shutdown.
		 Otherwise, wait until all data objects have been sent in onSendResult().
		*/
		if (pendingDOs.empty()) {
			HAGGLE_DBG("No pending data objects -- ready for shutdown!\n");
			signalIsReadyForShutdown();
		}
	}
}

void ApplicationManager::onShutdown()
{
	unregisterEventTypeForFilter(ipcFilterEvent);	
	unregisterWithKernel();
}

// SW: adding deleteApplicationState option to cleanup
// old application nodes from DB
int ApplicationManager::deRegisterApplication(ApplicationNodeRef& app, bool deleteApplicationState)
{
	HAGGLE_DBG("Removing Application node %s id=%s\n", app->getName().c_str(), app->getIdStr());

	numClients--;
	
// CBMEN, HL, Begin: Remove all data objects belonging to application
	SentToApplicationList::iterator it = pendingDOs.begin();

	while (it != pendingDOs.end()) {
		if ((*it).first == app) {
			Metadata *m = (*it).second->getMetadata()->getMetadata(DATAOBJECT_METADATA_APPLICATION);
			// We don't want to delete the shutdown message, if there is any
			// There has to be a better way to do this.
			if (m && 
				strcmp(m->getParameter(DATAOBJECT_METADATA_APPLICATION_NAME_PARAM),
				"All Applications") != 0) {
					it = pendingDOs.erase(it);
					continue;
			}
		}
		it++;
	}
// CBMEN, HL, End: Remove all data objects belonging to application
	
	// We need to modify the node that we insert, so we make a copy first in case
	// someone else is relying on the node that was in the node store
// SW: START: fixing mem-leak here where private event was not free'd
	ApplicationNodeRef app_copy_orig = app->copy();
	NodeRef app_copy = app_copy_orig; 
	//NodeRef app_copy = app->copy(); 
// SW: END: fixing mem-leak here where private event was not free'd
	
	// Remove the application node
	kernel->getNodeStore()->remove(app_copy);

	if(!kernel->firstClassApplications()) { // MOS
	  // Remove the application's filter
	  kernel->getDataStore()->deleteFilter(app->getFilterEvent());
	}

        const InterfaceRefList *lst = app_copy->getInterfaces();
        
        while (!lst->empty()) {
                app_copy->removeInterface(*(lst->begin()));
        }

	// Save the application node state
	kernel->getDataStore()->insertNode(app_copy);

// SW: START: delete application state
    if (deleteApplicationState) {
        kernel->getDataStore()->deleteNode(app_copy);
    }
// SW: END: delete application state

// SW: START: fixing mem-leak here where private event was not free'd
	app_copy_orig->clearEventInterests();
// SW: END: fixing mem-leak here where private event was not free'd

	// CBMEN, HL - Clear filter event so that private event gets unset.
	((ApplicationNodeRef) app_copy)->clearFilterEvent();

	return 1;
}

static EventType translate_event(int eid)
{
	switch (eid) {
		case LIBHAGGLE_EVENT_SHUTDOWN:
			return EVENT_TYPE_SHUTDOWN;
		case LIBHAGGLE_EVENT_NEIGHBOR_UPDATE:
			return EVENT_TYPE_NODE_CONTACT_NEW;
		case LIBHAGGLE_EVENT_NEW_DATAOBJECT:
			return EVENT_TYPE_DATAOBJECT_RECEIVED;
	}
	return -1;
}

int ApplicationManager::addApplicationEventInterest(ApplicationNodeRef& app, long eid)
{
	HAGGLE_DBG("Application %s registered event interest %d\n", app->getName().c_str(), eid);

	switch (eid) {
		case LIBHAGGLE_EVENT_SHUTDOWN:
			app->addEventInterest(EVENT_TYPE_SHUTDOWN);
			break;
		case LIBHAGGLE_EVENT_NEIGHBOR_UPDATE:
			app->addEventInterest(EVENT_TYPE_NODE_CONTACT_NEW);
			app->addEventInterest(EVENT_TYPE_NODE_CONTACT_END);
			onNeighborStatusChange(NULL);
			break;
	}
	return 0;
}

int ApplicationManager::sendToAllApplications(DataObjectRef& dObj, long eid)
{
	int numSent = 0;
	EventCriteria ec(translate_event(eid));

	NodeRefList apps;
        unsigned long num;

        num = kernel->getNodeStore()->retrieve(ec, apps);

	if (num == 0) {
		HAGGLE_DBG("No applications to send to for event id=%ld\n", eid);
		return 0;
	}
	for (NodeRefList::iterator it = apps.begin(); it != apps.end(); it++) {
		ApplicationNodeRef app = *it;

		DataObjectRef sendDO = dObj->copy();

#ifdef DEBUG_APPLICATION_API
		sendDO->print(NULL); // MOS - NULL means print to debug trace
#endif
		sendToApplication(sendDO, app);
		numSent++;
		HAGGLE_DBG("Sent event id=%ld to application %s [data object id=%s]\n", 
			eid, app->getName().c_str(), DataObject::idString(sendDO).c_str());
	}

	return numSent;
}

int ApplicationManager::updateApplicationInterests(ApplicationNodeRef& app)
{
	if (!app)
		return -1;

	if(kernel->firstClassApplications()) return 0; // MOS - disable application filter for first class apps

	long etype = app->getFilterEvent();

	if (etype == -1) {
		etype = registerEventType("Application filter event", onApplicationFilterMatchEvent);
		app->setFilterEvent(etype);
	}

	app->addEventInterest(etype);

	// TODO: This should be kept in a list so that it can be
	// removed when the application deregisters.
	Filter appfilter(app->getAttributes(), etype);

	// Insert the filter, and also match it immediately:
	kernel->getDataStore()->insertFilter(appfilter, true);

	HAGGLE_DBG("Registered interests for application %s\n", app->getName().c_str());

	return 0;
}

void ApplicationManager::onApplicationFilterMatchEvent(Event *e)
{
	DataObjectRefList& dObjs = e->getDataObjectList();

	if (dObjs.size() == 0) {
		HAGGLE_ERR("No Data objects in filter match event!\n");
		return;
	}
	EventCriteria ec(e->getType());

	HAGGLE_DBG("Filter match event - checking applications\n");

	NodeRefList apps;
        unsigned long num = kernel->getNodeStore()->retrieve(ec, apps);

	if (num == 0) {
		HAGGLE_ERR("No applications matched filter\n");
		return;
	}
	for (NodeRefList::iterator it = apps.begin(); it != apps.end(); it++) {
		ApplicationNodeRef app = *it;

		HAGGLE_DBG("Application %s's filter matched %lu data objects\n", 
			app->getName().c_str(), dObjs.size());

		for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
			DataObjectRef& dObj = *it;

			// Do not give node descriptions to applications.
			if (dObj->isNodeDescription()) {
				HAGGLE_DBG("Data object [%s] is a node description, not sending to %s\n", 
					DataObject::idString(dObj).c_str(), app->getName().c_str());
				continue;
			}

			if( this->applicationManagerHelper->shouldNotSendToApplication(dObj)) {
			    HAGGLE_DBG("Not passing databoject %s to application\n",dObj->getIdStr());
			    continue;
			}

			// CBMEN, HL, Begin
			if (e->getType() == EVENT_TYPE_SEND_OBSERVER_DATAOBJECT) {
				HAGGLE_ERR("Should never have a filter match for EVENT_TYPE_SEND_OBSERVER_DATAOBJECT\n");
				continue;
			}
			// CBMEN, HL, End

			// Have we already sent this data object to this app?
			if (app->getBloomfilter()->has(dObj)) {
				// Yep. Don't resend.
				HAGGLE_DBG("Application %s already has data object. Not sending.\n", 
					app->getName().c_str());
			} else {
			        if(app->getName() != MONITOR_APP_NAME) // MOS
				  sendCopyToApplication(dObj, app);
				if(monitorAppNode)
				  sendCopyToApplication(dObj, monitorAppNode); // MOS
			}
		}
	}
}

void ApplicationManager::onNeighborStatusChange(Event *e)
{
       	if (numClients == 0)
		return;

	// MOS
	if (getState() > MANAGER_STATE_RUNNING || dataStoreFinishedProcessing) {
		HAGGLE_DBG("In shutdown, ignoring neighbor status change\n");
		return;
	}
		
	HAGGLE_DBG("Contact update (new or end)! Notifying applications\n");
	
	DataObjectRef dObj = DataObject::create();
	
	if (!dObj)
		return;

	Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_EVENT, "All Applications", dObj->getMetadata());
	
	if (!ctrl_m)
		return;
	
	Metadata *event_m = ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
	
	if (!event_m)
		return;
	
	event_m->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM, intToStr(LIBHAGGLE_EVENT_NEIGHBOR_UPDATE));

	NodeRefList neighList;
        unsigned long num = kernel->getNodeStore()->retrieveNeighbors(neighList);

	if (num) {
		HAGGLE_DBG("Neighbor list size is %d\n", neighList.size());

		int numNeigh = 0;

		for (NodeRefList::iterator it = neighList.begin(); it != neighList.end(); it++) {
			NodeRef neigh = *it;
		
                        if (e && e->getType() == EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN) {
                                InterfaceRef neighIface = e->getInterface();

                                /*
                                  If this was an interface down event
                                  and it was the last interface that
                                  goes down we do nothing. The update
                                  will be handled in a 'node contact
                                  end' event instead.
                                 */
                                if (neighIface && neigh->hasInterface(neighIface) && 
                                    neigh->numActiveInterfaces() == 1)
                                        return;
                        }

			// Add node without bloomfilter to reduce the size of the data object
			Metadata *md = neigh->toMetadata(false);

			if (md) {
				// mark available interfaces
                                struct base64_decode_context b64_ctx;
                                size_t len;

				// find Node Metadata
				Metadata *mdNode = md;
				
                                /*
                                  Add extra information about which interfaces are marked up or down.
                                  This can be of interest to the application.
                                 */
				if (mdNode) {
					Metadata *mdIface = md->getMetadata("Interface");
					while (mdIface) {
                                                // interface type
						const char *typeString = mdIface->getParameter("type");
						Interface::Type_t ifaceType = Interface::strToType(typeString);
						// interface id
						const char *strBase64 = mdIface->getParameter("identifier");
						base64_decode_ctx_init(&b64_ctx);
						unsigned char *identifier = NULL;
						base64_decode_alloc(&b64_ctx, strBase64, strlen(strBase64), (char **)&identifier, &len);
						
                                                if (identifier) {
							InterfaceRef iface = kernel->getInterfaceStore()->retrieve(ifaceType, identifier); 
                                                        if (iface) {
								if (iface->isUp()) {
                                                                        mdIface->setParameter("status", "up");
								} else {
                                                                        mdIface->setParameter("status", "down");
								} 
							} else {
                                                                mdIface->setParameter("status", "down");
							}
                                                        free(identifier);
						}
						
						mdIface = md->getNextMetadata();
					}
				}
			}
			
			if (!event_m->addMetadata(md)) {
				HAGGLE_ERR("Could not add neighbor to IPC data object\n");
			} else {
				numNeigh++;
			}
		}
	}

	sendToAllApplications(dObj, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE);
}

// IRD, HK, Begin
void ApplicationManager::onSendInterestsPolicies(Event *e)
{

 	NodeRef node = e->getNode();

 	if (!node) {
 		HAGGLE_ERR("Node not found in Event\n");
 		return;
 	}

 	if (!e->getDataObject()) {
 		HAGGLE_ERR("Data Object not found in Event\n");
 	}

	DataObjectRef dObj = DataObject::create();
	
	if (!dObj) {
		HAGGLE_ERR("Could not create Data Object\n");
		return;
	}
	
	Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_EVENT, node->getName(), dObj->getMetadata());
							
	if (!ctrl_m) {
		HAGGLE_ERR("Could not add control metadata\n");
		return;
	}

	Metadata *event_m = ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
	
	if (!event_m) {
		HAGGLE_ERR("Could not add event metadata\n");
		return;
	}

	event_m->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM, intToStr(LIBHAGGLE_EVENT_INTERESTS_POLICIES_LIST));

	node.lock();
	
	const Attributes *attrs = e->getDataObject()->getAttributes();
	
	for (Attributes::const_iterator it = attrs->begin(); it != attrs->end(); it++) {
		const Attribute& a = (*it).second;
		HAGGLE_DBG("Adding interest policy %s to the reply\n", a.getString().c_str());
		
		Metadata *interest = event_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST, a.getValue());
		
		if (interest) {
			interest->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_POLICY, a.getName());
		}
	}
	node.unlock();
	
	dObj->setControlMessage(); // MOS
	dObj->setAttrsHashed(false);

	HAGGLE_DBG("Sending application interests policies\n");
	
	ApplicationNodeRef appNode = node;
	sendToApplication(dObj, appNode);
}
// IRD, HK, End

void ApplicationManager::onRetrieveNode(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	if (e->getNode()->getType() != Node::TYPE_APPLICATION) {
		HAGGLE_ERR("Retrieved node is not application\n");
		return;
	}
	
	ApplicationNodeRef appNode = e->getNode();
	
	HAGGLE_DBG("Sending registration reply to application %s\n", appNode->getName().c_str());
	
	if(resetBloomFilterAtRegistration) // MOS 
	  appNode->getBloomfilter()->reset();
	appNode->setProxyId(kernel->getThisNode()->getId()); // MOS
	appNode->setLocalApplication(); // MOS
	kernel->getNodeStore()->add(e->getNode());

	updateApplicationInterests(appNode);
	numClients++;

	// MOS - return local matches upon registration
	if(kernel->firstClassApplications()) { // MOS - disable aggregration
	  DataObjectRef dObj = appNode->getDataObject(); // MOS - get app node description
	  appNode->addNodeDescriptionAttribute(dObj); // MOS - add attribute to enable propagation
	  kernel->getDataStore()->insertDataObject(dObj,onInsertedDataObjectCallback); // MOS - app nodes now first-class citizens
	}
		
	DataObjectRef dObjReply = DataObject::create();
	
	if (!dObjReply) {
		HAGGLE_ERR("Could not allocate data object\n");
		return;
	}
	
	Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_REGISTRATION_REPLY, appNode->getName(), dObjReply->getMetadata());
	
	if (!ctrl_m) {
		HAGGLE_ERR("Could not allocate control metadata\n");
		return;
	}
	
	ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SESSION, intToStr(sessionid++));
	
	ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_MESSAGE, "OK");

	dObjReply->setControlMessage(); // MOS

	sendToApplication(dObjReply, appNode);
	
	HAGGLE_DBG("Sent registration reply to application %s\n", appNode->getName().c_str());	
	
	dObjReply->print(NULL); // MOS - NULL means print to debug trace
}

static const char *ctrl_type_names[] = { 
	"registration_request", 
	"registration_reply", 
	"deregistration", 
	"register_interest",
	"remove_interest",
	"get_interests",
	"register_event_interest", 
	"matching_dataobject",
	"delete_dataobject",
	"get_dataobjects",
	"send_node_description",
	"shutdown", 
	"event", 
	"set_param", 
	"configure_security", // CBMEN, HL
	"dynamic_configure",
	"get_interests_policies",  // IRD, HK
	"send_interests_policies",  // IRD, HK
	NULL 
};

static control_type_t ctrl_name_to_type(const char *name)
{
	unsigned int i = 0;
	
	if (!name)
		return CTRL_TYPE_INVALID;
	
	while (ctrl_type_names[i]) {
		if (strcmp(ctrl_type_names[i], name) == 0) {
			return (control_type_t)i;
		}
		i++;
	}
	
	return CTRL_TYPE_INVALID;
}

Metadata *ApplicationManager::addControlMetadata(const control_type_t type, const string app_name, Metadata *parent)
{
	if (!parent)
		return NULL;
	
	Metadata *m = parent->addMetadata(DATAOBJECT_METADATA_APPLICATION);
	
	if (!m) {
		return NULL;
	}
	
	m->setParameter(DATAOBJECT_METADATA_APPLICATION_NAME_PARAM, app_name);
	
	Metadata *mc = m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL);
	
	if (!mc) {
		return NULL;
	}
	
	mc->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM, ctrl_type_names[type]);
		
	if (type == CTRL_TYPE_REGISTRATION_REPLY) {
		if (!mc->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_DIRECTORY, HAGGLE_DEFAULT_STORAGE_PATH)) {
			return NULL;
		}
	}
	
	return mc;
}

void ApplicationManager::onReceiveFromApplication(Event *e)
{
	Node::Id_t id;
	size_t decodelen = sizeof(Node::Id_t);
	struct base64_decode_context ctx;
	const Attribute *ctrlAttr;
	/*
		If we are in shutdown, silently ignore the application.
	*/
	
	if (!e || !e->hasData())
		return;

	DataObjectRefList& dObjs = e->getDataObjectList();

	if (dObjs.size() == 0) {
		HAGGLE_ERR("No data objects in event\n");
		return;
	}
	
	while (dObjs.size()) {
		DataObjectRef dObj = dObjs.pop();

		if (!dObj->getRemoteInterface()) {
			HAGGLE_DBG("Data object has no source interface\n");
			// return; // MOS - be a bit more tolerant to internally generated objects
#ifdef DEBUG
	dObj->print(NULL, true); // MOS - NULL means print to debug trace
#endif
		}

		ctrlAttr = dObj->getAttribute(HAGGLE_ATTR_CONTROL_NAME);

		if (!ctrlAttr) {
			HAGGLE_ERR("Control data object from application does not have control attribute\n");
			return;
		}

		Metadata *m = dObj->getMetadata()->getMetadata(DATAOBJECT_METADATA_APPLICATION);
		
		if (!m) {
			HAGGLE_ERR("Control data object from application does not have application metadata\n");
			return;
		}
		
		const char *id_str = m->getParameter(DATAOBJECT_METADATA_APPLICATION_ID_PARAM);
		const char *name_str = m->getParameter(DATAOBJECT_METADATA_APPLICATION_NAME_PARAM);
		
		if (!id_str || !name_str) {
			HAGGLE_ERR("Control data object from application does not have id or name\n");
			return;
		}

		base64_decode_ctx_init(&ctx);
		base64_decode(&ctx, id_str, strlen(id_str), (char *)id, &decodelen);

		// MOS - make sure that app node ids on different nodes are different
		// this is important once we treat app nodes as first class citizens
		SHA_CTX ctx;
		SHA1_Init(&ctx);	
		SHA1_Update(&ctx, (unsigned char *)id, NODE_ID_LEN);
		SHA1_Update(&ctx, (unsigned char *)kernel->getThisNode()->getId(), NODE_ID_LEN);
		SHA1_Final((unsigned char *)id, &ctx);

		// Check if the node is in the node store. The result will be a null-node 
		// in case the application is not registered.
		NodeRef node = kernel->getNodeStore()->retrieve(id);

		
		if (node && node->getType() != Node::TYPE_APPLICATION) {
			HAGGLE_ERR("Node in store, which matches application's id, is not an application node\n");
			return;
		}

		ApplicationNodeRef appNode = node;

		Metadata *mc = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL);
		
		if (!mc) {
			HAGGLE_ERR("Data object from application is not a valid control data object\n");
			return;
		}

		bool monitor = appNode && appNode->getName() == MONITOR_APP_NAME; // MOS - passive monitor app - could be better done through a paramter in the future
		if(monitor && !monitorAppNode) monitorAppNode = appNode; // MOS

                // Do not save control data objects in the bloomfilter, otherwise
		// we won't be able to receive a similar message later.
		// if (kernel->getThisNode()->getBloomfilter()->has(dObj))
		//	kernel->getThisNode()->getBloomfilter()->remove(dObj); // MOS - control messages are not inserted into local BF

		/*
		  A control data object may have more than one control element.
		  Loop through them all...
		 */
		while (mc) {
			const char *type_str = mc->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM);
			
			if (!type_str || ctrl_name_to_type(type_str) == CTRL_TYPE_INVALID) {
				HAGGLE_ERR("Data object from application has an invalid control type\n");
				return;
			}
			
			switch (ctrl_name_to_type(type_str)) {
				case CTRL_TYPE_REGISTRATION_REQUEST:
					HAGGLE_DBG("Received registration request from Application \'%s\'\n", name_str);
				
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring registration from application since we are in shutdown\n");
						break;
					}

					if (!dObj->getRemoteInterface()) { // MOS - now checked only for intial registration
					  HAGGLE_DBG("Data object has no source interface, ignoring!\n");
					  return;
					}
					
					if (appNode) {
						HAGGLE_DBG("Application \'%s\' is already registered\n", name_str);
						
						// Create a temporary application node to serve as target for the 
						// return value, since the data object most likely came from a 
						// different interface than the application is registered on.
						NodeRef newAppNode = Node::create_with_id(Node::TYPE_APPLICATION, appNode->getId(), appNode->getName());

						if (!newAppNode)
							break;

						newAppNode->addInterface(dObj->getRemoteInterface());

						DataObjectRef dObjReply = DataObject::create();
						
						if (!dObjReply)
							break;
						
						// Create a reply saying "BUSY!"
						Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_REGISTRATION_REPLY, name_str, dObjReply->getMetadata());
						
						if (ctrl_m) {
							ApplicationNodeRef node = newAppNode;
							ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_MESSAGE, "Already registered");
							dObjReply->setControlMessage(); // MOS
							sendToApplication(dObjReply, node);
						}
					} else {
						HAGGLE_DBG("app name=\'%s\'\n", name_str);
						
						NodeRef appNode = Node::create_with_id(Node::TYPE_APPLICATION, id, name_str);

						if (!appNode)
							break;

						appNode->setProxyId(kernel->getThisNode()->getId()); // MOS
						appNode->setLocalApplication(); // MOS
						appNode->setMatchingThreshold(kernel->getThisNode()->getMatchingThreshold()); // MOS
						appNode->setMaxDataObjectsInMatch(kernel->getThisNode()->getMaxDataObjectsInMatch()); // MOS
						appNode->addInterface(dObj->getRemoteInterface());
						appNode->setDeleteStateOnDeRegister(deleteStateOnDeregister); // SW: keep track of delete state

						// The reply will be generated by the callback event handler.
						kernel->getDataStore()->retrieveNode(appNode, onRetrieveNodeCallback, true); 
						// MOS - this will callback even if no matching node found
					}
					break;
				case CTRL_TYPE_DEREGISTRATION_NOTICE:
					if (!appNode)
						break;
					
					HAGGLE_DBG("Application \'%s\' wants to deregister\n", appNode->getName().c_str());
// SW: START: delete application state
					{
					bool deleteStateOnDeregister = appNode->getDeleteStateOnDeRegister();
					deRegisterApplication(appNode, deleteStateOnDeregister);
					}
// SW: END: delete application state
				    kernel->addEvent(new Event(EVENT_TYPE_APP_NODE_CHANGED));
					break;
				case CTRL_TYPE_SHUTDOWN:
					if (!appNode)
						break;

					HAGGLE_DBG("Application \'%s\' wants to shutdown\n", appNode->getName().c_str());
					kernel->shutdown();
					break;
				case CTRL_TYPE_SET_PARAM:
				        if (appNode) { 
						// MOS - support set max data objects in match API call
					        const char *maximum_str = mc->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_MAX_DATAOBJECTS_IN_MATCH_PARAM);
						if (maximum_str) {
						  unsigned long maxDataObjectsInMatch = strtoul(maximum_str, NULL, 10);
						  appNode->setMaxDataObjectsInMatch(maxDataObjectsInMatch);
						  kernel->getThisNode()->setMaxDataObjectsInMatch(maxDataObjectsInMatch);
						  HAGGLE_DBG("Application \'%s\' sets max data objects in match to %lu\n", appNode->getName().c_str(),maxDataObjectsInMatch);
						}
						// MOS - support set matching threshold API call
						const char *threshold_str = mc->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_MATCHING_THRESHOLD_PARAM);
						if (threshold_str) {
						  unsigned long matchingThreshold = strtoul(threshold_str, NULL, 10);
						  appNode->setMatchingThreshold(matchingThreshold);
						  kernel->getThisNode()->setMatchingThreshold(matchingThreshold);
						  HAGGLE_DBG("Application \'%s\' sets matching threshold to %lu\n", appNode->getName().c_str(),matchingThreshold);
						}
// SW: START: delete application state
						const char *dereg_delete_str = mc->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_DEREG_DELETE_PARAM);
						if (dereg_delete_str) {
						  bool dereg_delete = 0 == strcmp(dereg_delete_str, "true");
						  if (dereg_delete) {
                            appNode->setDeleteStateOnDeRegister(true);
                          }
						  HAGGLE_DBG("Application \'%s\' set delete state on deregistration: %s\n",appNode->getName().c_str(), dereg_delete ? "true" : "false");
						}
// SW: END: delete application state
					} else {
						HAGGLE_ERR("No application node \'%s\' for request to set paramters\n", name_str);
					}
				  break;
				case CTRL_TYPE_REGISTER_INTEREST:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring register interest from application since we are in shutdown\n");
						break;
					}
					
					if (appNode) {
						unsigned long numattrs = 0;
						unsigned long numattrsThisNode = 0;						
						Metadata *interest = mc->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST);
						
						while (interest) {
							const char *interest_name_str = interest->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_NAME_PARAM);
							const char *interest_weight_str = interest->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_WEIGHT_PARAM);
							unsigned long weight = 1;
							
							if (interest_weight_str) {
								weight = strtoul(interest_weight_str, NULL, 10);
							}
							if (interest_name_str) {
								
							  if(!kernel->firstClassApplications()) { // MOS - disable aggregration for first class applications
								if (!monitor &&
								    kernel->getThisNode()->addAttribute(interest_name_str, interest->getContent(), weight)) {
									numattrsThisNode++;
									HAGGLE_DBG("Application \'%s\' adds interest %s:%s:%lu to thisNode\n", 
										   appNode->getName().c_str(), interest_name_str, 
										   interest->getContent().c_str(), weight);
								}
							  }

							  if (appNode->addAttribute(interest_name_str, interest->getContent(), weight)) {
							    numattrs++;
							    HAGGLE_DBG("Application \'%s\' adds interest %s:%s:%lu\n", 
								       appNode->getName().c_str(), interest_name_str, 
								       interest->getContent().c_str(), weight);
							  }
							}
							interest = mc->getNextMetadata();
						}

						if (numattrs) {
							updateApplicationInterests(appNode);
							NodeRef node = appNode;

							if(kernel->firstClassApplications()) {
							  DataObjectRef dObj = node->getDataObject(); // MOS - get app node description
							  dObj->setSignatureStatus(DataObject::SIGNATURE_MISSING); // CBMEN, HL - We need to re-sign or signature verification will fail
							  node->addNodeDescriptionAttribute(dObj); // MOS - add attribute to enable propagation
							  kernel->getDataStore()->insertDataObject(dObj,onInsertedDataObjectCallback); // MOS - app nodes now first-class citizens
							  numattrsThisNode = 1; // MOS - trigger code below
							}
							else {	
							  kernel->getDataStore()->insertNode(node); // MOS - this is important to allow recompilation of the node attributes in the FIXME below
							}
						}
						if (numattrsThisNode && !monitor) {
							// Push the updated node description to all neighbors
							kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));
							kernel->addEvent(new Event(EVENT_TYPE_APP_NODE_CHANGED));
						}						
					} else {
						HAGGLE_ERR("No application node \'%s\' for request to register interests\n", name_str);
					}
					break;
				case CTRL_TYPE_REMOVE_INTEREST:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring remove interest from application since we are in shutdown\n");
						break;
					}
					
					if (appNode) {
						unsigned long numattrs = 0;

						Metadata *interest = mc->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST);
						
						while (interest) {
							const char *interest_name_str = interest->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_NAME_PARAM);
							
							if (interest_name_str) {
								if (appNode->removeAttribute(interest_name_str, interest->getContent())) {
									numattrs++;
									HAGGLE_DBG("Application \'%s\' removes interest %s:%s\n", 
										   appNode->getName().c_str(), interest_name_str, interest->getContent().c_str());
								} else {
									HAGGLE_ERR("Could not remove interest %s=%s\n", name_str, interest->getContent().c_str());
								}
							}
							interest = mc->getNextMetadata();
						}
						if (numattrs) {
							updateApplicationInterests(appNode);
							
							// FIXME:
							// Insert updated node into node store. This is necessary because
							// the next call to update the node description will retrieve all
							// application nodes from the data store and recompile the node
							// description from their interests. Therefore, the updated app node
							// has to be inserted first. This is not very efficient, as we should
							// keep the current node in memory and only insert it when the application
							// deregisters.
							NodeRef node = appNode;
							kernel->getDataStore()->insertNode(node);
							
							// Update the node description and send it to all 
							// neighbors.
							
							if(kernel->firstClassApplications()) { // MOS - disable aggregration
							  DataObjectRef dObj = node->getDataObject(); // MOS - get app node description
							  node->addNodeDescriptionAttribute(dObj); // MOS - add attribute to enable propagation
							  kernel->getDataStore()->insertDataObject(dObj,onInsertedDataObjectCallback); // MOS - app nodes now first-class citizens
							  // Push the updated node description to all neighbors
							  kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));
						      kernel->addEvent(new Event(EVENT_TYPE_APP_NODE_CHANGED));
							}
							else {
							  kernel->getDataStore()->retrieveNode(Node::TYPE_APPLICATION, onRetrieveAppNodesCallback);
							}	
						}
					} else {
						HAGGLE_ERR("No application node \'%s\' for request to remove interests\n", name_str);
					}
					break;
				case CTRL_TYPE_GET_INTERESTS:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring get interests from application since we are in shutdown\n");
						break;
					}
					
					HAGGLE_DBG("Request for application interests\n");
					
					if (appNode) {
						DataObjectRef dObjReply = DataObject::create();
						
						if (!dObjReply)
							break;
						
						Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_EVENT, name_str, dObjReply->getMetadata());
												
						if (!ctrl_m) {
							HAGGLE_ERR("Could not add control metadata\n");
							break;
						}

						Metadata *event_m = ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
						
						if (!event_m) {
							HAGGLE_ERR("Could not add event metadata\n");
							break;
						}
						
						event_m->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM, intToStr(LIBHAGGLE_EVENT_INTEREST_LIST));
				
						appNode.lock();
						
						const Attributes *attrs = appNode->getAttributes();
						
						for (Attributes::const_iterator it = attrs->begin(); it != attrs->end(); it++) {
							const Attribute& a = (*it).second;
							HAGGLE_DBG("Adding interest %s to the reply\n", a.getString().c_str());
							
							Metadata *interest = event_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST, a.getValue());
							
							if (interest) {
								interest->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_NAME_PARAM, a.getName());
								interest->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_WEIGHT_PARAM, a.getWeightAsString());
							}
						}
						appNode.unlock();
						
						dObjReply->setControlMessage(); // MOS


						HAGGLE_DBG("Sending application interests\n");
						
						sendToApplication(dObjReply, appNode);
					} else {
						HAGGLE_ERR("No application node\n");
					}
					break;
				case CTRL_TYPE_REGISTER_EVENT_INTEREST:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring register event from application since we are in shutdown\n");
						break;
					}
					
					if (appNode) {						
						Metadata *event = mc->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
						
						HAGGLE_DBG("REGISTER event interest\n");
						while (event) {
							const char *event_type_str = event->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM);
							
							HAGGLE_DBG("event type is %s\n", event_type_str);
							if (event_type_str) {
								addApplicationEventInterest(appNode, atoi(event_type_str));
							}
							event = mc->getNextMetadata();
						}
					}
					break;
				case CTRL_TYPE_GET_DATAOBJECTS:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring get data objects from application since we are in shutdown\n");
						break;
					}
					
					if (!appNode)
						break;
					
					// Clear the bloomfilter:
					appNode->getBloomfilter()->reset();
					// And do a filter matching for this node:
					updateApplicationInterests(appNode);

					if(kernel->firstClassApplications()) {
					  DataObjectRef dObj = node->getDataObject(); // MOS - get app node description
					  node->addNodeDescriptionAttribute(dObj); // MOS - add attribute to enable propagation
					  kernel->getDataStore()->insertDataObject(dObj,onInsertedDataObjectCallback); // MOS - app nodes now first-class citizens
					}

					break;
				case CTRL_TYPE_DELETE_DATAOBJECT:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring delete data object from application since we are in shutdown\n");
						break;
					}
					
					if (appNode) {
						DataObjectId_t id;
						base64_decode_context ctx;
						bool keep_in_bloomfilter = false;
						Metadata *dobj_m = mc->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT);
				
						if (!dobj_m)
							break;
						
						const char *id_str = dobj_m->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_ID_PARAM);
					       
						if (!id_str)
							break;
						
						const char *keep_str = mc->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_BLOOMFILTER_PARAM);
						
						if (keep_str) {
							if (strcmp(keep_str, "yes") == 0)
								keep_in_bloomfilter = true;
						}
						size_t len = DATAOBJECT_ID_LEN;
						base64_decode_ctx_init(&ctx);
						
						if (base64_decode(&ctx, id_str, strlen(id_str), (char *)id, &len)) {
							kernel->getDataStore()->deleteDataObject(id, keep_in_bloomfilter);
							
							if (!keep_in_bloomfilter) {
								HAGGLE_DBG("Deleting data object from application %s's bloomfilter\n", 
									   appNode->getName().c_str());
								appNode->getBloomfilter()->remove(id);
							}
						}
					}
					break;
			        case CTRL_TYPE_SEND_NODE_DESCRIPTION:
					if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
						HAGGLE_DBG("Ignoring send node description from application since we are in shutdown\n");
						break;
					}	
					
					if (kernel->getNodeStore()->numNeighbors())
						kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));
					break;
					// CBMEN, HL, Begin
					case CTRL_TYPE_CONFIGURE_SECURITY:
						if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
							HAGGLE_DBG("Ignoring security configuration from application since we are in shutdown\n");
							break;
						}
						HAGGLE_DBG("Application %s asking to configure security\n", name_str);
						kernel->addEvent(new Event(EVENT_TYPE_SECURITY_CONFIGURE, dObj));
					break;

					// IRD, HK, Begin
					case CTRL_TYPE_GET_INTERESTS_POLICIES:
						if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
							HAGGLE_DBG("Ignoring get interests policies from application since we are in shutdown\n");
							break;
						}
						
						HAGGLE_DBG("Application %s requesting interests policies.\n", name_str);
 						kernel->addEvent(new Event(EVENT_TYPE_APP_NODE_INTERESTS_POLICIES_REQUESTED, node));
						break;					
					// IRD, HK, End
					case CTRL_TYPE_DYNAMIC_CONFIGURE:
					{
						if (getState() >= MANAGER_STATE_PREPARE_SHUTDOWN) {
							HAGGLE_DBG("Ignoring dynamic configuration event from application since we are in shutdown\n");
							break;
						}	

						HAGGLE_DBG("Got dynamic configuration event!\n");
						Metadata *config_m = mc->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_DYNAMIC_CONFIGURATION);
						if (!config_m) {
							break;
						}

						DataObjectRef dObj = DataObject::create();
						dObj->getMetadata()->addMetadata(config_m->copy());

						kernel->addEvent(new Event(EVENT_TYPE_DYNAMIC_CONFIGURE, dObj));
					}
					break;
					// CBMEN, HL, End
				case CTRL_TYPE_INVALID:
				default:
					HAGGLE_ERR("Data object from application has invalid control type=%s\n", type_str);
					return;
			}
			mc = m->getNextMetadata();
		}
	}
}

// MOS

void ApplicationManager::onInsertedDataObject(Event * e)
{
	if (!e || !e->hasData())
		return;
	
	DataObjectRef& dObj = e->getDataObject();

	if (dObj->isDuplicate()) {
		HAGGLE_DBG("Data object %s is a duplicate! Not generating DATAOBJECT_NEW event\n", dObj->getIdStr());
	} else {
		kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_NEW, dObj));
	}
}

// CBMEN, HL - Add default interest
bool ApplicationManager::addDefaultInterest(const Attribute& attr) {
	kernel->getThisNode()->addAttribute(attr);
	defaultInterests.add(attr);
	return true;
}

// MOS - default interest is only for testing, e.g. to enable flooding of node descriptions without using a specific forwarder,
void ApplicationManager::addDefaultInterests(NodeRef node)
{
  for (Attributes::iterator it = defaultInterests.begin(); it != defaultInterests.end(); it++) {
    node->addAttribute((*it).second.getName(), (*it).second.getValue(), (*it).second.getWeight());
    HAGGLE_DBG("adding default interest %s=%s:%d to node %s\n",(*it).second.getName().c_str(), (*it).second.getValue().c_str(), (*it).second.getWeight(),node->getName().c_str());
  }
}

// MOS

void ApplicationManager::onConfig(Metadata *m)
{
        // Parse attributes
        Metadata *mattr = m->getMetadata(DATAOBJECT_ATTRIBUTE_NAME); // MOS
        
        while (mattr) {
                const char *attrName = mattr->getParameter(DATAOBJECT_ATTRIBUTE_NAME_PARAM);
                const char *weightStr = mattr->getParameter(DATAOBJECT_ATTRIBUTE_WEIGHT_PARAM);
                unsigned long weight = weightStr ? strtoul(weightStr, NULL, 10) : 1;

                Attribute a(attrName, mattr->getContent(), weight);

		defaultInterests.add(a);
		
				HAGGLE_DBG("Adding defaultInterest %s\n", a.getString().c_str());
                mattr = m->getNextMetadata();
        }

  const char *param = m->getParameter("reset_bloomfilter_at_registration"); // MOS
	
  if (param) {
    if (strcmp(param, "true") == 0) {
      HAGGLE_DBG("Enabling Bloom filter reset at registration\n");
      resetBloomFilterAtRegistration = true;
    } else if (strcmp(param, "false") == 0) {
      HAGGLE_DBG("Disabling Bloom filter reset at registration\n");
      resetBloomFilterAtRegistration = false;
    }
  }

  param = m->getParameter("first_class_applications");
	
  if (param) {
    if (strcmp(param, "true") == 0) {
      HAGGLE_DBG("Enabling first class applications\n");
      kernel->setFirstClassApplications(true);
    } else if (strcmp(param, "false") == 0) {
      HAGGLE_DBG("Disabling first class applications\n");
      kernel->setFirstClassApplications(false);
    }
  }

// SW: 
  param = m->getParameter("delete_state_on_deregister");
	
  if (param) {
    if (strcmp(param, "true") == 0) {
      HAGGLE_DBG("Deleting state on application deregister \n");
	deleteStateOnDeregister = true;
    } else if (strcmp(param, "false") == 0) {
      HAGGLE_DBG("Not deleting state on application deregister\n");
	deleteStateOnDeregister = false;
    }
  }

  param = m->getParameter("forced_shutdown_after_timeout");
 
  if (param) {		
    char *endptr = NULL;
    unsigned long value = strtoul(param, &endptr, 10);
    if (endptr && endptr != param) {
      forcedShutdownAfterTimeout = value;
      HAGGLE_DBG("forced_shutdown_after_timeout=%lu\n", forcedShutdownAfterTimeout);
    }
  }

  Metadata *dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION);
  if (dm) {
  	configureObserver(dm);
  }
}

// MOS - refactored into its own function

void ApplicationManager::sendCopyToApplication(DataObjectRef dObj, ApplicationNodeRef target) {
  if (getState() > MANAGER_STATE_RUNNING) {
    HAGGLE_DBG("In shutdown, not sending new data objects to applications\n");
    return;
  }

  // Do not give node descriptions to applications.
  if (dObj->isNodeDescription()) {
    HAGGLE_DBG("Data object [%s] is a node description, not sending to %s\n", 
	       dObj->getIdStr(), target->getName().c_str());
  } else {
    string dObjName = "DataObject[App:" + target->getName() + "]";
    
    DataObjectRef dObjSend = dObj->copy();
    
    Metadata *ctrl_m = addControlMetadata(CTRL_TYPE_EVENT, target->getName(), dObjSend->getMetadata());
    
    if (!ctrl_m) {
      HAGGLE_ERR("Failed to add control metadata\n");
    } else {
      Metadata *event_m = ctrl_m->addMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT);
      
      if (!event_m) {
	HAGGLE_ERR("Failed to add event metadata\n");
      } else {
	event_m->setParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM, intToStr(LIBHAGGLE_EVENT_NEW_DATAOBJECT));
	
	dObjSend->setPersistent(false);
	
	/*
	  Indicate that this data object is for a
	  local application, which means the file path
	  to the local file will be added to the
	  metadata once the data object is transformed
	  to wire format.
	*/
	dObjSend->setIsForLocalApp();
	if(target->getName() == MONITOR_APP_NAME) dObjSend->setIsForMonitorApp();

#if 0
	unsigned char *raw;
	size_t len;
	
	dObjSend->getRawMetadataAlloc(&raw, &len);

	if (raw) {
	  printf("App - DataObject METADATA:\n%s\n", raw);
	  free(raw);
	}
#endif

	sendToApplication(dObjSend, target);
      }
    }
  }
}

// MOS - event-based interface for the above

void ApplicationManager::onSendDataObjectToApp(Event *e)
{
  DataObjectRef& dObj = e->getDataObject();

  if (getState() > MANAGER_STATE_RUNNING) {
    HAGGLE_DBG("In shutdown, not sending new data objects to applications\n");
    return;
  }

  if( this->applicationManagerHelper->shouldNotSendToApplication(dObj)) {
    HAGGLE_DBG("Not passing databoject to application id=|%s|\n",dObj->getIdStr());
    return;
  }

  NodeRefList apps = e->getNodeList();
  for (NodeRefList::iterator it = apps.begin(); it != apps.end(); it++) {
    ApplicationNodeRef app = *it;
    if(app->getName() != MONITOR_APP_NAME) // MOS
      sendCopyToApplication(e->getDataObject(), app);
    if(monitorAppNode) 
      sendCopyToApplication(e->getDataObject(), monitorAppNode); // MOS
  }  
}

// CBMEN, HL, Begin
void ApplicationManager::createNodeMetricsObserverDataObject() {

	Metadata *m, *nm;

	nm = new XMLMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_METRICS);
	if (!nm)
		return;

	m = nm->addMetadata("CPU");
	if (m) {
		struct rusage usage;
		if (getrusage(RUSAGE_SELF, &usage) == 0) {
			m->setParameter("user_cpu_time", usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1.0e6));
			m->setParameter("system_cpu_time", usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1.0e6));
			m->setParameter("max_rss", usage.ru_maxrss);
			m->setParameter("soft_page_faults", usage.ru_minflt);
			m->setParameter("hard_page_faults", usage.ru_majflt);
			m->setParameter("block_input_ops", usage.ru_inblock);
			m->setParameter("block_output_ops", usage.ru_oublock);
			m->setParameter("voluntary_context_switches", usage.ru_nvcsw);
			m->setParameter("involuntary_context_switches", usage.ru_nivcsw);

		}
	}

	m = nm->addMetadata("Memory");
	if (m) {
		struct mallinfo mi = mallinfo();
		m->setParameter("malloc_total", mi.arena + mi.hblkhd);
		m->setParameter("not_mmaped", mi.arena);
		m->setParameter("mmaped", mi.hblkhd);
		m->setParameter("allocated", mi.uordblks);
		m->setParameter("free", mi.fordblks);
		m->setParameter("releasable", mi.keepcost);
		m->setParameter("free_chunks", mi.ordblks);
		m->setParameter("fast_bin_blocks", mi.smblks);
		m->setParameter("available_in_fast_bins", mi.fsmblks);
		m->setParameter("sqlite", (int64_t) sqlite3_memory_used());
	}

	m = nm->addMetadata("Bandwidth");
	if (m) {
		char *buffer = (char *)malloc(1024 * sizeof(char));
		size_t len = 1024;
		char buffers[17][64];
		const InterfaceRefList* interfaces = kernel->getThisNode()->getInterfaces();
		FILE *fin;
		size_t line = 0;

		fin = fopen("/proc/net/dev", "r");
		if (!fin) {
			HAGGLE_ERR("Failed to read /proc/net/dev!\n");
			free(buffer);
			goto block_end;
		}

		while (fgets(buffer, len, fin) == buffer) {
			line++;
			if (line == 1) {
				continue;
			}

			if (sscanf(buffer, " %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s ",
		        buffers[0], buffers[1], buffers[2], buffers[3], buffers[4], buffers[5], buffers[6],
		        buffers[7], buffers[8], buffers[9], buffers[10], buffers[11], buffers[12], buffers[13],
		        buffers[14], buffers[15], buffers[16]) == 17) {

				for (InterfaceRefList::const_iterator it = interfaces->begin(); it != interfaces->end(); it++) {
					string iname = (*it)->getName();
					string name(buffers[0]);
					name = name.substr(0, name.size() - 1);

					if (iname == name) {
						Metadata *dm = m->addMetadata("Interface");
						if (dm) {
							dm->setParameter("name", name.c_str());
							dm->setParameter("rx_bytes", buffers[1]);
							dm->setParameter("tx_bytes", buffers[9]);
							dm->setParameter("rx_packets", buffers[2]);
							dm->setParameter("tx_packets", buffers[10]);
						}
					}
				}
			}
		}

		free(buffer);
		fclose(fin);
	}

block_end:
	m = nm->addMetadata("Kernel");
	if (m) {
		m->setParameter("event_queue_size", kernel->size());

		Metadata *dm = m->addMetadata("EventQueue");
		if (dm) {
			kernel->log_eventQueue(dm);
		}
	}

	HAGGLE_DBG2("Generating node metrics observer data object\n");
	kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, nm));
}

void ApplicationManager::createInterfacesObserverDataObject() {
	Metadata *m = new XMLMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_INTERFACES);

	if (!m)
		return;

	kernel->getInterfaceStore()->getInterfacesAsMetadata(m);
	kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
}

void ApplicationManager::createCertificatesObserverDataObject() {
	kernel->addEvent(new Event(new DebugCmd(DBG_CMD_OBSERVE_CERTIFICATES, DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_CERTIFICATES)));
}

void ApplicationManager::createNodeDescriptionObserverDataObject() {
	Metadata *m = new XMLMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_DESCRIPTION);

	if (!m)
		return;

	DataObjectRef dObj = kernel->getThisNode()->getDataObject();
	Metadata *dm = NULL;

	dObj.lock();
	dm = dObj->getMetadata()->getMetadata("Node");
	if (!dm) {
		dObj.unlock();
		return;
	}
	dm = dm->copy();

	m->addMetadata(dm);
	dm = dm->addMetadata("Attributes");

	if (dm) {
		const Attributes *attrs = dObj->getAttributes();

		for (Attributes::const_iterator it = attrs->begin(); it != attrs->end(); it++) {
			Metadata *dmm = dm->addMetadata("Attribute");
			if (dmm) {
				dmm->setParameter("name", (*it).second.getName());
				dmm->setParameter("value", (*it).second.getValue());
				dmm->setParameter("weight", (*it).second.getWeight());
			}
		}
	}
	dObj.unlock();

	kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
}

void ApplicationManager::createNodeStoreObserverDataObject() {
	Metadata *m = new XMLMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_STORE);

	if (!m)
		return;

	kernel->getNodeStore()->getNodesAsMetadata(m);
	kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
}

void ApplicationManager::createProtocolObserverDataObject() {
	kernel->addEvent(new Event(new DebugCmd(DBG_CMD_OBSERVE_PROTOCOLS, DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_PROTOCOLS)));
}

void ApplicationManager::createRoutingTableObserverDataObject() {
	kernel->addEvent(new Event(new DebugCmd(DBG_CMD_OBSERVE_ROUTING_TABLE, DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_ROUTING_TABLE)));
}

void ApplicationManager::createCacheStrategyObserverDataObject() {
	kernel->addEvent(new Event(new DebugCmd(DBG_CMD_OBSERVE_CACHE_STRAT, DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_CACHE_STRATEGY)));
}

void ApplicationManager::createDataStoreDumpObserverDataObject() {
	char path[1024];
	snprintf(path, 1024, "%s%shaggle_db_%lu.dump", kernel->getStoragePath().c_str(), PLATFORM_PATH_DELIMITER, dumpedDBs);
	dumpedDBs++;
	kernel->getDataStore()->dumpToFile(path);

	Metadata *m = new XMLMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_DATA_STORE_DUMP);

	if (!m)
		return;

	Metadata *dm = m->addMetadata("FilePath", path);
	
	if (!dm)
		return;

	kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
}

static const char *observableMetadatas[] = { 
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_METRICS, 
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_INTERFACES,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_CERTIFICATES,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_DESCRIPTION,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_NODE_STORE,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_PROTOCOLS,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_ROUTING_TABLE,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_CACHE_STRATEGY,
	DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_OBSERVE_DATA_STORE_DUMP,
	NULL 
};

void ApplicationManager::onObserverCallback(Event *e) {

	if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, not handling observer callback\n");
		return;
	}

	waitingForObserverCallback = false;
	HAGGLE_DBG("In onObserverCallback!\n");

	typedef void(ApplicationManager::*handlerFunc)(void);
	static const handlerFunc handlers[] = {
		&ApplicationManager::createNodeMetricsObserverDataObject,
		&ApplicationManager::createInterfacesObserverDataObject,
		&ApplicationManager::createCertificatesObserverDataObject,
		&ApplicationManager::createNodeDescriptionObserverDataObject,
		&ApplicationManager::createNodeStoreObserverDataObject,
		&ApplicationManager::createProtocolObserverDataObject,
		&ApplicationManager::createRoutingTableObserverDataObject,
		&ApplicationManager::createCacheStrategyObserverDataObject,
		&ApplicationManager::createDataStoreDumpObserverDataObject,
		NULL
	};

	for (HashMap<string, bool>::iterator it = enabledObservables.begin(); it != enabledObservables.end(); it++) {
		const char *observable = (*it).first.c_str();
		const char *tmp = NULL;
		for (size_t i = 0; ((tmp = observableMetadatas[i]) != NULL); i++) {
			if (strcmp(observable, tmp) == 0) {
				if (handlers[i]) {
					(this->*handlers[i])();
					break;
				} else {
					HAGGLE_ERR("Observable %s is not handled!\n", observable);
				}
			}
		}
	}

	if (observerCallbackDelay > 0) {
		kernel->addEvent(new Event(observerCallback, NULL, observerCallbackDelay));
		waitingForObserverCallback = true;
	}

}

void ApplicationManager::onSendObserverDataObject(Event *e) {

	HAGGLE_DBG("In onSendObserverDataObject\n");
	if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, not sending observer data objects to applications\n");
		return;
	}

	if (!e || !e->hasData()) {
		HAGGLE_DBG("Empty event!\n");
		return;
	}

	Metadata *msg = (Metadata *) e->getData();

	if (!tempFile) {
		int fd = -1;
		char filepath[60] = "";
		strncpy(filepath, baseTempFilePath.c_str(), sizeof(filepath));
	    if ((fd = mkstemp(filepath)) == -1 ||
	        (tempFile = fdopen(fd, "wb")) == NULL) {
	        if (fd != -1) {
	            unlink(filepath);
	            close(fd);
	        }
	        HAGGLE_ERR("Couldn't create temporary file.\n");
	        goto end;
	    } else {
	    	tempFilePath = string(filepath);
	    }
	}

    unsigned char *raw;
	size_t len;
	if (!msg->getRawAlloc(&raw, &len)) {
		goto cleanup_msg;
	}

	fwrite(raw, 1, len, tempFile);
    if (ferror(tempFile)) {
        HAGGLE_ERR("Couldn't write out to temporary file\n");
        goto cleanup_raw;
    } 

    dumpedObservables++;
    if (dumpedObservables < enabledObservables.size()) {
    	goto cleanup_raw;
    } else {
	    fclose(tempFile);
	    tempFile = NULL;
	    dumpedObservables = 0;
	    observerDOPublishedCount++;

	    DataObjectRef dObj = DataObject::create(tempFilePath, "");
	    if (!dObj) {
	    	goto cleanup_raw;
	    }

	    addAdditionalObserverAttributes(dObj);
	    dObj->print(NULL);
	    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_RECEIVED, dObj));

	}

cleanup_raw:
	free(raw);
cleanup_msg:
	delete msg;
end:
	return;
}

void ApplicationManager::onDynamicConfig(Metadata *m) {

	if (!m)
		return;

	Metadata *dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION);
	if (dm) {
		HAGGLE_DBG("Dynamic Configuration updates observer settings!\n");
		configureObserver(dm);
	}
}

void ApplicationManager::configureObserver(Metadata *m) {

	HAGGLE_DBG("Configuring Observer\n");

	Metadata *dm = NULL;

    dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_OBSERVER_CONFIGURATION_NOTIFICATION_INTERVAL);
    if (dm) {
    	const char *param = dm->getContent().c_str();
    	char *endptr = NULL;
        double tmp;
        tmp = strtod(param, &endptr);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting observerCallbackDelay to %f\n", tmp); 
            observerCallbackDelay = tmp;
        }
    }

    enabledObservables.clear();
    observerDOPublishedCount = 0;

    const char *observable = NULL;
    for (size_t i = 0; ((observable = observableMetadatas[i]) != NULL); i++) {
    	dm = m->getMetadata(observable);
    	if (dm) {
    		if (strcmp(dm->getContent().c_str(), "true") == 0) {
    			enabledObservables.insert(make_pair(observable, true));
    			HAGGLE_DBG("Enabling observable %s\n", observable);
    		}
    	}
    }

    if (!waitingForObserverCallback && observerCallbackDelay > 0) {
    	HAGGLE_DBG("Creating observer callback event \n");
    	kernel->addEvent(new Event(observerCallback, NULL, observerCallbackDelay));
    	waitingForObserverCallback = true;
    }

	Metadata *attr = NULL;
	dm = m->getMetadata("Attributes");
	if (!dm) {
		return;
	}

	attr = dm->getMetadata("Attr");
	if (!attr) {
		return;
	}

	string name, value;
	unsigned long weight;
	unsigned long every;
	const char *param;
	do {
		name = attr->getParameter("name");
		value = attr->getContent();

		weight = 0;
		param = attr->getParameter("weight");
		if (param) {
			char *endptr = NULL;
			unsigned long tmp;
			tmp = strtoul(param, &endptr, 10);

			if (endptr && endptr != param) {
				weight = tmp;
			}
		}

		every = 1;
		param = attr->getParameter("every");
		if (param) {
			char *endptr = NULL;
			unsigned long tmp;
			tmp = strtoul(param, &endptr, 10);

			if (endptr && endptr != param) {
				every = tmp;
			}
		}

		Attribute a(name, value, weight);
		HAGGLE_DBG("Adding additionalObserverAttribute %s every %u\n", a.getString().c_str(), every);
		additionalObserverAttributes.push_back(make_pair(a, every));
	} while ((attr = dm->getNextMetadata()));

}

void ApplicationManager::addAdditionalObserverAttributes(DataObjectRef& dObj) {
    for (List<Pair<Attribute, size_t> >::iterator it = additionalObserverAttributes.begin(); it != additionalObserverAttributes.end(); it++) {
        Attribute attr = (*it).first;
        size_t every = (*it).second;
        Attribute add = attr;

        if ((observerDOPublishedCount % every) != 0) {
        	HAGGLE_DBG("Skipping additionalObserverAttribute %s (%u/%u)\n", add.getString().c_str(), observerDOPublishedCount, every);
        	continue;
        }

        if (attr.getValue() == "%%replace_current_time%%") {
            char tmp[30];
            snprintf(tmp, 30, "%llu", (unsigned long long)time(NULL));
            add = Attribute(attr.getName(), string(tmp), attr.getWeight());
        } else if (attr.getValue() == "%%replace_current_node_name%%") {
            add = Attribute(attr.getName(), kernel->getThisNode()->getName(), attr.getWeight());
        } else if (attr.getValue() == "%%replace_current_node_id%%") {
            add = Attribute(attr.getName(), kernel->getThisNode()->getIdStr(), attr.getWeight());
        }

        HAGGLE_DBG("Adding additionalObserverAttribute %s (%u/%u)\n", add.getString().c_str(), observerDOPublishedCount, every);
        dObj->addAttribute(add);
    }
}
// CBMEN, HL, End
