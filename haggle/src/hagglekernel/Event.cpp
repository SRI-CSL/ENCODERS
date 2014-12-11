/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM, JLM)
 *   Sam Wood (SW)
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
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
#include <stdlib.h>

#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "Event.h"
#include "DataObject.h"
#include "Interface.h"

const char *Event::eventNames[MAX_NUM_EVENT_TYPES] = {
	"EVENT_TYPE_PREPARE_STARTUP",
	"EVENT_TYPE_STARTUP",
	"EVENT_TYPE_PREPARE_SHUTDOWN",
	"EVENT_TYPE_SHUTDOWN",	
	"EVENT_TYPE_NODE_CONTACT_NEW",
	"EVENT_TYPE_NODE_CONTACT_END",
	"EVENT_TYPE_NODE_UPDATED",
	"EVENT_TYPE_APP_NODE_UPDATED", // MOS
	"EVENT_TYPE_APP_NODE_CHANGED", // SW
	"EVENT_TYPE_ROUTE_TO_APP", // SW
	"EVENT_TYPE_NODE_DESCRIPTION_SEND",
	"EVENT_TYPE_NEIGHBOR_INTERFACE_UP",
	"EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN",
	"EVENT_TYPE_LOCAL_INTERFACE_UP",
	"EVENT_TYPE_LOCAL_INTERFACE_DOWN",
	"EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION",
	"EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK",
	"EVENT_TYPE_DATAOBJECT_AGING_DECODER_NETWORKCODING",
	"EVENT_TYPE_DATAOBJECT_AGING_DECODER_FRAGMENTATION",
	"EVENT_TYPE_DATAOBJECT_CONVERT_FRAGMENTATION_NETWORKCODING",
	"EVENT_TYPE_DATAOBJECT_NEW",
	"EVENT_TYPE_DATAOBJECT_DELETED",
	"EVENT_TYPE_DATAOBJECT_DELETE_NETWORKCODEDBLOCK",
	"EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_NETWORKCODEDBLOCKS",
    "EVENT_TYPE_DATAOBJECT_DELETE_FRAGMENT",
    "EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_FRAGMENTS",
    "EVENT_TYPE_DATAOBJECT_DISABLE_NETWORKCODING",
    "EVENT_TYPE_DATAOBJECT_ENABLE_NETWORKCODING",
    "EVENT_TYPE_RECEIVE_BEACON",
	"EVENT_TYPE_DATAOBJECT_FORWARD",
	"EVENT_TYPE_DATAOBJECT_SEND",
	"EVENT_TYPE_DATAOBJECT_SEND_TO_APP", // MOS
    "EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING",
    "EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION",
	"EVENT_TYPE_DATAOBJECT_VERIFIED",
	"EVENT_TYPE_DATAOBJECT_RECEIVED",
	"EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL",
	"EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL",
	"EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL",
	"EVENT_TYPE_DATAOBJECT_SEND_FAILURE",
	"EVENT_TYPE_DATAOBJECT_INCOMING",
	"EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION",
	"EVENT_TYPE_TARGET_NODES",
	"EVENT_TYPE_DELEGATE_NODES",
	"EVENT_TYPE_RESOURCE_POLICY_NEW",
// SW: START: SendPriorityManager
	"EVENT_TYPE_SEND_PRIORITY",
    "EVENT_TYPE_SEND_PRIORITY_SUCCESS",
    "EVENT_TYPE_SEND_PRIORITY_FAILURE",
// SW: END: SendPriorityManager
	"EVENT_TYPE_SECURITY_CONFIGURE", // CBMEN, HL
	"EVENT_TYPE_SEND_OBSERVER_DATAOBJECT", // CBMEN, HL
	"EVENT_TYPE_DYNAMIC_CONFIGURE", // CBMEN, HL
	"EVENT_TYPE_DEBUG_CMD",
	"EVENT_TYPE_CALLBACK",
// SW: START REPLICATION MANAGER
    "EVENT_TYPE_REPL_MANAGER",
// SW: END REPLICATION MANAGER
};

int Event::num_event_types = MAX_NUM_PUBLIC_EVENT_TYPES;

// This will store the callback functions for registered private events
EventCallback < EventHandler > *Event::privCallbacks[MAX_NUM_PRIVATE_EVENT_TYPES];

Event::Event(EventType _type, const DataObjectRef& _dObj, unsigned long flags, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	dObj(_dObj),
	data(NULL),
	doesHaveData(_dObj),
	flags(flags)
{
	if (!EVENT_TYPE(type)) {
	        HAGGLE_ERR("trying to allocate an invalid event type (%d)!\n", type);
                return;
	}
	if (dObj) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_DATAOBJECT_RECEIVED || 
		    type == EVENT_TYPE_DATAOBJECT_VERIFIED || 
		    type == EVENT_TYPE_DATAOBJECT_NEW || 
		    type == EVENT_TYPE_DATAOBJECT_INCOMING ||
		    type == EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION ||
		    type == EVENT_TYPE_SECURITY_CONFIGURE ||
		    type == EVENT_TYPE_DYNAMIC_CONFIGURE) { // CBMEN, HL
		} else if (type == EVENT_TYPE_DATAOBJECT_DELETED) {
			// For simplicity's sake, we allow a deleted with just one data 
			// object, but to do that, we need to add it to the data object 
			// list (because the recipient of the event can't know how the 
			// event was created).
			dObjs.push_back(dObj);
			// Clear the dObj, so this event looks just like if it was 
			// created with the (..., DataObjectRefList, ...) constructor
			dObj = NULL;
		} else {
			HAGGLE_ERR("Event type %s does not accept a data object as data!\n",
				eventNames[type]);
		}
	}
}


Event::Event(EventType _type, const InterfaceRef& _iface, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)), 
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	iface(_iface),
	data(NULL),
	doesHaveData(_iface),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }

	if (iface) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_LOCAL_INTERFACE_UP || 
		    type == EVENT_TYPE_LOCAL_INTERFACE_DOWN || 
		    type == EVENT_TYPE_NEIGHBOR_INTERFACE_UP || 
		    type == EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN ||
		    type == EVENT_TYPE_RECEIVE_BEACON
		    ) {
		} else {
			HAGGLE_ERR("Event type %s does not accept an interface as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(EventType _type, const NodeRef& _node, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	node(_node),
	data(NULL),
	doesHaveData(_node),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }
	if (node) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_NODE_CONTACT_NEW || 
		    type == EVENT_TYPE_NODE_CONTACT_END) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a node as data!\n",
				eventNames[type]);
		}
	}
}
Event::Event(EventType _type, const PolicyRef& _policy, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	policy(_policy),
	data(NULL),
	doesHaveData(_policy),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }

	if (policy) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_RESOURCE_POLICY_NEW) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a policy as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(EventType _type, const DataObjectRef&  _dObj, const NodeRef& _node, unsigned long _flags, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	dObj(_dObj),
	node(_node),
	data(NULL),
	//	doesHaveData(_dObj && _node),
	doesHaveData(_dObj), // MOS - we sometimes need to raise events without node
	flags(_flags)
{
	if (!EVENT_TYPE(type)) {
                return;
        }
	if (doesHaveData) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_DATAOBJECT_FORWARD || 
		    type == EVENT_TYPE_DATAOBJECT_INCOMING || 
		    type == EVENT_TYPE_DATAOBJECT_RECEIVED || 
		    type == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL ||
		    type == EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL ||
		    type == EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL ||
		    type == EVENT_TYPE_DATAOBJECT_SEND_FAILURE  ||
		    type == EVENT_TYPE_DATAOBJECT_DELETE_NETWORKCODEDBLOCK ||
		    type == EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_NETWORKCODEDBLOCKS ||
		    type == EVENT_TYPE_DATAOBJECT_DELETE_FRAGMENT ||
                    type == EVENT_TYPE_ROUTE_TO_APP || // SW:
		    type == EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_FRAGMENTS ) {
			
		} else if (type == EVENT_TYPE_DATAOBJECT_SEND || 
			   type == EVENT_TYPE_DATAOBJECT_SEND_TO_APP || // MOS
			   type == EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING ||
			   type == EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION){
		    //JJ allow send networkcoding event to be sent to target node
			// For simplicity's sake, we allow a send with just a target node,
			// but to do that, we need to add it to the node list (because the
			// Recipient of the event can't know how the event was created).
			nodes.push_front(node);
			// Clear the node, so this event looks just like if it was 
			// created with the (..., DataObject, NodeRefList, ...) constructor
			node = NULL;
		} else {
			HAGGLE_ERR("Event type %s does not accept a data object and a node as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(const DebugCmdRef& _dbgCmd, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_DEBUG_CMD),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	dbgCmd(_dbgCmd),
	data(NULL),
	doesHaveData(_dbgCmd),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }

	if (dbgCmd) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_DEBUG_CMD) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a debug command as data!\n",eventNames[type]);
		}
	}
}
        
Event::Event(EventType _type, const NodeRef& _node, const NodeRefList& _nodes, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	node(_node),
	nodes(_nodes),
	data(NULL),
	doesHaveData(_node),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }
	if (node && !(nodes.empty())) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_TARGET_NODES || 
		    type == EVENT_TYPE_NODE_UPDATED ||
		    type == EVENT_TYPE_APP_NODE_UPDATED) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a node and a node list as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(EventType _type, const DataObjectRef& _dObj, const NodeRefList& _nodes, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	dObj(_dObj),
	nodes(_nodes),
	data(NULL),
	doesHaveData(_dObj && !(_nodes.empty())),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }
	if (dObj && !(nodes.empty())) {
		if (EVENT_TYPE_PRIVATE(type) ||
			type == EVENT_TYPE_DATAOBJECT_SEND ||
		        type == EVENT_TYPE_DATAOBJECT_SEND_TO_APP || // MOS
			//JJ event constructed by onDelegateNodes
			type == EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING ||
			type == EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION ) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a data object and a node list as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(EventType _type, const DataObjectRef& _dObj, const NodeRef& _node, const NodeRefList& _nodes, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	dObj(_dObj),
	node(_node),
	nodes(_nodes),
	data(NULL),
	doesHaveData(_dObj && !(_nodes.empty())),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }
	if (dObj && !(nodes.empty())) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_DELEGATE_NODES) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a data object, a node and a node list as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(EventType _type, const DataObjectRefList& _dObjs, unsigned long flags, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	dObjs(_dObjs),
	data(NULL),
	doesHaveData(_dObjs.size() > 0),
	flags(flags)
{
	if (!EVENT_TYPE(type)) {
		HAGGLE_ERR("trying to allocate an invalid event type!\n");
                return;
	}
	if (dObj) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_DATAOBJECT_DELETED) {
		} else {
			HAGGLE_ERR("Event type %s does not accept a list of data objects as data!\n",
				eventNames[type]);
		}
	}
}

Event::Event(EventType _type, void *_data, double _delay) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif
	HeapItem(),
	type(_type),
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	data(_data),
	doesHaveData(_data != NULL),
	flags(0)
{
	if (!EVENT_TYPE(type)) {
                return;
        }

	if (data != NULL) {
		if (EVENT_TYPE_PRIVATE(type) || 
		    type == EVENT_TYPE_PREPARE_STARTUP || 
		    type == EVENT_TYPE_STARTUP || 
		    type == EVENT_TYPE_PREPARE_SHUTDOWN || 
		    type == EVENT_TYPE_SHUTDOWN ||
		    type == EVENT_TYPE_DATAOBJECT_CONVERT_FRAGMENTATION_NETWORKCODING) {
		} else {
			switch(type) {
				case EVENT_TYPE_DATAOBJECT_RECEIVED:
				case EVENT_TYPE_DATAOBJECT_VERIFIED:
				case EVENT_TYPE_DATAOBJECT_NEW:
				case EVENT_TYPE_DATAOBJECT_DELETED:
				case EVENT_TYPE_DATAOBJECT_INCOMING:
				case EVENT_TYPE_SECURITY_CONFIGURE: // CBMEN, HL
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a data object!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_DATAOBJECT_FORWARD:
				case EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL:
				case EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL:
				case EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL:
				case EVENT_TYPE_DATAOBJECT_SEND_FAILURE:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a data object and a node!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_TARGET_NODES:
				case EVENT_TYPE_NODE_UPDATED:
				case EVENT_TYPE_APP_NODE_UPDATED:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a node and a node list!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_DATAOBJECT_SEND:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a data object and a node list!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_DELEGATE_NODES:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a data object, a node and a node list!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_LOCAL_INTERFACE_UP:
				case EVENT_TYPE_LOCAL_INTERFACE_DOWN:
				case EVENT_TYPE_NEIGHBOR_INTERFACE_UP:
				case EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only an interface!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_NODE_CONTACT_NEW:
				case EVENT_TYPE_NODE_CONTACT_END:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a node!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_RESOURCE_POLICY_NEW:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a resource policy!\n",
						eventNames[type]);
				break;
				
				case EVENT_TYPE_DEBUG_CMD:
					HAGGLE_ERR("Event type %s does not accept void * as data - "
						"only a debug command!\n",
						eventNames[type]);
				break;
				
				default:
				break;
			}
		}
	}
}

Event::Event(const EventCallback < EventHandler > *_callback, void *_data, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	data(_data),
	doesHaveData(_data ? true : false),
	flags(0)
{
}

Event::Event(const EventCallback<EventHandler> *_callback, const DataObjectRef&  _dObj, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	dObj(_dObj),
	data(NULL),
	doesHaveData(_dObj ? true : false),
	flags(0)
{
}

Event::Event(const EventCallback<EventHandler> *_callback, const InterfaceRef& _iface, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	iface(_iface),
	data(NULL),
	doesHaveData(_iface ? true : false),
	flags(0)
{
}

Event::Event(const EventCallback<EventHandler> *_callback, const NodeRef& _node, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	node(_node),
	data(NULL),
	doesHaveData(_node ? true : false),
	flags(0)
{
}

Event::Event(const EventCallback<EventHandler> *_callback, const PolicyRef& _policy, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	policy(_policy),
	data(NULL),
	doesHaveData(_policy ? true : false),
	flags(0)
{
}


Event::Event(const EventCallback<EventHandler> *_callback, const DataObjectRefList& _dObjs, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	dObjs(_dObjs),
	data(NULL),
	doesHaveData(_dObjs.empty() ? false : true),
	flags(0)
{
}

Event::Event(const EventCallback<EventHandler> *_callback, const DebugCmdRef& _dbgCmd, double _delay) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_EVENT),
#endif 
	HeapItem(),
	type(EVENT_TYPE_CALLBACK), 
	timeout(absolute_time_double(_delay)),
	scheduled(false),
	autoDelete(true),
	canceled(false), // CBMEN, HL
	callback(_callback), 
	dbgCmd(_dbgCmd),
	data(NULL),
	doesHaveData(_dbgCmd ? true : false),
	flags(0)
{
}

Event::~Event()
{
}

void Event::setTimeout(double t)
{
	timeout = absolute_time_double(t);
}

void Event::setScheduled(bool _scheduled)
{
	scheduled = _scheduled;
}

bool Event::isScheduled() const
{
	return scheduled;
}

void Event::setAutoDelete(bool _autoDelete)
{
	autoDelete = _autoDelete;
}

bool Event::shouldDelete() const
{
	return autoDelete;
}

// CBMEN, HL, Begin
void Event::setCanceled(bool _canceled)
{
	canceled = _canceled;
}

bool Event::isCanceled() const
{
	return canceled;
}
// CBMEN, HL, End

string Event::getDescription(void)
{
	char tmp[64];
	// Start with an empty description:
	string description = "";
	// Fallback: no data object => set the id to "-"
	string dObjIdStr = "-";
	string dObjIdListStr = "-";
	// Fallback: no node => set the id to "-"
	string nodeIdStr = "-";
	string nodeIdListStr = "-";
	// Fallback: no interface => set the id to "-"
	string ifaceStr = "-";
	// Fallback: no policy => set the id to "-"
	string policyStr = "-";
	// Fallback: no data => set the id to "-"
	string dataStr = "-";
	// This will be overwritten below:
	string eventTypeStr = "??";
	// Fallback: no event name => set it to:
	string eventNameStr = "[unknown event type]";
	// Fallback: Initial flag listing is empty:
	string flagsStr = "";
	
	// If there is a data object:
	if (dObj) {
		// Report this as the data object's id str + it's number:
		snprintf(tmp, 64, " %s-%u", dObj->getIdStr(), dObj->getNum());
		dObjIdStr = tmp;
	}
	
	if (dObjs.size())
		dObjIdListStr = "";

	for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
		snprintf(tmp, 64, "%s-%u ", (*it)->getIdStr(), (*it)->getNum());
		dObjIdListStr += tmp;
	}

	// If there is a node:
	if (node) {
		nodeIdStr = node->getTypeStr();
		nodeIdStr.append(":");
		// Report the node's id str:
		nodeIdStr.append(node->getIdStr());
	}
	
	if (nodes.size())
		nodeIdListStr = "";

	for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
		nodeIdListStr.append((*it)->getTypeStr());
		nodeIdListStr.append(":");
		nodeIdListStr.append((*it)->getIdStr());
		nodeIdListStr.append(" ");
	}

	// If there is an interface:
	if (iface) {
		// Report the interface's id str:
		ifaceStr = iface->getIdentifierStr();
	}
	
	// If there is a policy:
	if (policy) {
		// Report that there is (there is no way of identifying policies):
		policyStr = "+";
	}
	
	// If there is private data:
	if (data) {
		// Report that there is (there is really no way of telling what this 
		// is):
		dataStr = "+";
	}
	
	// Find the type number of this event:
	snprintf(tmp, 64, "%d", getType());
	eventTypeStr = tmp;

	// Find the name of this event:
	eventNameStr = getName();
	
	// Set up the flags field:
	// Note: for now, only the lowest bit is ever set, so this "loop" only goes
	// around once. If more flag bits are ever used, please adjust this loop.
	for (unsigned int i = 0; i < 1; i++) {
		if ((flags >> i) & 1)
			flagsStr = "1" + flagsStr;
		else
			flagsStr = "0" + flagsStr;
	}
	
	// Start with the event number:
	description = eventTypeStr + '\t';
	// Then the data object id:
	description += dObjIdStr + '\t';
	// Then the data objec id list 
	description += dObjIdListStr + '\t';
	// Then the node id:
	description += nodeIdStr + '\t';
	// Then the node id list
	description += nodeIdListStr + '\t';
	// Then the interface id:
	description += ifaceStr + '\t';
	// Then the policy id:
	description += policyStr + '\t';
	// Then the data id:
	description += dataStr + '\t';
	// Then the event's name:
	description += eventNameStr + '\t';
	// End with the flags field
	description += flagsStr;
	
	// Done:
	return description;
}

int Event::registerType(const char *name, EventCallback < EventHandler > *_callback)
{
	char *buf;
	EventType _type;

	if (num_event_types >= MAX_NUM_EVENT_TYPES)
		return -1;

	/* Find first free private event type slot */
	for (_type = EVENT_TYPE_PRIVATE_MIN; _type < EVENT_TYPE_PRIVATE_MAX; _type++) {
		if (privCallbacks[Event::privTypeToCallbackIndex(_type)] == NULL) {

			buf = (char *) malloc(strlen(name) + 1);

			if (!buf) {
				return -1;
			}

			strcpy(buf, name);

			num_event_types++;

			eventNames[_type] = buf;
			privCallbacks[Event::privTypeToCallbackIndex(_type)] = _callback;
			HAGGLE_DBG("Registered private event %d/%d:\'%s\'\n", _type, EVENT_TYPE_MAX, name);
			return _type;
		}
	}

	return -1;
}

int Event::unregisterType(EventType _type)
{
	if (!EVENT_TYPE_PRIVATE(_type)) {
		HAGGLE_ERR("Tried to delete event non-private event type %d\n", _type);
		return -1;
	}

	if (privCallbacks[privTypeToCallbackIndex(_type)] != NULL) {
		HAGGLE_DBG("Deleting event type %d: %s\n", _type, eventNames[_type]);

		delete privCallbacks[privTypeToCallbackIndex(_type)];
		privCallbacks[privTypeToCallbackIndex(_type)] = NULL;
		free(const_cast < char *>(eventNames[_type]));
		eventNames[_type] = NULL;

		return --num_event_types;
	} else {
		HAGGLE_ERR("Tried to delete empty event type %d, double delete?\n", _type);
		return -1;
	}
}


bool Event::compare_less(const HeapItem& i) const
{
	return (timeout < static_cast<const Event&>(i).timeout);
}
bool Event::compare_greater(const HeapItem& i) const
{
	return (timeout > static_cast<const Event&>(i).timeout);
}
