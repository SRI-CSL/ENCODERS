/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CacheGlobalOptimizerFactory.h"

CacheGlobalOptimizer *
CacheGlobalOptimizerFactory::getNewGlobalOptimizer(
    HaggleKernel *_kernel,
    string name)
{
    CacheGlobalOptimizer *globalOptimizer = NULL;

    if (name == "" || name == CACHE_GLOBAL_OPT_FIXED_WEGHTS_NAME) {
        globalOptimizer = new CacheGlobalOptimizerFixedWeights(_kernel);
    }

    if (!globalOptimizer) {
        HAGGLE_ERR("Could not instantiate global optimizer.\n");
    }
    else {
        HAGGLE_DBG("Instantiated global optimizer: %s\n", name.c_str());
    }

    return globalOptimizer;
}
