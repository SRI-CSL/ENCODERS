/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include "CacheStrategyUtility.h"
#include "CacheKnapsackOptimizerFactory.h"
#include "CacheUtilityFunctionFactory.h"
#include "CacheGlobalOptimizerFactory.h"

// for stats data object maintenience
#include "CacheReplacementTotalOrder.h"

// for malloc memory reading stats
#include <malloc.h>

CacheStrategyUtility::CacheStrategyUtility(
    DataManager *m) :
        CacheStrategyAsynchronous(m, CACHE_STRAT_UTILITY_NAME),
        computePeriodMs(0),
        pollPeriodMs(0),
        max_capacity_kb(0),
        watermark_capacity_kb(0),
        purgeOnInsert(false),
        ksOptimizer(NULL),
        utilFunction(NULL),
        globalOptimizer(NULL),
        periodicPurgeEventType(0),
        periodicPurgeEvent(NULL),
        current_num_do(0),
        total_do_inserted(0),
        total_do_inserted_bytes(0),
        total_do_evicted(0),
        total_do_hard_evicted(0),
        total_do_hard_evicted_bytes(0),
        total_do_evicted_bytes(0),
        current_size(0),
// JM: START memory DB threshold
        db_size_threshold(99999),
        total_db_evicted(0),
        allow_db_purging(false),
        self_test(false),
// JM: END memory DB threshold
        manage_only_remote_files(true),
        manage_locally_sent_files(true),
        publish_stats_dataobject(false),
        stats_replacement_strat(NULL),
        keep_in_bloomfilter(false),
        handle_zero_size(false),
// SW: START: remove from local bloomfilter.
        bloomfilter_remove_delay_ms(0),
        bloomfilterRemoveDelayEventType(0),
//SW: END: remove from local bloomfilter.
        current_dupe_do_recv(0),
        current_drop_on_insert(0)
#define __CLASS__ CacheStrategyUtility
{
}

CacheStrategyUtility::~CacheStrategyUtility() 
{
    if (stats_replacement_strat) {
        delete stats_replacement_strat;
    }

// SW: START: remove from local bloomfilter.
    if (bloomfilterRemoveDelayEventType > 0) {
        Event::unregisterType(bloomfilterRemoveDelayEventType);
    }
//SW: END: remove from local bloomfilter.
    Event::unregisterType(periodicPurgeEventType);

    if (periodicPurgeEvent) {
        if (periodicPurgeEvent->isScheduled()) {
            periodicPurgeEvent->setAutoDelete(true);   
        }
        else {
            delete periodicPurgeEvent;
        }
    }

    if (ksOptimizer) {
        delete ksOptimizer;
    }

    if (utilFunction) {
        delete utilFunction;
    }

    if (globalOptimizer) {
        delete globalOptimizer;
    }

    // clear out cache
    while (!utilMetadata.empty()) {
        do_util_metadata_t::iterator it = utilMetadata.begin();
        DataObjectUtilityMetadata *do_info = (*it).second;
        utilMetadata.erase(it);
        if (!do_info) {
            HAGGLE_ERR("NULL data object\n");
            continue;
        }
        delete do_info;
    }

}

/**
* The following configuration file options are supported:
* 
* global_optimizer -
*
* utility_function -
*
* max_capacity_kb - 
*
* watermark_capacity_kb -
* 
* compute_period_ms -
* 
* purge_poll_period_ms -
*
* purge_on_insert - 
* 
* manage_only_remote_files -
*
* manage_locally_sent_files - 
* 
* publish_stats_dataobject -
* 
* keep_in_bloomfilter -
*
* handle_zero_size -
*
* manage_db_purging = <bool>   This option, if true, will allow internal DB memory management.  
*
* db_size_threshold = <long long>  This option sets the watermark for
* internal DB DO entries.   When this value is exceeded, the database
* will purge enries until the size is within this watermark. 
*
* self_benchmark_test = <bool> This option, if true, sets up the self
* benchmark test.  Normal operation can not occur when this is set to
* true.  
* 
* bloomfilter_remove_delay_ms -
*
*/

void
CacheStrategyUtility::_onConfig(
    const Metadata& m)
{
    const char *param;
    const Metadata *dm;

    if (m.getName() != getName()) {
        HAGGLE_ERR("Wrong config.\n");
        return;
    }

    // load knapsack optimizer

    param = m.getParameter("knapsack_optimizer");

    if (!param) {
        HAGGLE_DBG("No knapsack optimizer specified, using default.\n");
        ksOptimizer = CacheKnapsackOptimizerFactory::getNewKnapsackOptimizer(getManager()->getKernel());
    } 
    else {
        ksOptimizer = CacheKnapsackOptimizerFactory::getNewKnapsackOptimizer(getManager()->getKernel(), param);
    }

    if (!ksOptimizer) {
        HAGGLE_ERR("Could not initialize knapsack optimizer.\n");
        return;
    }

    dm = m.getMetadata(param);
    if (dm) {
        ksOptimizer->onConfig(*dm);
    }

    // load global optimizer 

    param = m.getParameter("global_optimizer");
    if (!param) {
        HAGGLE_DBG("No  specified, using default.\n");
        globalOptimizer = CacheGlobalOptimizerFactory::getNewGlobalOptimizer(getManager()->getKernel());
    } 
    else {
        globalOptimizer = CacheGlobalOptimizerFactory::getNewGlobalOptimizer(getManager()->getKernel(), param);
    }

    if (!globalOptimizer) {
        HAGGLE_ERR("Could not initialize global optimizer.\n");
        return;
    }

    dm = m.getMetadata(param);
    if (dm) {
        globalOptimizer->onConfig(*dm);
    }

    // load utility function

    param = m.getParameter("utility_function");
    if (!param) {
        HAGGLE_DBG("No utility function specified, using default.\n");
        utilFunction = CacheUtilityFunctionFactory::getNewUtilityFunction(getManager(), globalOptimizer);
    } 
    else {
        utilFunction = CacheUtilityFunctionFactory::getNewUtilityFunction(getManager(), globalOptimizer, param);
    }

    if (!utilFunction) {
        HAGGLE_ERR("Could not initialize utility function.\n");
        return;
    }

    dm = m.getMetadata(param);
    if (dm) {
        utilFunction->onConfig(*dm);
    }

    param = m.getParameter("max_capacity_kb");
    if (!param) {
        HAGGLE_ERR("No maximum capacity specified\n");
        return;
    }

    max_capacity_kb = atoi(param);

    if (max_capacity_kb < 0) {
        HAGGLE_ERR("Invalid max capacity.\n");
        return;
    }

    param = m.getParameter("watermark_capacity_kb");
    if (!param) {
        HAGGLE_ERR("No watermark capacity specified\n");
        return;
    }

    watermark_capacity_kb = atoi(param);

    if ((watermark_capacity_kb < 0) || (watermark_capacity_kb > max_capacity_kb)) {
        HAGGLE_ERR("Invalid watermark capacity.\n");
        return;
    }

    param = m.getParameter("compute_period_ms");
    if (param) {
        computePeriodMs = atoi(param);
    }

    if (computePeriodMs  < 0) {
        HAGGLE_ERR("Invalid compute period.\n");
        return;
    }

    param = m.getParameter("purge_poll_period_ms");
    if (param) {
        pollPeriodMs = atoi(param);
    }

    if (pollPeriodMs < 0) {
        HAGGLE_ERR("Invalid poll period.\n");
        return;
    }

    param = m.getParameter("purge_on_insert");
    if (param) {
        purgeOnInsert = (0 == strcmp("true", param));
    }

    param = m.getParameter("manage_only_remote_files");
    if (param) {
        manage_only_remote_files = (0 == strcmp("true", param));
    }

    param = m.getParameter("manage_locally_sent_files");
    if (param) {
        manage_locally_sent_files = (0 == strcmp("true", param));
    }

    param = m.getParameter("publish_stats_dataobject");
    if (param) {
        publish_stats_dataobject = (0 == strcmp("true", param));
    }

    param = m.getParameter("keep_in_bloomfilter");
    if (param) {
        keep_in_bloomfilter = (0 == strcmp("true", param));
    }

    param = m.getParameter("handle_zero_size");
    if (param) {
        handle_zero_size = (0 == strcmp("true", param));
    }

//JM: START DB 
    param = m.getParameter("manage_db_purging");
    if (param) {
        allow_db_purging = (0 == strcmp("true", param));
    }

    param = m.getParameter("db_size_threshold");
    if (param) {
        db_size_threshold = atoll(param);
    }

    param = m.getParameter("self_benchmark_test");
    if (param) {
        self_test = (0 == strcmp("true", param));
    }
//JM: END
    param = m.getParameter("bloomfilter_remove_delay_ms");
    if (param) {
        bloomfilter_remove_delay_ms = atoi(param);
    }

// SW: START: remove from local bloomfilter.
    if (bloomfilter_remove_delay_ms >= 0) {
        bloomfilterRemoveDelayEventType = registerEventType("BFRemoveDelay", onBloomfilterRemoveDelay);
        if (bloomfilterRemoveDelayEventType <= 0) {
            HAGGLE_ERR("Could not register bloomfilter remove delay event.\n");
        }
    }
//SW: END: remove from local bloomfilter.

    if (publish_stats_dataobject) {
        CacheReplacementTotalOrder *replacement = 
            new CacheReplacementTotalOrder(
                getManager(),
                "Timestamp",
                "PublisherID",
                "CacheStrategyUtility",
                "stats");
        // only keep the most up to date stats in the data base
        stats_replacement_strat =
            new CacheStrategyReplacementPurger(
                getManager(),
                replacement,
                NULL,
                false);

        if (!stats_replacement_strat) {
            HAGGLE_ERR("Could not allocate replacement strat\n");
        }
        else {
            stats_replacement_strat->start();
        }
    }

    periodicPurgeEventType = registerEventType("periodic purge event", onPeriodicEvent);
    periodicPurgeEvent = new Event(periodicPurgeEventType);
    // we re-use the event
	periodicPurgeEvent->setAutoDelete(false);

    firePeriodic();

    if (!purgeOnInsert && (pollPeriodMs <= 0)) {
        HAGGLE_DBG("WARNING: All purging is disabled, was this intended?\n");
    }

    // add debug printing 

    HAGGLE_DBG("Successfully initialized utiltiy cache strategy, knapsack optimizer: %s, global optimizer: %s, utiltiy function: %s,  max capacity kb: %lld, watermark capacity kb: %lld, compute period ms: %d, purge poll period ms: %d, purge on insert; %s, manage only remote files: %s, publish stats dataobject: %s, db_purge: %s (%lld), self test:%s \n", getKnapsackOptimizer()->getName().c_str(), getGlobalOptimizer()->getName().c_str(), getUtilityFunction()->getName().c_str(),  max_capacity_kb, watermark_capacity_kb, computePeriodMs, pollPeriodMs, purgeOnInsert ? "yes" : "no", manage_only_remote_files ? "yes" : "no", publish_stats_dataobject ? "yes (CacheStrategyUtility=stats)" : "no", allow_db_purging ? "yes" : "no", db_size_threshold, self_test ? "yes" : "no");
}

/*
 * Returns true iff 
 */
bool 
CacheStrategyUtility::isResponsibleForDataObject(
    DataObjectRef &dObj)
{
    string id = string(dObj->getIdStr());
/*
    // THIS IS NOT THREAD SAFE!! added getOrigSize
    // to grab unaltered file size.
    if (utilMetadata.find(id) != utilMetadata.end()) {
        return true;
    }
*/

    if (dObj->isDuplicate()) {
        return false;
    }

    // SW: TODO: NOTE: this might not be the best way to check if it's from
    // a local application, but it works for now...
    bool isLocal = dObj->getRemoteInterface() && dObj->getRemoteInterface()->isApplication();

    bool notResponsible = dObj->isControlMessage() || dObj->isNodeDescription();
    bool isResponsible = !notResponsible;

    if (!handle_zero_size) {
        isResponsible = isResponsible && (dObj->getOrigDataLen() > 0);
    }

    if (!manage_locally_sent_files) {
        isResponsible = isResponsible && dObj->isPersistent();
    }

    if (stats_replacement_strat && stats_replacement_strat->isResponsibleForDataObject(dObj)) {
        isResponsible = true;
    } else if (manage_only_remote_files) {
        isResponsible = isResponsible && !isLocal;
    }
    return isResponsible;
}

/*
 * Callback when a new data object is inserted. Updates the utility strategy
 * state and notifies the utility functions.
 */
void
CacheStrategyUtility::_handleNewDataObject(
    DataObjectRef &dObj)
{
    if (!isResponsibleForDataObject(dObj)) {
        //HAGGLE_DBG("Ignoring data object, in-eligible for caching\n");
        return;
    }

    if (stats_replacement_strat && stats_replacement_strat->isResponsibleForDataObject(dObj)) {
        stats_replacement_strat->handleNewDataObject(dObj);
        return;
    }

    string id = string(dObj->getIdStr());

    if (utilMetadata.find(id) != utilMetadata.end()) {
        // the DO is already in the cache!
        HAGGLE_DBG("Received data object already in the cache: %s\n", id.c_str());
        current_dupe_do_recv++;
        return;
    }

    getUtilityFunction()->notifyInsertion(dObj);

    int cost = dObj->getOrigDataLen();
    // NOTE: we handle zero sized data objects by "faking"
    // a size of 1.
    if (cost == 0) {
        cost = 1; 
    }

    string strResults="";
    if (Trace::trace.getTraceType() == TRACE_TYPE_DEBUG2) {
        strResults.append(id);
        strResults.append("[I]=");
    }
    double utiltiy = getUtilityFunction()->compute(id, strResults);
    HAGGLE_DBG2("%s --> %f\n", strResults.c_str(), utiltiy);
    Timeval now = Timeval::now();

    bool purgeNow = false;
    if (utiltiy < getGlobalOptimizer()->getMinimumThreshold()) {
        HAGGLE_DBG("Minimum threshold for incoming data object %s is insufficient: %f < %f\n", id.c_str(), utiltiy, getGlobalOptimizer()->getMinimumThreshold());
        current_drop_on_insert++;
        purgeNow = true;
    }

    if (!purgeOnInsert && ((current_size + cost) > max_capacity_kb*1024))  {
        HAGGLE_DBG("Cache is full and purge on insert is disabled, evicting new data object!\n");
        purgeNow = true;
        total_do_hard_evicted++;
        total_do_hard_evicted_bytes += cost;
    }

    if (purgeNow) {
        // we need to properly remove from bloomfilter even when capacity is violated
        getUtilityFunction()->notifyDelete(dObj);

        // CBMEN, HL - Begin
        // Remove any pending send events for this data object
        HAGGLE_STAT("purging send events for dObj %s\n", dObj->getIdStr());
        getManager()->getKernel()->cancelEvents(EVENT_TYPE_DATAOBJECT_SEND, dObj);
        // CBMEN, HL, End

        // delayed delete (although never inserted)
        if (!keep_in_bloomfilter) {
            int delay = (bloomfilter_remove_delay_ms <= 0) ? 1000 : bloomfilter_remove_delay_ms;
            DataObjectId_t *heapId = (DataObjectId_t *)malloc(sizeof(DataObjectId_t));
            DataObject::idStrToId(id, *heapId);
            getManager()->getKernel()->addEvent(new Event(bloomfilterRemoveDelayEventType, heapId, delay/(double)1000));
        }

        return;
    }

    DataObjectUtilityMetadata *do_metadata = new DataObjectUtilityMetadata(
        dObj,
        id,
        cost,
        utiltiy,
        now,
        dObj->getCreateTime());

    if (purgeOnInsert) {
        do_metadata->setEnableDeletion(false);
    }

    if (false == utilMetadata.insert(make_pair(id, do_metadata)).second) {
        HAGGLE_ERR("Somehow data object already in cache\n"); 
        delete do_metadata;
        return;
    }

    current_size += cost;
    current_num_do++;
    total_do_inserted++;
    total_do_inserted_bytes += cost;

    if (!purgeOnInsert) {
        getManager()->insertDataObjectIntoDataStore(dObj);
        return;
    }

    bool was_deleted = false;
    _purgeCache(id, &was_deleted);

    if (was_deleted) {
        HAGGLE_DBG("Purged incoming data object %s on insert.\n", id.c_str());
        current_drop_on_insert++;
        return;
    }

    // DO still in cache, mark to allow deletion in future
    do_metadata->setEnableDeletion(true);
    getManager()->insertDataObjectIntoDataStore(dObj);
}

/*
 * Callback when a data object is sucessfully sent. Notifies the utility
 * functions.
 */
void
CacheStrategyUtility::_handleSendSuccess(
    DataObjectRef &dObj,
    NodeRef &node)
{
    if (!isResponsibleForDataObject(dObj)) {
        //HAGGLE_DBG("Ignoring data object, in-eligible for caching\n");
        return;
    }

    if (stats_replacement_strat && stats_replacement_strat->isResponsibleForDataObject(dObj)) {
        //HAGGLE_DBG("Ignoring data object, send success of stats data object\n");
        //return;
    }

    getUtilityFunction()->notifySendSuccess(dObj, node);
}

bool 
CacheStrategyUtility::deleteDataObjectFromCache(string doid, bool mem_do)
{
    do_util_metadata_t::iterator it = utilMetadata.find(doid);
    if (it == utilMetadata.end()) {
        HAGGLE_DBG("Data object not in cache, possibly already removed.\n");
        return false;
    }
    DataObjectUtilityMetadata * do_metadata = (*it).second;
    utilMetadata.erase(it);
    if (!do_metadata) {
        HAGGLE_ERR("NULL metadata in cache (perhaps removed early in purge)?.\n");
        return false;
    }

    if (mem_do) {
       total_db_evicted++;
       current_num_do--;
       current_size -= do_metadata->getCost();
    } else {
       current_size -= do_metadata->getCost();
       current_num_do--;
       total_do_evicted++;
       total_do_evicted_bytes += do_metadata->getCost();
    }
    HAGGLE_DBG("Removed DB DO size: %d, create time: %s, id: %s\n", do_metadata->getCost(), do_metadata->getCreateTime().getAsString().c_str(), do_metadata->getId().c_str());

    if (!keep_in_bloomfilter) {
        int delay = (bloomfilter_remove_delay_ms <= 0) ? 1000 : bloomfilter_remove_delay_ms;
        DataObjectId_t *heapId = (DataObjectId_t *)malloc(sizeof(DataObjectId_t));
        DataObject::idStrToId(do_metadata->getId(), *heapId);
        getManager()->getKernel()->addEvent(new Event(bloomfilterRemoveDelayEventType, heapId, delay/(double)1000));
    }
    delete do_metadata;

    return true;
}

/*
 * Callback on data object deletion events. Keeps the cache stat in sync
 * with the database, and notifies the utility functions.
 */
void 
CacheStrategyUtility::_handleDeletedDataObject(
    DataObjectRef &dObj, bool mem_do)
{
    if (!isResponsibleForDataObject(dObj)) {
        //HAGGLE_DBG("Ignoring data object, in-eligible for caching\n");
        return;
    }

    if (stats_replacement_strat && stats_replacement_strat->isResponsibleForDataObject(dObj)) {
        //HAGGLE_DBG("Ignoring data object, delete of stats data object\n");
        return;
    }

    if (deleteDataObjectFromCache(string(dObj->getIdStr()), mem_do)) {
        getUtilityFunction()->notifyDelete(dObj);
    } else {
        HAGGLE_DBG("Data object may have already been removed");
    }
}

/**
 * The main function to purge the cache and recompute utilities. 
 * If HAGGLE_DEBUG2 is set, this will add a new debug printout per utility computation,
 * which wills how the results of each individual utility value per data object.
 * Useful for verifying new utilities are acting correctly, not recommended for production
 * run.   To use this feature, pass any non empty string to getUtilityFunction()->compute(id, string).
 * The compute() routines will append current values and status for the data object identified by 'id'
 * only if the string being passed is non empty.   If the string is empty, the routines ignore it.
 *
 */
void
CacheStrategyUtility::_purgeCache(string doid, bool *o_was_deleted)
{
    List<DataObjectUtilityMetadata *> process_dos;
    List<DataObjectUtilityMetadata *> delete_dos;
    long long total_size = 0;
    // iterate across data objects and compute utilities for
    // DOS that have not been computed recently
    for (do_util_metadata_t::iterator it = utilMetadata.begin(); it != utilMetadata.end(); it++) {
        DataObjectUtilityMetadata *do_info = (*it).second;
        Timeval now = Timeval::now();
        if (!do_info) {
            HAGGLE_ERR("NULL DO in cache\n");
            continue;
        }
        total_size += do_info->getCost();
        // was the utility recently computed?
        double delta = now.getTimeAsMilliSecondsDouble() - do_info->getComputedTime().getTimeAsMilliSecondsDouble();
        if (delta > computePeriodMs) {
            string strResults="";
            if (Trace::trace.getTraceType() == TRACE_TYPE_DEBUG2) {
              strResults.append(do_info->getId());
              strResults.append("[T]=");
            }
            double new_utility = getUtilityFunction()->compute(do_info->getId(), strResults);
            do_info->setUtility(new_utility, now);
            HAGGLE_DBG2("%s --> %f\n", strResults.c_str(), new_utility);
        }

        // does the utility meet the threshold?
        if (do_info->getUtility() < getGlobalOptimizer()->getMinimumThreshold()) {
            delete_dos.push_front(do_info);
        }
        else {
            process_dos.push_front(do_info);
        }
    }

    if (total_size != current_size) {
        HAGGLE_ERR("Fatal error: count mismatch of cache sizes! (%lld != %lld)\n", total_size, current_size);
        return;
    }

    if (total_size > (watermark_capacity_kb*1024)) {
        CacheKnapsackOptimizerResults results = getKnapsackOptimizer()->solve(&process_dos, &delete_dos, watermark_capacity_kb*1024 );
        if (results.getHadError()) {
            HAGGLE_ERR("Knapsack optimizer failed\n");
        }
    }
    else {
        //HAGGLE_DBG("High water mark short-circuit, bypassing knapsack...\n");
    }


    int num_deleted = 0;
    long long bytes_freed = 0;

    while (!delete_dos.empty()) {
        DataObjectUtilityMetadata *to_delete = delete_dos.front();
        delete_dos.pop_front();
        if (!to_delete) {
            HAGGLE_ERR("NULL utility\n");
            continue;
        }
        DataObjectId_t id;
        DataObject::idStrToId(to_delete->getId(), id);
		bool deletionEnabled = to_delete->getEnableDeletion();

        if (to_delete->getId() == doid && o_was_deleted) {
            *o_was_deleted = true;
        }

        bytes_freed += to_delete->getCost();

        num_deleted++;

        // some DOs should not be deleted immedaitely 
        // (they may not even be inserted yet)
        if (deletionEnabled) {
            getManager()->getKernel()->getDataStore()->deleteDataObject(id, true);
        }

        DataObjectRef stale_dobj = to_delete->getDataObject();
        // SW: this will delete "to_delete"
        _handleDeletedDataObject(stale_dobj);
    }

    // sanity check
    if (current_size > (watermark_capacity_kb*1024)) {
        HAGGLE_ERR("Optimizer failed (did not remove enough bytes)\n");
        return;
    }

//JM: start mem purging
    if (allow_db_purging && (current_num_do > db_size_threshold)) {
        CacheKnapsackOptimizerResults results = getKnapsackOptimizer()->solve(&process_dos, &delete_dos, db_size_threshold+1, true );
        if (results.getHadError()) {
            HAGGLE_ERR("Knapsack optimizer failed\n");
        }
    }
    unsigned int db_num_deleted = 0;

    while (!delete_dos.empty()) {
        DataObjectUtilityMetadata *to_delete = delete_dos.front();
        delete_dos.pop_front();
        if (!to_delete) {
            HAGGLE_ERR("NULL utility\n");
            continue;
        }
        DataObjectId_t id;
        DataObject::idStrToId(to_delete->getId(), id);
		bool deletionEnabled = to_delete->getEnableDeletion();

        if (to_delete->getId() == doid && o_was_deleted) {
            *o_was_deleted = true;
        }

        db_num_deleted++;

        // some DOs should not be deleted immedaitely 
        // (they may not even be inserted yet)
        if (deletionEnabled) {
            getManager()->getKernel()->getDataStore()->deleteDataObject(id, true);
        }

        DataObjectRef stale_dobj = to_delete->getDataObject();
        // SW: this will delete "to_delete"
        _handleDeletedDataObject(stale_dobj, true);
    }

    // JM: End

    if (publish_stats_dataobject) {
        _publishStatsDataObject();
    }

	// SW: NOTE: these numbers will be updated when the delete callbacks occur
    HAGGLE_DBG("Removed %d data objects, freed: %lld bytes: total capacity: %.2f\%, watermark capacity: %.2f\% , Purged %d DB entries\n", num_deleted, bytes_freed, 100*current_size/((double)max_capacity_kb * 1024), 100*current_size/((double)watermark_capacity_kb * 1024), db_num_deleted);
}

/* 
 * Periodic callback to purge the cache.
 */
void 
CacheStrategyUtility::onPeriodicEvent(Event *e)
{
   firePeriodic(); 
   //Verify social groups are properly processed, DEBUG only - JLM
   //getManager()->getKernel()->getNodeStore()->socialPrintGroups();

}

// SW: START: remove from local bloomfilter.
void 
CacheStrategyUtility::onBloomfilterRemoveDelay(Event *e)
{
    if (!e || !e->hasData()) {
        HAGGLE_ERR("No data.\n");
        return;
    }
    DataObjectId_t *heapId = (DataObjectId_t *)e->getData();
    if (!heapId) {
        HAGGLE_ERR("NULL dObj heap id.\n");
        return;
    }
    getManager()->removeFromLocalBF(*heapId);
    free(heapId);
}
//SW: END: remove from local bloomfilter.

/*
 * Periodic event handler executed by strat's thread. Reschedules the event.
 */
void 
CacheStrategyUtility::_handlePeriodic()
{
    if (self_test) {
       selfTest();
    }
    _purgeCache();

   if (!periodicPurgeEvent->isScheduled() &&
        pollPeriodMs > 0) {
        periodicPurgeEvent->setTimeout(pollPeriodMs/(double)1000);
        getManager()->getKernel()->addEvent(periodicPurgeEvent);
   }
}

/*
 * Return a string representation of a list of utility based caching 
 * metadata. 
 */
string 
CacheStrategyUtility::_printUtilMetadataList(
    do_util_metadata_list_t *to_print)
{
    string stats = string("");
    stats.append("data object id,create time,compute time,size,utility\n");
    for (do_util_metadata_list_t::iterator it = to_print->begin();
         it != to_print->end(); it++) 
    {
        DataObjectUtilityMetadata *do_metadata = *(it);
        string tmp = string("");
        
        stringprintf(tmp, "%s %s %s %d %.2f\n", 
            do_metadata->getId().c_str(),
            do_metadata->getCreateTime().getAsString().c_str(),
            do_metadata->getComputedTime().getAsString().c_str(),
            do_metadata->getCost(),
            do_metadata->getUtility());

        stats.append(tmp);
    }
    return stats;
}

/*
 * Used to sort the cache according to some metric.
 * Useful for printing the top K cache elements.
 */
class PrintHelperHeapItem : public HeapItem {
public: 
    // SW: NOTE: normally this would be separate 
    // classes, but here we use an enum to reduce
    // code bloat (this is an internal helper class)
	typedef enum {
        HIGH_UTIL,
        LOW_UTIL,
        HIGH_COST,
        LOW_COST,
        OLD_CREATION,
        NEW_CREATION
    } PrinterHelperType_t;

    PrintHelperHeapItem(
        PrinterHelperType_t _type,
        DataObjectUtilityMetadata *_do_metadata) :
            type(_type),
            do_metadata(_do_metadata) {};

    double getMetric() const {
        switch (type) {
        case LOW_UTIL:     return -1 * do_metadata->getUtility();
        case HIGH_UTIL:    return do_metadata->getUtility();
        case LOW_COST:     return -1 * do_metadata->getCost();
        case HIGH_COST:    return do_metadata->getCost();
        case OLD_CREATION: return -1 * do_metadata->getCreateTime().getTimeAsMilliSecondsDouble();
        case NEW_CREATION: return do_metadata->getCreateTime().getTimeAsMilliSecondsDouble();
        default:           return 0;
        }
    }

    DataObjectUtilityMetadata *getMetadata() { return do_metadata; }

    static void getTopKHelper(
        int K,
        PrinterHelperType_t type,
        do_util_metadata_t utilMetadata,
        do_util_metadata_list_t *o_top10);

    // heap item methods

    bool compare_less(const HeapItem& i) const {
        PrintHelperHeapItem other_heap_item = static_cast<const PrintHelperHeapItem&>(i);
        return getMetric() < other_heap_item.getMetric();
    }

    bool compare_greater(const HeapItem& i) const {
        PrintHelperHeapItem other_heap_item = static_cast<const PrintHelperHeapItem&>(i);
        return getMetric() > other_heap_item.getMetric();
    }

protected:
    PrinterHelperType_t type;
    DataObjectUtilityMetadata *do_metadata;
};

/*
 * Helper to return the top K data objects in the cache, according to 
 * a specified sort function. Mainly used for statistics repoorting.
 */
void 
PrintHelperHeapItem::getTopKHelper(
    int K,
    PrinterHelperType_t type,
    do_util_metadata_t utilMetadata,
    do_util_metadata_list_t *o_top10)
{
    if (!o_top10) {
        HAGGLE_ERR("NULL output list\n");
        return;
    }
    Heap Kheap;

    int k_count = 0;

    for (do_util_metadata_t::iterator it = utilMetadata.begin(); 
         it != utilMetadata.end(); it++) {
        DataObjectUtilityMetadata *do_info = (*it).second;
        if (!do_info) {
            HAGGLE_ERR("NULL do metadata in map\n");
            continue;
        }
        PrintHelperHeapItem *ele = new PrintHelperHeapItem(type, do_info);
        PrintHelperHeapItem *top = static_cast<PrintHelperHeapItem *>(Kheap.front());
        if (k_count++ < K) {
            Kheap.insert(ele);
        }
        else if (!top) {
            HAGGLE_ERR("Somehow popped NULL heap element\n");
        }
        else if (top->getMetric() < ele->getMetric()) {
            Kheap.extractFirst();
            Kheap.insert(ele);
            delete top;
        }
        else {
            delete ele;
        }
    }

    while (!Kheap.empty()) {
        PrintHelperHeapItem *heap_item = 
            static_cast<PrintHelperHeapItem *>(Kheap.extractFirst());
        if (!heap_item) {
            HAGGLE_ERR("NULL heap item\n");
            continue;
        }
        DataObjectUtilityMetadata *do_meta = heap_item->getMetadata();
        if (!do_meta) {
            HAGGLE_ERR("NULL data object metadata\n");
            continue;
        }
        o_top10->push_front(do_meta);
        delete heap_item;
    }
}

/*
 * Generates a string with utility based caching statistics.
 */
string
CacheStrategyUtility::_generateStatsString()
{
    string stats = string("");
    string tmp;
    stringprintf(tmp, "\n===== Utility Based Caching Stats (%s) =====\n", (Timeval::now()).getAsString().c_str());
    stats.append(tmp);
    stats.append("------------ parameters ---------------\n");
    stringprintf(tmp, "Knapsack optimizer: %s\n", getKnapsackOptimizer()->getName().c_str());
    stats.append(tmp);
    stringprintf(tmp, "Global optimizer: %s\n", getGlobalOptimizer()->getPrettyName().c_str());
    stats.append(tmp);
    stringprintf(tmp, "Utility function: utility(d) = %s\n", getUtilityFunction()->getPrettyName().c_str());
    stats.append(tmp);
    stringprintf(tmp, "Utility compute period (ms): %d\n", computePeriodMs);
    stats.append(tmp);
    if (pollPeriodMs > 0) {
        stringprintf(tmp, "Poll purging: enabled (%d ms period)\n", pollPeriodMs);
        stats.append(tmp);
    }
    else {
        stats.append("Poll purging: disabled\n");
    }
    stringprintf(tmp, "Event-based purging: %s\n", purgeOnInsert ? "enabled" : "disabled");
    stats.append(tmp);

    stringprintf(tmp, "Keep in bloomfilter (%d ms): %s\n", bloomfilter_remove_delay_ms, keep_in_bloomfilter ? "enabled" : "disabled");
    stats.append(tmp);

    stringprintf(tmp, "Handle zero sized objects: %s\n", handle_zero_size ? "enabled" : "disabled");
    stats.append(tmp);

    stringprintf(tmp, "Manage only remote files: %s\n", manage_only_remote_files ? "enabled" : "disabled");
    stats.append(tmp);

    stringprintf(tmp, "Manage locally sent files: %s\n", manage_locally_sent_files ? "enabled" : "disabled");
    stats.append(tmp);

    stringprintf(tmp, "Manage DB entries: %s\n", allow_db_purging ? "enabled" : "disabled");
    stats.append(tmp);

    stringprintf(tmp, "Threshold DB entries: %d\n", db_size_threshold);
    stats.append(tmp);
    stats.append("------------ statistics ---------------\n");
    
    stringprintf(tmp, "Total Capacity (kb/kb): %lld/%lld = %.2f\%\n", current_size / (long long)1024, max_capacity_kb, 100*current_size/((double)max_capacity_kb * 1024));
    stats.append(tmp);
    stringprintf(tmp, "Watermark Capacity (kb/kb): %lld/%lld = %.2f\%\n",  current_size / (long long)1024, watermark_capacity_kb, 100*current_size/((double)watermark_capacity_kb * 1024));
    stats.append(tmp);

    stringprintf(tmp, "Data objects in cache: %d\n", current_num_do);
    stats.append(tmp);

    stringprintf(tmp, "DB entries purged: %d\n", total_db_evicted);
    stats.append(tmp);

    stringprintf(tmp, "Duplicate data objects received: %d\n", current_dupe_do_recv);
    stats.append(tmp);

    stringprintf(tmp, "Total data objects dropped on insertion: %d\n", current_drop_on_insert);
    stats.append(tmp);

    stringprintf(tmp, "Total data objects inserted: %d (%lld bytes)\n", total_do_inserted, total_do_inserted_bytes);
    stats.append(tmp);

    stringprintf(tmp, "Total data objects evicted: %d (%lld bytes)\n", total_do_evicted, total_do_evicted_bytes);
    stats.append(tmp);

    stringprintf(tmp, "Total data objects dropped due to capacity violation: %d (%lld bytes)\n", total_do_hard_evicted, total_do_hard_evicted_bytes);
    stats.append(tmp);


    do_util_metadata_list_t metadata_list;

    stats.append("----- Top 10 Utility Data Objects -----\n");

    PrintHelperHeapItem::getTopKHelper(10, PrintHelperHeapItem::HIGH_UTIL, utilMetadata, &metadata_list);
    stats.append(_printUtilMetadataList(&metadata_list));
    metadata_list.clear();

    stats.append("--- Bottom 10 Utility Data Objects ----\n");

    PrintHelperHeapItem::getTopKHelper(10, PrintHelperHeapItem::LOW_UTIL, utilMetadata, &metadata_list);
    stats.append(_printUtilMetadataList(&metadata_list));
    metadata_list.clear();

    stats.append("----- Top 10 Largest Data Objects -----\n");

    PrintHelperHeapItem::getTopKHelper(10, PrintHelperHeapItem::HIGH_COST, utilMetadata, &metadata_list);
    stats.append(_printUtilMetadataList(&metadata_list));
    metadata_list.clear();

    stats.append("------ Top 10 Oldest Data Objects -----\n");

    PrintHelperHeapItem::getTopKHelper(10, PrintHelperHeapItem::OLD_CREATION, utilMetadata, &metadata_list);
    stats.append(_printUtilMetadataList(&metadata_list));
    metadata_list.clear();

    stats.append("=======================================\n");
    return stats;
}

void 
CacheStrategyUtility::_handlePrintDebug()
{
    string stats = _generateStatsString();
    printf(stats.c_str());
}

// CBMEN, HL, Begin
void
CacheStrategyUtility::_handleGetCacheStrategyAsMetadata(Metadata *m)
{
    Metadata *dm = m->addMetadata("CacheStrategyUtility");
    if (!dm) {
        return;
    }

    dm->setParameter("knapsack_optimizer", getKnapsackOptimizer()->getName());
    dm->setParameter("global_optimizer", getGlobalOptimizer()->getPrettyName());
    dm->setParameter("utility_function", getUtilityFunction()->getPrettyName());
    dm->setParameter("utility_compute_period_ms", computePeriodMs);
    dm->setParameter("poll_period_ms", pollPeriodMs);
    dm->setParameter("event_based_purging", purgeOnInsert ? "enabled" : "disabled");
    dm->setParameter("keep_in_bloomfilter", keep_in_bloomfilter ? "enabled" : "disabled");
    dm->setParameter("bloomfilter_remove_delay_ms", bloomfilter_remove_delay_ms);
    dm->setParameter("handle_zero_size", handle_zero_size ? "enabled" : "disabled");
    dm->setParameter("manage_only_remote_files", manage_only_remote_files ? "enabled" : "disabled");
    dm->setParameter("manage_locally_sent_files", manage_locally_sent_files ? "enabled" : "disabled");
    dm->setParameter("allow_db_purging", allow_db_purging ? "enabled" : "disabled");
    dm->setParameter("db_size_threshold", db_size_threshold);
    dm->setParameter("current_size_kb", current_size / (long long) 1024);
    dm->setParameter("max_capacity_kb", max_capacity_kb);
    dm->setParameter("current_capacity_ratio", 100*current_size/((double)max_capacity_kb * 1024));
    dm->setParameter("watermark_capacity_kb", watermark_capacity_kb);
    dm->setParameter("watermark_capacity_ratio", 100*current_size/((double)watermark_capacity_kb * 1024));
    dm->setParameter("current_num_do", current_num_do);
    dm->setParameter("total_db_evicted", total_db_evicted);
    dm->setParameter("current_dupe_do_recv", current_dupe_do_recv);
    dm->setParameter("current_drop_on_insert", current_drop_on_insert);
    dm->setParameter("total_do_inserted", total_do_inserted);
    dm->setParameter("total_do_inserted_bytes", total_do_inserted_bytes);
    dm->setParameter("total_do_evicted", total_do_evicted);
    dm->setParameter("total_do_evicted_bytes", total_do_evicted_bytes);
    dm->setParameter("total_do_hard_evicted", total_do_hard_evicted);
    dm->setParameter("total_do_hard_evicted_bytes", total_do_hard_evicted_bytes);

    PrintHelperHeapItem::PrinterHelperType_t types[] = {PrintHelperHeapItem::HIGH_UTIL, PrintHelperHeapItem::LOW_UTIL, PrintHelperHeapItem::HIGH_COST, PrintHelperHeapItem::OLD_CREATION};
    string names[] = {"Top10Utility", "Bottom10Utility", "Top10Largest", "Top10Oldest"};

    do_util_metadata_list_t metadata_list;
    for (size_t i = 0; i < 4; i++)
    {
        Metadata *dmm = dm->addMetadata(names[i]);
        if (!dmm) {
            continue;
        }

        PrintHelperHeapItem::getTopKHelper(10, types[i], utilMetadata, &metadata_list);
        for (do_util_metadata_list_t::iterator it = metadata_list.begin(); it != metadata_list.end(); it++) {
            Metadata *dmmm = dmm->addMetadata("DataObject");
            if (!dmmm) {
                break;
            }
            DataObjectUtilityMetadata *do_metadata = *it;
            dmmm->setParameter("id", do_metadata->getId());
            dmmm->setParameter("create_time", do_metadata->getCreateTime().getAsString());
            dmmm->setParameter("computed_time", do_metadata->getComputedTime().getAsString());
            dmmm->setParameter("cost", do_metadata->getCost());
            dmmm->setParameter("utility", do_metadata->getUtility());
        }
        metadata_list.clear();
    }

    kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m->copy()));
}

/*
 * Publish a data object with utility based caching statistics.
 */
void
CacheStrategyUtility::_publishStatsDataObject()
{
    DataObjectRef dObj = DataObject::create();
    dObj->addAttribute("CacheStrategyUtility", "stats");
    dObj->addAttribute("Timestamp", Timeval::now().getAsString().c_str());
    dObj->addAttribute("PublisherID", getManager()->getKernel()->getThisNode()->getIdStr());
    dObj->addAttribute("Statistics", _generateStatsString().c_str());

#ifdef DEBUG
    dObj->print(NULL); // MOS - NULL means print to debug trace
#endif

    getManager()->getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_RECEIVED, dObj));
}

CacheGlobalOptimizer *
CacheStrategyUtility::getGlobalOptimizer()
{
    if (!globalOptimizer) {
        HAGGLE_ERR("No global optimizer set.\n");
        globalOptimizer = CacheGlobalOptimizerFactory::getNewGlobalOptimizer(getManager()->getKernel());
    }
    return globalOptimizer;
}

CacheUtilityFunction *
CacheStrategyUtility::getUtilityFunction() 
{
    if (!utilFunction) {
        HAGGLE_ERR("No util function set.\n");
        utilFunction = CacheUtilityFunctionFactory::getNewUtilityFunction(getManager(), getGlobalOptimizer());
    }
    return utilFunction;
}

CacheKnapsackOptimizer *
CacheStrategyUtility::getKnapsackOptimizer()
{
    if (!ksOptimizer) {
        HAGGLE_ERR("No knapsack optimizer set.\n");
        ksOptimizer = CacheKnapsackOptimizerFactory::getNewKnapsackOptimizer(getManager()->getKernel());
    }
    return ksOptimizer;
}

void
CacheStrategyUtility::quitHook()
{
    if (stats_replacement_strat) {
        stats_replacement_strat->stop();
    }

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Total Capacity (kb/kb): %lld/%lld = %.2f\%\n", current_size / (long long) 1024, max_capacity_kb, 100*current_size/((double)max_capacity_kb * 1024));

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Duplicate data objects received: %d\n", current_dupe_do_recv);

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Total data objects dropped on insertion: %d\n", current_drop_on_insert);

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Watermark Capacity (kb/kb): %lld/%lld = %.2f\%\n",  current_size / (long long) 1024, watermark_capacity_kb, 100*current_size/((double)watermark_capacity_kb * 1024));

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Data objects in cache: %d\n", current_num_do);

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Total data objects inserted: %d (%lld bytes)\n", total_do_inserted, total_do_inserted_bytes);

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Total data objects evicted: %d (%lld bytes)\n", total_do_evicted, total_do_evicted_bytes);
    
    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Total DB entries evicted: %d \n", total_db_evicted);

    HAGGLE_STAT("CacheStrategyUtility Summary Statistics - Total data objects dropped due to capacity violation: %d (%lld bytes)\n", total_do_hard_evicted, total_do_hard_evicted_bytes);
}


/**
* selfTest() depends upon periodic purging.   Every call, it will first fill up the database, taking memory snapshots,
* then purge, taking more memory snapshots.   It creates a number of random data objects, inserts them into the system,
* and records current memory usage into a file called 'mem.results', for every interation.   It does in the following steps:
*
* 1.  Add, approximately, 20 DO's per second, done every poll period (e.g. 10 seconds, means add 200 for that interval).
*
* 2.  When the number of DO's in the system match the threshold setting, we drop the threshold by the same number
* we increased it in step 1.   In this case, we decrease it by 20, allowing the memory threshold purger to do its job.
*
* 3.  Repeat step 1.
*
* 4.  Repeat step 2.
*
* 5.  Reset system functions to original defaults.
*
* This allows the system to approach maximum usage, drop to zero, back to maximum usage, then back to zero.
* The file can be parse into a plot showing how much memory is freed.   This test works regardless of using
* in memory database, the improved all memory database, or disk based database.
* 
* WARNING: We use mallinfo() to determine the amount of memory used and released, as the system does NOT
* see any memory freed at all, due to the fact dlmallopt(-1) is set in main.cpp.  This tells free to not
* return freed memory to the system.    Thus, using mallinfo is the only means available to show how much
* memory is actually freed to the application (but not to the system).
* 
*  
*/
void
CacheStrategyUtility::selfTest()
{
    static int init=0;
    static float amount_do=0;
    static int count=0;
    char countStr[50];
    static float threshold_backup;
    static char *direction;
    if (!init) {
       init=1;
       threshold_backup=db_size_threshold;
       amount_do=20.0*pollPeriodMs/1000; //20 per second seems reasonable
       if (amount_do > db_size_threshold/10) {
         amount_do = db_size_threshold/10;
       }
       direction="Start";

    }   // JM: Start DB purging
        // JM: Testing code only, to prove it works.   Lets do linear testing.
        struct mallinfo mi=mallinfo();
	//Due to file permission difficulties in android, we'll write it as a log
        HAGGLE_DBG("\nThreshold(%s): %lld/%d -- Used bytes: %d, Free bytes: %d, SQL: %lld\n\n", direction, db_size_threshold,current_num_do, mi.uordblks, mi.fordblks, sqlite3_memory_used());
        //send db_size_threshold DO's
	//init = 1 means send DO's
	//init = 2 means we are in purging mode
        if ((init == 1) || (init == 3)) {
          float upperlimit=current_num_do+amount_do;
          if (upperlimit > db_size_threshold) {
            upperlimit = db_size_threshold+1;
	    init++;
          }
          if (init ==1) {
             direction="Up1";
          } else if (init == 3) {
             direction="Up2";
          } else { 
             direction="StateChangeFromUp";
          }

          for(int i=current_num_do; i<upperlimit; i++) {
	    DataObjectRef dObj = createDataObject(2);
	    dObj->addAttribute("ContentOriginator", "self");
	    dObj->addAttribute("ContentType", "DelByRelTTL");
	    dObj->addAttribute("ContentType2", "DelByAbsTTL");
	    dObj->addAttribute("purge_by_timestamp", "2000000000");
	    dObj->addAttribute("purge_after_seconds", "2000000000");
            char buffer[1025];
            snprintf(buffer, 1024, "%llu", (unsigned long long)time(NULL));
            sprintf(countStr, "%d", count++);
	    dObj->addAttribute("ContentCreationTime", buffer);
	    dObj->addAttribute("count", countStr);
            dObj->calcId();
	    _handleNewDataObject(dObj);
          }
         } else if ((init == 2) || (init == 4)) {   //init==2, reduction
            db_size_threshold -= amount_do;
            if (db_size_threshold < 0.0) {
              init++;
              db_size_threshold=threshold_backup;
            }
            if (init == 2) {
               direction="Down1";
            } else if (init == 4) {
               direction="Down2";
            } else {
               direction="StateChangeFromDown";
            }
         } else { //if (init == 5) 
           //clear seltTest?
           self_test = false;
           db_size_threshold=threshold_backup;
           HAGGLE_DBG("Self Test completed!\n");
           //remove any last DO's
           //write any STAT information
           getKernel()->shutdown();
           //return;
         }
        // JM: End testing
}

//stolen from Benchmark Manager
/**
* This routine was taken from the benchmark manager, to create Dataobjects in code.
* It is used for the self test.    Depending on compile options, benchmark manager
* may not be included, so the relevant code is copied here to be always available.
*
* @see BenchmarkManager::createDataObject
*
*/
DataObjectRef CacheStrategyUtility::createDataObject(unsigned int numAttr)
{
	char name[128];
	char value[128];
	unsigned int r;

	unsigned char macaddr[6];
	macaddr[0] = (unsigned char) RANDOM_INT(255);
	macaddr[1] = (unsigned char) RANDOM_INT(255);
	macaddr[2] = (unsigned char) RANDOM_INT(255);
	macaddr[3] = (unsigned char) RANDOM_INT(255);
	macaddr[4] = (unsigned char) RANDOM_INT(255);
	macaddr[5] = (unsigned char) RANDOM_INT(255);
	
	unsigned char macaddr2[6];
	macaddr2[0] = (unsigned char) RANDOM_INT(255);
	macaddr2[1] = (unsigned char) RANDOM_INT(255);
	macaddr2[2] = (unsigned char) RANDOM_INT(255);
	macaddr2[3] = (unsigned char) RANDOM_INT(255);
	macaddr2[4] = (unsigned char) RANDOM_INT(255);
	macaddr2[5] = (unsigned char) RANDOM_INT(255);
	
	EthernetAddress addr(macaddr);
	EthernetAddress addr2(macaddr2);
	InterfaceRef localIface = Interface::create<EthernetInterface>(macaddr, "eth", addr, 0);		
	InterfaceRef remoteIface = Interface::create<EthernetInterface>(macaddr2, "eth2", addr2, 0);		
	DataObjectRef dObj = DataObject::create(NULL, 0, localIface, remoteIface);

	for (unsigned int i = 0; i < numAttr; i++) {
		int tries = 0;
		do {
			r = RANDOM_INT(32000);
			sprintf(name, "name");
			sprintf(value, "value %d", r);
			if (tries++ > 10) {
				HAGGLE_ERR("WARNING: Cannot generate unique attributes in data object... check attribute pool size!\n");
				break;
			}
		} while (dObj->getAttribute(name, value));

		dObj->addAttribute(name, value, 1); //r);
	}

	return dObj;
}


