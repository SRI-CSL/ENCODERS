/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _CACHE_PURGER_FACTORY_H
#define _CACHE_PURGER_FACTORY_H

class CachePurgerFactory;

#include "DataManager.h"
#include "Metadata.h"
#include "CachePurger.h"

class CachePurgerFactory {
public:
    static CachePurger *getNewCachePurger(
        DataManager *dm, 
        const string name);
};

#endif /* _CACHE_PURGER_FACTORY_H */
