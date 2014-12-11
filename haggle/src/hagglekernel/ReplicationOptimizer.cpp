/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#include "ReplicationOptimizer.h"

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


ReplicationOptimizer::ReplicationOptimizer() :
    knapsack(NULL),
    utilFunction(NULL),
    globalOptimizer(NULL),
    computePeriodMs(10000),
    errorCount(0)
{

}

ReplicationOptimizer::~ReplicationOptimizer()
{
    if (knapsack) {
        delete knapsack;
    }

    if (utilFunction) {
        delete utilFunction;
    }

    if (globalOptimizer) {
        delete globalOptimizer;
    }

    // remove the utility cache entries

    for (meta_map_t::iterator it = meta.begin(); it != meta.end(); it++) {
        ReplicationDataObjectUtilityMetadataRef do_info = (*it).second;
        meta.erase(it);
        if (!do_info) {
            errorCount++;
            HAGGLE_ERR("NULL meta.\n");
            continue;
        }
    }

    for (dobj_id_map_t::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
        dobjs.erase(it);
    }

    for (neighbor_node_id_map_t::iterator it = utils.begin(); it != utils.end(); it++) {
        utils.erase(it);
    }
}

string
ReplicationOptimizer::getKey(string dobj_id, string node_id)
{
    return dobj_id + string(":") + node_id;
}

void
ReplicationOptimizer::setComputePeriodMs(int _computePeriodMs)
{
    computePeriodMs = _computePeriodMs;
}

/**
 * Find the next best data object to replicate towards neighbor with id
 * `node_id`, given that we have an estimated `bytes_left` bytes left to
 *  send. If `bytes_left == 0` then we assume the bag size is infinite
 * and therefore send data objects with highest utility first.
 * If the computed bytes_left < 0 then the bag is full and no data objects
 * will be replicated.
 */
bool
ReplicationOptimizer::getNextDataObject(string node_id, List<string> &dobj_blacklist, int bytes_left, DataObjectRef& o_dobj, NodeRef& o_node)
{
    neighbor_node_id_map_t::iterator it = utils.find(node_id);
    if (it == utils.end()) {
        // no data objects for node
        if (dobjs.size() != 0) {
            errorCount++;
            HAGGLE_ERR("Neighbors map missing data objects.\n");
        }
        return false;
    }

    {
        node_map_t::iterator it = node_map.find(node_id);
        if (it == node_map.end()) {
            HAGGLE_ERR("Node missing from data structure.\n");
            return false;
        }
        o_node = (*it).second;
        if (!o_node) {
            HAGGLE_ERR("NULL node.\n");
            return false;
        }
    }
    // remove in-flight data objects from the bag size
    for (List<string>::iterator itt = dobj_blacklist.begin(); itt != dobj_blacklist.end(); itt++) {
        string dobj_id = (*itt);
        if (bytes_left != 0) {   //0 means unilmited
        	bytes_left = bytes_left - getCost(dobj_id, node_id);
		if (bytes_left == 0) { bytes_left = -1; }
 	}
    }

    if (bytes_left < 0) {
        // buffer already full
        return false;
    }

    // used when bytes_left == 0
    double best_utility = 0;
    int best_cost = 0;
    string best_dobj_id = "";

    List<ReplicationDataObjectUtilityMetadataRef> process_dos;
    List<ReplicationDataObjectUtilityMetadataRef> include_dos;
    List<ReplicationDataObjectUtilityMetadataRef> exclude_dos;

    for (;it != utils.end() && (*it).first == node_id; it++) {
        ReplicationDataObjectUtilityMetadataRef do_info = (*it).second;
        Timeval now = Timeval::now();
        if (!do_info) {
            errorCount++;
            HAGGLE_ERR("NULL DO in cache\n");
            continue;
        }
        string do_id = do_info->getId();
        DataObjectId_t id;
        DataObject::idStrToId(do_id, id);

        bool inBloomfilter = o_node->getBloomfilter()->has(id);
        if (inBloomfilter) {
            HAGGLE_DBG("Skipping, %s already in bloomfilter.\n", Node::nameString(o_node).c_str());
            continue;
        }

        // skip over data objects in the black list
        bool inBlacklist = false;
        for (List<string>::iterator itt = dobj_blacklist.begin(); itt != dobj_blacklist.end(); itt++) {
            if ((*itt) == do_id) {
                inBlacklist = true;
                break;
            }
        }
        if (inBlacklist) {
            continue;
        }
        // recompute the utility if need be
        double delta = now.getTimeAsMilliSecondsDouble() - do_info->getComputedTime().getTimeAsMilliSecondsDouble();
        if (delta > computePeriodMs && utilFunction) {
            double new_utility = utilFunction->compute(do_info->getId(), do_info->getNodeId());
            do_info->setUtility(new_utility, now);

        }
        // skip utilites beneeath threshold
        int cost = do_info->getCost();
        double util = do_info->getUtility();
        if (util >= globalOptimizer->getMinimumThreshold()) {
            process_dos.push_front(do_info);
            if (util > best_utility || (util == best_utility && cost < best_cost)) {
                best_utility = util;
                best_cost = cost;
                best_dobj_id = do_info->getId();
            }
        }
    }

    if (process_dos.size() == 0) {
        // nothing to replicate
        return false;
    }

    if (bytes_left == 0) {
        DO_map_t::iterator it = do_map.find(best_dobj_id);
        if (it == do_map.end()) {
            errorCount++;
            HAGGLE_ERR("Missing data object %s.\n", best_dobj_id.c_str());
            return false;
        }
        o_dobj = (*it).second;
        return true;
    }

    // call solver
    ReplicationKnapsackOptimizerResults results = knapsack->solve(&process_dos, &include_dos, &exclude_dos, bytes_left);
    if (results.getHadError()) {
        errorCount++;
        HAGGLE_ERR("error during knapsack.\n");
        return false;
    }

    if (include_dos.size() == 0) {
        HAGGLE_DBG("Nothing to replicate.\n");
        return false;
    }

    ReplicationDataObjectUtilityMetadataRef best = include_dos.front();
    best_dobj_id = best->getId();
    DO_map_t::iterator ittt = do_map.find(best_dobj_id);
    if (ittt == do_map.end()) {
        errorCount++;
        HAGGLE_ERR("Missing data object %s.\n", best_dobj_id.c_str());
        return false;
    }
    o_dobj = (*ittt).second;
    return true; //JM was false
}

void
ReplicationOptimizer::setKnapsackOptimizer(ReplicationKnapsackOptimizer *_ks)
{
    knapsack = _ks;
}

void
ReplicationOptimizer::setUtilFunction(ReplicationUtilityFunction *_utilFunction)
{
    utilFunction = _utilFunction;
}

void
ReplicationOptimizer::setGlobalOptimizer(ReplicationGlobalOptimizer *_globalOptimizer)
{
    globalOptimizer = _globalOptimizer;
}

int
ReplicationOptimizer::getCost(string dobj_id, string node_id)
{
    int dataLen = 0;
    {
        DO_map_t::iterator it = do_map.find(dobj_id);
        if (it == do_map.end()) {
            errorCount++;
            HAGGLE_ERR("Missing data object: %s\n", dobj_id.c_str());
            return 0;
        }

        DataObjectRef dObj = (*it).second;
        if (!dObj) {
            errorCount++;
            HAGGLE_ERR("NULL data object.\n");
            return 0;
        }

        dataLen = (int) dObj->getDataLen();
        // include metadata size
        Metadata *m = dObj->getMetadata();
        if (!m) {
            errorCount++;
            HAGGLE_ERR("Missing metadata.\n");
            return 0;
        }
        dataLen += m->getContent().size();
    }

    {
        node_map_t::iterator it = node_map.find(node_id);
        if (it == node_map.end()) {
            errorCount++;
            HAGGLE_ERR("Missing node: %s\n", node_id.c_str());
            return 0;
        }
    }

    return dataLen;
}

// CBMEN, HL, Begin
int
ReplicationOptimizer::getCost(DataObjectRef dObj, string node_id)
{
    int dataLen = 0;
    {
        dataLen = (int) dObj->getDataLen();
        // include metadata size
        Metadata *m = dObj->getMetadata();
        if (!m) {
            errorCount++;
            HAGGLE_ERR("Missing metadata.\n");
            return 0;
        }
        dataLen += m->getContent().size();
    }

    {
        node_map_t::iterator it = node_map.find(node_id);
        if (it == node_map.end()) {
            errorCount++;
            HAGGLE_ERR("Missing node: %s\n", node_id.c_str());
            return 0;
        }
    }

    return dataLen;
}
// CBMEN, HL, End

void
ReplicationOptimizer::notifyInsertion(DataObjectRef dObj)
{
    if (utilFunction) {
        utilFunction->notifyInsertion(dObj);
    }

    string dobj_id = DataObject::idString(dObj);
    {
        dobj_id_map_t::iterator it = dobjs.find(dobj_id);
        if (it != dobjs.end()) {
            errorCount++;
            HAGGLE_ERR("Tried to double insert data object (dobjs): %s\n", dobj_id.c_str());
            return;
        }
    }

    {
        DO_map_t::iterator it = do_map.find(dobj_id);
        if (it != do_map.end()) {
            errorCount++;
            HAGGLE_ERR("Tried to double insert data object (map): %s\n", dobj_id.c_str());
            return;
        }

        do_map.insert(make_pair(dobj_id, dObj));
    }

    for (node_map_t::iterator it = node_map.begin(); it != node_map.end(); it++) {
        string neighbor_id = (*it).first;
        double util = 0;
        if (utilFunction) {
            util = utilFunction->compute(dobj_id, neighbor_id);
        }
        Timeval now = Timeval::now();
        int cost = getCost(dObj, neighbor_id);
        ReplicationDataObjectUtilityMetadataRef do_info = new ReplicationDataObjectUtilityMetadata(dobj_id, neighbor_id, cost, util, now, now);
        meta.insert(make_pair(getKey(dobj_id, neighbor_id), ReplicationDataObjectUtilityMetadataRef(do_info)));
        dobjs.insert(make_pair(dobj_id, ReplicationDataObjectUtilityMetadataRef(do_info)));
        utils.insert(make_pair(neighbor_id, ReplicationDataObjectUtilityMetadataRef(do_info)));
    }
}


void
ReplicationOptimizer::notifyDelete(DataObjectRef dObj)
{
    if (utilFunction) {
        utilFunction->notifyDelete(dObj);
    }

    string dobj_id = DataObject::idString(dObj);

    {
        dobj_id_map_t::iterator it = dobjs.find(dobj_id);
        if (it == dobjs.end()) {
            errorCount++;
            HAGGLE_ERR("Data object missing: %s\n", dobj_id.c_str());
            return;
        }
        dobjs.erase(it);
    }

    dobj_id_map_t::iterator it = dobjs.find(dobj_id);
    if (it == dobjs.end()) {
        // no nodes?
        if (utils.size() != 0) {
            errorCount++;
            HAGGLE_ERR("Missing utility for <node, data object> pairs.\n");
        }
        return;
    }

    // go through the data object utility states and remove them from
    // all the data structures
    for (; (it != dobjs.end()) && ((*it).first == dobj_id); it++) {
        ReplicationDataObjectUtilityMetadataRef do_info((*it).second);
        if (!do_info) {
            errorCount++;
            HAGGLE_ERR("NULL metadata.\n");
            continue;
        }

        // CBMEN, HL, Begin
        meta_map_t::iterator mit = meta.find(getKey(do_info->getId(), do_info->getNodeId()));
        if (mit != meta.end()) {
            meta.erase(mit);
        }
        neighbor_node_id_map_t::iterator itt = utils.find(do_info->getNodeId());
        while ((itt != utils.end()) && ((*itt).first == do_info->getNodeId())) {
            while ((itt != utils.end()) && ((*itt).second.getObj() != do_info.getObj())) {
                itt++;
            }
            if (itt == utils.end()) {
                break;
            }
            if ((*itt).second.getObj() == do_info.getObj()) {
                utils.erase(itt);
                itt = utils.find(do_info->getNodeId());
            } else {
                break;
            }
        }

        // CBMEN, HL, End
    }

    // CBMEN, HL, Begin
    it = dobjs.find(dobj_id);
    while (it != dobjs.end()) {
        dobjs.erase(it);
        it = dobjs.find(dobj_id);
    }

    DO_map_t::iterator dit = do_map.find(dobj_id);
    if (dit != do_map.end()) {
        do_map.erase(dit);
    }

    // CBMEN, HL, End
}

void
ReplicationOptimizer::notifySendSuccess(DataObjectRef dObj, NodeRef node)
{
    if (utilFunction) {
        utilFunction->notifySendSuccess(dObj, node);
    }

    string dobj_id = DataObject::idString(dObj);
    string node_id = Node::idString(node);
    string key = getKey(dobj_id, node_id);

    meta_map_t::iterator it = meta.find(key);
    if (it == meta.end()) {
        errorCount++;
        HAGGLE_ERR("Missing entry.\n");
        return;
    }

    ReplicationDataObjectUtilityMetadataRef do_info = (*it).second;
    if (!do_info) {
        errorCount++;
        HAGGLE_ERR("Missing metadata.\n");
        return;
    }

    Timeval now = Timeval::now();
    double delta = now.getTimeAsMilliSecondsDouble() - do_info->getComputedTime().getTimeAsMilliSecondsDouble();

    if (delta > computePeriodMs && utilFunction) {
        double new_utility = utilFunction->compute(do_info->getId(), do_info->getNodeId());
        do_info->setUtility(new_utility, now);
    }
}

void
ReplicationOptimizer::notifySendFailure(DataObjectRef dObj, NodeRef node)
{
    if (utilFunction) {
        utilFunction->notifySendFailure(dObj, node);
    }

    string dobj_id = DataObject::idString(dObj);
    string node_id = Node::idString(node);
    string key = getKey(dobj_id, node_id);

    meta_map_t::iterator it = meta.find(key);
    if (it == meta.end()) {
        errorCount++;
        HAGGLE_ERR("Missing entry.\n");
        return;
    }

    ReplicationDataObjectUtilityMetadataRef do_info = (*it).second;
    if (!do_info) {
        errorCount++;
        HAGGLE_ERR("Missing metadata.\n");
        return;
    }

    Timeval now = Timeval::now();
    double delta = now.getTimeAsMilliSecondsDouble() - do_info->getComputedTime().getTimeAsMilliSecondsDouble();

    if (delta > computePeriodMs && utilFunction) {
        double new_utility = utilFunction->compute(do_info->getId(), do_info->getNodeId());
        do_info->setUtility(new_utility, now);
    }
}

void
ReplicationOptimizer::notifyUpdatedContact(NodeRef node)
{

    if (utilFunction) {
        utilFunction->notifyUpdatedContact(node);
    }

    string neighbor_id = Node::idString(node);

    {
        node_map_t::iterator it = node_map.find(neighbor_id);
        if (it == node_map.end()) {
            HAGGLE_ERR("Node not in data structure.\n");
        } else {
        node_map.erase(it); }
        node_map.insert(make_pair(neighbor_id, node));
    }

    for (dobj_id_map_t::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
        string dobj_id = (*it).first;
        ReplicationDataObjectUtilityMetadataRef oldMeta = (*it).second;
        if (!oldMeta) {
            errorCount++;
            HAGGLE_ERR("Meta does not exist.\n");
            continue;
        }

        Timeval now = Timeval::now();
        double delta = now.getTimeAsMilliSecondsDouble() - oldMeta->getComputedTime().getTimeAsMilliSecondsDouble();
        double util = 0;
        if (delta <= computePeriodMs) {
            continue;
        }
        if (utilFunction) {
            util = utilFunction->compute(dobj_id,neighbor_id);
        }
        oldMeta->setUtility(util, now);
    }
}

void
ReplicationOptimizer::notifyNewContact(NodeRef node)
{
    if (utilFunction) {
        utilFunction->notifyNewContact(node);
    }

    string neighbor_id = Node::idString(node);

    {
        node_map_t::iterator it = node_map.find(neighbor_id);
        if (it != node_map.end()) {
            HAGGLE_ERR("Node already in data structure.\n");
            return;
        }
        node_map.insert(make_pair(neighbor_id, node));
    }

    {
        neighbor_node_id_map_t::iterator it = utils.find(neighbor_id);
        if (it != utils.end()) {
            errorCount++;
            HAGGLE_ERR("Tried to double insert neighbor: %s\n", neighbor_id.c_str());
            return;
        }
    }

    for (DO_map_t::iterator it = do_map.begin(); it != do_map.end(); it++) {
        string dobj_id = (*it).first;
        double util = 0;
        if (utilFunction) {
            util = utilFunction->compute(dobj_id,neighbor_id);
        }
        Timeval now = Timeval::now();
        int cost = getCost(dobj_id, neighbor_id);
        ReplicationDataObjectUtilityMetadataRef do_info = new ReplicationDataObjectUtilityMetadata(dobj_id, neighbor_id, cost, util, now, now);
        meta.insert(make_pair(getKey(dobj_id, neighbor_id), ReplicationDataObjectUtilityMetadataRef(do_info)));
        dobjs.insert(make_pair(dobj_id, ReplicationDataObjectUtilityMetadataRef(do_info)));
        utils.insert(make_pair(neighbor_id, ReplicationDataObjectUtilityMetadataRef(do_info)));
    }
}

void
ReplicationOptimizer::notifyDownContact(NodeRef node)
{
    if (utilFunction) {
        utilFunction->notifyDownContact(node);
    }

    string neighbor_id = Node::idString(node);

    {
        node_map_t::iterator it = node_map.find(neighbor_id);
        if (it == node_map.end()) {
            HAGGLE_ERR("Node not in data structure.\n");
            return;
        }
        node_map.erase(it);
    }

    neighbor_node_id_map_t::iterator it = utils.find(neighbor_id);
    if (it == utils.end()) {
        // no data objects?
        if (dobjs.size() != 0) {
            errorCount++;
            HAGGLE_ERR("Missing utility for <node, data object> pairs.\n");
        }
        return;
    }

    for (; it != utils.end() && (*it).first == neighbor_id; it++) {
        ReplicationDataObjectUtilityMetadataRef do_info((*it).second);
        // CBMEN, HL, Begin
        meta_map_t::iterator mit = meta.find(getKey(do_info->getId(), do_info->getNodeId()));
        if (mit != meta.end()) {
            meta.erase(mit);
        }
        if (!do_info) {
            errorCount++;
            HAGGLE_ERR("NULL metadata.\n");
            continue;
        }
        dobj_id_map_t::iterator itt = dobjs.find(do_info->getId());
        List<string> toErase;
        for (; itt !=dobjs.end() && (*itt).first == do_info->getId(); itt++) {
            if ((*itt).second.getObj() == do_info.getObj()) {
                toErase.push_back((*itt).first);
            }
        }

        for (List<string>::iterator eit = toErase.begin(); eit != toErase.end(); eit++) {
            itt = dobjs.find(*eit);
            if (itt != dobjs.end()) {
                dobjs.erase(itt);
            }
        }

        // CBMEN, HL, End
    }

    it = utils.find(neighbor_id);
    while (it != utils.end()) {
        utils.erase(it);
        it = utils.find(neighbor_id);
    }

}

void
ReplicationOptimizer::computeUtilities()
{
    for (meta_map_t::iterator it = meta.begin(); it != meta.end(); it++) {
        ReplicationDataObjectUtilityMetadataRef do_info = (*it).second;
        if (!do_info) {
            errorCount++;
            HAGGLE_ERR("NULL DO in cache\n");
            continue;
        }
        Timeval now = Timeval::now();
        // was the utility recently computed?
        double delta = now.getTimeAsMilliSecondsDouble() - do_info->getComputedTime().getTimeAsMilliSecondsDouble();
        if (delta > computePeriodMs && utilFunction) {
            double new_utility = utilFunction->compute(do_info->getId(), do_info->getNodeId());
HAGGLE_DBG("do:node %s:%s has a utility value of %f\n", do_info->getId().c_str(), do_info->getNodeId().c_str());
            do_info->setUtility(new_utility, now);
        }
    }
}

bool
ReplicationOptimizer::runSelfTest()
{
    return true;
}
