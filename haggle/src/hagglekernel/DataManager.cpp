/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   James Mathewson (JLM)
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
#include <string.h>

#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "EventQueue.h"
#include "DataStore.h"
#include "DataManager.h"
#include "DataObject.h"
#include "Node.h"
#include "Event.h"
#include "Interface.h"
#include "Attribute.h"
#include "Filter.h"
#include "XMLMetadata.h"

// SW: JLM: START CACHE STRATEGY:
#include "CacheStrategyFactory.h"
#include "CacheStrategy.h"
// SW: JLM: END CACHE STRATEGY.
using namespace haggle;

DataTask::DataTask(const DataTaskType_t _type, DataObjectRef _dObj) : 
	type(_type), completed(false), dObj(_dObj)
{
}

DataTask::~DataTask()
{
}

DataHelper::DataHelper(DataManager *m, const EventType _etype) : 
	ManagerModule<DataManager>(m, "DataHelper"), taskQ("DataHelper"), etype(_etype)
{
}

DataHelper::~DataHelper()
{
	if (isRunning())
		stop();
	
	while (!taskQ.empty()) {
		DataTask *task = NULL;
		taskQ.retrieve(&task);
		delete task;
	}
}

void DataHelper::doTask(DataTask *task)
{
        DataObject::DataState_t state = DataObject::DATA_STATE_NOT_VERIFIED;

	switch (task->type) {
		case DATA_TASK_VERIFY_DATA:
			HAGGLE_DBG("DataHelper tries to verify the data in a data object\n");
                        state = task->dObj->verifyData();

			if (state == DataObject::DATA_STATE_VERIFIED_BAD) {
			  HAGGLE_ERR("Could not verify the data hash of data object %s ! - Discarding...\n", 
				     DataObject::idString(task->dObj).c_str());
			  getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, task->dObj, false)); // MOS
				delete task;
				return;
			} else if (state == DataObject::DATA_STATE_NOT_VERIFIED) {
                                HAGGLE_ERR("Could not verify data... No hash? - Discarding ...\n");
				getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, task->dObj, false)); // MOS
				delete task;
				return;
                        } else if (state == DataObject::DATA_STATE_VERIFIED_OK) {
                                HAGGLE_DBG("Data object's data is OK\n");
                        } else if (state == DataObject::DATA_STATE_NO_DATA) {
                                HAGGLE_ERR("Instructed to verify a data object without data\n");
                        }
			task->completed = true;
			break;
		default:
                        HAGGLE_ERR("Unknown data task\n");
			delete task;
			return;
	}
	// Return result if the private event is valid
	if (Event::isPrivate(etype))
		addEvent(new Event(etype, task));
	else
		delete task;
}

bool DataHelper::run()
{	
	HAGGLE_DBG("DataHelper running...\n");
	
	while (!shouldExit()) {
		QueueEvent_t qe;
		DataTask *task = NULL;
		
		qe = taskQ.retrieve(&task);
		
		switch (qe) {
		case QUEUE_ELEMENT:
			doTask(task);
			break;
		case QUEUE_WATCH_ABANDONED:
			HAGGLE_DBG("DataHelper instructed to exit...\n");
			return false;
		default:
			HAGGLE_ERR("Unknown data task queue return value\n");
		}
	}
	return false;
}

void DataHelper::cleanup()
{
	while (!taskQ.empty()) {
		DataTask *task = NULL;
		taskQ.retrieve(&task);
		delete task;
	}
}

DataManager::DataManager(HaggleKernel * _kernel, const bool _setCreateTimeOnBloomfilterUpdate) : 
	Manager("DataManager", _kernel), localBF(NULL), 
	setCreateTimeOnBloomfilterUpdate(_setCreateTimeOnBloomfilterUpdate), 
	keepInBloomfilterOnAging(true), 
// SW: JLM: START CACHE STRATEGY:
	cacheStrategy(NULL),
// SW: JLM: END CACHE STRATEGY.
	periodicBloomfilterUpdateEventType(-1), periodicBloomfilterUpdateEvent(NULL),periodicBloomfilterUpdateInterval(0), // MOS
	isNodeDescUpdateOnReceptionEnabled(false) // MOS
{	
	this->networkCodingConfiguration = new NetworkCodingConfiguration();
	this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
	this->fragmentationConfiguration = new FragmentationConfiguration();
	this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();

// SW: START: frag/block max match fix:
        kernel->getDataStore()->setNetworkCodingConfiguration(this->networkCodingConfiguration);
        kernel->getDataStore()->setNetworkCodingDataObjectUtility(this->networkCodingDataObjectUtility); 
        kernel->getDataStore()->setFragmentationConfiguration(this->fragmentationConfiguration);
        kernel->getDataStore()->setFragmentationDataObjectUtility(this->fragmentationDataObjectUtility);
// SW: END: frag/block max match fix.

	if (setCreateTimeOnBloomfilterUpdate) {
		HAGGLE_DBG("Will set create time in node description when updating bloomfilter\n");
	}
}

bool DataManager::init_derived()
{
#define __CLASS__ DataManager
	int ret;

	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_VERIFIED, onVerifiedDataObject);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_DELETED, onDeletedDataObject);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_INCOMING, onIncomingDataObject);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}
	ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onSendResult);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_DEBUG_CMD, onDebugCmd);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event\n");
		return false;
	}
	onInsertedDataObjectCallback = newEventCallback(onInsertedDataObject);
	onAgedDataObjectsCallback = newEventCallback(onAgedDataObjects);
	periodicBloomfilterUpdateCallback = newEventCallback(onPeriodicBloomfilterUpdate); // MOS

	// Insert time stamp for when haggle starts up into the data store:
	RepositoryEntryRef timestamp = RepositoryEntryRef(new RepositoryEntry("DataManager", "Startup timestamp", Timeval::now().getAsString().c_str()));
	kernel->getDataStore()->insertRepository(timestamp);

	agingMaxAge = DEFAULT_AGING_MAX_AGE;
	agingPeriod = DEFAULT_AGING_PERIOD;

	agingEvent = registerEventType("Aging Event", onAging);

	// Start aging:
	onAgedDataObjects(NULL);
	
	dataTaskEvent = registerEventType("DataTaskEvent", onDataTaskComplete);

	helper = new DataHelper(this, dataTaskEvent);
	
	if (!helper) {
		HAGGLE_ERR("Could not create data manager helper\n");
		return false;
	}
	
	localBF = Bloomfilter::create(Bloomfilter::TYPE_COUNTING);
	
	if (!localBF) {
		HAGGLE_ERR("Could not create data manager bloomfilter\n");
		return false;
	}
	onGetLocalBFCallback = newEventCallback(onGetLocalBF);
	RepositoryEntryRef lbf = new RepositoryEntry(getName(), "Bloomfilter");
	kernel->getDataStore()->readRepository(lbf, onGetLocalBFCallback);

	// MOS
	periodicBloomfilterUpdateEventType = registerEventType("Periodic Bloomfilter Update Event", 
						       onPeriodicBloomfilterUpdate);

	if (periodicBloomfilterUpdateEventType < 0) {
	  HAGGLE_ERR("Could not register periodic bloomfilter update event type...\n");
	  return false;
	}

	periodicBloomfilterUpdateEvent = new Event(periodicBloomfilterUpdateEventType);

	if (!periodicBloomfilterUpdateEvent)
	  return false;

	periodicBloomfilterUpdateEvent->setAutoDelete(false);
	if(periodicBloomfilterUpdateInterval > 0) {
	  periodicBloomfilterUpdateEvent->setTimeout(periodicBloomfilterUpdateInterval);
	  kernel->addEvent(periodicBloomfilterUpdateEvent);
	}

	HAGGLE_DBG("Starting data helper...\n");
	helper->start();

	return true;
}

DataManager::~DataManager()
{
	if (helper)
		delete helper;
	
	Event::unregisterType(dataTaskEvent);
	Event::unregisterType(agingEvent);

	// MOS
	if (periodicBloomfilterUpdateEvent) {
	  if (periodicBloomfilterUpdateEvent->isScheduled())
	    periodicBloomfilterUpdateEvent->setAutoDelete(true);
	  else
	    delete periodicBloomfilterUpdateEvent;
	}

    // SW: fix mem-leak on periodic bloom filter
    if (periodicBloomfilterUpdateCallback) {
        delete periodicBloomfilterUpdateCallback;
    }
    
	Event::unregisterType(periodicBloomfilterUpdateEventType);

	if (onInsertedDataObjectCallback)
		delete onInsertedDataObjectCallback;

	if (onAgedDataObjectsCallback)
		delete onAgedDataObjectsCallback;

	if (onGetLocalBFCallback)
		delete onGetLocalBFCallback;
	
	if (localBF)
		delete localBF;

// SW: JLM: START CACHE STRATEGY:
    if (cacheStrategy) {
        cacheStrategy->quit();
        delete cacheStrategy;
    }
// SW: JLM: END CACHE STRATEGY:

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
}

void DataManager::onShutdown()
{
	if (helper) {
		HAGGLE_DBG("Stopping data helper...\n");
		helper->stop();
	}

    if (cacheStrategy) {
		HAGGLE_DBG("Stopping cache strategy...\n");
        cacheStrategy->stop();
    }
	RepositoryEntryRef lbf = new RepositoryEntry(getName(), "Bloomfilter", localBF->getRaw(), localBF->getRawLen());
	
	kernel->getDataStore()->insertRepository(lbf);
	
	unregisterWithKernel();
}

void DataManager::onDebugCmd(Event *e)
{
	if (e) {
		if (e->getDebugCmd()->getType() == DBG_CMD_PRINT_DATAOBJECTS) {
			unsigned int n = 0;
			
			printf("+++++++++++++++++++++++++++++++\n");
			printf("%u last data objects sent:\n", MAX_DATAOBJECTS_LISTED);
			printf("-------------------------------\n");

			for (List<string>::iterator it = dataObjectsSent.begin(); it != dataObjectsSent.end(); it++) {
				printf("%u %s\n", n++, (*it).c_str());
			}
			printf("%u last data objects received:\n", MAX_DATAOBJECTS_LISTED);
			printf("-------------------------------\n");

			n = 0;

			for (List<string>::iterator it = dataObjectsReceived.begin(); it != dataObjectsReceived.end(); it++) {
				printf("%u %s\n",  n++, (*it).c_str());
			}
			printf("+++++++++++++++++++++++++++++++\n");
		}
// SW: START: cache strat debug:
        else if (e->getDebugCmd()->getType() == DBG_CMD_PRINT_CACHE_STRAT) {
            if (cacheStrategy && !cacheStrategy->isDone()) {
                cacheStrategy->printDebug();
            }
        }
// SW: END: cache strat debug:
// CBMEN, HL, Begin
        else if (e->getDebugCmd()->getType() == DBG_CMD_OBSERVE_CACHE_STRAT) {
        	if (cacheStrategy && !cacheStrategy->isDone()) {
        		Metadata *m = new XMLMetadata(e->getDebugCmd()->getMsg());
        		if (m) {
	        		cacheStrategy->getCacheStrategyAsMetadata(m);
        		}
        	}
        }
// CBMEN, HL, End
	}
}

void DataManager::onGetLocalBF(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	DataStoreQueryResult *qr = static_cast < DataStoreQueryResult * >(e->getData());
	
	HAGGLE_DBG("Got repository callback\n");
	
	// Are there any repository entries?
	if (qr->countRepositoryEntries() != 0) {
		RepositoryEntryRef re;
		
		// Then this is most likely the local bloomfilter:
		
		re = qr->detachFirstRepositoryEntry();
		// Was there a repository entry? => was this really what we expected?
		if (re) {
			HAGGLE_DBG("Retrieved bloomfilter from data store\n");
			// Yes:
			
			Bloomfilter *tmpBF = Bloomfilter::create(re->getValueBlob(), re->getValueLen());

			if (tmpBF) {
				if (localBF)
					delete localBF;
				
				localBF = tmpBF;
				kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);
			}
		}
		RepositoryEntryRef lbf = new RepositoryEntry("DataManager", "Local Bloomfilter");
		kernel->getDataStore()->deleteRepository(lbf);
	} else {
		// Don't do anything... for now.
	}
	
	delete qr;
}

#if defined(ENABLE_METADAPARSER)
bool DataManager::onParseMetadata(Metadata *md)
{

        return true;
}
#endif
/*
	public event handler on verified DataObject
 
	means that the DataObject is verified by the SecurityManager
*/ 
void DataManager::onVerifiedDataObject(Event *e)
{
	if (!e || !e->hasData())
		return;

	DataObjectRef dObj = e->getDataObject();
	
	if (!dObj) {
		HAGGLE_DBG("Verified data object event without data object!\n");
		return;
	}

	if (dataObjectsReceived.size() >= MAX_DATAOBJECTS_LISTED) {
		dataObjectsReceived.pop_front();
	}
	dataObjectsReceived.push_back(dObj->getIdStr());

	HAGGLE_DBG("%s Received data object [%s]\n", getName(), dObj->getIdStr());

#ifdef DEBUG
	// dObj->print(NULL); // MOS - NULL means print to debug trace
#endif
	if (dObj->getSignatureStatus() == DataObject::SIGNATURE_INVALID) {
		// This data object had a bad signature, we should remove
		// it from the bloomfilter
		HAGGLE_DBG("Data object [%s] had bad signature, removing from bloomfilter\n", dObj->getIdStr());
		localBF->remove(dObj);
		kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);
		return;
	}

	if (dObj->getDataState() == DataObject::DATA_STATE_VERIFIED_BAD) {
		HAGGLE_ERR("Data in data object flagged as bad! -- discarding\n");
		if (localBF->has(dObj)) {
			// Remove the data object from the bloomfilter since it was bad.
			localBF->remove(dObj);
			kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);
		}
		return;
	} else if (dObj->getDataState() == DataObject::DATA_STATE_NOT_VERIFIED && helper) {
		// Call our helper to verify the data in the data object.
                if (dObj->dataIsVerifiable()) {
                        helper->addTask(new DataTask(DATA_TASK_VERIFY_DATA, dObj));
                        return;
                }
	}
	
	handleVerifiedDataObject(dObj);
}

void DataManager::onSendResult(Event *e)
{
	DataObjectRef& dObj = e->getDataObject();
	NodeRef node = e->getNode();

	if (!dObj) {
		HAGGLE_ERR("No data object in send result\n");	
		return;
	}

        if(dObj->isControlMessage()) {  // MOS - keep control messages out of Bloom filter
		return;
	}

	if (!node) {
		HAGGLE_ERR("No node in send result\n");	
		return;
	}

	if (!node->isStored()) {
		HAGGLE_DBG2("Send result node %s is not in node store, trying to retrieve\n", node->getName().c_str());
		NodeRef peer = kernel->getNodeStore()->retrieve(node);

		if (peer) {
			HAGGLE_DBG2("Found node %s in node store, using the one in store\n", node->getName().c_str());
			node = peer;
		} else {
			HAGGLE_ERR("Did not find node %s in node store\n", node->getName().c_str());
		}
	}
	if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL) {
		// Add data object to node's bloomfilter.
		HAGGLE_DBG("Adding data object [%s] to node %s's bloomfilter\n", DataObject::idString(dObj).c_str(), node->getName().c_str());
		node->getBloomfilter()->add(dObj);


	  if (dataObjectsSent.size() >= MAX_DATAOBJECTS_LISTED) {
		dataObjectsSent.pop_front();
	  }
	  dataObjectsSent.push_back(dObj->getIdStr());
// SW: JLM: START CACHE STRATEGY:
          if ((e->getType() == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL) && cacheStrategy && !cacheStrategy->isDone()) {
            cacheStrategy->handleSendSuccess(dObj, node);
          }
// SW: JLM: END CACHE STRATEGY.
	}
}

void DataManager::onIncomingDataObject(Event *e)
{
	if (!e || !e->hasData())
		return;

	DataObjectRef& dObj = e->getDataObject();
	
	if (!dObj) {
		HAGGLE_DBG("Incoming data object event without data object!\n");
		return;
	}

        if(dObj->isControlMessage()) {  // MOS - keep control messages out of Bloom filter
		return;
	}

	// Add the data object to the bloomfilter of the one who sent it:
	NodeRef peer = e->getNode();

        // JM End: Associate origin social group to DO

	if (!peer || peer->getType() == Node::TYPE_UNDEFINED) {
		// No valid node in event, try to figure out from interface

		// Find the interface it came from:
		const InterfaceRef& iface = dObj->getRemoteInterface();

		if (iface) {
		        HAGGLE_DBG2("Incoming data object [%s] has no valid peer node but valid peer interface %s\n", DataObject::idString(dObj).c_str(), iface->getIdentifierStr());
			peer = kernel->getNodeStore()->retrieve(iface);
		        if(peer && peer->getType() != Node::TYPE_UNDEFINED) HAGGLE_DBG2("Setting incoming data object peer node to %s\n", peer->getName().c_str());
		} else {
			HAGGLE_DBG("No valid peer interface in data object, cannot figure out peer node\n");
		}
	}
	
	if (peer) {
	  if (peer->getType() != Node::TYPE_APPLICATION && peer->getType() != Node::TYPE_UNDEFINED) {
			// Add the data object to the peer's bloomfilter so that
			// we do not send the data object back.
			HAGGLE_DBG("Adding data object [%s] to peer node %s's (%s num=%lu) bloomfilter\n", 
				DataObject::idString(dObj).c_str(), peer->getName().c_str(), peer->isStored() ? "stored" : "not stored", peer->getNum());
			/*
			LOG_ADD("%s: BLOOMFILTER:ADD %s\t%s:%s\n", 
				Timeval::now().getAsString().c_str(), dObj->getIdStr(), 
				peer->getTypeStr(), peer->getIdStr());
			*/
			peer->getBloomfilter()->add(dObj);

		}
	} else {
		HAGGLE_DBG("No valid peer node for incoming data object [%s]\n", dObj->getIdStr());
	}

	// Check if this is a control message from an application. We do not want 
	// to bloat our bloomfilter with such messages, because they are sent
	// everytime an application connects.
	if (!dObj->isControlMessage()) {
		// Add the incoming data object also to our own bloomfilter
		// We do this early in order to avoid receiving duplicates in case
		// the same object is received at nearly the same time from multiple neighbors
		if (localBF->has(dObj)) {
			HAGGLE_DBG("Data object [%s] already in our bloomfilter, marking as duplicate...\n", dObj->getIdStr());
			dObj->setDuplicate();
		} else {
		  if(!dObj->isNodeDescription()) { // MOS - local BF only contains data objects in new design
			localBF->add(dObj);
		        HAGGLE_DBG("Adding data object [%s] to our bloomfilter, #objs=%d\n", DataObject::idString(dObj).c_str(), localBF->numObjects());
			kernel->getThisNode()->getBloomfilter()->add(dObj); // MOS
			if(isNodeDescUpdateOnReceptionEnabled) {
			  // MOS - immediately disseminate updated bloomfilter
			  kernel->getThisNode()->setNodeDescriptionCreateTime();
			  kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));	
			}
		  }
		  if(!periodicBloomfilterUpdateEvent->isScheduled()) // MOS - update now happening periodically
		      kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);
		}
	}
}

// SW: JLM: START CACHE STRATEGY:

// We intercept the `handleVerifiedDataObject` call to now call
// into the cache strat, if it exists. The cache strat module
// will then call `insertDataObjectIntoDataStore` if the new object
// should be inserted
void
DataManager::insertDataObjectIntoDataStore(DataObjectRef& dObj)
{
    if (!dObj) {
        HAGGLE_ERR ("Trying to insert null data object into DB.\n");
        return;
    }

    // insert into database (including filtering)
    if (dObj->isPersistent ()) {
        kernel->getDataStore ()->insertDataObject (dObj,
            onInsertedDataObjectCallback);
    }
    else {
        // do not expect a callback for a non-persistent data object,
        // but we still call insertDataObject in order to filter the data object.
        kernel->getDataStore ()->insertDataObject (dObj, NULL);
    }
}

void
DataManager::handleVerifiedDataObject(DataObjectRef& dObj)
{
    if (!dObj) {
        HAGGLE_ERR ("Handle verified object received null object.\n");
        return;
    }

        if(networkCodingConfiguration->isNetworkCodingEnabled(dObj,NULL) &&
	   !networkCodingConfiguration->isForwardingEnabled()) {
	  if(networkCodingDataObjectUtility->isNetworkCodedDataObject(dObj)) {
	    if (dObj->isDuplicate()) {
	      HAGGLE_DBG("Data object %s is a duplicate! Not generating DATAOBJECT_NEW event\n", dObj->getIdStr());
	    } else {
	      kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_NEW, dObj));
	      return;
	    }
	  }
	}

        if(fragmentationConfiguration->isFragmentationEnabled(dObj,NULL) &&
	   !fragmentationConfiguration->isForwardingEnabled()) {
	  if(fragmentationDataObjectUtility->isFragmentationDataObject(dObj)) {
	    if (dObj->isDuplicate()) {
	      HAGGLE_DBG("Data object %s is a duplicate! Not generating DATAOBJECT_NEW event\n", dObj->getIdStr());
	    } else {
	      kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_NEW, dObj));
	      return;
	    }
	  }
	}

    // MOS - add data object to Bloomfilter to cover the case 
    // where there was no incoming event (e.g. encrypting data object from local app)
    if (dObj->getABEStatus() != DataObject::ABE_NOT_NEEDED && !localBF->has(dObj)) {
      localBF->add(dObj);
      HAGGLE_DBG("Adding encrypted data object [%s] to our bloomfilter, #objs=%d\n", DataObject::idString(dObj).c_str(), localBF->numObjects());
      kernel->getThisNode()->getBloomfilter()->add(dObj); // MOS
    }

    if (cacheStrategy && !cacheStrategy->isDone() && cacheStrategy->isResponsibleForDataObject(dObj)) {
        cacheStrategy->handleNewDataObject(dObj);
    }
    else {
        //default action for dObj's that are NOT handled by cache strat code
        insertDataObjectIntoDataStore (dObj);
    }
}

// SW: JLM: END CACHE STRATEGY.

void DataManager::onDataTaskComplete(Event *e)
{
	if (!e || !e->hasData())
		return;

	DataTask *task = static_cast<DataTask *>(e->getData());

	if (task->type == DATA_TASK_VERIFY_DATA && task->completed) {
		handleVerifiedDataObject(task->dObj);
	}
	delete task;
}

// SW: START: remove from local bloomfilter.
void 
DataManager::removeFromLocalBF(const DataObjectId_t &id)
{
    localBF->remove(id);
    kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);
}
//SW: END: remove from local bloomfilter.

void DataManager::onDeletedDataObject(Event * e)
{
	if (!e || !e->hasData())
		return;
	
	DataObjectRefList dObjs = e->getDataObjectList();
	unsigned int n_removed = 0;
	bool cleanup = false; // MOS

	for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
		/* 
		  Do not remove Node descriptions from the bloomfilter. We do not
		  want to receive old node descriptions again.
		  If the flag in the event is set, it means we should keep the data object
		  in the bloomfilter.
		*/
        DataObjectRef dObj = (*it);
		if (!dObj->isNodeDescription() && !e->getFlags()) {
		  if(localBF->has(dObj)) {
		        HAGGLE_DBG("Removing deleted data object [id=%s] from bloomfilter, #objs=%d\n", 
				   DataObject::idString(dObj).c_str(), localBF->numObjects());
			localBF->remove(dObj);
			n_removed++;
		  } else {
			HAGGLE_DBG("Deleted data object data object [id=%s] not found in bloomfilter\n", dObj->getIdStr());
		  }
		} else {
			HAGGLE_DBG("Keeping deleted data object [id=%s] in bloomfilter\n", dObj->getIdStr());
		}

		if(!dObj->hasValidSignature()) {
		  cleanup = true; // MOS - allow new incoming data object from co-certified node 
		}

// SW: JLM: START CACHE STRATEGY:
        if (cacheStrategy && !cacheStrategy->isDone() && cacheStrategy->isResponsibleForDataObject(dObj)) {
            cacheStrategy->handleDeletedDataObject(dObj); 
        }
// SW: JLM: END CACHE STRATEGY.

// CBMEN, HL - Begin
        // Remove any pending send events for this data object
        HAGGLE_DBG2("Cancelling send events for dObj %s\n", dObj->getIdStr());
        kernel->cancelEvents(EVENT_TYPE_DATAOBJECT_SEND, dObj);
// CBMEN, HL, End

	}
	
	if (n_removed > 0 || cleanup) 
	    kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);
}

/*
	callback on successful insert of a DataObject into the DataStore,
        or if the data object was a duplicate, in which case the data object
        is marked as such
*/
void DataManager::onInsertedDataObject(Event * e)
{
	if (!e || !e->hasData())
		return;
	
	DataObjectRef& dObj = e->getDataObject();
	if(kernel->discardObsoleteNodeDescriptions() && dObj->isNodeDescription()) return; // MOS
	
	/*
		The DATAOBJECT_NEW event signals that a new data object has been 
		received and is inserted into the data store. Other managers can now be
		sure that this data object exists in the data store - as long as they
		query it through the data store task queue (since the query will be 
		processed after the insertion task).
	*/
	if (dObj->isDuplicate()) {
		HAGGLE_DBG("Data object %s is a duplicate! Not generating DATAOBJECT_NEW event\n", dObj->getIdStr());
	} else {
		kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_NEW, dObj));
	}
}

void DataManager::onAgedDataObjects(Event *e)
{
	if (!e || kernel->isShuttingDown())
		return;

	if (e->hasData()) {
		DataObjectRefList dObjs = e->getDataObjectList();

		HAGGLE_DBG("Aged %lu data objects\n", dObjs.size());

		if (dObjs.size() >= DATASTORE_MAX_DATAOBJECTS_AGED_AT_ONCE) {
			// Call onAging() immediately in case there are more data
			// objects to age.
			onAging(NULL);
		} else {
			// Delay the aging for one period
			kernel->addEvent(new Event(agingEvent, NULL, (double)agingPeriod));
		}
	} else {
		// No data in event -> means this is the first time the function
		// is called and we should start the aging timer.
		kernel->addEvent(new Event(agingEvent, NULL, (double)agingPeriod));
	}
}

void DataManager::onAging(Event *e)
{
	HAGGLE_DBG("Data object aging\n");

	if (kernel->isShuttingDown()) {
		HAGGLE_DBG("In shutdown, ignoring event\n");
		return;
	}

	// Delete from the data store any data objects we're not interested
	// in and are too old.
	// FIXME: find a better way to deal with the age parameter. 
	kernel->getDataStore()->ageDataObjects(Timeval(agingMaxAge, 0), onAgedDataObjectsCallback, keepInBloomfilterOnAging);
}

// MOS - update this node's non-counting abstraction from counting Bloomfilter periodically

void DataManager::onPeriodicBloomfilterUpdate(Event *e)
{
	if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, ignoring periodic bloomfilter update\n");
		return;
	}

	HAGGLE_DBG("Periodic bloomfilter update\n");

	if (kernel->isShuttingDown()) {
		HAGGLE_DBG("In shutdown, ignoring event\n");
		return;
	}

	kernel->getThisNode()->setBloomfilter(*localBF, setCreateTimeOnBloomfilterUpdate);

	if (!periodicBloomfilterUpdateEvent->isScheduled() &&
	    periodicBloomfilterUpdateInterval > 0) {
		periodicBloomfilterUpdateEvent->setTimeout(periodicBloomfilterUpdateInterval);
		kernel->addEvent(periodicBloomfilterUpdateEvent);
	}
}

// SW: JLM: START CACHE STRATEGY:
void
DataManager::setCacheStrategy(CacheStrategy * _cacheStrategy)
{
    if (cacheStrategy) {
        HAGGLE_ERR("CacheStrategy already set!\n");
        return;
    }
    cacheStrategy = _cacheStrategy; 
}
// SW: JLM: END CACHE STRATEGY.

void DataManager::onConfig(Metadata *m)
{
	bool agingHasChanged = false;
	
	const char *param = m->getParameter("set_createtime_on_bloomfilter_update");
	
	if (param) {
		if (strcmp(param, "true") == 0) {
			setCreateTimeOnBloomfilterUpdate = true;
			HAGGLE_DBG("setCreateTimeOnBloomfilterUpdate set to 'true'\n");
		}
		else if (strcmp(param, "false") == 0) {
			setCreateTimeOnBloomfilterUpdate = false;
			HAGGLE_DBG("setCreateTimeOnBloomfilterUpdate set to 'false'\n");
		}
	}

	// MOS
	param = m->getParameter("periodic_bloomfilter_update_interval");
	
	if (param) {		
	  char *endptr = NULL;
	  unsigned long period = strtoul(param, &endptr, 10);
	  
	  if (endptr && endptr != param) {
	    if (periodicBloomfilterUpdateInterval == 0 && 
		period > 0 && 
		!periodicBloomfilterUpdateEvent->isScheduled()) {
	      kernel->addEvent(new Event(periodicBloomfilterUpdateCallback, NULL));
	    } 
	    periodicBloomfilterUpdateInterval = period;
	    HAGGLE_DBG("periodic_bloomfilter_update_interval=%lu\n", 
		       periodicBloomfilterUpdateInterval);
	  }
	}

	// MOS - experimental setting
	param = m->getParameter("node_desc_update_on_reception");
	if (param) {
		if (strcmp(param, "true") == 0) {
			isNodeDescUpdateOnReceptionEnabled = true;
			HAGGLE_DBG("node_desc_update_on_reception set to 'true'\n");
		}
		else if (strcmp(param, "false") == 0) {
			isNodeDescUpdateOnReceptionEnabled = false;
			HAGGLE_DBG("node_desc_update_on_reception set to 'false'\n");
		}
	}

	Metadata *dm = m->getMetadata("Bloomfilter");

	if (dm) {
		bool reset_bloomfilter = false;
		const char *param = dm->getParameter("default_error_rate");

		if (param) {
			char *endptr = NULL;
			double error_rate = strtod(param, &endptr);

			if (endptr && endptr != param) {
				Bloomfilter::setDefaultErrorRate(error_rate);
				HAGGLE_DBG("config default bloomfilter error rate %.3lf\n", 
					   Bloomfilter::getDefaultErrorRate());
				reset_bloomfilter = true;
			}
		}
		
		param = dm->getParameter("default_capacity");

		if (param) {
			char *endptr = NULL;
			unsigned int capacity = (unsigned int)strtoul(param, &endptr, 10);
			
			if (endptr && endptr != param) {
				Bloomfilter::setDefaultCapacity(capacity);
				HAGGLE_DBG("config default bloomfilter capacity %u\n", 
					   Bloomfilter::getDefaultCapacity());
				reset_bloomfilter = true;
			}
		}
		
		if (reset_bloomfilter) {
			if (localBF)
				delete localBF;
			
			localBF = Bloomfilter::create(Bloomfilter::TYPE_COUNTING);
			
			if (!localBF) {
				HAGGLE_ERR("Could not create data manager bloomfilter\n");
			}
		}
	}


// Database timeout configuration

	// Set default to 1.5 sec
	max_wait.tv_sec = 1;
	max_wait.tv_usec = 500000;
	// Set default to true
	setDropNewObjectWhenDatabaseTimeout = true;

	dm = m->getMetadata("DatabaseTimeout");
	if (dm) {
		param = dm->getParameter("database_timeout_ms");
		if (param) {
			char *endptr = NULL;
			unsigned long timeout = strtoul(param, &endptr, 10);
			if (endptr && endptr != param) {
				HAGGLE_DBG("database_timeout_ms=%lu\n", timeout);

				max_wait.tv_sec = (timeout / 1000);
				max_wait.tv_usec = 1000 * (timeout - (max_wait.tv_sec * 1000));
			}
		}

		param = dm->getParameter("drop_new_object_when_database_timeout");
		if (param) {
			if (0 == (strcmp(param, "true"))) {
				setDropNewObjectWhenDatabaseTimeout = true;
			} else if (0 == (strcmp(param, "false"))) {
				setDropNewObjectWhenDatabaseTimeout = false;
			} else {
				HAGGLE_ERR("Unknown parameter for drop_new_object_when_database_timeout, need `true` or `false`.\n");
			}
		}

	}

	HAGGLE_DBG("Database timeout max_wait.tv_sec=%lu\n", max_wait.tv_sec);
	HAGGLE_DBG("Database timeout max_wait.tv_usec=%lu\n", max_wait.tv_usec);
	if (setDropNewObjectWhenDatabaseTimeout) {
		HAGGLE_DBG("drop_new_object_when_database_timeout=true\n");
	} else {
		HAGGLE_DBG("drop_new_object_when_database_timeout=false\n");
	}
//	

	dm = m->getMetadata("Aging");

	if (dm) {
		const char *param = dm->getParameter("period");
		
		if (param) {
			char *endptr = NULL;
			unsigned long period = strtoul(param, &endptr, 10);
			
			if (endptr && endptr != param) {
				agingPeriod = period;
				HAGGLE_DBG("config agingPeriod=%lu\n", agingPeriod);
				LOG_ADD("# %s: agingPeriod=%lu\n", getName(), agingPeriod);
				agingHasChanged = true;
			}
		}
		
		param = dm->getParameter("max_age");
		
		if (param) {
			char *endptr = NULL;
			unsigned long period = strtoul(param, &endptr, 10);
			
			if (endptr && endptr != param) {
				agingMaxAge = period;
				HAGGLE_DBG("config agingMaxAge=%lu\n", agingMaxAge);
				LOG_ADD("# %s: agingMaxAge=%lu\n", getName(), agingMaxAge);
				agingHasChanged = true;
			}
		}
		
		param = dm->getParameter("keep_in_bloomfilter");
		
		if (param) {
			if (strcmp(param, "true") == 0) {
				HAGGLE_DBG("config aging: keep_in_bloomfilter=true\n");
				LOG_ADD("# %s: aging: keep_in_bloomfilter=true\n", getName());
				keepInBloomfilterOnAging = true;
			} else if (strcmp(param, "false") == 0) {
				HAGGLE_DBG("config aging: keep_in_bloomfilter=false\n");
				LOG_ADD("# %s: aging: keep_in_bloomfilter=false\n", getName());
				keepInBloomfilterOnAging = false;
			}
		}
	}
	
	if (agingHasChanged)
		onAging(NULL);

    // SW: JLM: START CACHE STRATEGY:
    dm = m->getMetadata("CacheStrategy");
    if (dm) {
        param = dm->getParameter("name");
	    CacheStrategy *_cacheStrategy = 
            CacheStrategyFactory::getNewCacheStrategy(this, param);
        dm = dm->getMetadata(param);
        if (dm && _cacheStrategy) {
            _cacheStrategy->onConfig(*dm);
            _cacheStrategy->start();
            HAGGLE_DBG("Loaded cache strategy: %s\n", _cacheStrategy->getName());
        }
        setCacheStrategy(_cacheStrategy);
    }
    // SW: JLM: END CACHE STRATEGY.
  
 
    // SW: START: config.xml options to DataStore:
	dm = m->getMetadata("DataStore");
    if (dm) {
        kernel->getDataStore()->onConfig(dm);        
    }
}
