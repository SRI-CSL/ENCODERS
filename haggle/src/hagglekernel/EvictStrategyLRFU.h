/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _EVICT_STRAT_LRFU_H
#define _EVICT_STRAT_LRFU_H

#include <math.h>
#include "DataManager.h"
#include "Metadata.h"
#include "EvictStrategy.h"

#define EVICT_STRAT_LRFU_NAME "LRFU"

#define EVICT_STRAT_LRFU_COUNT_TYPE_TIME "TIME"
#define EVICT_STRAT_LRFU_COUNT_TYPE_VIRTUAL "VIRTUAL"

class EvictStrategyLRFU : public EvictStrategy {
private:
    double pValue;
    double lambda;
    string countType;
protected:
    double fx_calc(double value);
public:
    EvictStrategyLRFU(HaggleKernel *kernel) :
        EvictStrategy(kernel, EVICT_STRAT_LRFU_NAME),
        pValue(0),
        lambda(0),
        countType("") {};
    ~EvictStrategyLRFU() {};
    void onConfig(const Metadata* m);
    void updateInfoDataObject(DataObjectRef &dObj, unsigned int count, Timeval time);
    double getEvictValueForDataObject(DataObjectRef &dObj);
};

#endif /* _EVICT_STRAT_LRFU_H */
