/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _EVICT_STRAT_MAN_H
#define _EVICT_STRAT_MAN_H

class EvictStrategyManager;

#include "Metadata.h"
#include "EvictStrategy.h"

class EvictStrategyManager;

class EvictStrategyManager {
private:
    HaggleKernel *kernel;
    string defaultStrategyName;
    bool initialized;
    unsigned int count;
    typedef HashMap<string, EvictStrategy *> strategies_t;
    strategies_t strategies;
protected:
    EvictStrategy *getDefaultStrategy();
    void addStrategy(string name, EvictStrategy *strat);
public:
    EvictStrategyManager(HaggleKernel *_kernel);
    ~EvictStrategyManager();
    void onConfig(const Metadata& m);
    void updateDataObject(DataObjectRef dObj);
    double getEvictValueForDataObject(
        DataObjectRef dObj,
        string name = string(""));
    HaggleKernel *getKernel();
};

#endif /* _EVICT_STRAT_MAN_H */
