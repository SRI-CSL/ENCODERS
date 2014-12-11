/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
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
#ifndef _FORWARDER_H
#define _FORWARDER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class Forwarder;

#include "ManagerModule.h"
#include "ForwardingManager.h"
#include "RepositoryEntry.h"
#include "Metadata.h"

#define MAX_GENERATED_DELEGATES_DEFAULT 1
#define MAX_GENERATED_TARGETS_DEFAULT 1

/**
	Forwarding module base class.
	
	The forwarding manager will not actively call ->start() on its forwarding 
	object, so it is up to the forwarding module itself to start its thread if 
	it wants to run as a thread.
*/
class Forwarder : public ManagerModule<ForwardingManager> {
protected:
	// The max number of delegates to generate when
	// generateDelegatesFor() is called.
	unsigned long max_generated_delegates;
	// The max number of targets to generate when
	// generateTargetsFor() is called
	unsigned long max_generated_targets;
    // SW: START INIT RACE: used to avoid onConfig race condition in ForwardingManager
    	unsigned int repositoryCallbackCount;
    // SW: END INIT RACE.
public:
	Forwarder(ForwardingManager *m = NULL, 
		const string name = "Unknown forwarding module") :
		ManagerModule<ForwardingManager>(m, name),
		max_generated_delegates(MAX_GENERATED_DELEGATES_DEFAULT),
		max_generated_targets(MAX_GENERATED_TARGETS_DEFAULT),
		repositoryCallbackCount(0) {}
	virtual ~Forwarder() {}


        // SW: START SHORT CIRCUIT: used for light weight desimination 
        virtual bool useNewDataObjectShortCircuit(const DataObjectRef& dObj) { return false;};

        virtual void doNewDataObjectShortCircuit(const DataObjectRef& dObj) { };
        // SW: END SHORT CIRCUIT.
        
        // SW: START REP: used to determine whether repository data is relevent to this forwarder
        virtual bool isReleventRepositoryData(RepositoryEntryRef &re);

        virtual void getRepositoryData(EventCallback<EventHandler> *repositoryCallback);
        // SW: END REP. 
	
	// Only useful for asynchronous modules
	virtual void quit() {}
    // SW: allow forwarders to save state
	virtual void quit(RepositoryEntryList *rl) {}

	DataObjectRef createRoutingInformationDataObject();
	
	/**
	 This function determines if the given data object contains routing information
	 for this forwarding module. 
	 
	 Returns: true iff the data object routing information created by this forwarding module.
	 */
	virtual bool hasRoutingInformation(const DataObjectRef& dObj);
	
	/**
		This function returns a string with the node id for the node which 
		created the routing information
		
		Returns: if routing information is valid, a string containing a node id. 
		Otherwise NULL.
	*/
	const string getNodeIdFromRoutingInformation(const DataObjectRef& dObj) const;
	
	/**
		This function returns a string with the metric for the node which 
		created the given metric data object.
		
		Returns: if routing information is valid, a string containing a 
		metric. Otherwise NULL.
	*/
	const Metadata *getRoutingInformation(const DataObjectRef& dObj) const;
	
	/**
		Convenience function that allows the forwarder module to check
		a delegate node against a list of target nodes in order to make
		sure a data object is not delegated to a node which is also a 
		target.
	*/
	bool isTarget(const NodeRef &delegate, const NodeRefList *targets) const;

	/**
		A forwarding module should implement addRoutingInformation() in order
		to generate the Metadata containing routing information which is specific
		for that forwarding module.
	 */
	virtual bool addRoutingInformation(DataObjectRef& dObj, Metadata *m) { return false; }

	/*
		The following functions are called by the forwarding manager as part of
		event processing in the kernel. They are therefore called from the 
		kernel thread, and multiprocessing issues need to be taken into account.
		
		The reason for these functions to all be declared virtual ... {} is so 
		that specific forwarding modules can override only those functions they
		actually need to do their job. This means that functions can be declared
		here (and called by the forwarding manager) that only one forwarding 
		algorithm actually uses.
	*/		
	/**
		Called when a data object has come in that has a "Routing" attribute.
		
		Also called for each such data object that is in the data store on 
		startup.
		
		Since the format of the data in such a data object is unknown to the 
		forwarding manager, it is up to the forwarder to make sure the data is
		in the correct format.
		
		Also, the given metric data object may have been sent before, due to 
		limitations in the forwarding manager.
	*/
	virtual void newRoutingInformation(const DataObjectRef& dObj) {}	
	/**
		Called when a neighbor node is discovered.
	*/
	virtual void newNeighbor(const NodeRef &neighbor) {}
	/**
		Called when a neighbor node description is received.
	*/
	virtual void newNeighborNodeDescription(const NodeRef &neighbor) {} // MOS


	/**
		Called when a node just ended being a neighbor.
	*/
	virtual void endNeighbor(const NodeRef &neighbor) {}
	
	/**
		Generates an event (EVENT_TYPE_DELEGATE_NODES) providing all the nodes 
		that are good delegate forwarders for the given node.
		
		This function is given a target to which to send a data object, and 
		answers the question: To which delegate forwarders can I send the given
		data object, so that it will reach the given target?
		
		If no nodes are found, no event should be created.
	*/
	virtual void generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets) {}
	
	/**
		Generates an event (EVENT_TYPE_TARGET_NODES) providing all the target 
		nodes that the given node is a good delegate forwarder for.
		
		This function is given a current neighbor, and answers the question: 
		For which nodes is the given node a good delegate forwarder?
		
		If no nodes are found, no event should be created.
	*/
	virtual void generateTargetsFor(const NodeRef &neighbor) {}
	
	virtual void generateRoutingInformationDataObject(const NodeRef& neighbor, const NodeRefList *trigger_list = NULL) {}
	
	virtual size_t getSaveState(RepositoryEntryList& rel) { return 0; }
	virtual bool setSaveState(RepositoryEntryRef& e) { return false; }
	
	/**
		Prints the current routing table, without any enclosing text to show
		where it starts or stops.
	*/
	virtual void printRoutingTable(void) {}
	virtual void getRoutingTableAsMetadata(Metadata *m) {}; // CBMEN, HL
	// Derived module overrides onForwarderConfig to handle protocol specific config.
	virtual void onForwarderConfig(const Metadata& m) { }
	void onConfig(const Metadata& m);
    // SW: START INIT RACE:
	virtual bool isInitialized() { return false; }
	void incrementRepositoryCallbackCount() { repositoryCallbackCount++; }
    // SW: END INIT RACE.
};

#endif
