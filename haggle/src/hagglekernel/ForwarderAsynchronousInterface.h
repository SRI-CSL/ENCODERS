/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

// SW: NOTE: the content of this file was adapted from ForwarderAsynchronous.h

#ifndef _FORWARDERASYNCHRONOUSINTERFACE_H
#define _FORWARDERASYNCHRONOUSINTERFACE_H

class ForwarderAsynchronousInterface;

#include "ForwarderAsynchronous.h"

class ForwarderAsynchronousInterface : public Forwarder {
    friend class ForwarderAsynchronous;
	const EventType eventType;
	
	GenericQueue<ForwardingTask *> *taskQ;	

protected:
	/**
		Does the actual work of newForwardingDataObject.
	*/
	virtual bool newRoutingInformation(const Metadata *m) { return false; }
	
	/**
		Does the actual work of newNeighbor.
	*/
	virtual void _newNeighbor(const NodeRef &neighbor) {}
	
	/**
		Does the actual work of endNeighbor.
	*/
	virtual void _endNeighbor(const NodeRef &neighbor) {}
	
	/**
		Does the actual work of getTargetsFor.
	*/
	virtual void _generateTargetsFor(const NodeRef &neighbor) {}
	
	/**
		Does the actual work of getDelegatesFor.
	*/
	virtual void _generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets) {}

	/**
		Does the actual work or printRoutingTable().
	*/
	virtual void _printRoutingTable(void) {}
	virtual void _getRoutingTableAsMetadata(Metadata *m) {}
public:
	ForwarderAsynchronousInterface(ForwarderAsynchronous *forwarderAsync, const string name = "Asynchronous forwarding module");
	virtual ~ForwarderAsynchronousInterface();
	void quit();
    void quit(RepositoryEntryList *rl);
	
	/** See the parent class function with the same name. */
	void newRoutingInformation(const DataObjectRef& dObj);
	/** See the parent class function with the same name. */
	void newNeighbor(const NodeRef &neighbor);
	/** See the parent class function with the same name. */
	void endNeighbor(const NodeRef &neighbor);
	/** See the parent class function with the same name. */
	void generateTargetsFor(const NodeRef &neighbor);
	/** See the parent class function with the same name. */
	void generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets);

	void generateRoutingInformationDataObject(const NodeRef &neighbor, const NodeRefList *trigger_list = NULL);
	/** See the parent class function with the same name. */
	void printRoutingTable(void);
	void getRoutingTableAsMetadata(Metadata *m); // CBMEN, HL
	virtual void _onForwarderConfig(const Metadata& m) {}
	void onForwarderConfig(const Metadata& m);
};

#endif /* FORWARDERASYNCHRONOUSINTERFACE */
