/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _CACHE_STRAT_H
#define _CACHE_STRAT_H

class CacheStrategy;

#include "DataManager.h"
#include "Metadata.h"

class CacheStrategy : public ManagerModule<DataManager> {
public:
    CacheStrategy(DataManager *m, const string name);
    virtual ~CacheStrategy() {};
    virtual void onConfig(const Metadata& m);
    virtual bool isResponsibleForDataObject(DataObjectRef &dObj);
    virtual void handleNewDataObject(DataObjectRef &dObj);
    virtual void handleSendSuccess(DataObjectRef &dObj, NodeRef &node);
    virtual void handleDeletedDataObject(DataObjectRef &dObj);
    virtual void printDebug();
    virtual void getCacheStrategyAsMetadata(Metadata *m); // CBMEN, HL
    virtual void shutdown() {};
    virtual void quit();
    virtual bool isDone() { return false; };
};

#endif /* _CACHE_STRAT_H */
