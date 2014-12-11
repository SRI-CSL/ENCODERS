/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_UTIL_FUNC_FACTORY_H
#define _REPL_UTIL_FUNC_FACTORY_H

/**
 * This factory is responsible for instantiating `ReplicationUtilityFunction`
 * objects.
 */
class ReplicationUtilityFunctionFactory;

#include "ReplicationManager.h"
#include "ReplicationUtilityFunction.h"
#include "ReplicationGlobalOptimizer.h"

/**
 * Object used to instantiate replication utiltiy functions.
 */
class ReplicationUtilityFunctionFactory {

public:

    /**
     * Construct and instantiate a ReplicationUtilityFunction given the 
     * manager, global optimzer, and the name.
     * @return The instantiated ReplicationUtilityFunction.
     */
    static ReplicationUtilityFunction *getNewUtilityFunction(
        ReplicationManager *manager /**< The replication manager used by the utility function. */,
        ReplicationGlobalOptimizer *globalOptimizer /** The global optimizer used by the utiltiy function. */,
        string name = string("")) /**< The name of the utility function class to instantiate. */;
};

#endif /* _REPL_UTIL_FUNC_FACTORY_H */
