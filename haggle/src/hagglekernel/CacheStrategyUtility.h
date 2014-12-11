/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#ifndef _CACHE_STRAT_UTILITY_H
#define _CACHE_STRAT_UTILITY_H

class DataObjectUtilityMetadata;
class CacheStrategyUtility;

#include "CacheStrategyAsynchronous.h"
#include <libcpphaggle/Map.h>
#include "CacheKnapsackOptimizer.h"
#include "CacheUtilityFunction.h"
#include "CacheGlobalOptimizer.h"
#include "CacheStrategyReplacementPurger.h"

typedef Map<string, DataObjectUtilityMetadata *> do_util_metadata_t;
typedef List<DataObjectUtilityMetadata *> do_util_metadata_list_t;

#define CACHE_STRAT_UTILITY_NAME "CacheStrategyUtility"

class DataObjectUtilityMetadata {
protected:
	DataObjectRef dObj;
    string id;
    int cost;
    double utility;
    Timeval compute_date;
    Timeval create_date;
    bool enableDeletion;
public:
    DataObjectUtilityMetadata(DataObjectRef _dObj, string _id, int _cost, double _utility, Timeval _compute_date, Timeval _create_date) :
        dObj(_dObj),
        id(_id),
        cost(_cost),
        utility(_utility),
        compute_date(_compute_date),
        create_date(_create_date),
        enableDeletion(true) {}
    string getId() { return id; }
    int getCost() { return cost; }
    void setUtility(double _utility, Timeval _compute_date) { utility = _utility; compute_date = _compute_date; }
    double getUtility() { return utility; }
    Timeval getComputedTime() { return compute_date; }
    Timeval getCreateTime() { return create_date; }
    void setEnableDeletion(bool _enableDeletion) { enableDeletion = _enableDeletion; }
    bool getEnableDeletion() { return enableDeletion; }
    DataObjectRef getDataObject() { return dObj; }
};

class CacheStrategyUtility : public CacheStrategyAsynchronous {
private:
    int computePeriodMs;
    int pollPeriodMs;
    do_util_metadata_t utilMetadata;
    long long max_capacity_kb;
    long long watermark_capacity_kb;
    bool useXMLSocialGrp;
    string socialGroupName;
    bool purgeOnInsert;
    CacheKnapsackOptimizer *ksOptimizer;
    CacheUtilityFunction *utilFunction;
    CacheGlobalOptimizer *globalOptimizer;
    EventType periodicPurgeEventType;
    Event *periodicPurgeEvent;
    int current_num_do;
    int total_do_inserted;
    long long total_do_inserted_bytes;
    int total_do_evicted;
    int total_db_evicted;
    int total_do_hard_evicted;
    long long total_do_hard_evicted_bytes;
    long long total_do_evicted_bytes;
    long long current_size;
    long long db_size_threshold;
    bool allow_db_purging;
    bool self_test;
    bool manage_only_remote_files;
    bool manage_locally_sent_files;
    bool publish_stats_dataobject;
    CacheStrategyReplacementPurger *stats_replacement_strat;
    bool keep_in_bloomfilter;
    bool handle_zero_size;

// SW: START: remove from local bloomfilter.
    int bloomfilter_remove_delay_ms;
    EventType bloomfilterRemoveDelayEventType;
//SW: END: remove from local bloomfilter.

    int current_dupe_do_recv;
    int current_drop_on_insert;

    string _printUtilMetadataList(do_util_metadata_list_t *to_print);
    string _generateStatsString();
    void _publishStatsDataObject();
public:
    bool isResponsibleForDataObject(DataObjectRef &dObj);
    CacheStrategyUtility(DataManager *m);
    ~CacheStrategyUtility();
    CacheUtilityFunction *getUtilityFunction();
    CacheKnapsackOptimizer *getKnapsackOptimizer();
    CacheGlobalOptimizer *getGlobalOptimizer();
    void onPeriodicEvent(Event *e);
// SW: START: remove from local bloomfilter.
    void onBloomfilterRemoveDelay(Event *e);
//SW: END: remove from local bloomfilter.
    void quitHook();
    bool deleteDataObjectFromCache(string doid, bool mem_do=false);

protected:
    void _onConfig(const Metadata& m);
    void _handleSendSuccess(DataObjectRef &dObj, NodeRef &node);
    void _handleNewDataObject(DataObjectRef &dObj);
    void _handleDeletedDataObject(DataObjectRef &dObj, bool mem_do=false);
    void _handlePeriodic();
    void selfTest();
    DataObjectRef createDataObject(unsigned int numAttr);
    void _handlePrintDebug();
    void _handleGetCacheStrategyAsMetadata(Metadata *m); // CBMEN, HL
    void _purgeCache(string doid = string(""), bool *o_was_deleted = NULL);
};

#endif /* _CACHE_STRAT_UTILTIY */
