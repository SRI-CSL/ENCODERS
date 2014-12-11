/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM, JLM)
 */

#ifndef _CACHE_REPL_GLOBAL_OPT_H
#define _CACHE_REPL_GLOBAL_OPT_H

#include "HaggleKernel.h"
#include "CacheGlobalOptimizer.h"

class CacheReplGlobalOptimizer : public CacheGlobalOptimizer {
private:
    HaggleKernel *kernel;
    string name;
public:
    CacheReplGlobalOptimizer(HaggleKernel *_kernel, string _name);
    virtual ~CacheReplGlobalOptimizer() {};
/*    virtual double getMinimumThreshold() = 0;
    virtual double getWeightForName(string name) = 0;
    virtual void onConfig(const Metadata& m) {};
    string getName() { return name; }
    virtual string getPrettyName() { return name; }
    HaggleKernel *getKernel() { return kernel; } */
};

#define CACHE_REPL_FIXED_WEGHTS_NAME "CacheReplGlobalOptimizerFixedWeights"
#define CACHE_REPL_DEFAULT_THRESHOLD 0
class CacheReplGlobalOptimizerFixedWeights : public CacheReplGlobalOptimizer {
private:
    double minThreshold;
    typedef Map<string, double> weight_registry_t;
    weight_registry_t weights;
public:
    CacheReplGlobalOptimizerFixedWeights(HaggleKernel *_kernel);
    ~CacheReplGlobalOptimizerFixedWeights();
    double getMinimumThreshold();
    double getWeightForName(string name);
    void onConfig(const Metadata& m);
    virtual string getPrettyName();
};

#endif /* _CACHE_REPL_GLOBAL_OPT_H */
