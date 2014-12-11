/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _CACHE_REPLACEMENT_FACTORY_H
#define _CACHE_REPLACEMENT_FACTORY_H

class CacheReplacementFactory;

#include "DataManager.h"
#include "Metadata.h"
#include "CacheReplacement.h"

class CacheReplacementFactory {
public:
    static CacheReplacement *getNewCacheReplacement(
        DataManager *dm,
        const string name);
};

#endif /* _CACHE_REPLACEMENT_FACTORY_H */
