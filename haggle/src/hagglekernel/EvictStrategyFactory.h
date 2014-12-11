/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _EVICT_STRAT_FACTORY_H
#define _EVICT_STRAT_FACTORY_H

class EvictStrategyFactory;

#include "EvictStrategy.h"

class EvictStrategyFactory {
public:
    static EvictStrategy *getNewEvictStrategy(
        HaggleKernel *kernel,
        string className);
};

#endif /* _EVICT_STRAT_FACTORY_H */
