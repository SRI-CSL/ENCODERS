/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM, JLM)
 */

#include "CacheGlobalOptimizerFactory.h"

CacheReplGlobalOptimizer *
CacheReplGlobalOptimizerFactory::getNewGlobalOptimizer(
    HaggleKernel *_kernel,
    string name)
{
    CacheReplGlobalOptimizer *globalOptimizer = NULL;

    if (name == "" || name == CACHE_REPL_FIXED_WEGHTS_NAME) {
        globalOptimizer = new CacheReplGlobalOptimizerFixedWeights(_kernel);
    }

    if (!globalOptimizer) {
        HAGGLE_ERR("Could not instantiate global optimizer.\n");
    }
    else {
        HAGGLE_DBG("Instantiated global optimizer: %s\n", name.c_str());
    }

    return globalOptimizer;
}
