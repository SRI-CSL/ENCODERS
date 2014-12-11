/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _CACHE_GLOBAL_OPT_FACTORY_H
#define _CACHE_GLOBAL_OPT_FACTORY_H

class CacheGlobalOptimizerFactory;
class CacheReplGlobalOptimizerFactory;

#include "CacheGlobalOptimizer.h"

class CacheGlobalOptimizerFactory {
public:
    static CacheGlobalOptimizer *getNewGlobalOptimizer(
        HaggleKernel *kernel,
        string name = string(""));
};


//class CacheReplGlobalOptimizerFactory : public CacheGlobalOptimizerFactory{};

#endif /* _CACHE_GLOBAL_OPT_FACTORY_H */
