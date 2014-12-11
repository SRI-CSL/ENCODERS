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
#ifndef _FORWARDINGMANAGER_H
#define _FORWARDINGMANAGER_H

/*
 Forward declarations of all data types declared in this file. This is to
 avoid circular dependencies. If/when a data type is added to this file,
 remember to add it here.
 */
class ForwardingManager;

#include <libcpphaggle/List.h>
#include <libcpphaggle/Pair.h>

using namespace haggle;

#include "Event.h"
#include "Manager.h"
#include "DataObject.h"
#include "Node.h"
#include "Forwarder.h"
#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "networkcoding/forwarding/NetworkCodingForwardingManagerHelper.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/forwarding/FragmentationForwardingManagerHelper.h"

// SW: START FORWARDING CLASSIFIER:
#include "ForwardingClassifier.h"
// SW: END FORWARDING CLASSIFIER.

#define MAX_NODES_TO_FIND_FOR_NEW_DATAOBJECTS (30)  // MOS - increased to 30 - now a parameter

typedef List<Pair<Pair<const DataObjectRef, const NodeRef>, int> > forwardingList;

/** */
class ForwardingManager : public Manager
{
    EventCallback<EventHandler> *dataObjectQueryCallback;
    EventCallback<EventHandler> *delayedDataObjectQueryCallback;
    EventCallback<EventHandler> *nodeQueryCallback;
    EventCallback<EventHandler> *forwardDobjCallback;
    EventCallback<EventHandler> *repositoryCallback;
    EventCallback<EventHandler> *periodicDataObjectQueryCallback;
    EventCallback<EventHandler> *periodicRoutingUpdateCallback;

    EventType moduleEventType, routingInfoEventType, 
            periodicDataObjectQueryEventType, periodicRoutingUpdateEventType;
	
    // SW: START: RACE CONDITION DURING INIT PUNTS
    void onInitPunt(Event *e);
    void onConfigPunt(Event *e);
    EventType on_punt_init_event;
    EventType on_config_punt_event;
    // SW: END: RACE CONDITION DURING INIT PUNTS

    Event *periodicDataObjectQueryEvent;
    Event *periodicRoutingUpdateEvent;
    unsigned long periodicDataObjectQueryInterval;
    unsigned long periodicRoutingUpdateInterval;
    unsigned long maxNodesToFindForNewDataObjects; // MOS
    unsigned long dataObjectRetries; // MOS
    unsigned long dataObjectRetriesShortCircuit; // MOS
    unsigned long nodeDescriptionRetries; // MOS
    forwardingList forwardedObjects;
    Forwarder *forwardingModule;
    List<NodeRef> pendingQueryList;
    bool enableForwardingMetadata; // MOS
    bool recursiveRoutingUpdates;
    bool doQueryOnNewDataObject;
    // Period in seconds to do periodic node queries during node
    // contacts. Zero to disable.

// SW: START NEW NBR NODE DESCRIPTION PUSH:
    bool doPushNodeDescriptions;
    void forwardNodeDescriptions(const NodeRef& node);
// SW: END NEW NBR NODE DESCRIPTION PUSH.
// MOS: START ENABLE TARGET GENERATION:
    bool enableTargetGeneration;
// MOS: END ENABLE TARGET GENERATION.
// MOS: START RANDOMIZED FORWARDING DELAY
    unsigned long maxNodeDescForwardingDelayLinearMs;
    unsigned long maxNodeDescForwardingDelayBaseMs;
    unsigned long maxForwardingDelayLinearMs;
    unsigned long maxForwardingDelayBaseMs;
// MOS: END RANDOMIZED FORWARDING DELAY
    bool neighborForwardingShortcut; // MOS
    unsigned long loadReductionMinQueueSize; // MOS
    unsigned long loadReductionMaxQueueSize; // MOS
    bool noForwarder; // SW

    bool enableMultihopBloomfilters;

    void onPrepareStartup();
    void onPrepareShutdown();

    // See comment in ForwardingManager.cpp about isNeighbor()
    bool isNeighbor(const NodeRef& node);bool addToSendList(DataObjectRef& dObj,
            const NodeRef& node, int repeatCount = 0);
    /**
     This function changes out the current forwarding module (initially none)
     to the given forwarding module.

     The current forwarding module's state is stored in the repository before
     the forwarding module is stopped and deleted.

     The given forwarding module's state (if any) is retreived from the
     repository, and all forwarding data objects that the forwarding module
     is interested in is retreived from the data store.

     This function takes possession of the given forwarding module, and will
     take responsibility for releasing it.
     */
    void setForwardingModule(Forwarder *f, bool deRegisterEvents = false);

    bool init_derived();

    Forwarder *getForwarder() {
        return forwardingModule;
    }
    bool shouldForward(const DataObjectRef& dObj, const NodeRef& node, NodeRef& actual_node); // MOS extended by actual_node
    void forwardByDelegate(DataObjectRef &dObj, const NodeRef &target,
            const NodeRefList *other_targets = NULL);
    void addForwardingMetadata(DataObjectRef &dObj); // MOS
    void addForwardingMetadataDelegated(DataObjectRef &dObj); // MOS
    void onShutdown();
    void onForwardingTaskComplete(Event *e);
    void onDataObjectForward(Event *e);
    void onDeletedDataObject(Event * e); // MOS
    unsigned long getDataObjectRetries(DataObjectRef &dObj); // MOS
    void onSendDataObjectResult(Event *e);
    void onDataObjectQueryResult(Event *e);
    double randomizedForwardingDelay(NodeRefList& ns, DataObjectRef& dObj); // MOS
    void onNodeQueryResult(Event *e);
    void onNodeUpdated(Event *e);
    void onRoutingInformation(Event *e);
    void onNewDataObject(Event *e);
    void onPeriodicRoutingUpdate(Event *e); // MOS
    void onNewNeighbor(Event *e);
    void onEndNeighbor(Event *e);
    void onRepositoryData(Event *e);
    void onTargetNodes(Event *e);
    void onDelegateNodes(Event *e);
    void onDelayedDataObjectQuery(Event *e);
    void onPeriodicDataObjectQuery(Event *e);

    void onConfig(Metadata *m);
    // SW: separate onConfigImplementation to work with init punts
    void onConfigImplementation(Metadata *m);
    void findMatchingDataObjectsAndTargets(NodeRef& node);
    void onDebugCmd(Event *e);
    size_t metadataToRecurseList(Metadata *m, NodeRefList& trigger_list);
    Metadata *recurseListToMetadata(Metadata *m,
            const NodeRefList& trigger_list);
    void recursiveRoutingUpdate(NodeRef peer, Metadata *m);
public:
    ForwardingManager(HaggleKernel *_kernel = haggleKernel);
    ~ForwardingManager();
private:
    NetworkCodingConfiguration *networkCodingConfiguration;
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
    FragmentationConfiguration *fragmentationConfiguration;
    FragmentationDataObjectUtility* fragmentationDataObjectUtility;
    ForwardingManagerHelper* forwardingManagerHelper;
    FragmentationForwardingManagerHelper* fragmentationForwardingManagerHelper;

    bool probabilisticLoadReduction(); // MOS
};

#endif /* _FORWARDINGMANAGER_H */
