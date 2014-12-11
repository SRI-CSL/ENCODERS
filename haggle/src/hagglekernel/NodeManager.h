/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   James Mathewson (JM, JLM)
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
#ifndef _NODEMANAGER_H
#define _NODEMANAGER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

class NodeManager;

#include "Event.h"
#include "Manager.h"
#include "Filter.h"

#include "NodeStore.h"
#include "ReplicationManagerUtility.h" 
/** */
class NodeManager : public Manager
{	
	typedef struct {
		DataObjectRef dObj;
		unsigned long retries;
	} SendEntry_t;
	typedef List< Pair<NodeRef, SendEntry_t> > SendList_t;

// SW: START: include 1-hop neighborhood  in ND metadata
    bool send_neighborhood;
// SW: END: include 1-hop neighborhood  in ND metadata.
    bool sendAppNodeDOs;
// JM: START social group label
   bool useSocialName;
   string nodeSocialName;
   string nodeSocialId;
// JM: END social group label
	size_t thumbnail_size;
	char *thumbnail;
	unsigned long nodeDescriptionRetries;
	double nodeDescriptionRetryWait;
// SW: START REFRESH:
	EventType periodicNodeDescriptionRefreshEventType;
	Event *periodicNodeDescriptionRefreshEvent;
	unsigned long nodeDescriptionRefreshPeriodMs;
	unsigned long nodeDescriptionRefreshJitterMs;
// SW: END REFRESH.
// SW: START PURGE:
	EventType periodicNodeDescriptionPurgeEventType;
	Event *periodicNodeDescriptionPurgeEvent;
    unsigned long nodeDescriptionPurgeMaxAgeMs;
    unsigned long nodeDescriptionPurgePollPeriodMs;
// SW: END PURGE.
	SendList_t sendList;
	EventCallback<EventHandler> *onRetrieveNodeCallback;
	EventCallback<EventHandler> *onRetrieveThisNodeCallback;
	EventCallback<EventHandler> *onInsertedNodeCallback;
        EventType nodeDescriptionEType;
	bool isInSendList(const NodeRef& node, const DataObjectRef& dObj);
	void addToSendList(const NodeRef& node, const DataObjectRef& dObj); // MOS
        int sendNodeDescription(NodeRefList& neighList); // MOS - modified
	int sendIndividualNodeDescription(NodeRef node, NodeRefList& neighList); // MOS
        void onApplicationFilterMatchEvent(Event *e);
        void onSendNodeDescription(Event *e);
        void onReceiveNodeDescription(Event *e);
        void onLocalInterfaceUp(Event *e);
        void onLocalInterfaceDown(Event *e);
        void onNeighborInterfaceUp(Event *e);
        void onNeighborInterfaceDown(Event *e);
        void onNewNodeContact(Event *e);
	void onSendResult(Event *e);
	void onRetrieveNode(Event *e);
	void onRetrieveThisNode(Event *e);
	void onNodeInformation(Event *e);
	void nodeUpdate(NodeRef& node);
	void onInsertedNode(Event *e);

#if defined(ENABLE_METADAPARSER)
        bool onParseMetadata(Metadata *md);
#endif
	
	void onPrepareShutdown();
	bool init_derived();
	void onConfig(Metadata *m);

// SW: START REFRESH:
    void pushNodeDescriptionToNeighbors();
    void pushNewNodeDescriptionToNeighbors();
    void onPeriodicNodeDescriptionRefresh(Event * e);
// SW: END REFRESH.
// SW: START PURGE:
    void onPeriodicNodeDescriptionPurge(Event * e);
// SW: END PURGE.
    bool acceptNeighborNodeDescriptionsFromThirdParty;

public:
        NodeManager(HaggleKernel *_haggle = haggleKernel);
        ~NodeManager();
};


#endif /* _NODEMANAGER_H */
