/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM, JLM)
 */

#include "CacheReplGlobalOptimizer.h"
#include "CacheGlobalOptimizer.h"

CacheReplGlobalOptimizer::CacheReplGlobalOptimizer(
    HaggleKernel *_kernel,
    string _name) :
        kernel(_kernel),
        name(_name)
{
}

CacheReplGlobalOptimizerFixedWeights::CacheReplGlobalOptimizerFixedWeights(
    HaggleKernel *_kernel) :
        CacheReplGlobalOptimizer(_kernel, CACHE_GLOBAL_OPT_FIXED_WEGHTS_NAME),
        minThreshold(CACHE_GLOBAL_OPT_DEFAULT_THRESHOLD)
{
}

/* JM
double
CacheReplGlobalOptimizerFixedWeights::getMinimumThreshold()
{
    return minThreshold;
}

double 
CacheReplGlobalOptimizerFixedWeights::getWeightForName(string name)
{
    weight_registry_t::const_iterator it; 
    it = weights.find(name);
    if (it == weights.end()) {
        HAGGLE_ERR("No weight for name: %s\n", name.c_str());
        return 0;
    }
    return (*it).second;
}

void 
CacheReplGlobalOptimizerFixedWeights::onConfig(const Metadata& m)
{
    const char *param;
    const Metadata *dm;

    param = m.getParameter("min_utility_threshold");
    if (param) {
        minThreshold = atof(param);
    }

    if (minThreshold < 0 || minThreshold > 1) {
        HAGGLE_ERR("Invalid threshold: %.2f\n", minThreshold);
        return;
    }

    int i = 0;
    while ((dm = m.getMetadata("Factor", i++))) {
        param = dm->getParameter("name");
        if (!param) {
            HAGGLE_ERR("factor does not have a name.\n");
            continue;
        }
        string name = string(param);
        param = dm->getParameter("weight");
        if (!param) {
            HAGGLE_ERR("factor does not have weight.\n");
            continue;
        }}

        double weight = atof(param);

        if (weight < -1 || weight > 1) {
            HAGGLE_ERR("factor has invalid weight: %.2f\n", weight);
            continue;
        }

        weights.insert(make_pair(name, weight)); 
        HAGGLE_DBG("Adding fixed weight factor: %s, %.2f\n", name.c_str(), weight);
    }

    HAGGLE_DBG("Successfully initialized global optimizer, min threshold: %.2f,  %d factors.\n", minThreshold, i);
}

CacheReplGlobalOptimizerFixedWeights::~CacheReplGlobalOptimizerFixedWeights()
{
    while (!weights.empty()) {
        weight_registry_t::iterator it = weights.begin();
        weights.erase(it);
    }
}

string
CacheReplGlobalOptimizerFixedWeights::getPrettyName()
{
    string pretty;
    stringprintf(pretty, "%s, threshold: %.2f\n\tfactors: [", getName().c_str(),  minThreshold);
    for (weight_registry_t::const_iterator it = weights.begin();
         it != weights.end();
         it++)
    {
        string factor;
        stringprintf(factor, "(%s, %.2f)", (*it).first.c_str(), (*it).second);
        pretty = pretty + factor;
    }
    pretty = pretty + string("]");
    return pretty;
}
*/
