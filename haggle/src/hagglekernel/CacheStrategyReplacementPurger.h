/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _CACHE_STRAT_REP_PURGE_H
#define _CACHE_STRAT_REP_PURGE_H

class CacheStrategyReplacementPurger;

#include "CacheStrategyAsynchronous.h"
#include "CacheReplacement.h"
#include "CachePurger.h"
#include "DataManager.h"
#include "Metadata.h"

#define CACHE_STRAT_REP_PURGE_NAME "CacheStrategyReplacementPurger"

class CacheStrategyReplacementPurger : public CacheStrategyAsynchronous {
private:
    CacheReplacement *cacheReplacement;
    CachePurger *cachePurger;
    bool keep_in_bloomfilter;
public:
    bool isResponsibleForDataObject(DataObjectRef &dObj);
    CacheStrategyReplacementPurger(
        DataManager *m,
        CacheReplacement *_cacheReplacement = NULL,
        CachePurger *_cachePurger = NULL,
        bool _keep_in_bloomfilter = false); // MOS - changed to false for consistency with purger
    ~CacheStrategyReplacementPurger();
protected:
    void _onConfig(const Metadata& m);
    void _handleNewDataObject(DataObjectRef &dObj);
};

#endif /* _CACHE_STRAT_REP_PURGE */
