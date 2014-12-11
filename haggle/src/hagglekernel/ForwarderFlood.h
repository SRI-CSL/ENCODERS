/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#ifndef _FORWARDERFLOOD_H
#define _FORWARDERFLOOD_H

class ForwarderFlood;

#include "Forwarder.h"

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

using namespace haggle;

/*
 * A basic flood forwarder.
 */
class ForwarderFlood : public Forwarder {
#define FLOOD_NAME "Flood"
#define FORWARDER_FLOOD_PUSH_NAME "push_on_contact"
#define FORWARDER_FLOOD_PUSH_ON_NEW_NODE_DESC_NAME "push_on_new_node_description"
private:  
    HaggleKernel *kernel;
    bool doPush;
    bool doPushOnNewNodeDesc; // MOS
    bool useShortcut;
    bool enableDelegateGeneration; // MOS
    bool reactiveFlooding; // MOS
	typedef HashMap<string, string> FloodCache_t;
    FloodCache_t floodCache;
	Mutex floodCacheMutex;
public:
    ForwarderFlood(ForwardingManager *m = NULL, const string name = FLOOD_NAME);
    ~ForwarderFlood();
    void generateTargetsFor(const NodeRef &neighbor);
    void generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets);
    void onForwarderConfig(const Metadata& m);
    bool useNewDataObjectShortCircuit(const DataObjectRef& dObj);
    void doNewDataObjectShortCircuit(const DataObjectRef& dObj);
    void newNeighbor(const NodeRef &neighbor);
    void newNeighborNodeDescription(const NodeRef &neighbor); // MOS
    void neighborUpdate(const NodeRef &neighbor); // MOS

    void cacheDataObjectForFlood(const DataObjectRef &dObj);
    void uncacheDataObjectForFlood(string dObjId);
    void getRoutingTableAsMetadata(Metadata *m); // CBMEN, HL
};

#endif /* FORWARDERFLOOD */
