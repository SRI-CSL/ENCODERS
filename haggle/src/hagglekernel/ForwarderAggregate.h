/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#ifndef _FORWARDERAGGREGATE_H
#define _FORWARDERAGGREGATE_H 

class ForwarderAggregate;

#include "Forwarder.h"
#include "DataObject.h"
#include "Node.h"
#include "ForwardingClassifier.h"
#include "ForwarderAsynchronous.h"

class ForwarderAggregate : public ForwarderAsynchronous {
private:
    List<Forwarder*> forwarderList;
    ForwardingClassifier* classifier;
    Forwarder *defaultForwarder;
    bool run(void); // called when the aggregate thread is started
    Forwarder *getForwarderForDataObject(const DataObjectRef& dObj); 
public:
    ForwarderAggregate(ForwardingManager *m = NULL, const EventType type = -1, ForwardingClassifier* classifier = NULL, const string name = "ForwarderAggregate");
    ~ForwarderAggregate();
    bool hasRoutingInformation(const DataObjectRef& dObj);
    void newRoutingInformation(const DataObjectRef& dObj);
    void newNeighbor(const NodeRef &neighbor);
    void newNeighborNodeDescription(const NodeRef &neighbor); // MOS
    void endNeighbor(const NodeRef &neighbor);
    void generateTargetsFor(const NodeRef &neighbor);
    void generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets);
    void generateRoutingInformationDataObject(const NodeRef &neighbor, const NodeRefList *trigger_list = NULL);
    void printRoutingTable(void);
    void getRoutingTableAsMetadata(Metadata *m); // CBMEN, HL
    void onForwarderConfig(const Metadata& m);
    void addDefaultForwarder(Forwarder* f);
    void addForwarderForClassName(Forwarder* f, string className);
    virtual void quit(); 

    bool useNewDataObjectShortCircuit(const DataObjectRef& dObj);
    void doNewDataObjectShortCircuit(const DataObjectRef& dObj);
    // allow saving/loading of forwarder state
    bool isReleventRepositoryData(RepositoryEntryRef& re); 
    bool setSaveState(RepositoryEntryRef& re);
    virtual void getRepositoryData(EventCallback<EventHandler> *repositoryCallback);
    virtual bool isInitialized();
};

#endif /* FORWARDERAGGREGATE */
