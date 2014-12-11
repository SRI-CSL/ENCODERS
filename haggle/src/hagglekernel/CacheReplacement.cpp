/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Jihwa Lee (JL)
 */

#include "CacheReplacement.h"

CacheReplacement::CacheReplacement(
    DataManager *m,
    const string name) :
        ManagerModule<DataManager>(m, name)
{
}

CacheReplacement::~CacheReplacement() 
{
}

void 
CacheReplacement::onConfig(
    const Metadata& m)
{
}

bool 
CacheReplacement::isResponsibleForDataObject(
    DataObjectRef &dObj) 
{
    return false; 
}

void
CacheReplacement::getOrganizedDataObjectsByNewDataObject(
    DataObjectRef &dObj,
    DataObjectRefList *o_subsumed,
    DataObjectRefList *o_equiv,
    DataObjectRefList *o_nonsubsumed,
    bool &isDatabaseTimeout)
{
    if (NULL == o_subsumed || NULL == o_equiv || NULL == o_nonsubsumed) {
        HAGGLE_ERR("Null args\n");
        return;
    }
    *o_nonsubsumed = DataObjectRefList();
    *o_equiv = DataObjectRefList();
    *o_nonsubsumed = DataObjectRefList();

    return;
}
