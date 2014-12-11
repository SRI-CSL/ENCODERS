/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */


#include "ReplicationManagerUtility.h"
#include "ReplicationGlobalOptimizer.h"
#include "ReplicationGlobalOptimizerFactory.h"
#include "ReplicationKnapsackOptimizer.h"
#include "ReplicationKnapsackOptimizerFactory.h"
#include "ReplicationUtilityFunction.h"
#include "ReplicationUtilityFunctionFactory.h"

ReplicationManagerUtility::ReplicationManagerUtility(ReplicationManager *m) :
    ReplicationManagerAsynchronous(m, REPL_MANAGER_UTILITY_NAME),
    connectState(new ReplicationConnectState()),
    sendRateAverager(new ReplicationSendRateAverager()),
    nodeDurationAverager(new ReplicationNodeDurationAverager()),
    concurrentSend(new ReplicationConcurrentSend()),
    replicationOptimizer(new ReplicationOptimizer()),
    periodicEventType(0),
    periodicEvent(NULL),
    pollPeriodMs(REPL_MANAGER_DEFAULT_POLL_PERIOD_MS),
    enabled(false),
    replCooldownMs(500),
    lastReplication(Timeval::now()),
    dataObjectsInserted(0),
    dataObjectsDeleted(0),
    errorCount(0),
    runSelfTest(true)
#define __CLASS__ ReplicationManagerUtility
{

}

ReplicationManagerUtility::~ReplicationManagerUtility() 
{
    Event::unregisterType(periodicEventType);

    if (periodicEvent) {
        if (periodicEvent->isScheduled()) {
           periodicEvent->setAutoDelete(true);   
        }
        else {
            delete periodicEvent;
        }
    }

    if (connectState) {
        delete connectState;
    }

    if (sendRateAverager) {
        delete sendRateAverager;
    }

    if (nodeDurationAverager) {
        delete nodeDurationAverager;
    }

    if (concurrentSend) {
        delete concurrentSend;
    }

    if (replicationOptimizer) {
        delete replicationOptimizer;
    }

    for (ext_send_map_t::iterator it = ext_send_map.begin(); it != ext_send_map.end(); it++) {
        ext_send_map.erase(it);         
    }
}

void 
ReplicationManagerUtility::onPeriodicEvent(Event *e)
{
   firePeriodic(); 
}

void
ReplicationManagerUtility::_onConfig(const Metadata& m)
{
    const char *param;

    param = m.getParameter("enabled");
    if (param) {
        if (0 == strcmp("true", param)) {
           enabled = true;
        }
    }
    getKernel()->setRMEnabled(enabled);
 
    // load knapsack

    param = m.getParameter("knapsack_optimizer");

    ReplicationKnapsackOptimizer *ksOptimizer = NULL;

    if (!param) {
        HAGGLE_DBG("No knapsack optimizer specified, using default.\n");
        ksOptimizer = ReplicationKnapsackOptimizerFactory::getNewKnapsackOptimizer(getManager()->getKernel());
    } 
    else {
        ksOptimizer = ReplicationKnapsackOptimizerFactory::getNewKnapsackOptimizer(getManager()->getKernel(), param);
    }

    if (!ksOptimizer) {
        HAGGLE_ERR("Could not initialize knapsack optimizer.\n");
        return;
    }

    const Metadata *dm = m.getMetadata(param);
    if (dm) {
        ksOptimizer->onConfig(*dm);
    }

    replicationOptimizer->setKnapsackOptimizer(ksOptimizer);

    // load global optimizer 

    ReplicationGlobalOptimizer *globalOptimizer = NULL;

    param = m.getParameter("global_optimizer");
    if (!param) {
        HAGGLE_DBG("No optimizer specified, using default.\n");
        globalOptimizer = ReplicationGlobalOptimizerFactory::getNewGlobalOptimizer(getManager()->getKernel());
    } 
    else {
        globalOptimizer = ReplicationGlobalOptimizerFactory::getNewGlobalOptimizer(getManager()->getKernel(), param);
    }

    replicationOptimizer->setGlobalOptimizer(globalOptimizer);

    if (!globalOptimizer) {
        HAGGLE_ERR("Could not initialize global optimizer.\n");
        return;
    }

    dm = m.getMetadata(param);
    if (dm) {
        globalOptimizer->onConfig(*dm);
    }

    // load utility function

    ReplicationUtilityFunction *utilFunction = NULL;

    param = m.getParameter("utility_function");
    if (!param) {
        HAGGLE_DBG("No utility function specified, using default.\n");
        utilFunction = ReplicationUtilityFunctionFactory::getNewUtilityFunction(getManager(), globalOptimizer);
    } 
    else {
        utilFunction = ReplicationUtilityFunctionFactory::getNewUtilityFunction(getManager(), globalOptimizer, param);
    }

    if (!utilFunction) {
        HAGGLE_ERR("Could not initialize utility function.\n");
        return;
    }
    
    dm = m.getMetadata(param);
    if (dm) {
        utilFunction->onConfig(*dm);
    }

    replicationOptimizer->setUtilFunction(utilFunction);

    param = m.getParameter("forward_poll_period_ms");
    if (param) {
       pollPeriodMs = atoi(param);
    }

    if (pollPeriodMs < 0) {
        HAGGLE_ERR("Invalid poll period.\n");
        return;
    }

    int computePeriodMs = 0;

    param = m.getParameter("compute_period_ms");
    if (param) {
        computePeriodMs = atoi(param);
    }

    if (computePeriodMs  < 0) {
        HAGGLE_ERR("Invalid compute period.\n");
        return;
    }
    replicationOptimizer->setComputePeriodMs(computePeriodMs);

    param = m.getParameter("replication_cooldown_ms");
    if (param) {
        replCooldownMs = atoi(param);
    }

    param = m.getParameter("max_repl_tokens");
    if (param) {
        concurrentSend->setMaxTokens(atoi(param));
        if (concurrentSend->getMaxTokens() < 1) 
        	concurrentSend->setMaxTokens(1);
    }

    param = m.getParameter("token_per_node");
    if (param) {
        if (0 == strcmp("true", param)) {
           concurrentSend->setTokensPerNode(true);
        } else {
           concurrentSend->setTokensPerNode(false);
        }
    }
    getKernel()->setRMEnabled(enabled);

    param = m.getParameter("run_self_test");
    if (param) {
        if (0 == strcmp("false", param)) {
            runSelfTest = false;
        }
        else if (0 == strcmp("true", param)) {
            runSelfTest = true;
        }
    }

    periodicEventType = registerEventType("periodic event", onPeriodicEvent);
    periodicEvent = new Event(periodicEventType);
    periodicEvent->setAutoDelete(false);
    firePeriodic();

    bool pass = true;
    if (runSelfTest) {
        if (!connectState || !connectState->runSelfTest()) {
            HAGGLE_ERR("connect state self test failed.\n");
            errorCount++;
            pass = false;
        }

        if (!sendRateAverager || !sendRateAverager->runSelfTest()) {
            HAGGLE_ERR("send rate averager self test failed.\n");
            errorCount++;
            pass = false;
        }

        if (!nodeDurationAverager || !nodeDurationAverager->runSelfTest()) {
            HAGGLE_ERR("node duration averager self test failed.\n");
            errorCount++;
            pass = false;
        }

        if (!concurrentSend || !concurrentSend->runSelfTest()) {
            HAGGLE_ERR("concurrent send test failed.\n");
            errorCount++;
            pass = false;
        }
    }

    HAGGLE_DBG("Loaded utility replication manager is %s, self tests: %s, poll period (ms): %d\n", enabled ? "Enabled" : "Disabled", runSelfTest ? (pass ? "PASSED" : "FAILED") : "DISABLED", pollPeriodMs  );
}


int
ReplicationManagerUtility::getExtSendCost(string node_id)
{
    if (node_id == "") {
        errorCount++;
        HAGGLE_ERR("Bad node id.\n");
        return 0;
    }
    ext_send_map_t::iterator it = ext_send_map.find(node_id); 
    if (it == ext_send_map.end()) {
        HAGGLE_DBG("Node missing from ext_send_map; %s\n", node_id.c_str());
        return 0;
    }
    
    int total = 0;
    for (; it != ext_send_map.end() && (*it).first == node_id; it++) {
        total += (*it).second.second;
    }
    return total;
}

bool
ReplicationManagerUtility::inExtSendMap(string node_id, string dobj_id)
{
    if (node_id == "") {
        errorCount++;
        HAGGLE_ERR("Bad node id.\n");
        return false;
    }
    if (dobj_id == "") {
        errorCount++;
        HAGGLE_ERR("Bad data object id.\n");
        return false;
    }
    ext_send_map_t::iterator it = ext_send_map.find(node_id); 
    if (it == ext_send_map.end()) {
        return false;
    }
    for (; it != ext_send_map.end() && (*it).first == node_id; it++) {
        if ((*it).second.first == dobj_id) {
            return true;
        }
    }
    return false;
}

void
ReplicationManagerUtility::addToExtSendMap(string node_id, string dobj_id, int dobj_cost)
{
    if (inExtSendMap(node_id, dobj_id)) {
        return;
    }
    ext_send_map.insert(make_pair(node_id, make_pair(dobj_id, dobj_cost)));
}

void 
ReplicationManagerUtility::removeFromExtSendMap(string node_id, string dobj_id)
{
    if (!inExtSendMap(node_id, dobj_id)) {
        errorCount++;
        HAGGLE_ERR("Missing node, data object pair to remove.\n");
        return;
    }

    ext_send_map_t::iterator it = ext_send_map.find(node_id); 
    for (; it != ext_send_map.end() && (*it).first == node_id; it++) {
        if ((*it).second.first == dobj_id) {
            ext_send_map.erase(it);         
            return;
        }
    }
}

void
ReplicationManagerUtility::_notifySendDataObject(DataObjectRef dObj, NodeRefList nl)
{

    // exclude node descriptions
    if (!isInterestingDataObject(dObj)) {
        return;
    }

    string dobj_id = DataObject::idString(dObj);
    for (NodeRefList::iterator it = nl.begin(); it != nl.end(); it++) {
        NodeRef node = (*it);

        if (!node) {
            errorCount++;
            HAGGLE_ERR("Missing node.\n");
            continue;
        }

     	if (!isInterestingNode(node)) {
            continue;
    	}

        string node_id = Node::idString(node);

        if (!concurrentSend->tokenExists(node_id, dobj_id)) {
            // send was not triggered by the replication manager! add it and the cost
            // to the external list
            if (inExtSendMap(node_id, dobj_id)) {
                // CBMEN, HL - Double sending due to signature, this is okay.
            } else {
                int cost = replicationOptimizer->getCost(dObj, node_id);
                addToExtSendMap(node_id, dobj_id, cost);
            }

            // CBMEN, HL, Begin - Remove parent also if needed
            string parentId = kernel->getDataStore()->getIdOrParentId(dObj);
            if (parentId != dobj_id) {
                if (inExtSendMap(node_id, parentId)) {
                    removeFromExtSendMap(node_id, parentId);
                }
                if (concurrentSend->tokenExists(node_id, parentId)) {
                    concurrentSend->freeToken(node_id, parentId);
                }
            }
            // CBMEN, HL, End
        }
    }
}

void
ReplicationManagerUtility::_notifySendSuccess(DataObjectRef dObj, NodeRef node, unsigned long flags)
{

    if (!isInterestingNode(node)) {
        return;
    }

    if (!isInterestingDataObject(dObj)) {
        return;
    }

    string dobj_id = DataObject::idString(dObj);
    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);

    // CBMEN, HL, Begin - Remove parent also if needed
    string parentId = kernel->getDataStore()->getIdOrParentId(dObj);
    if (parentId != dobj_id) {
        if (inExtSendMap(node_id, parentId)) {
            removeFromExtSendMap(node_id, parentId);
        }
        if (concurrentSend->tokenExists(node_id, parentId)) {
            concurrentSend->freeToken(node_id, parentId);
        }
    }
    // CBMEN, HL, End
   
    if (!concurrentSend->tokenExists(node_id, dobj_id)) {
        // send was not triggered by the replication manager! 
        if (!inExtSendMap(node_id, dobj_id)) {
            errorCount++;
            HAGGLE_ERR("Send success without a send: %s, dobj: %s, flags: %lu\n", node_id.c_str(), dobj_id.c_str(), flags); 
        } else {
            removeFromExtSendMap(node_id, dobj_id);
        }
        return;
    }

    concurrentSend->freeToken(node_id, dobj_id);

    if (replicationOptimizer) {
        replicationOptimizer->notifySendSuccess(dObj, node);
    }

    replicateToNeighbor(node_id);
}

void
ReplicationManagerUtility::sendDataObjectToNode(NodeRef node, DataObjectRef dObj) 
{
    //dont run code if ReplicationManagerUtility is NOT enabled - JM
    if (!enabled) {
   	return;
    }
    if (!node) {
        errorCount++;
        HAGGLE_ERR("No node.\n");
        return;
    }

    if (!dObj) {
        errorCount++;
        HAGGLE_ERR("No data object.\n");
        return;
    }

    //send it
    getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj, node)); 
}

void
ReplicationManagerUtility::_notifySendFailure(DataObjectRef dObj, NodeRef node)
{
    if (!isInterestingNode(node)) {
        return;
    }

    if (!isInterestingDataObject(dObj)) {
        return;
    }

    string dobj_id = DataObject::idString(dObj);
    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);

    // CBMEN, HL, Begin - Remove parent also if needed
    string parentId = kernel->getDataStore()->getIdOrParentId(dObj);
    if (parentId != dobj_id) {
        if (inExtSendMap(node_id, parentId)) {
            removeFromExtSendMap(node_id, parentId);
        }
        if (concurrentSend->tokenExists(node_id, parentId)) {
            concurrentSend->freeToken(node_id, parentId);
        }
    }
    // CBMEN, HL, End

    if (!concurrentSend->tokenExists(node_id, dobj_id)) {
        // send was not triggered by the replication manager! 
        if (!inExtSendMap(node_id, dobj_id)) {
            errorCount++;
            HAGGLE_ERR("Send failure without a send: %s, dobj: %s\n", node_id.c_str(), dobj_id.c_str()); 
        } else {
            removeFromExtSendMap(node_id, dobj_id);
        }

        return;
    }

    concurrentSend->freeToken(node_id, dobj_id);

    if (replicationOptimizer) {
        replicationOptimizer->notifySendFailure(dObj, node);
    }

    //replicateToNeighbor(node_id);
}


bool
ReplicationManagerUtility::isInterestingNode(NodeRef node, bool availableCheck)
{
    //dont run code if ReplicationManagerUtility is NOT enabled - JM
    if (!enabled) {
   	return false;
    }
    if (!node) {
        return false;
    }

    if (node == kernel->getThisNode()) {
        HAGGLE_DBG("Received our own node description.\n");
        return false;
    }

    if (node->isLocalApplication())  {
        HAGGLE_DBG("Ignore application nodes.\n");
        return false;
    }

    if (availableCheck && !node->isAvailable()) {
        HAGGLE_DBG("Node is not available, ignoring.\n");
        return false;
    }

    if (availableCheck && !node->isNeighbor()) {
        HAGGLE_DBG("Node is not a neighbor, ignoring.\n");
        return false;
    }

    if (node->getType() != Node::TYPE_PEER) {
        HAGGLE_DBG("Skipping non peer node description\n");
        return false;
    }
    return true;
}

bool
ReplicationManagerUtility::isInterestingDataObject(DataObjectRef dObj)
{
    //dont run code if ReplicationManagerUtility is NOT enabled - JM
    if (!enabled) {
   	return false;
    }
    if (!dObj) {
        return false;
    }
    if (dObj->isNodeDescription()) {
        return false;
    }
    return true;
}

void
ReplicationManagerUtility::_notifyUpdatedContact(NodeRef node)
{
    if (!isInterestingNode(node)) {
        return;
    }

    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);

    if (replicationOptimizer) {
        replicationOptimizer->notifyUpdatedContact(node);
    }

    HAGGLE_LOG("Node %s is seen\n", node_id.c_str());

    replicateToNeighbor(node_id);
}


//New node seen
void
ReplicationManagerUtility::_notifyNewContact(NodeRef node)
{
    if (!isInterestingNode(node)) {
        return;
    }

    string node_id = Node::idString(node);

    if (replicationOptimizer) {
        replicationOptimizer->notifyNewContact(node);
    }

    replicateToNeighbor(node_id);
    HAGGLE_LOG("Node %s is seen\n", node_id.c_str());
}

void
ReplicationManagerUtility::_notifyDownContact(NodeRef node)
{
/*
    if (!isInterestingNode(node, false)) {
        return;
    }
*/
    //dont run code if ReplicationManagerUtility is NOT enabled - JM
    if (!enabled) {
   	return;
    }
    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);

    connectState->notifyDownContact(node_id);

    if (replicationOptimizer) {
        replicationOptimizer->notifyDownContact(node);
    }
}

void
ReplicationManagerUtility::_notifyInsertDataObject(DataObjectRef dObj)
{
    if (!isInterestingDataObject(dObj)) {
        return;
    }

    if (replicationOptimizer) {
        replicationOptimizer->notifyInsertion(dObj);
    }

    dataObjectsInserted++;

    string do_id = DataObject::idString(dObj); 
    replicateAllNeighbors();
}

void
ReplicationManagerUtility::_notifyNewNodeDescription(NodeRef node)
{
    if (!isInterestingNode(node)) {
        return;
    }

    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);

    //JM
    Timeval nodeCreateTime = node->getNodeDescriptionCreateTime();

    DataObjectRef dObj = node->getDataObject();
    InterfaceRef recvIface = NULL;
    const char *avg_bytes_per_sec_s, *avg_connect_millis_s;
    char *endptr;
    unsigned long avg_bytes_per_sec, avg_connect_millis;

    if (!dObj) {
        HAGGLE_ERR("Could not get underlying data object for node: %s\n", node_name.c_str());
        errorCount++;
        return;
    }

    node.lock();
    dObj.lock();
    Metadata *nm = dObj->getMetadata();
    
    if (!nm) {
        HAGGLE_ERR("No meta data at all for node: %s\n", node_name.c_str());
        errorCount++;
        goto err;
    }

    nm = nm->getMetadata("Node");

    if (!nm) {
        HAGGLE_ERR("No node metadata for node: %s\n", node_name.c_str());
        errorCount++;
        goto err;
    }

    recvIface = dObj->getRemoteInterface();
    if (!recvIface || !recvIface->isUp()) {
        HAGGLE_DBG2("Did not receive node description for %s from the neighbor, ignoring!\n", node_name.c_str());
        goto err;
    }

    if (!node->hasInterface(recvIface)) {
        HAGGLE_DBG2("Did not receive node description for %s from the neighbor (does not have interface), ignoring!\n", node_name.c_str());
        goto err;
    }

    avg_bytes_per_sec_s = nm->getParameter(REPL_MANAGER_AVG_BYTES_PER_SEC);
    if (!avg_bytes_per_sec_s) {
        HAGGLE_ERR("No bytes per sec for node: %s, metadata: %s\n", node_name.c_str(), dObj->getMetadata()->getContent().c_str());
        errorCount++;
        goto err;
    }

    avg_connect_millis_s = nm->getParameter(REPL_MANAGER_AVG_CONNECT_MILLIS);
    if (!avg_connect_millis_s) {
        HAGGLE_ERR("No bytes per sec for node: %s, metadata: %s\n", node_name.c_str(), dObj->getMetadata()->getContent().c_str());
        errorCount++;
        goto err;
    }

    node.unlock();
    dObj.unlock();

    endptr = NULL;
    avg_bytes_per_sec = (unsigned long) strtoul(avg_bytes_per_sec_s, &endptr, 10);

    endptr = NULL;
    avg_connect_millis = (unsigned long) strtoul(avg_connect_millis_s, &endptr, 10);

    connectState->update(node_id, avg_bytes_per_sec, avg_connect_millis, nodeCreateTime);

    replicateToNeighbor(node_id);

    HAGGLE_DBG2("Got new node description, avg bytes per sec: %lu, avg connect millis: %lu\n", avg_bytes_per_sec, avg_connect_millis);
    return;

err:
    dObj.unlock();
    node.unlock();
    return;
}

void
ReplicationManagerUtility::_notifyDeleteDataObject(DataObjectRef dObj)
{
    if (!isInterestingDataObject(dObj)) {
        return;
    }

    dataObjectsDeleted++;

    if (replicationOptimizer) {
        replicationOptimizer->notifyDelete(dObj);
    }
}

void
ReplicationManagerUtility::_notifySendStats(NodeRef node, DataObjectRef dObj, long send_delay, long send_bytes)
{
    if (!isInterestingNode(node)) {
        return;
    }

    if (!isInterestingDataObject(dObj)) {
        return;
    }

    string node_id = Node::idString(node);
    string dobj_id = DataObject::idString(dObj);

    sendRateAverager->update(send_delay, send_bytes);
}

void
ReplicationManagerUtility::_notifyNodeStats(NodeRef node, Timeval duration)
{
    if (!isInterestingNode(node, false)) {
        return;
    }

    string node_id = Node::idString(node);

    nodeDurationAverager->update(duration);
}

void
ReplicationManagerUtility::_replicateAll()
{
    replicateAllNeighbors();
}

void
ReplicationManagerUtility::_sendNodeDescription(DataObjectRef dObj, NodeRefList n)
{
    //dont run code if ReplicationManagerUtility is NOT enabled - JM
    if (!enabled) {
   	return;
    }
    if (!dObj) {
        HAGGLE_ERR("Missing data object.\n");
        errorCount++;
        return;
    }

    //if Replication
    Metadata *nm = dObj->getMetadata();
    nm = nm->getMetadata("Node");
    if (!nm) {
        HAGGLE_ERR("No node metadata\n");
        errorCount++;
        return;
    }

    long avgBps = sendRateAverager->getAverageBytesPerSec();

    long avgConnectMillis = nodeDurationAverager->getAverageConnectMillis(); 
    dObj.lock();
    nm->setParameter(REPL_MANAGER_AVG_BYTES_PER_SEC, avgBps);
    nm->setParameter(REPL_MANAGER_AVG_CONNECT_MILLIS, avgConnectMillis);
    dObj.unlock();
    getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObj,n ));  
}

void
ReplicationManagerUtility::_handlePeriodic()
{
    //dont run code if ReplicationManagerUtility is NOT enabled - JM
    if (!enabled) {
   	return;
    }
    if (replicationOptimizer) {
        replicationOptimizer->computeUtilities();
    }

    if (!periodicEvent->isScheduled() &&
        pollPeriodMs > 0) {
        periodicEvent->setTimeout(pollPeriodMs/(double)1000);
        kernel->addEvent(periodicEvent);
    }
}


void
ReplicationManagerUtility::onExit()
{
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Error Count: %ld\n", errorCount);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Data Objects Inserted: %ld\n", dataObjectsInserted);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Data Objects Deleted: %ld\n", dataObjectsDeleted);
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Connection Duration Number Connections: %ld\n", nodeDurationAverager->getNumConnections());
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Avg Connection Duration (ms): %ld\n", nodeDurationAverager->getAverageConnectMillis());
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Send Avg Bytes Per Sec (bps): %ld\n", sendRateAverager->getAverageBytesPerSec());
    HAGGLE_STAT("Summary Statistics - ReplicationManager Statistics - Send Connections: %ld\n", sendRateAverager->getTotalDataPoints());
}

void 
ReplicationManagerUtility::replicateToNeighbor(string node_id)
{
    if ((lastReplication.getTimeAsMilliSecondsDouble() + replCooldownMs) > (Timeval::now()).getTimeAsMilliSecondsDouble() ) {
         HAGGLE_DBG("coold down time not exceeded %f < %f\n", lastReplication.getTimeAsMilliSecondsDouble() + replCooldownMs, (Timeval::now()).getTimeAsMilliSecondsDouble());
       return;
    }
    bool hadToken = false;
    while (concurrentSend->tokenAvailable(node_id)) {
        hadToken = true;
        List<string> blacklist_dobjs;
        concurrentSend->getReservedDataObjectsForNode(node_id, blacklist_dobjs);
        int bytes_remaining = connectState->getEstimatedBytesLeft(node_id);
        if (bytes_remaining > 0) {
            bytes_remaining = bytes_remaining - (int) getExtSendCost(node_id);
        }
        // remove bytes for entries in send map
        DataObjectRef dobj = NULL;
        NodeRef node = NULL;
        if (!replicationOptimizer->getNextDataObject(node_id, blacklist_dobjs, bytes_remaining, dobj, node)) {
            return;
        }
	if (!node || !dobj) { return; }
        string next_dobj_id = DataObject::idString(dobj);
        concurrentSend->reserveToken(node_id, next_dobj_id);
        sendDataObjectToNode(node, dobj);
    }
    if (!hadToken) {
        HAGGLE_DBG("No tokens available for node: %s.\n", node_id.c_str());
    } else {
       lastReplication = Timeval::now();
    }

}

void
ReplicationManagerUtility::replicateAllNeighbors()
{
    NodeRefList nbrs; 
    getKernel()->getNodeStore()->retrieveNeighbors(nbrs);
    for (NodeRefList::iterator it  = nbrs.begin(); it != nbrs.end(); it++) {
        // CBMEN, HL - Don't replicate to nodes that are not interesting
        if (!isInterestingNode(*it)) {
            continue;
        }
        string nbr_id = Node::idString(*it);
        replicateToNeighbor(nbr_id);
    }
}
