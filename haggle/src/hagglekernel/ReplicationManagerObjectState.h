/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_MANAGER_OBJECT_STATE_H
#define _REPL_MANAGER_OBJECT_STATE_H

class ReplicationManagerObjectState;

#include "DataObject.h"
#include "Node.h"

class CachedNodeIterator {
private:
    Map<string, NodeRef> nodes;
    Map<string, NodeRef>::iterator it;
public:
    CachedNodeIterator(Map<string, NodeRef> _nodes) : nodes(_nodes), it(nodes.begin()) {};
    NodeRef getNode() { return (*it).second; };
    void next() { it++; };
    bool has_next() { return it != nodes.end(); };
};

class CachedDataObjectIterator {
private:
    Map<string, DataObjectRef> objs;
    Map<string, DataObjectRef>::iterator it;
public:
    CachedDataObjectIterator(Map<string, DataObjectRef> dobjs) : objs(dobjs), it(objs.begin()) {};
    DataObjectRef getDataObject() { return (*it).second; };
    void next() { it++; };
    bool has_next() { return it != objs.end(); };
};

class ReplicationManagerObjectState {
private:
    // node / dobj caches
    Map<string, DataObjectRef> cachedDataObjects;
    Map<string, NodeRef> cachedNodes;

    // match ratio cache
    BasicMap<string, string> dataObjectIdMatchIndex;
    BasicMap<string, string> nodeIdMatchIndex;

    typedef struct match {
        string node_id;
        int ratio;
        int threshold;
    } match_t;

    Map<string, match> dataObjectIdMatch;

    long dataObjectsInserted;
    long dataObjectsDeleted;
    long nodesInserted;
    long nodesDeleted;
    long errorCount;

    static bool runSelfTest1();
public:
    ReplicationManagerObjectState() : 
        dataObjectsInserted(0),
        dataObjectsDeleted(0),
        nodesInserted(0),
        nodesDeleted(0),
        errorCount(0) {};

    // some helper/utility functions
    static string getKey(string dobj_id, string node_id);
    static string nodeIdFromKey(string key);
    static string dataObjectIdFromKey(string key);

    // cache data objects & nodes
    void cacheNode(NodeRef node);
    void unCacheNode(string node_id);
    bool isCachedNode(string node_id);
    void cacheDataObject(DataObjectRef dObj);
    void unCacheDataObject(string dobj_id);
    bool isCachedDataObject(string dobj_id);

    // cache match strength
    void setMatchStrength(string dobj_id, string node_name, int ratio, int threshold);
    bool getMatchStrength(string dobj_id, string node_id, int *o_ratio, int *o_threshold);

    CachedNodeIterator* getNodeIterator() { return new CachedNodeIterator(cachedNodes); }
    CachedDataObjectIterator* getObjectIterator() { return new CachedDataObjectIterator(cachedDataObjects); }

    long getErrorCount() { return errorCount; }

    static bool runSelfTest();
    void printStats();
    void printDebug();
};

#endif
