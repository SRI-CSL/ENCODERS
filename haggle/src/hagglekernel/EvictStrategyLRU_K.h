/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _EVICT_STRATEGY_LRU_K_H
#define _EVICT_STRATEGY_LRU_K_H

#include "DataManager.h"
#include "Metadata.h"

#include "EvictStrategy.h"

#define EVICT_STRAT_LRU_K_NAME "LRU_K"
#define EVICT_STRAT_LRU_K_COUNT_TYPE_TIME "TIME"
#define EVICT_STRAT_LRU_K_COUNT_TYPE_VIRTUAL "VIRTUAL"

class EvictStrategyLRU_K : public EvictStrategy {
private:
    unsigned int k;
    string countType;
public:
    EvictStrategyLRU_K(HaggleKernel *kernel) :
        EvictStrategy(kernel, string(EVICT_STRAT_LRU_K_NAME)),
        k(0),
        countType("") {};
    ~EvictStrategyLRU_K() {};
    void onConfig(const Metadata* m);
    void updateInfoDataObject(DataObjectRef &dObj, unsigned int count, Timeval time);
    double getEvictValueForDataObject(DataObjectRef &dObj);
};

#endif /* _EVICT_STRATEGY_LRU_K_H */
