/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _CACHE_KS_OPT_FACTORY_H
#define _CACHE_KS_OPT_FACTORY_H

class CacheKnapsackOptimizerFactory;

#include "CacheKnapsackOptimizer.h"

class CacheKnapsackOptimizerFactory {
public:
    static CacheKnapsackOptimizer *getNewKnapsackOptimizer(
        HaggleKernel *kernel,
        string name = string(""));
};

#endif /* _CACHE_KS_OPT_FACTORY_H */
