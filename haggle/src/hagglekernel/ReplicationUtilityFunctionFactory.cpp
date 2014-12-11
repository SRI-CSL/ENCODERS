/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ReplicationUtilityFunctionFactory.h"

ReplicationUtilityFunction *
ReplicationUtilityFunctionFactory::getNewUtilityFunction(
    ReplicationManager *_manager,
    ReplicationGlobalOptimizer *_globalOptimizer,
    string name)
{
    ReplicationUtilityFunction *utilFunction = NULL;

    if (name == "" || name == REPL_UTIL_AGGREGATE_SUM_NAME) {
        utilFunction = new ReplicationUtilityAggregateSum(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_AGGREGATE_MIN_NAME) {
        utilFunction = new ReplicationUtilityAggregateMin(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_AGGREGATE_MAX_NAME) {
        utilFunction = new ReplicationUtilityAggregateMax(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_RANDOM_NAME) {
        utilFunction = new ReplicationUtilityRandom(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_NOP_NAME) {
        utilFunction = new ReplicationUtilityNOP(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_WAIT_NAME) {
        utilFunction = new ReplicationUtilityWait(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_LOCAL_NAME) {
        utilFunction = new ReplicationUtilityLocal(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_ATTR_NAME) {
        utilFunction = new ReplicationUtilityAttribute(_manager, _globalOptimizer);
    }

    if (name == REPL_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME) {
        utilFunction = new ReplicationUtilityNeighborhoodOtherSocial(_manager, _globalOptimizer);
    }

    if (!utilFunction) {
        HAGGLE_ERR("Could not instantiate utility function: %s\n", name.c_str());
    }
    else {
        HAGGLE_DBG("Instantiated utility function: %s\n", name.c_str());
    }

    return utilFunction;
}
