/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _CACHE_PURGER_PARALLEL_H
#define _CACHE_PURGER_PARALLEL_H

class CachePurgerParallel;

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"
#include "CachePurger.h"

#define CACHE_PURGER_PARALLEL_NAME "CachePurgerParallel"

class CachePurgerParallel : public CachePurger {
private:
    List<CachePurger *> purgerList;
public:
    CachePurgerParallel(DataManager *m = NULL);

    ~CachePurgerParallel();

    void onConfig(const Metadata& m);

    bool isResponsibleForDataObject(DataObjectRef &dObj);

    void schedulePurge(DataObjectRef &dObj);

    void initializationPurge();
};

#endif /* _CACHE_PURGER_PARALLEL_H */
