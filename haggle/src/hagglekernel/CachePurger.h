/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _CACHE_PURGER_H
#define _CACHE_PURGER_H

class CachePurger;

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"

class CachePurger : public ManagerModule<DataManager> {
public:
    CachePurger(
        DataManager *m = NULL, 
        const string name = "Base purger module");

    ~CachePurger();

    virtual void onConfig(const Metadata& m);

    virtual bool isResponsibleForDataObject(DataObjectRef &dObj);

    virtual void schedulePurge(DataObjectRef &dObj);

    virtual void initializationPurge();
};

#endif /* _CACHE_PURGER_H */
