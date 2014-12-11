/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CachePurgerFactory.h"
#include "CachePurgerAbsTTL.h"
#include "CachePurgerRelTTL.h"
#include "CachePurgerParallel.h"

CachePurger *
CachePurgerFactory::getNewCachePurger(
    DataManager *dm,
    const string name)
{
    CachePurger *purger = NULL;
    if (NULL == dm) {
        HAGGLE_ERR("Bad arguments: could not construct purger\n");
        return purger;
    }

    if (name == CACHE_PURGER_PARALLEL_NAME) {
        purger = new CachePurgerParallel(dm); 
    }

    if (name == CACHE_PURGER_REL_TTL_NAME) {
        purger = new CachePurgerRelTTL(dm);
    }
    
    if (name == CACHE_PURGER_ABS_TTL_NAME) {
        purger = new CachePurgerAbsTTL(dm);
    }

    return purger;
}
