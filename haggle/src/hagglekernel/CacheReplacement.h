/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Jihwa Lee (JL)
 */

#ifndef _CACHE_REPLACEMENT_H
#define _CACHE_REPLACEMENT_H

class CacheReplacement;

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"

class CacheReplacement : public ManagerModule<DataManager> {
public:
    CacheReplacement(
        DataManager *m = NULL, 
        const string name = "Base replacement module");

    ~CacheReplacement();

    virtual void onConfig(const Metadata& m);

    virtual bool isResponsibleForDataObject(DataObjectRef &dObj);

    // SCPN : added bool &isDatabaseTimeout which indicates database timeout.
    // 'true' means database timeout and we should drop the new object for order consistency.
    virtual void getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout);
};

#endif /* _CACHE_REPLACEMENT_H */
