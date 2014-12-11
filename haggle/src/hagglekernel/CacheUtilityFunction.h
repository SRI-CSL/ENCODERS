/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 *   Jihwa Lee (JL)
 */

#ifndef _CACHE_UTIL_FUNC_H
#define _CACHE_UTIL_FUNC_H

#include "HaggleKernel.h"
#include "CacheGlobalOptimizer.h"
#include "EvictStrategyManager.h"
#include "CacheReplacement.h"

class CacheUtilityFunction {
private:
    string name;
    DataManager *dataManager;
    CacheGlobalOptimizer *globalOptimizer;
public:
    CacheUtilityFunction(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer, string _name) :
        name(_name),
        dataManager(_dataManager),
        globalOptimizer(_globalOptimizer) {};
    virtual ~CacheUtilityFunction() {}
    virtual double compute(string do_id, string &strResult) = 0;
    virtual double compute(string do_id) {
        string zilch="";
        return compute(do_id, zilch);
    } ;
    virtual void onConfig(const Metadata& m);
    virtual void selfTest(int testLevel) {};
    virtual void selfBenchMark(int benchMarkLevel) {};
    virtual void notifyInsertion(DataObjectRef dObj) {};
    virtual void notifyDelete(DataObjectRef dObj) {};
    virtual void notifySendSuccess(DataObjectRef dObj, NodeRef node) {};
    HaggleKernel *getKernel() { return dataManager->getKernel(); }
    DataManager *getDataManager() { return dataManager; }
    CacheGlobalOptimizer *getGlobalOptimizer() { return globalOptimizer; }
    string getName() { return name; }
    virtual string getShortName() = 0;
    virtual string getPrettyName() { return name; }
};

// START: aggregate functions:

class CacheUtilityAggregate : public CacheUtilityFunction {
protected:
    List<CacheUtilityFunction *> factors;
public:
    CacheUtilityAggregate(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer, string name) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, name) {};
    virtual ~CacheUtilityAggregate();
    void notifyInsertion(DataObjectRef dObj);
    void notifyDelete(DataObjectRef dObj);
    void notifySendSuccess(DataObjectRef dObj, NodeRef node);
    void onConfig(const Metadata& m);
    virtual double compute(string do_id, string &strResult) = 0;
    virtual string getPrettyName() = 0;
};

class CacheUtilityAggregateMax : public CacheUtilityAggregate {
#define CACHE_UTIL_AGGREGATE_MAX_NAME "CacheUtilityAggregateMax"
public:
    CacheUtilityAggregateMax(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityAggregate(_dataManager, _globalOptimizer, CACHE_UTIL_AGGREGATE_MAX_NAME) {};
    double compute(string do_id, string &strResult);
    string getPrettyName();
    string getShortName() { return string( "MAX"); };
};

class CacheUtilityAggregateMin : public CacheUtilityAggregate {
#define CACHE_UTIL_AGGREGATE_MIN_NAME "CacheUtilityAggregateMin"
public:
    CacheUtilityAggregateMin(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityAggregate(_dataManager, _globalOptimizer, CACHE_UTIL_AGGREGATE_MIN_NAME) {};
    double compute(string do_id, string &strResult);
    string getPrettyName();
    string getShortName() { return string( "MIN"); };
};

class CacheUtilityAggregateSum : public CacheUtilityAggregate {
#define CACHE_UTIL_AGGREGATE_SUM_NAME "CacheUtilityAggregateSum"
public:
    CacheUtilityAggregateSum(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityAggregate(_dataManager, _globalOptimizer, CACHE_UTIL_AGGREGATE_SUM_NAME) {};
    double compute(string do_id, string &strResult);
    string getPrettyName();
    string getShortName() { return string( "SUM"); };
};

// END: aggregate functions.

class CacheUtilityPopularity : public CacheUtilityFunction {
#define CACHE_UTIL_POPULARITY_NAME "CacheUtilityPopularity"
private:
    typedef HashMap<string, double> pop_map_t;
    EvictStrategyManager *evictManager;
    pop_map_t pop_map;
    double max_evict_value;
    // SW: NOTE: we need the min and max to properly normalize w/
    // real time or virtual time
    double min_evict_value;
public:
    CacheUtilityPopularity(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_POPULARITY_NAME),
        evictManager(NULL),
        max_evict_value(0),
        min_evict_value(-1) {};
    ~CacheUtilityPopularity();
    void notifyDelete(DataObjectRef dObj);
    void notifyInsertion(DataObjectRef dObj);
    void notifySendSuccess(DataObjectRef dObj, NodeRef node);
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    string getPrettyName() { return string("pop(d)"); };
    string getShortName() { return string( "POP"); };
};

class CacheUtilityNeighborhood : public CacheUtilityFunction {
#define CACHE_UTIL_NEIGHBORHOOD_NAME "CacheUtilityNeighborhood"
protected:
    bool discrete_probablistic;
    int neighbor_fudge;
public:
    CacheUtilityNeighborhood(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_NEIGHBORHOOD_NAME),
        discrete_probablistic(false),
        neighbor_fudge(0) {};
    ~CacheUtilityNeighborhood() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    string getShortName() { return string( "NHOOD"); };
    string getPrettyName() { return string("nbrhood(d)"); };
};

// social neighborhood 

class CacheUtilityNeighborhoodSocial : public CacheUtilityFunction {
#define CACHE_UTIL_NEIGHBORHOOD_SOCIAL_NAME "CacheUtilityNeighborhoodSocial"
private:
    typedef HashMap<string, int> social_neighbor_map_t;
    social_neighbor_map_t soc_nbr_map;
    //unsigned long long local_size;

protected:
    bool discrete_probablistic; 
    int neighbor_fudge;
    float default_value;
    bool only_phy_neighbors;
    string social_identifier;
public:
    CacheUtilityNeighborhoodSocial(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_NEIGHBORHOOD_SOCIAL_NAME),
        discrete_probablistic(false),
        neighbor_fudge(0),
        //local_size(0),
        default_value(1.0),
        only_phy_neighbors(true),
        social_identifier("") {};
    ~CacheUtilityNeighborhoodSocial() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    void notifyDelete(DataObjectRef dObj);
    void notifyInsertion(DataObjectRef dObj);
    string getPrettyName() { return string("nbrhoodsocial(d)"); };
    string getShortName() { return string("NHSOCIAL"); };
};

// other social neighborhood 

class CacheUtilityNeighborhoodOtherSocial : public CacheUtilityFunction {
#define CACHE_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME "CacheUtilityNeighborhoodOtherSocial" /**< \fn Full name for configuration */
private:
    typedef HashMap<string, int> social_neighbor_group_count_t;
    social_neighbor_group_count_t soc_nbr_map; /**< map association between social group and a count of how many of its members have the DO in question */

//protected:
    int exp_neighbor_groups; /**< This sets the minimum expected groups for compute().  If the number of groups seen by the node is less than this value, then it will assume the groups exist, but are unseen, and are included in value calculations. */
    int max_group_count; /**< Maximum nodes per same social group to consider, as we give a value of 1-1/(2^#), where # is the number of nodes having the content.   By limiting this to '1', you can make a discrete value with a weight of 2 (0,1), or give it a higher value, to add more weight to more copies in other groups */
    float default_value; /**< This returns the value for DO's which are not affected by this function. */
    bool only_phy_neighbors; /**< Only use 1-hop neighboor nodes for compute() calculation if true, else, use all known unexpired nodes. */

    bool lessismore; /**< The result from compute() tends to be higher, the more copies seen (if set to false).  If set to true, we return 1-value, which gives fewer copies a higher value.  */
    bool exclude_my_group;  /**<  Exclude all nodes in my social group for calculation purpose, if true. */  
public:
    CacheUtilityNeighborhoodOtherSocial(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME),
        exp_neighbor_groups(1),
        max_group_count(1),
        default_value(1.0),
        exclude_my_group(true),
	lessismore(false),
        only_phy_neighbors(true) {};
    ~CacheUtilityNeighborhoodOtherSocial() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    /**
    *  Return a string for the formulaic printing.
    */
    string getPrettyName() { return string("nbrhoodothersocial(d)"); }; 
    /**
    *  Return a string for the individual DO value computation.
    */
    string getShortName() { return string("NHOTHERSOCIAL"); };  
};

class CacheUtilityRandom : public CacheUtilityFunction {
#define CACHE_UTIL_RANDOM_NAME "CacheUtilityRandom"
public:
    CacheUtilityRandom(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_RANDOM_NAME) {};
    ~CacheUtilityRandom() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    string getPrettyName() { return string("random(d)"); };
    string getShortName() { return string("RND"); };
};

// SW: START SECURE NC:

class CacheUtilitySecureCoding : public CacheUtilityFunction {
#define CACHE_UTIL_SECURE_CODING_NAME "CacheUtilitySecureCoding"
private:

    typedef struct {
        int num_blocks;
        int total_blocks;
        int delete_blocks;
	Timeval last_update;
    } BlockCountEntry_t;

    float time_to_delete; 
    typedef HashMap<string, BlockCountEntry_t> block_count_map_t;
    block_count_map_t block_count;
    //JM:
    typedef struct {
       string parent_id;
       //flag to delete DO, not parent
       bool originalToBeDeleted;
    } parent_info_t;
    typedef Map<string, parent_info_t> parent_id_map_t;
    parent_id_map_t parent_id_map;

    NetworkCodingDataObjectUtility *ncUtil;
    double max_ratio;
public:
    CacheUtilitySecureCoding(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer);
    ~CacheUtilitySecureCoding();
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    string getPrettyName() { return string("securecoding(d)"); };
    string getShortName() { return string("SECOD"); };
    void notifyDelete(DataObjectRef dObj);
    void notifyInsertion(DataObjectRef dObj);
};

// SW: END SECURE NC.

class CacheUtilityImmunityNewByTime : public CacheUtilityFunction {
#define CACHE_NEW_DO_TIME_WINDOW_IMMUNITY_NAME "CacheUtilityNewTimeImmunity"
private:
    typedef HashMap<string, Timeval> receivetime_map_t;
    receivetime_map_t time_map;
    double time_window_in_ms;
    bool linear_declining;
public:
    CacheUtilityImmunityNewByTime(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_NEW_DO_TIME_WINDOW_IMMUNITY_NAME), time_window_in_ms(2000.0), linear_declining(false) {};
    ~CacheUtilityImmunityNewByTime() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    string getPrettyName() { return string("newtimeimmune(d)"); };
    string getShortName() { return string("IMM"); };
    void notifyDelete(DataObjectRef dObj);
    void notifyInsertion(DataObjectRef dObj);
};

class CacheUtilityNOP : public CacheUtilityFunction {
#define CACHE_UTIL_NOP_NAME "CacheUtilityNOP"
public:
    CacheUtilityNOP(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_NOP_NAME) {};
    ~CacheUtilityNOP() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    string getPrettyName() { return string("NOP"); };
    string getShortName() { return string("NOP"); };
};

class CacheUtilityReplacement : public CacheUtilityFunction {
public:
    CacheUtilityReplacement(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer, string _name) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, _name) {};
    virtual ~CacheUtilityReplacement() {};
    double compute(string do_id, string &strResult);
    void notifyInsertion(DataObjectRef dObj);
    void notifyDelete(DataObjectRef dObj);
    virtual void onConfig(const Metadata& m) = 0;
    virtual string getPrettyName() = 0;
    virtual bool isResponsibleForDataObject(DataObjectRef dObj) = 0;
    virtual void getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout) = 0;
};

#define CACHE_UTIL_REPLACEMENT_PRIORITY_FIELD_NAME "priority"
#define CACHE_UTIL_REPLACEMENT_PRIORITY_NAME_FIELD_NAME "name"

class CacheUtilityReplacementPriority : public CacheUtilityReplacement {
#define CACHE_UTIL_REPLACEMENT_PRIORITY_NAME "CacheUtilityReplacementPriority"
private:
    List<Pair<CacheUtilityReplacement *, int> > sorted_replacement_list;
public:
    CacheUtilityReplacementPriority(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityReplacement(_dataManager, _globalOptimizer, CACHE_UTIL_REPLACEMENT_PRIORITY_NAME) {};
    ~CacheUtilityReplacementPriority();
    void onConfig(const Metadata& m);
    string getPrettyName();
    string getShortName() { return string("RP"); };

    bool isResponsibleForDataObject(DataObjectRef dObj);
    void getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout);
};

#define CACHE_UTIL_REPLACEMENT_METRIC "metric_field"
#define CACHE_UTIL_REPLACEMENT_ID "id_field"
#define CACHE_UTIL_REPLACEMENT_TAG "tag_field"
#define CACHE_UTIL_REPLACEMENT_TAG_VALUE "tag_field_value"

class CacheUtilityReplacementTotalOrder : public CacheUtilityReplacement {
#define CACHE_UTIL_REPLACEMENT_TO_NAME "CacheUtilityReplacementTotalOrder"
protected:
    CacheReplacement *cacheReplacement;
public:
    CacheUtilityReplacementTotalOrder(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityReplacement(_dataManager, _globalOptimizer, CACHE_UTIL_REPLACEMENT_TO_NAME) {};
    ~CacheUtilityReplacementTotalOrder();
    void onConfig(const Metadata& m);
    string getPrettyName();
    string getShortName() { return string("PO"); };
    bool isResponsibleForDataObject(DataObjectRef dObj);
    void getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout);
};

#define CACHE_UTIL_PURGER_METRIC "purge_type"
#define CACHE_UTIL_PURGER_TAG "tag_field"
#define CACHE_UTIL_PURGER_TAG_VALUE "tag_field_value"
#define CACHE_UTIL_PURGER_MIN_DB_TIME_S "min_db_time_seconds"

class CacheUtilityTimePurger : public CacheUtilityFunction {
private:
    bool linear_declining;
protected:
    string metricField;
    string tagField;
    string tagFieldValue;
    double minDBtimeS;
    bool isResponsibleForDataObject(DataObjectRef& dObj);
    virtual double getEvictDate(DataObjectRef& dObj) = 0;
public:
    CacheUtilityTimePurger(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer, string _name) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, _name), linear_declining(false) {};
    virtual ~CacheUtilityTimePurger() {};
    double compute(string do_id, string &strResult);
    void notifyInsertion(DataObjectRef dObj);
    void onConfig(const Metadata& m);
    virtual string getPrettyName() = 0;
    void notifyDelete(DataObjectRef dObj);
};

class CacheUtilityPurgerRelTTL : public CacheUtilityTimePurger {
#define CACHE_UTIL_PURGER_REL_TTL_NAME "CacheUtilityPurgerRelTTL"
protected:
    double getEvictDate(DataObjectRef& dObj);
public:
    CacheUtilityPurgerRelTTL(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityTimePurger(_dataManager, _globalOptimizer, CACHE_UTIL_PURGER_REL_TTL_NAME) {};
        
    string getPrettyName();
    string getShortName() { return string("RTTL"); }
};

class CacheUtilityPurgerAbsTTL : public CacheUtilityTimePurger {
#define CACHE_UTIL_PURGER_ABS_TTL_NAME "CacheUtilityPurgerAbsTTL"

protected:
    double getEvictDate(DataObjectRef& dObj);
public:
    CacheUtilityPurgerAbsTTL(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityTimePurger(_dataManager, _globalOptimizer, CACHE_UTIL_PURGER_ABS_TTL_NAME) {};
   
    string getPrettyName();
    string getShortName() { return string("ATTL"); }
};

class CacheUtilityAttribute : public CacheUtilityFunction {
#define CACHE_UTIL_ATTR_NAME "CacheUtilityAttribute"
private:
    string attr_name;
    double attr_max_value;
    //unsigned long long local_size;
public:
    CacheUtilityAttribute(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_ATTR_NAME),
        attr_name(""), //local_size(0), 
	attr_max_value(1) {};
    ~CacheUtilityAttribute() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    void notifyInsertion(DataObjectRef dObj);
    void notifyDelete(DataObjectRef dObj);
    string getPrettyName() { return string("attr(d)"); };
    string getShortName() { return string("ATTR"); };
};

class CacheUtilityHopCount : public CacheUtilityFunction {
#define CACHE_UTIL_HOP_COUNT_NAME "CacheUtilityHopCount"
private:
    string hc_attr_name;
    string hc_attr_max_name;
    string hc_attr_min_name;
    double geometric_series_constant;
public:
    CacheUtilityHopCount(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_HOP_COUNT_NAME),
        hc_attr_name(""), hc_attr_max_name(""), hc_attr_min_name(""), geometric_series_constant(0) {};
    ~CacheUtilityHopCount() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    void notifyInsertion(DataObjectRef dObj);
    void notifyDelete(DataObjectRef dObj);
    string getPrettyName() { return string("hopcount(d)"); };
    string getShortName() { return string("HOPCNT"); };
};

class CacheUtilityLocal : public CacheUtilityFunction {
#define CACHE_UTIL_LOCAL_NAME "CacheUtilityLocal" /**< \fn Full name for configuration */

private:
    bool localImmunity; /**< This option, if true, will give all locally published content a value of 1.   If false, it is treated as other content. */
    typedef HashMap<string, int> local_map_t;
    local_map_t local_map;  /**< map between DO (which is local) and size */
    //unsigned long long local_size;
public:
    CacheUtilityLocal(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_LOCAL_NAME), //local_size(0), 
	localImmunity(false) {}
    ~CacheUtilityLocal() {}
    double compute(string do_id, string &strResult);
    void onConfig(const Metadata& m);
    void notifyInsertion(DataObjectRef dObj);
    void notifyDelete(DataObjectRef dObj);
    /**
    *  Return a string for the formulaic printing.
    */
    string getPrettyName() { return string("Local(d)"); };
    /**
    *  Return a string for the individual DO value computation.
    */
    string getShortName() { return string("LCL"); };
};

#endif /* _CACHE_UTIL_FUNC_H */
