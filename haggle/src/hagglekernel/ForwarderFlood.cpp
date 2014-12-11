/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include <libcpphaggle/Platform.h>
#include "ForwarderFlood.h"
#include "XMLMetadata.h"

// TODO: free the callback upon completion

class ForwarderFloodRetrieveDataObjectCallback : public EventHandler 
{
private:
    ForwardingManager *manager;
    HaggleKernel *kernel;
    ForwarderFlood *flood;
    string dobjId;
    NodeRef neighbor;
    // avoid memleak: cleanup event after fired
    EventCallback<EventHandler> *wrapperCallback; 
public:
    ForwarderFloodRetrieveDataObjectCallback(
        ForwardingManager *_manager,
        HaggleKernel *_kernel,
        ForwarderFlood *_flood,
        string _dobjId,
        NodeRef _neighbor) :
            manager(_manager),
            kernel(_kernel),
            flood(_flood),
            dobjId(_dobjId),
            neighbor(_neighbor),
            wrapperCallback(NULL) {}
    
    ~ForwarderFloodRetrieveDataObjectCallback() {
        if (wrapperCallback) {
            delete wrapperCallback;
        }
    };

    void retrieveCallback(Event *e)
    {
        DataObjectRef dObj;
        bool shouldUncache = false;
        if (!e || !e->hasData()) {
            // this can occur when the data object is no longer in the DB
            HAGGLE_DBG("No data in callback\n");
            shouldUncache = true;
        } else {
            dObj = e->getDataObject();
            if (!dObj) {
                shouldUncache = true;
            }
        }

        if (shouldUncache) {
            HAGGLE_DBG("Data object no longer in DB, removing from cache\n");
            flood->uncacheDataObjectForFlood(dobjId);
        }
        else {
            string nodeName = neighbor->getName();
            HAGGLE_DBG("Pushing flooded data object to neighbor: %s\n", nodeName.c_str());
            kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_FORWARD, dObj, neighbor));
        }

        delete this;
    }

    void setWrapperCallback(EventCallback<EventHandler> *_wrapperCallback) {
        wrapperCallback = _wrapperCallback;
    }
};

ForwarderFlood::ForwarderFlood(ForwardingManager *m, const string name) :
	Forwarder(m, name),
	kernel(getManager()->getKernel()),
	doPush(false), doPushOnNewNodeDesc(true),
	useShortcut(true), // MOS - only true should be used
	enableDelegateGeneration(false),
	reactiveFlooding(false)
{
    HAGGLE_DBG("Forwarding module \'%s\' initialized\n", getName()); 
}

ForwarderFlood::~ForwarderFlood()
{
    HAGGLE_DBG("ForwarderFlood deconstructor.\n");
}

void 
ForwarderFlood::uncacheDataObjectForFlood(string dObjId) 
{
    Mutex::AutoLocker l(floodCacheMutex);
    
	FloodCache_t::iterator it = floodCache.find(dObjId);

    if (it != floodCache.end()) {
        floodCache.erase(it);
    }
}

void 
ForwarderFlood::cacheDataObjectForFlood(const DataObjectRef &dObj) 
{
    Mutex::AutoLocker l(floodCacheMutex);

    if (dObj->isNodeDescription()) {
        // this is handled in the ForwardingManager 
        return;
    }

    string dObjId = string(dObj->getIdStr());
    FloodCache_t::iterator it = floodCache.find(dObjId);

    if (it != floodCache.end()) {
        HAGGLE_DBG("Data object already in flood cache\n");
        return;
    }
    
    HAGGLE_DBG("Caching data object in flood cache\n");

    floodCache.insert(make_pair(dObjId, string("")));
}

/*
 *  Generate a list of nodes for which `neighbor' is a good
 *  delegate for.
 */ 
void ForwarderFlood::generateTargetsFor(const NodeRef &neighbor)
{
    HAGGLE_DBG("ForwarderFlood does not generate targets.\n"); // MOS
    /*
    HAGGLE_DBG("ForwarderFlood generating a list targets that this node is a delegate for.\n");
    NodeRefList allNodesRefList;
    kernel->getNodeStore()->retrieveAllNodes(allNodesRefList);
    kernel->addEvent(new Event(EVENT_TYPE_TARGET_NODES, neighbor, allNodesRefList));
    */
}

/* 
 * Generate a list of all the delegates for a particular target.
 * 'dObj' is the object that is being forwarded.
 * 'target' is the destination node.
 * 'other_targets' are other possible destinations.
 */
void ForwarderFlood::generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets)
{
    if (!enableDelegateGeneration && useNewDataObjectShortCircuit(dObj)) {
        HAGGLE_DBG("Bypassing generateDelegates (using short-circuit)\n");
        return;
    }

    HAGGLE_DBG("ForwarderFlood generating a list of delegates.\n");

    NodeRefList delegates;
    kernel->getNodeStore()->retrieveNeighbors(delegates);
    kernel->addEvent(new Event(EVENT_TYPE_DELEGATE_NODES, dObj, target, delegates));
    HAGGLE_DBG("Generated %lu delegates for target %s\n", delegates.size(), target->getName().c_str());
    
    if (doPush) {
        cacheDataObjectForFlood(dObj);
    }
}

/* 
 * Handler that is called when a configuration object is received locally.
 */
void ForwarderFlood::onForwarderConfig(const Metadata& m)
{
    if (strcmp(getName(), m.getName().c_str()) != 0) {
        return;
    }

    const char *param;
    param = m.getParameter(FORWARDER_FLOOD_PUSH_NAME);

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling push on contact\n");
            doPush = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling push on contact\n");
            doPush = false;
        }
    }

    param = m.getParameter(FORWARDER_FLOOD_PUSH_ON_NEW_NODE_DESC_NAME);

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling push on new node description\n");
            doPushOnNewNodeDesc = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling push on new node description\n");
            doPushOnNewNodeDesc = false;
        }
    }

    // MOS - enable delegate generation tiggered by node/data object queries
    param = m.getParameter("enable_delegate_generation");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling delegate generation\n");
            enableDelegateGeneration = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling delegate generation\n");
            enableDelegateGeneration = false;
        }
    }

    // MOS - only makes sense together with enable_delegate_generation
    param = m.getParameter("reactive_flooding");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling reactive flooding\n");
            reactiveFlooding = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling reactive flooding\n");
            reactiveFlooding = false;
        }
    }

/* // SW: the semantics of not using the shortcut are unclear, 
   // disabling this option for now
    param = m.getParameter("use_shortcut");

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling flood shortcut\n");
            useShortcut = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling flood shortcut\n");
            useShortcut = false;
        }
    }
*/

    HAGGLE_DBG("Flood forwarder configuration (push enabled: %s)\n", doPush ? "true" : "false");
}

bool ForwarderFlood::useNewDataObjectShortCircuit(const DataObjectRef& dObj)
{
  if(dObj->isNodeDescription()) return true;
  if (doPush) {
    cacheDataObjectForFlood(dObj); // MOS - needs to happen at onNewDataObject (might be better to have a separate function)
  }
  if(reactiveFlooding) {
    // MOS - do not proactively initiate flooding at data source
    InterfaceRef iface = dObj->getRemoteInterface();
    if(iface && iface->isApplication()) return false;
  }
  return useShortcut;
}

void ForwarderFlood::doNewDataObjectShortCircuit(const DataObjectRef& dObj)
{
    NodeRef bogusTarget;
    NodeRefList delegates;
    kernel->getNodeStore()->retrieveNeighbors(delegates);
    kernel->addEvent(new Event(EVENT_TYPE_DELEGATE_NODES, dObj, bogusTarget, delegates));
    // MOS - moved to useNewDataObjectShortCircuit
    // no need to cache node descriptions (already in the node store)
    // if (doPush) {
    //     cacheDataObjectForFlood(dObj);
    //  }
}


void ForwarderFlood::newNeighbor(const NodeRef &neighbor)
{
    if (!doPush) {
        HAGGLE_DBG("Bypassing flood push on new neighbor\n");
        return;
    }
    
    HAGGLE_DBG("Flood push on new neighbor\n");

    neighborUpdate(neighbor);
}

void ForwarderFlood::newNeighborNodeDescription(const NodeRef &neighbor)
{
    if (!doPush) {
        HAGGLE_DBG("Bypassing flood push on new neighbor node description\n");
        return;
    }

    if (!doPushOnNewNodeDesc) {
        HAGGLE_DBG("Bypassing flood push on new neighbor node description\n");
        return;
    }

    HAGGLE_DBG("Flood push on new neighbor node description\n");
    
    neighborUpdate(neighbor);
}

void ForwarderFlood::neighborUpdate(const NodeRef &neighbor)
{
    Mutex::AutoLocker l(floodCacheMutex);

    // NOTE: it appears new neighbor may be fired before we've received the latest
    // node description 
    NodeRef latestNeighbor = getManager()->getKernel()->getNodeStore()->retrieve(neighbor, true);
    if (!latestNeighbor) {
        HAGGLE_DBG("Could not get latest node ref for neighbor\n");
        return;
    }

    for (FloodCache_t::iterator it = floodCache.begin();
         it != floodCache.end(); it++) {
        string id_str = (*it).first;
        DataObjectId_t id;
        DataObject::idStrToId(id_str, id);

        // START: shortcitcuit DB, as per MOS suggestion:
        if (latestNeighbor->has(id)) {
	    // HAGGLE_DBG("Taking flood DB shortcircuit for DO: %s\n", id_str.c_str());
            continue;
        }
        // END: shortcitcuit DB, as per MOS suggestion:

	HAGGLE_DBG("Retrieving cached data object %s\n", id_str.c_str());

        ForwarderFloodRetrieveDataObjectCallback *retrieveCallback =
            new ForwarderFloodRetrieveDataObjectCallback(
                getManager(), 
                getKernel(), 
                this,
                id_str,
                neighbor);

        EventCallback<EventHandler> *callback = (EventCallback<EventHandler> *)
            new EventCallback <ForwarderFloodRetrieveDataObjectCallback>(
                retrieveCallback,
                &ForwarderFloodRetrieveDataObjectCallback::retrieveCallback);

        // NOTE: this is necessary to avoid memleak of callback
        retrieveCallback->setWrapperCallback(callback);

        getManager()->getKernel()->getDataStore()->retrieveDataObject(id, callback);
    }
}

// CBMEN, HL, Begin
void ForwarderFlood::getRoutingTableAsMetadata(Metadata *m) {
    Metadata *dm = m->addMetadata("Forwarder");
    if (!dm) {
        return;
    }

    dm->setParameter("type", "ForwarderFlood");
    dm->setParameter("name", getName());
}
// CBMEN, HL, End
