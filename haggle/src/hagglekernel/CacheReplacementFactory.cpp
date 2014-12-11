/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CacheReplacementFactory.h"
#include "CacheReplacementTotalOrder.h"
#include "CacheReplacementPriority.h"

CacheReplacement *
CacheReplacementFactory::getNewCacheReplacement(
    DataManager *dm,
    const string name)
{
    CacheReplacement *replacement = NULL;
    if (NULL == dm) {
        HAGGLE_ERR("Bad arguments: could not construct replacement\n");
        return replacement;
    }

    if (name == CACHE_REPLACEMENT_TOTAL_ORDER_NAME) {
        replacement = new CacheReplacementTotalOrder(dm);
    }

    if (name == CACHE_REPLACEMENT_PRIORITY_NAME) {
        replacement = new CacheReplacementPriority(dm);
    }
    
    return replacement;
}
