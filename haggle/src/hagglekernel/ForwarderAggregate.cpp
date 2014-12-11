/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include "ForwarderAggregate.h"

#include "ForwarderProphet.h"
#include "ForwarderAlphaDirect.h"

ForwarderAggregate::ForwarderAggregate(ForwardingManager *m, const EventType type, ForwardingClassifier* classifier, const string name) :
    ForwarderAsynchronous(m, type, name),
    classifier(classifier),
    defaultForwarder(NULL)
{
}

ForwarderAggregate::~ForwarderAggregate()
{
    if (classifier) {
        classifier->quit();
        delete classifier;
    }

    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        delete forwarder;
    }
   
}

bool ForwarderAggregate::run() 
{   
    // kick off the main run loop
    ForwarderAsynchronous::run();
    
    return true;    
}

void ForwarderAggregate::quit() 
{
    Forwarder* forwarder;
    // NOTE: although this may seem un-thread safe, since all of
    // the ForwarderAsynchronousInterfaces use this single thread,
    // the "quit()" will be synchronized.
    RepositoryEntryList *rl = new RepositoryEntryList();
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->quit(rl);
    }
    
    ForwarderAsynchronous::quit(rl);
}

bool ForwarderAggregate::hasRoutingInformation(const DataObjectRef &dObj)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        if (forwarder->hasRoutingInformation(dObj)) {
           return true; 
        }
    }
    return false;
}

void ForwarderAggregate::newRoutingInformation(const DataObjectRef &dObj)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        if (forwarder->hasRoutingInformation(dObj)) {
            forwarder->newRoutingInformation(dObj);
        }
    }
}


void ForwarderAggregate::newNeighbor(const NodeRef &neighbor)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->newNeighbor(neighbor);
    }
}

void ForwarderAggregate::newNeighborNodeDescription(const NodeRef &neighbor)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->newNeighborNodeDescription(neighbor);
    }
}

void ForwarderAggregate::endNeighbor(const NodeRef &neighbor)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->endNeighbor(neighbor);
    }
}

void ForwarderAggregate::generateTargetsFor(const NodeRef &neighbor)
{
    // TODO: for now, we use the defaultForwarder to generate the neighbors,
    // to provide backwards compatibility with Prophet

    Forwarder *f = defaultForwarder;

    if (NULL == f) {
        HAGGLE_DBG("Could not use default forwarder to generate targets.\n");
        return;
    }

    f->generateTargetsFor(neighbor);
}

/*
 * Helper function to select the forwarder for a data object.
 * If the DO can't be classified, then we revert to the default forwarder.
 */
Forwarder *ForwarderAggregate::getForwarderForDataObject(
    const DataObjectRef& dObj)
{
    if (!dObj) {
        HAGGLE_ERR("Null data object\n");
        return NULL;
    }

    Forwarder *f = NULL;
    if (NULL != classifier) {
        f = classifier->getForwarderForDataObject(dObj);
    }

    if (NULL == f) {
        HAGGLE_DBG("Using default forwarder, could not classify content\n");
        f = defaultForwarder;
    }

    if (NULL == f) {
        HAGGLE_DBG("Could not get a forwarder for the data object.\n");
    }

    return f;
}

void ForwarderAggregate::generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets)
{
    Forwarder *f = getForwarderForDataObject(dObj);
    if (NULL == f) {
        HAGGLE_ERR("Could not get forwarder\n");
        return;
    }
    f->generateDelegatesFor(dObj, target, other_targets); 
}

void ForwarderAggregate::generateRoutingInformationDataObject(const NodeRef& node, const NodeRefList *trigger_list)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->generateRoutingInformationDataObject(node, trigger_list);
    }
}

void ForwarderAggregate::printRoutingTable(void)
{
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->printRoutingTable();
    }
}

// CBMEN, HL, Begin
void ForwarderAggregate::getRoutingTableAsMetadata(Metadata *m) {

    Metadata *dm = m->addMetadata("Forwarder");
    if (!dm) {
        return;
    }

    dm->setParameter("type", "ForwarderAggregate");
    dm->setParameter("name", getName());

    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        Forwarder *forwarder = *it;
        forwarder->getRoutingTableAsMetadata(dm);
    }
}
// CBMEN, HL, End

void ForwarderAggregate::addDefaultForwarder(Forwarder* f) 
{
    if (NULL == f) {
        HAGGLE_ERR("No default forwarder specified.\n");
        return;
    }

    if (NULL != defaultForwarder) {
        HAGGLE_ERR("Overriding existing data forwarer, not supported\n");
        return;
    }

    defaultForwarder = f;

    // check if the forwarder is already in the list
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        if (f == forwarder) {
            return; // nothing to do
        }
    }

    // forwarder is not in the list, add it
    forwarderList.push_back(f);
}

void ForwarderAggregate::addForwarderForClassName(Forwarder* f, string className)
{
    if (NULL == f) {
        HAGGLE_ERR("Got NULL forwarder.\n");
        return;
    }

    if (FORWARDING_CLASSIFIER_INVALID_CLASS == className) {
        HAGGLE_ERR("Cannot register for invalid class name\n");
        return;
    }
    
    forwarderList.push_back(f);
    if (classifier) {
        classifier->registerForwarderForClass(f, className);
    } else {
        HAGGLE_ERR("No classifier for forwarder registration\n");
        return;
    }

    HAGGLE_DBG("Added forwarder %s for content class name: %s\n", f->getName(), className.c_str());
}

void ForwarderAggregate::onForwarderConfig(const Metadata& m) 
{
    HAGGLE_ERR("ForwarderAggregate does not currently take options.\n");
}

bool ForwarderAggregate::useNewDataObjectShortCircuit(const DataObjectRef& dObj)
{
    Forwarder *f = getForwarderForDataObject(dObj);
    if (NULL == f) {
        HAGGLE_DBG("No forwarder for data object\n");
        return false;
    }

    return f->useNewDataObjectShortCircuit(dObj);
}

void ForwarderAggregate::doNewDataObjectShortCircuit(const DataObjectRef& dObj)
{
    
    Forwarder *f = getForwarderForDataObject(dObj);
    if (NULL == f) {
        HAGGLE_ERR("No forwarder for data object\n");
        return;
    }

    if (!(f->useNewDataObjectShortCircuit(dObj))) {
        HAGGLE_ERR("Should not use short circuit with this DataObject\n");
        return;
    }

    f->doNewDataObjectShortCircuit(dObj);
}

bool ForwarderAggregate::isReleventRepositoryData(RepositoryEntryRef& re) 
{
    if (Forwarder::isReleventRepositoryData(re)) {
        return true;
    }

    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        if (forwarder->isReleventRepositoryData(re)) {
            return true;
        }
    }
   
    return false;
}

bool ForwarderAggregate::setSaveState(RepositoryEntryRef& re)
{ 
    HAGGLE_DBG("ForwarderAggregate loading saved state.\n");
    bool success = true;

    if (Forwarder::isReleventRepositoryData(re)) {
        // TODO: for now, aggregate does not have state
        return success;
    }

    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        if (forwarder->isReleventRepositoryData(re)) {
            success = (forwarder->setSaveState(re) && success);
        }
    }
     
    return success;
}

void ForwarderAggregate::getRepositoryData(EventCallback<EventHandler> *repositoryCallback)
{
    HAGGLE_DBG("ForwarderAggregate searching for repository data.\n");
    Forwarder* forwarder;
    for (List<Forwarder*>::iterator it = forwarderList.begin(); 
        it != forwarderList.end(); it++) {
        forwarder = *it;
        forwarder->getRepositoryData(repositoryCallback);
    }

    Forwarder::getRepositoryData(repositoryCallback);
}

bool ForwarderAggregate::isInitialized() {
    // a callback for each forwarder, and the aggregate itself
    return repositoryCallbackCount == (forwarderList.size() + 1);
}
