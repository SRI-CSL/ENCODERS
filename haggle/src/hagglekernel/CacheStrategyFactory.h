/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _CACHE_STRAT_FACTORY_H
#define _CACHE_STRAT_FACTORY_H

class CacheStrategyFactory;

#include "DataManager.h"
#include "Metadata.h"
#include "CacheStrategy.h"

class CacheStrategyFactory {
public:
    static CacheStrategy *getNewCacheStrategy(
        DataManager *dm, 
        string name);
};

#endif /* _CACHE_STRAT_FACTORY_H */
