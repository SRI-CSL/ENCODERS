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

// SW: NOTE: the content of this file was adapted from ForwarderAsynchronous.cpp

#include "ForwarderAsynchronousInterface.h"

ForwarderAsynchronousInterface::ForwarderAsynchronousInterface(ForwarderAsynchronous *forwarderAsync, const string name):
    Forwarder(forwarderAsync->getManager(), name),
    eventType(forwarderAsync->getEventType()),
    taskQ(forwarderAsync->getTaskQueue())
{
}

ForwarderAsynchronousInterface::~ForwarderAsynchronousInterface()
{
}

void ForwarderAsynchronousInterface::quit(RepositoryEntryList *rl)
{
    ForwardingTask *task = new ForwardingTask(this, FWD_TASK_QUIT);
    if (NULL != rl) {
        task->setRepositoryEntryList(rl);
    }
    taskQ->insert(task);
}

void ForwarderAsynchronousInterface::quit()
{
    quit(NULL);
}

void ForwarderAsynchronousInterface::newRoutingInformation(const DataObjectRef &dObj)
{
    if (!dObj) {
        return;
    }

    taskQ->insert(new ForwardingTask(this, FWD_TASK_NEW_ROUTING_INFO, dObj));
}

void ForwarderAsynchronousInterface::newNeighbor(const NodeRef &neighbor)
{
    if (!neighbor) {
        return;
    }

    taskQ->insert(new ForwardingTask(this, FWD_TASK_NEW_NEIGHBOR, neighbor));
}

void ForwarderAsynchronousInterface::endNeighbor(const NodeRef &neighbor)
{
    if (!neighbor) {
        return;
    }
        
    taskQ->insert(new ForwardingTask(this, FWD_TASK_END_NEIGHBOR, neighbor));
}

void ForwarderAsynchronousInterface::generateTargetsFor(const NodeRef &neighbor)
{
    if (!neighbor) {
        return;
    }
	
    taskQ->insert(new ForwardingTask(this, FWD_TASK_GENERATE_TARGETS, neighbor));
}

void ForwarderAsynchronousInterface::generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets)
{
    if (!dObj || !target) {
        return;
    }
	
    taskQ->insert(new ForwardingTask(this, FWD_TASK_GENERATE_DELEGATES, dObj, target, other_targets));
}

void ForwarderAsynchronousInterface::generateRoutingInformationDataObject(const NodeRef& node, const NodeRefList *trigger_list)
{
    taskQ->insert(new ForwardingTask(this, FWD_TASK_GENERATE_ROUTING_INFO_DATA_OBJECT, node, trigger_list));
}

void ForwarderAsynchronousInterface::printRoutingTable(void)
{
    taskQ->insert(new ForwardingTask(this, FWD_TASK_PRINT_RIB));
}

// CBMEN, HL, Begin
void ForwarderAsynchronousInterface::getRoutingTableAsMetadata(Metadata *m) {
    this->_getRoutingTableAsMetadata(m);
}
// CBMEN, HL, End

void ForwarderAsynchronousInterface::onForwarderConfig(const Metadata& m)
{
    taskQ->insert(new ForwardingTask(this, m));
}
