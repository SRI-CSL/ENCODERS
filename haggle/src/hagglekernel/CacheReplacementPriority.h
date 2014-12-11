/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Jihwa Lee (JL)
 */

#ifndef _CACHE_REPLACEMENT_PRIORITY_H
#define _CACHE_REPLACEMENT_PRIORITY_H

class CacheReplacementPriority;

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"
#include "CacheReplacement.h"

#define CACHE_REPLACEMENT_PRIORITY_NAME "CacheReplacementPriority"
#define CACHE_REPLACEMENT_PRIORITY_FIELD_NAME "priority"
#define CACHE_REPLACEMENT_PRIORITY_NAME_FIELD_NAME "name"

class CacheReplacementPriority : public CacheReplacement {
private:
    List<Pair<CacheReplacement *, int> > sorted_replacement_list;
public:
    CacheReplacementPriority(DataManager *m = NULL);

    ~CacheReplacementPriority();

    void onConfig(const Metadata& m);

    bool isResponsibleForDataObject(DataObjectRef &dObj);

    void getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout);
};

#endif /* _CACHE_REPLACEMENT_PRIORITY_H */
