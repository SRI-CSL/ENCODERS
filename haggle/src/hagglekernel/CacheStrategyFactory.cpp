/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CacheStrategyFactory.h"

#include "CacheStrategyReplacementPurger.h"
#include "CacheStrategyUtility.h"

CacheStrategy *
CacheStrategyFactory::getNewCacheStrategy(
    DataManager *dm,
    string name)
{
    CacheStrategy *cacheStrat = NULL;

    if (name == CACHE_STRAT_REP_PURGE_NAME) {
        cacheStrat = new CacheStrategyReplacementPurger(dm);
        if (!cacheStrat) {
            HAGGLE_ERR("Could not instantiate replacement purger\n");
            return NULL;
        }
    }

    if (name == CACHE_STRAT_UTILITY_NAME) {
        cacheStrat = new CacheStrategyUtility(dm);
        if (!cacheStrat) {
            HAGGLE_ERR("Could not instantiate utility\n");
            return NULL;
        }
    }

    return cacheStrat;
}
