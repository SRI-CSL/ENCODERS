/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _CACHE_UTIL_FUNC_FACTORY_H
#define _CACHE_UTIL_FUNC_FACTORY_H

class CacheUtilityFunctionFactory;

#include "CacheUtilityFunction.h"
#include "CacheGlobalOptimizer.h"
#include "CacheReplGlobalOptimizer.h"

class CacheUtilityFunctionFactory {
public:
    static CacheUtilityFunction *getNewUtilityFunction(
        DataManager *_dataManager,
        CacheGlobalOptimizer *globalOptimizer,
        string name = string(""));
    /*static CacheUtilityFunction *getNewUtilityFunction(
        DataManager *_dataManager,
        CacheReplGlobalOptimizer *globalOptimizer,
        string name = string(""));*/
};

class CacheReplUtilityFunctionFactory: public CacheUtilityFunctionFactory {};
/*
public:
    static CacheUtilityFunction *getNewUtilityFunction(
        DataManager *_dataManager,
        CacheGlobalOptimizer *globalOptimizer,
        string name = string(""));
};*/
#endif /* _CACHE_UTIL_FUNC_FACTORY_H */
