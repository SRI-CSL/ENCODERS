/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ReplicationManagerObjectState.h"

string
ReplicationManagerObjectState::getKey(string dobj_id, string node_name)
{
    return node_name + string(":") + dobj_id;
}

string
ReplicationManagerObjectState::nodeIdFromKey(string key)
{
    return key.substr(0, key.find_last_of(":"));
}

string
ReplicationManagerObjectState::dataObjectIdFromKey(string key)
{
    return key.substr(key.find_last_of(":") + 1);
}

void
ReplicationManagerObjectState::cacheNode(NodeRef node)
{
    string node_name = Node::nameString(node);
    string node_id = Node::idString(node);

    if (isCachedNode(node_name)) {
        HAGGLE_ERR("Node: %s already cached!\n", node_name.c_str());
        errorCount++;
        return;
    }

    cachedNodes.insert(make_pair(node_name, node));
    proxy_id_to_node_name.insert(make_pair(node_id, node_name));

    nodesInserted++;
}

bool
ReplicationManagerObjectState::isCachedNode(string node_name)
{
    Map<string, NodeRef>::iterator it = cachedNodes.find(node_name);
    if (it != cachedNodes.end()) {
        return true;
    }
    return false;
}

void
ReplicationManagerObjectState::unCacheNode(string node_name)
{
    {
        Map<string, NodeRef>::iterator it = cachedNodes.find(node_name);
        if (it == cachedNodes.end()) {
            HAGGLE_ERR("Node is not cached.\n");
            errorCount++;
            return;
        }
        NodeRef node = (*it).second;
        cachedNodes.erase(it);
        string node_id = Node::idString(node);
        Map<string, string>::iterator itt = proxy_id_to_node_name.find(node_id);
        if (itt == proxy_id_to_node_name.end()) {
            HAGGLE_ERR("Proxy id is missing for node: %s\n", node_id.c_str());
            errorCount++;
            return;
        }
        proxy_id_to_node_name.erase(itt);
    }

    { // synch up the indexes
        bool found1 = false;
        Map<string, string>::iterator it = nodeIdMatchIndex.find(node_name);
        for (; it != nodeIdMatchIndex.end(); it++) {
            string matchKey = (*it).second;
            nodeIdMatchIndex.erase(it);
            found1 = true;
            // remove from match
            Map<string, match>::iterator ittt = dataObjectIdMatch.find(matchKey);
            if (ittt == dataObjectIdMatch.end()) {
                HAGGLE_ERR("Key: %s missing from match map\n", matchKey.c_str());
                break;
            }
            dataObjectIdMatch.erase(ittt);
            // remove any left over node entries in node index if it's empty now
            string dobj_id = dataObjectIdFromKey(matchKey);
            bool found2 = false;
            //Map<string, string>::iterator itt = dataObjectIdMatchIndex.find(dobj_id);
            Map<string, string>::iterator itt = dataObjectIdMatchIndex.begin();
            for (; itt != dataObjectIdMatchIndex.end(); itt++) {
                if ((*itt).second == matchKey) {
                    found2 = true;
                    dataObjectIdMatchIndex.erase(itt);
                    break;
                }
            }
            if (!found2) {
                HAGGLE_ERR("Did not find data object: %s in object index\n", dobj_id.c_str());
                errorCount++;
            }
        }
        if (!found1) {
            HAGGLE_DBG2("Did not find node: %s in node index\n", node_name.c_str());
        }
    }

    nodesDeleted++;
}

void
ReplicationManagerObjectState::cacheDataObject(DataObjectRef dObj)
{
    string dobj_id = DataObject::idString(dObj);
    if (isCachedDataObject(dobj_id)) {
        HAGGLE_ERR("Data Object already cached!\n");
        errorCount++;
        return;
    }

    dataObjectsInserted++;
    cachedDataObjects.insert(make_pair(dobj_id, dObj));
}

bool
ReplicationManagerObjectState::isCachedDataObject(string dobj_id)
{
    Map<string, DataObjectRef>::iterator it = cachedDataObjects.find(dobj_id);
    if (it != cachedDataObjects.end()) {
        return true;
    }
    return false;
}



void
ReplicationManagerObjectState::unCacheDataObject(string dobj_id)
{
    {
        Map<string, DataObjectRef>::iterator it = cachedDataObjects.find(dobj_id);
        if (it == cachedDataObjects.end()) {
            HAGGLE_ERR("Data Object %s is not cached.\n", dobj_id.c_str());
            errorCount++;
            return;
        }

        cachedDataObjects.erase(it);
    }

    { // synch up the indexes
        bool found1 = false;
        Map<string, string>::iterator it = dataObjectIdMatchIndex.find(dobj_id);
        for (; it != dataObjectIdMatchIndex.end(); it++) {
            string matchKey = (*it).second;
            dataObjectIdMatchIndex.erase(it);
            found1 = true;
            // remove from match
            Map<string, match>::iterator ittt = dataObjectIdMatch.find(matchKey);
            if (ittt == dataObjectIdMatch.end()) {
                HAGGLE_ERR("Key: %s missing from match map\n", matchKey.c_str());
                break;
            }
            dataObjectIdMatch.erase(ittt);
            // remove any left over node entries in node index if it's empty now
            string node_name = nodeIdFromKey(matchKey);
            bool found2 = false;
            Map<string, string>::iterator itt = nodeIdMatchIndex.find(node_name);
            for (; itt != nodeIdMatchIndex.end(); itt++) {
                if ((*itt).second == matchKey) {
                    found2 = true;
                    nodeIdMatchIndex.erase(itt);
                    break;
                }
            }
            if (!found2) {
                HAGGLE_ERR("Did not find node: %s in node index\n", node_name.c_str());
                errorCount++;
            }
        }
        if (!found1) {
            HAGGLE_DBG2("Did not find data object: %s in object index\n", dobj_id.c_str());
        }
    }

    dataObjectsDeleted++;
}


void
ReplicationManagerObjectState::setMatchStrength(string dobj_id, string node_name, int ratio, int threshold)
{
    string key = getKey(dobj_id, node_name);

    bool addToIndex = true;
    Map<string, match>::iterator it = dataObjectIdMatch.find(key);
    if (it != dataObjectIdMatch.end()) {
        // we do this because mutliple applications for the same node_name may match,
        // only keep the best one we've seen (greedy)
        int new_match = ratio - threshold;
        match_t m = (*it).second;
        int old_match = m.ratio - m.threshold;

        if (new_match <= old_match) {
            HAGGLE_DBG2("Ignoring new match for dobj: %s, and node: %s, %d - %d <= %d - %d \n", dobj_id.c_str(), node_name.c_str(), ratio, threshold, m.ratio, m.threshold);
            return;
        }
        
        HAGGLE_DBG2("Overriding existing match for dobj: %s, and node: %s\n", dobj_id.c_str(), node_name.c_str());
        dataObjectIdMatch.erase(it);
        addToIndex = false;
     }
 
     match m;
     m.ratio = ratio;
     m.threshold = threshold;
 
     dataObjectIdMatch.insert(make_pair(key, m));
 
     // store the key in a reverse index for easy removal
    if (addToIndex) {
        dataObjectIdMatchIndex.insert(make_pair(dobj_id, key));
        nodeIdMatchIndex.insert(make_pair(node_name, key));
     }
 
    HAGGLE_DBG2("dobj: %s matched %s, with ratio %d, threshold: %d\n", dobj_id.c_str(), node_name.c_str(), ratio, threshold);
}

bool
ReplicationManagerObjectState::getMatchStrength(string dobj_id, string node_name, int *o_ratio, int *o_threshold)
{
    string key = getKey(dobj_id, node_name);

    if (!o_ratio) {
        HAGGLE_ERR("NULL ratio ptr.\n");
        errorCount++;
        return false;
    }

    if (!o_threshold) {
        HAGGLE_ERR("NULL threshold ptr.\n");
        errorCount++;
        return false;
    }

    // NOTE: data objects with 0 overlap in attributes will not
    // show up in the dataObjectIdMatch data structure
    int ratio = 0;
    int threshold = 0;
 
    Map<string, match>::iterator it = dataObjectIdMatch.find(key);
    if (it != dataObjectIdMatch.end()) {
        match m = (*it).second;
        ratio = m.ratio;
        threshold = m.threshold;
    }

    *o_ratio = ratio;
    *o_threshold = threshold;

    return true;
}

bool
ReplicationManagerObjectState::runSelfTest()
{
    bool passed = true;
    HAGGLE_DBG("Running self tests...\n");
    // test 1
    if (!runSelfTest1()) {
        HAGGLE_ERR("Self test #1 FAILED.\n");
        passed = false;
    }
    
    return passed;
}

bool
ReplicationManagerObjectState::runSelfTest1()
{
    ReplicationManagerObjectState *objectState = new ReplicationManagerObjectState();

    bool status = true;

    string d1_name = string(tmpnam(NULL));
    FILE *f1 = fopen(d1_name.c_str(), "w");
    fputs ("file1", f1);
    // NOTE: we add t1 as a hack to work around uninit error in DataObject::create
    DataObjectRef d1 = DataObjectRef(DataObject::create(d1_name, "t1"));

    string d1_id = DataObject::idString(d1);

    objectState->cacheDataObject(d1);

    string d2_name = string(tmpnam(NULL));
    FILE *f2 = fopen(d2_name.c_str(), "w");
    fputs ("file1", f2);
    DataObjectRef d2 = DataObjectRef(DataObject::create(d2_name, "t2"));

    string d2_id = DataObject::idString(d2);
    int obj_count = 0;

    if (!objectState->isCachedDataObject(d1_id)) {
        HAGGLE_ERR("Data object 1 is not cached\n");
        status = false;
        goto DONE;
    }

    objectState->cacheDataObject(d2);

    if (!objectState->isCachedDataObject(d2_id)) {
        HAGGLE_ERR("Data object 2 is not cached\n");
        status = false;
        goto DONE;
    }

    
    for (CachedDataObjectIterator dobjs = objectState->getObjectIterator(); dobjs.has_next(); dobjs.next()) {
        obj_count++;
    }

    if (2 != obj_count) {
        HAGGLE_ERR("Wrong count in iterator (got %d != expected %d)\n", obj_count, 2);
        status = false;
        goto DONE;
    }

    {
        NodeRef n1 = NodeRef(Node::create());
        objectState->cacheNode(n1);
        int node_count = 0;

        for (CachedNodeIterator nodes = objectState->getNodeIterator(); nodes.has_next(); nodes.next()) {
            node_count++;
        }
        if (1 != node_count) {
            HAGGLE_ERR("Wrong count in node iterator (got %d != expected %d)\n", node_count, 1);    
            status = false;
            goto DONE;
        }

        string node_name = Node::nameString(n1);
        objectState->setMatchStrength(d1_id, node_name, 1, 1);

        string test_key = getKey(d1_id, node_name);
        string got_node_name = nodeIdFromKey(test_key);
        if (got_node_name != node_name) {
            HAGGLE_ERR("Computed node id incorrectly (got: %s, expected: %s)\n", got_node_name.c_str(), node_name.c_str());
            status = false;
            goto DONE;
        }

        string got_dobj_id = dataObjectIdFromKey(test_key);
        if (got_dobj_id != d1_id) {
            HAGGLE_ERR("Computed dobj id incorrectly (got: %s, expected: %s)\n", got_dobj_id.c_str(), d1_id.c_str());
            status = false;
            goto DONE;
        }

        if (2 != objectState->getMatchStrength(d1_id, node_name)) {
            HAGGLE_ERR("Could not get proper match strength\n");
            status = false;
            goto DONE;
        }

        int nbr, total;
        objectState->getEncodingCounts(d1_id, node_name, &nbr, &total);

        if (nbr != 0 || total != 0) {
            HAGGLE_ERR("Invalid counts 1\n");
            status = false;
            goto DONE;
        }

        objectState->updateSendCountersForSent(d1_id, node_name);
        objectState->updateSendCountersForSuccess(d1_id, node_name);

        objectState->getEncodingCounts(d1_id, node_name, &nbr, &total);

        if (nbr != 1 || total != 1) {
            HAGGLE_ERR("Invalid counts 2 (nbr got: %d, expected 1, total got: %d, expected: 1\n", nbr, total);
            status = false;
            goto DONE;
        }

        objectState->updateSendCountersForSent(d1_id, node_name);
        objectState->updateSendCountersForFailure(d1_id, node_name);

        objectState->getEncodingCounts(d1_id, node_name, &nbr, &total);

        if (nbr != 1 || total != 1) {
            HAGGLE_ERR("Invalid counts 3 (nbr got: %d, expected 1, total got: %d, expected: 1\n", nbr, total);
            status = false;
            goto DONE;
        }

        objectState->updateSendCountersForSent(d1_id, node_name);
        objectState->updateSendCountersForReject(d1_id, node_name);

        objectState->getEncodingCounts(d1_id, node_name, &nbr, &total);

        if (nbr != 1 || total != 1) {
            HAGGLE_ERR("Invalid counts 4 (nbr got: %d, expected 1, total got: %d, expected: 1\n", nbr, total);
            status = false;
            goto DONE;
        }

        objectState->updateSendCountersForSent(d1_id, node_name);
        objectState->updateSendCountersForSent(d1_id, node_name);
        objectState->updateSendCountersForSent(d1_id, node_name);
        objectState->updateSendCountersForReject(d1_id, node_name);

        objectState->getEncodingCounts(d1_id, node_name, &nbr, &total);

        if (nbr != 1 || total != 1) {
            HAGGLE_ERR("Invalid counts 4 (nbr got: %d, expected 1, total got: %d, expected: 1\n", nbr, total);
            status = false;
            goto DONE;
        }

        objectState->setMatchStrength(d2_id, node_name, 1, 1);

        objectState->unCacheDataObject(d1_id);

        objectState->unCacheNode(node_name);
    }

    if (0 != objectState->getErrorCount()) {
        HAGGLE_ERR("Errors occurred\n");
        status = false;
        goto DONE;
    }

DONE:
    fclose(f1);
    fclose(f2);
    delete objectState;
    return status;
}

void
ReplicationManagerObjectState::printStats()
{
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Data Objects Inserted: %ld\n", dataObjectsInserted);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Nodes Inserted: %ld\n", nodesInserted);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Data Objects Deleted: %ld\n", dataObjectsDeleted);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Nodes Deleted: %ld\n", nodesDeleted);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Error Count: %ld\n", errorCount);
}

void
ReplicationManagerObjectState::printDebug()
{
    for (Map<string, DataObjectRef>::iterator it = cachedDataObjects.begin(); it != cachedDataObjects.end(); it++) {
        HAGGLE_ERR("RMDEBUG: cached data object: %s\n", (*it).first.c_str());
    }

    for (Map<string, NodeRef>::iterator it = cachedNodes.begin(); it != cachedNodes.end(); it++) {
        HAGGLE_ERR("RMDEBUG: cached nodes: %s\n", (*it).first.c_str());
    }

    for (Map<string, string>::iterator it = proxy_id_to_node_name.begin(); it != proxy_id_to_node_name.end(); it++) {
        HAGGLE_ERR("RMDEBUG: cached node id: %s, name: %s\n", (*it).first.c_str(), (*it).second.c_str());
    }

    for (Map<string, string>::iterator it = nodeIdMatchIndex.begin(); it != nodeIdMatchIndex.end(); it++) {
        HAGGLE_ERR("RMDEBUG: node index: %s, %s\n", (*it).first.c_str(), (*it).second.c_str());
    }

    for (Map<string, string>::iterator it = dataObjectIdMatchIndex.begin(); it != dataObjectIdMatchIndex.end(); it++) {
        HAGGLE_ERR("RMDEBUG: data object index: %s, %s\n", (*it).first.c_str(), (*it).second.c_str());
    }

    for (Map<string, match>::iterator it = dataObjectIdMatch.begin(); it != dataObjectIdMatch.end(); it++) {
        HAGGLE_ERR("RMDEBUG: data object match: %s\n", (*it).first.c_str());
    }
}
