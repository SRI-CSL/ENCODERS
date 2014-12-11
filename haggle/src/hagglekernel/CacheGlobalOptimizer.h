/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _CACHE_GLOBAL_OPT_H
#define _CACHE_GLOBAL_OPT_H

#include "HaggleKernel.h"

class CacheGlobalOptimizer {
private:
    HaggleKernel *kernel;
    string name;
public:
    CacheGlobalOptimizer(HaggleKernel *_kernel, string _name);
    virtual ~CacheGlobalOptimizer() {};
    virtual double getMinimumThreshold() = 0;
    virtual double getWeightForName(string name) = 0;
    virtual void onConfig(const Metadata& m) {};
    virtual string getName() { return name; }
    virtual string getPrettyName() { return name; }
    virtual HaggleKernel *getKernel() { return kernel; }
};

#define CACHE_GLOBAL_OPT_FIXED_WEGHTS_NAME "CacheGlobalOptimizerFixedWeights"
#define CACHE_GLOBAL_OPT_DEFAULT_THRESHOLD 0
class CacheGlobalOptimizerFixedWeights : public CacheGlobalOptimizer {
private:
    double minThreshold;
    typedef Map<string, double> weight_registry_t;
    weight_registry_t weights;
public:
    CacheGlobalOptimizerFixedWeights(HaggleKernel *_kernel);
    ~CacheGlobalOptimizerFixedWeights();
    double getMinimumThreshold();
    double getWeightForName(string name);
    void onConfig(const Metadata& m);
    virtual string getPrettyName();
};

#endif /* _CACHE_GLOBAL_OPT_H */
