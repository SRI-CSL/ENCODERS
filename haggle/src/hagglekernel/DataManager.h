/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
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
#ifndef _DATAMANAGER_H
#define _DATAMANAGER_H


// default values for simple aging
#define DEFAULT_AGING_MAX_AGE 24*3600	// max age of data objects [s]
#define DEFAULT_AGING_PERIOD  3600	// period between aging processes [s]

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class DataManager;

#include "Manager.h"
#include "Event.h"
#include "Bloomfilter.h"

#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"

// SW: JLM: START CACHE STRATEGY:
#include "CacheStrategy.h"
// SW: JLM: END CACHE STRATEGY.

typedef enum {
	DATA_TASK_VERIFY_DATA,
} DataTaskType_t;

class DataTask {
public:
	DataTaskType_t type;
	bool completed;
	DataObjectRef dObj;
	DataTask(const DataTaskType_t _type, DataObjectRef _dObj = NULL);
        ~DataTask();
};

class DataHelper : public ManagerModule<DataManager> {
	GenericQueue<DataTask *> taskQ;
	const EventType etype; // Private event used to communicate with data manager
	void doTask(DataTask *task);
	bool run();
        void cleanup();
public:
	DataHelper(DataManager *m, const EventType _etype);
	~DataHelper();

	bool addTask(DataTask *task) { return taskQ.insert(task); }
};

/** */
class DataManager : public Manager
{
// SW: JLM: START CACHE STRATEGY:
    friend class CacheStrategyAsynchronous;
// SW: JLM: END CACHE STRATEGY.
private:
	EventCallback <EventHandler> *onInsertedDataObjectCallback;
	EventCallback <EventHandler> *onGetLocalBFCallback;
	EventCallback <EventHandler> *onAgedDataObjectsCallback;
	EventCallback <EventHandler> *periodicBloomfilterUpdateCallback; // MOS
	EventType dataTaskEvent;
	EventType agingEvent;
	DataHelper *helper;
	/* We keep a local bloomfilter in the data manager in addition to the one in "this node". 
	 The reason for this is that the local bloomfilter here is a counting bloomfilter that
	 allows us to both add and remove data from it. When we update the local bloomfilter,
	 we also convert it into a non-counting bloomfilter that we set in "this node". The 
	 non-counting version is much smaller in size, and hence more suitable for sending out
	 in the node description.
	 */
	Bloomfilter *localBF;
	bool setCreateTimeOnBloomfilterUpdate;
	bool keepInBloomfilterOnAging;
	unsigned long agingMaxAge;
	unsigned long agingPeriod;
// Database timeout
	struct timeval max_wait;
	bool setDropNewObjectWhenDatabaseTimeout;
//
// SW: JLM: START CACHE STRATEGY:
     CacheStrategy *cacheStrategy;
// SW: JLM: END CACHE STRATEGY.
	// MOS
	EventType periodicBloomfilterUpdateEventType;
	Event *periodicBloomfilterUpdateEvent;
	unsigned long periodicBloomfilterUpdateInterval;
	bool isNodeDescUpdateOnReceptionEnabled;
#define MAX_DATAOBJECTS_LISTED 10
	List<string> dataObjectsSent; // List of data objects sent.
	List<string> dataObjectsReceived; // List of data objects received.
	void onDebugCmd(Event *e);
	NetworkCodingConfiguration *networkCodingConfiguration;
	NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
	FragmentationConfiguration *fragmentationConfiguration;
	FragmentationDataObjectUtility* fragmentationDataObjectUtility;
public:
        DataManager(HaggleKernel *_haggle = haggleKernel, bool setCreateTimeOnBloomfilterUpdate = false);
        ~DataManager();
        void onGetLocalBF(Event *e);

    // this function is used externally by replacement modules to insert
    // into the data store and fire the proper events
    void insertDataObjectIntoDataStore(DataObjectRef& dObj);
    // Database timeout
    timeval getDatabaseTimeout() { return max_wait; }
    bool isDropNewObjectWhenDatabaseTimeout() { return setDropNewObjectWhenDatabaseTimeout; }

// SW: START: remove from local bloomfilter.
    void removeFromLocalBF(const DataObjectId_t &id);
//SW: END: remove from local bloomfilter.
private:
// SW: JLM: START CACHE STRATEGY:
    void setCacheStrategy(CacheStrategy *cacheStrategy);
// SW: JLM: END CACHE STRATEGY.
    void handleVerifiedDataObject(DataObjectRef& dObj);
        void onVerifiedDataObject(Event *e);
	void onInsertedDataObject(Event *e);
        void onDeletedDataObject(Event *e);
	void onSendResult(Event *e);
	void onIncomingDataObject(Event *e);
        void onNewRelation(Event *e);
	void onDataTaskComplete(Event *e);
	void onAgedDataObjects(Event *e);
	void onAging(Event *e);
	void onPeriodicBloomfilterUpdate(Event *e); // MOS
	void onShutdown();
	void onConfig(Metadata *m);
#if defined(ENABLE_METADAPARSER)
        bool onParseMetadata(Metadata *m);
#endif
	bool init_derived();
};


#endif /* _DATAMANAGER_H */
