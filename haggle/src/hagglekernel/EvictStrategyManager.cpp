/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "EvictStrategyManager.h"
#include "EvictStrategyFactory.h"

EvictStrategyManager::EvictStrategyManager(
    HaggleKernel *_kernel) :
        kernel(_kernel), 
        defaultStrategyName(""),
        initialized(false), 
        count(1)
{
}

EvictStrategyManager::~EvictStrategyManager()
{
    while (!strategies.empty()) {
        strategies_t::iterator it = strategies.begin();
        EvictStrategy *strat = (*it).second;
        strategies.erase(it);
        delete strat;
    }
}

void
EvictStrategyManager::onConfig(const Metadata& m)
{
    
    const Metadata *dm;
    const char *param;

    bool hasDefault = false;

    param = m.getParameter("default");
    if (param) {
        hasDefault = true;
        defaultStrategyName = string(param);
    }

    int i = 0;
    while ((dm = m.getMetadata("EvictStrategy", i++))) {
        const char *param = dm->getParameter("name");
        if (!param) {
            HAGGLE_ERR("No name specified.\n");
            continue;
        }
        string stratName = string(param);
        param = dm->getParameter("className");
        if (!param) {
            HAGGLE_ERR("No strat name specified.\n");
            continue;
        }
        string className = string(param);
        EvictStrategy *strat =  
            EvictStrategyFactory::getNewEvictStrategy(getKernel(), className);
        if (!strat) {
            HAGGLE_ERR("No strategy for name: %s\n", className.c_str());
            continue;
        }
        strat->onConfig(dm);
        if (!hasDefault) {
            // by default, we take the first specified strat
            defaultStrategyName = stratName;
            hasDefault = true;
        }

        addStrategy(stratName, strat);
    }

    initialized = true;
}

void
EvictStrategyManager::updateDataObject(DataObjectRef dObj)
{

    if (!initialized) {
        HAGGLE_ERR("Evict strategy manager not initialized.\n");
        return;
    }


    Timeval now = Timeval::now();
    strategies_t::iterator it = strategies.begin();
    for (; it != strategies.end(); it++) {
        EvictStrategy *strat = (*it).second;
        if (!strat) {
            HAGGLE_ERR("Strategy is NULL\n");
            continue;
        }


        strat->updateInfoDataObject(dObj, count, now);
    }

    count++;
}

EvictStrategy *
EvictStrategyManager::getDefaultStrategy()
{
    strategies_t::iterator it = strategies.find(defaultStrategyName);
    if (it == strategies.end()) {
        HAGGLE_ERR("No default strategy in cache.\n");
        return NULL;
    }
    return (*it).second;
}

void
EvictStrategyManager::addStrategy(
    string name, 
    EvictStrategy *strat)
{
    strategies_t::iterator it = strategies.find(name);
    if (it != strategies.end()) {
        HAGGLE_ERR("Strategy already registered for name: %s\n", name.c_str());
        return;
    }
    strategies.insert(make_pair(name, strat));
}

double
EvictStrategyManager::getEvictValueForDataObject(
    DataObjectRef dObj, 
    string name)
{
    strategies_t::iterator it = strategies.find(name);
    if (it != strategies.end()) {
        EvictStrategy *strat = (*it).second;
        if (!strat) {
            HAGGLE_ERR("Strat is NULL.\n");
            return -1.0;
        }
        return strat->getEvictValueForDataObject(dObj);
    }

    EvictStrategy *defaultStrategy = getDefaultStrategy();
    if (!defaultStrategy) {
        return -1.0;
    }

    return defaultStrategy->getEvictValueForDataObject(dObj);
}

HaggleKernel *
EvictStrategyManager::getKernel()
{
    return kernel;
}
