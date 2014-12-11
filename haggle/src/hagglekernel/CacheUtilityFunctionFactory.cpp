/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "CacheUtilityFunctionFactory.h"

CacheUtilityFunction *
CacheUtilityFunctionFactory::getNewUtilityFunction(
    DataManager *_dataManager,
    CacheGlobalOptimizer *_globalOptimizer,
    string name)
{
    CacheUtilityFunction *utilFunction = NULL;

    if (name == "" || name == CACHE_UTIL_AGGREGATE_SUM_NAME) {
        utilFunction = new CacheUtilityAggregateSum(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_AGGREGATE_MIN_NAME) {
        utilFunction = new CacheUtilityAggregateMin(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_AGGREGATE_MAX_NAME) {
        utilFunction = new CacheUtilityAggregateMax(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_POPULARITY_NAME) {
        utilFunction = new CacheUtilityPopularity(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NEIGHBORHOOD_NAME) {
        utilFunction = new CacheUtilityNeighborhood(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NEIGHBORHOOD_SOCIAL_NAME) {
        utilFunction = new CacheUtilityNeighborhoodSocial(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME) {
        utilFunction = new CacheUtilityNeighborhoodOtherSocial(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_RANDOM_NAME) {
        utilFunction = new CacheUtilityRandom(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_NEW_DO_TIME_WINDOW_IMMUNITY_NAME) {
        utilFunction = new CacheUtilityImmunityNewByTime(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NOP_NAME) {
        utilFunction = new CacheUtilityNOP(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_LOCAL_NAME) {
        utilFunction = new CacheUtilityLocal(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_REPLACEMENT_PRIORITY_NAME) {
        utilFunction = new CacheUtilityReplacementPriority(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_REPLACEMENT_TO_NAME) {
        utilFunction = new CacheUtilityReplacementTotalOrder(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_PURGER_REL_TTL_NAME) {
        utilFunction = new CacheUtilityPurgerRelTTL(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_PURGER_ABS_TTL_NAME) {
        utilFunction = new CacheUtilityPurgerAbsTTL(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_ATTR_NAME) {
        utilFunction = new CacheUtilityAttribute(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_SECURE_CODING_NAME) {
        utilFunction = new CacheUtilitySecureCoding(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_HOP_COUNT_NAME) {
        utilFunction = new CacheUtilityHopCount(_dataManager, _globalOptimizer);
    }

    if (!utilFunction) {
        HAGGLE_ERR("Could not instantiate utility function: %s\n", name.c_str());
    }
    else {
        HAGGLE_DBG("Instantiated utility function: %s\n", name.c_str());
    }

    return utilFunction;
}

/* CacheUtilityFunction *
CacheReplUtilityFunctionFactory::getNewUtilityFunction(
    DataManager *_dataManager,
    CacheGlobalOptimizer *_globalOptimizer,
    string name)
{
    CacheUtilityFunction *utilFunction = NULL;

    if (name == "" || name == CACHE_UTIL_AGGREGATE_SUM_NAME) {
        utilFunction = new CacheUtilityAggregateSum(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_AGGREGATE_MIN_NAME) {
        utilFunction = new CacheUtilityAggregateMin(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_AGGREGATE_MAX_NAME) {
        utilFunction = new CacheUtilityAggregateMax(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_POPULARITY_NAME) {
        utilFunction = new CacheUtilityPopularity(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NEIGHBORHOOD_NAME) {
        utilFunction = new CacheUtilityNeighborhood(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NEIGHBORHOOD_SOCIAL_NAME) {
        utilFunction = new CacheUtilityNeighborhoodSocial(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME) {
        utilFunction = new CacheUtilityNeighborhoodOtherSocial(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_RANDOM_NAME) {
        utilFunction = new CacheUtilityRandom(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_NEW_DO_TIME_WINDOW_IMMUNITY_NAME) {
        utilFunction = new CacheUtilityImmunityNewByTime(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_NOP_NAME) {
        utilFunction = new CacheUtilityNOP(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_LOCAL_NAME) {
        utilFunction = new CacheUtilityLocal(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_REPLACEMENT_PRIORITY_NAME) {
        utilFunction = new CacheUtilityReplacementPriority(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_PURGER_globalReplOptimizerCacheReplGlobalOptimizerFactoryREL_TTL_NAME) {re [-Wreorder]
CacheUtilityFunction.cpp: In member function `virtual double Ca
        utilFunction = new CacheUtilityPurgerRelTTL(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_PURGER_ABS_TTL_NAME) {
        utilFunction = new CacheUtilityPurgerAbsTTL(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_ATTR_NAME) {
        utilFunction = new CacheUtilityAttribute(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_SECURE_CODING_NAME) {
        utilFunction = new CacheUtilitySecureCoding(_dataManager, _globalOptimizer);
    }

    if (name == CACHE_UTIL_HOP_COUNT_NAME) {
        utilFunction = new CacheUtilityHopCount(_dataManager, _globalOptimizer);
    }

    if (!utilFunction) {
        HAGGLE_ERR("Could not instantiate Repl utility function: %s\n", name.c_str());
    }
    else {
        HAGGLE_DBG("Instantiated Repl utility function: %s\n", name.c_str());
    }

    return utilFunction;
} */

