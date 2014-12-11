/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "EvictStrategyFactory.h"
#include "EvictStrategyLRFU.h"
#include "EvictStrategyLRU_K.h"

EvictStrategy *
EvictStrategyFactory::getNewEvictStrategy(HaggleKernel *kernel, string name)
{
    EvictStrategy *strat = NULL;
    if (name == EVICT_STRAT_LRU_K_NAME) {
        strat = new EvictStrategyLRU_K(kernel);
    }

    if (name == EVICT_STRAT_LRFU_NAME) {
        strat = new EvictStrategyLRFU(kernel);
    }

    if (!strat) {
        HAGGLE_ERR("Could not get evict strategy for name: %s\n", name.c_str());
    }

    return strat;
}
