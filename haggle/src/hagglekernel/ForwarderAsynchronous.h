/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

/* Copyright 2009 Uppsala University
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
#ifndef _FORWARDERASYNCHRONOUS_H
#define _FORWARDERASYNCHRONOUS_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ForwarderAsynchronous;
class ForwardingTask;

#include "Forwarder.h"
#include "ForwarderAsynchronousInterface.h"
#include "DataObject.h"
#include "Node.h"

#include <libcpphaggle/String.h>
#include <libcpphaggle/Mutex.h>
#include <libcpphaggle/GenericQueue.h>
#include <haggleutils.h>

/**
	This enum is used in the actions to tell the run loop what to do.
*/
typedef enum {
	// Add bew metric data to the routing table
	FWD_TASK_NEW_ROUTING_INFO,
	// This neighbor was just seen in the neighborhood
	FWD_TASK_NEW_NEIGHBOR,
	// This neighbor just left the neighborhood
	FWD_TASK_END_NEIGHBOR,
	// Get the nodes that delegateNode is a good delegate forwarder for
	FWD_TASK_GENERATE_TARGETS,
	// Get the nodes that are good delegate forwarders for this node
	FWD_TASK_GENERATE_DELEGATES,
	// Generate the routing information that is sent 
	// to any new neighbors
	FWD_TASK_GENERATE_ROUTING_INFO_DATA_OBJECT,
#ifdef DEBUG
	// Print the routing table:
	FWD_TASK_PRINT_RIB,
#endif
	FWD_TASK_CONFIG,
	// Terminate the run loop
	FWD_TASK_QUIT
} ForwardingTaskType_t;

/**
	These action elements are used to send data to the run loop, in order to
	make processing asynchronous.
*/
class ForwardingTask {
private:
    ForwarderAsynchronousInterface* forwarder;
	const ForwardingTaskType_t type;
	DataObjectRef dObj;
	NodeRef	node;
	NodeRefList *nodes;
	RepositoryEntryList *rel;
	Metadata *m;
public:
	ForwardingTask(ForwarderAsynchronousInterface* _forwarder, const ForwardingTaskType_t _type, const DataObjectRef& _dObj = NULL, const NodeRef& _node = NULL, const NodeRefList *_nodes = NULL) :
		forwarder(_forwarder), type(_type), dObj(_dObj), node(_node), nodes(_nodes ? _nodes->copy() : NULL), rel(NULL), m(NULL) {}
	ForwardingTask(ForwarderAsynchronousInterface* _forwarder, const ForwardingTaskType_t _type, const NodeRef& _node, const NodeRefList *_nodes = NULL) :
		forwarder(_forwarder), type(_type), dObj(NULL), node(_node), nodes(_nodes ? _nodes->copy() : NULL), rel(NULL), m(NULL) {}
	ForwardingTask(ForwarderAsynchronousInterface* _forwarder, const Metadata& _m) : 
		forwarder(_forwarder), type(FWD_TASK_CONFIG), dObj(NULL), node(NULL), nodes(NULL), rel(NULL), m(_m.copy()) {}
	DataObjectRef& getDataObject() { return dObj; }
	void setDataObject(const DataObjectRef& _dObj) { dObj = _dObj; }
	NodeRef& getNode() { return node; }
	RepositoryEntryList *getRepositoryEntryList() { return rel; }
	NodeRefList *getNodeList() { return nodes; }
	void setRepositoryEntryList(RepositoryEntryList *_rel) { rel = _rel; }
	ForwardingTaskType_t getType() const { return type; }
	Metadata *getConfig() { return m; }
    ForwarderAsynchronousInterface *getForwarder() { return forwarder; }
	~ForwardingTask() { if (rel) delete rel; if (nodes) delete nodes; if (m) delete m; }
};

/**
	Asynchronous forwarding module. A forwarding module should inherit from this
	module if it is doing too much processing to be executing in the kernel 
	thread.
*/
class ForwarderAsynchronous : public Forwarder {
protected:
    HaggleKernel *kernel;
	const EventType eventType;
	GenericQueue<ForwardingTask *> *taskQ;	
	bool run(void);

    // hooks for clean shutdown
    virtual void _interfaceQuitHook(const EventType type, ForwardingTask *task, ForwarderAsynchronousInterface *f);
    virtual void _threadQuitHook(const EventType type, ForwardingTask *task); 
public:
	ForwarderAsynchronous(
        ForwardingManager *m = NULL, 
        const EventType type = -1, 
        const string name = "Asynchronous forwarding module", 
        GenericQueue<ForwardingTask *> *taskQ = new GenericQueue<ForwardingTask *>());

	/**
	 Generally, the thread should not be stopped by doing a delete 
	 on the forwarding module object. This is because, once in the destructor,
	 the state of the object might already have been deleted, and then it
	 is too late to save it. Use the quit() function instead, because it will
	 stop the thread and allow it to save its state before the destructor is
	 called. The stop() in the destructor is only a safeguard in case the thread 
	 is for some reason already running
	 */
	virtual ~ForwarderAsynchronous();

    const EventType getEventType() {
        return eventType;
    }

    GenericQueue<ForwardingTask *> *getTaskQueue() {
        return taskQ;
    }
	
	/**
	 Call quit() when the forwarding module thread should exit. After calling quit, 
	 the forwarding module will save its state to the data store and then exit. 
	 */
	virtual void quit();
	virtual void quit(RepositoryEntryList *rl);
};

#endif
