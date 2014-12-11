/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Mark-Oliver Stehr (MOS)
 *   Jihwa Lee (JL)
 */

#include "CacheStrategyReplacementPurger.h"
#include "CacheReplacementFactory.h"
#include "CachePurgerFactory.h"

CacheStrategyReplacementPurger::CacheStrategyReplacementPurger(
    DataManager *m,
    CacheReplacement *_cacheReplacement,
    CachePurger *_cachePurger,
    bool _keep_in_bloomfilter) :
        CacheStrategyAsynchronous(m, CACHE_STRAT_REP_PURGE_NAME),
        cacheReplacement(_cacheReplacement),
        cachePurger(_cachePurger),
        keep_in_bloomfilter(_keep_in_bloomfilter)
{
    if (cachePurger) {
        cachePurger->initializationPurge();
    }
}

CacheStrategyReplacementPurger::~CacheStrategyReplacementPurger() 
{
    if (cacheReplacement) {
        delete cacheReplacement;
    }

    if (cachePurger) {
        delete cachePurger;
    }
}

void
CacheStrategyReplacementPurger::_onConfig(
    const Metadata& m)
{
    const char *param;
    const Metadata *dm;

    if (m.getName() != getName()) {
        HAGGLE_ERR("Wrong config.\n");
        return;
    }

    if (cachePurger || cacheReplacement) {
        HAGGLE_ERR("Already configured!\n");
        return;
    }

    // init purger

    param = m.getParameter("purger");
    if (!param) {
        HAGGLE_ERR("No purger specified for replacement purger\n");
        return;
    }

    cachePurger = CachePurgerFactory::getNewCachePurger(getManager(), param);

    if (!cachePurger) {
        HAGGLE_ERR("Could not instantiate purger\n");
        return;
    }

    dm = m.getMetadata(param);
    if (dm) {
        cachePurger->onConfig(*dm);
    }

    cachePurger->initializationPurge();

    // init replacement

    param = m.getParameter("replacement");
    if (!param) {
        HAGGLE_ERR("No replacement specified for replacement purger\n");
        return;
    }

    cacheReplacement = CacheReplacementFactory::getNewCacheReplacement(getManager(), param);

    if (!cacheReplacement) {
        HAGGLE_ERR("Could not instantiate replacement\n");
        return;
    }

    dm = m.getMetadata(param);
    if (dm) {
        cacheReplacement->onConfig(*dm);
    }

    param = m.getParameter("keep_in_bloomfilter");
    if (param) {
        keep_in_bloomfilter = (0 == strcmp("true", param));
    }

    HAGGLE_DBG("Successfully initialized replacement purger cache strategy.\n");
}

bool 
CacheStrategyReplacementPurger::isResponsibleForDataObject(
    DataObjectRef &dObj)
{
    if (cacheReplacement && cacheReplacement->isResponsibleForDataObject(dObj)) {
        return true;
    }

    if (cachePurger && cachePurger->isResponsibleForDataObject(dObj)) {
        return true;
    }

    return false;
}

void
CacheStrategyReplacementPurger::_handleNewDataObject(
    DataObjectRef &dObj)
{
    int stored = false;

    bool replacementResponsible = false;
    // sequentially check cache replacement 
    if (cacheReplacement 
        && cacheReplacement->isResponsibleForDataObject(dObj))
    { 
        replacementResponsible = true;

        DataObjectRefList subsumed = DataObjectRefList();
        DataObjectRefList equiv = DataObjectRefList();
        DataObjectRefList nonsubsumed = DataObjectRefList();

        bool isDatabaseTimeout = false;
        cacheReplacement->getOrganizedDataObjectsByNewDataObject(
            dObj,
            &subsumed,
            &equiv,
            &nonsubsumed,
            isDatabaseTimeout);

        if (isDatabaseTimeout && getManager()->isDropNewObjectWhenDatabaseTimeout())
        {
            // Do nothing:
            // When database timeout, just drop the new object.
            // Do NOT insert it into database to keep order consistency.
            HAGGLE_ERR("Database timeout: Drop the new object\n");
        }
        else
        {
            if ((subsumed.size() > 0 || equiv.size() > 0) ||
                (subsumed.size() == 0 && equiv.size() == 0 && nonsubsumed.size() == 0)) {

                for (DataObjectRefList::iterator it = subsumed.begin(); it != subsumed.end(); it++) {
                    getManager()->getKernel()->getDataStore()->deleteDataObject(*it, keep_in_bloomfilter);
                }

                getManager()->insertDataObjectIntoDataStore(dObj);
                stored = true;
            }
        }
    }

    // schedule purges, (logically) in parallel
    if (cachePurger
        && cachePurger->isResponsibleForDataObject(dObj)) 
    {
        if (!replacementResponsible) {
            getManager()->insertDataObjectIntoDataStore(dObj);
            cachePurger->schedulePurge(dObj);
        }
        else if (stored) {
            cachePurger->schedulePurge(dObj);
        }
    }
}
