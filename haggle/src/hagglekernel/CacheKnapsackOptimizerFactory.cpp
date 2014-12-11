/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CacheKnapsackOptimizerFactory.h"

CacheKnapsackOptimizer *
CacheKnapsackOptimizerFactory::getNewKnapsackOptimizer(
    HaggleKernel *_kernel,
    string name)
{
    CacheKnapsackOptimizer *ksOptimizer = NULL;

    if (name == "" || name == CACHE_KNAPSACK_GREEDY_NAME) {
        ksOptimizer = new CacheKnapsackOptimizerGreedy();
    }

    if (!ksOptimizer) {
        HAGGLE_ERR("Could not instantiate knapsack optimizer.\n");
    }
    else {
        HAGGLE_DBG("Initialized knapsack optimzer: %s\n", name.c_str());
    }

    return ksOptimizer;
}
