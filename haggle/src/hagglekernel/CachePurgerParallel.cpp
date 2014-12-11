/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CachePurgerParallel.h"

#include "CachePurgerFactory.h"

CachePurgerParallel::CachePurgerParallel(
    DataManager *m) :
        CachePurger(m, CACHE_PURGER_PARALLEL_NAME)
{
}

CachePurgerParallel::~CachePurgerParallel()
{
    while (!purgerList.empty()) {
        CachePurger *purger = purgerList.front();
        purgerList.pop_front();
        delete purger;
    }
}

void 
CachePurgerParallel::onConfig(
    const Metadata& m)
{
    if (m.getName() != getName()) {
        HAGGLE_ERR("Wrong config.\n");
        return;
    }

    int i = 0;
    const char *param = NULL;
    const Metadata *purgerConfig;
    CachePurger *purger;
    while (NULL != (purgerConfig = m.getMetadata("CachePurger", i++))) {
        param = purgerConfig->getParameter("name");
        if (!param) {
            HAGGLE_ERR("No purger class specified\n");
            return;
        }

        purger = CachePurgerFactory::getNewCachePurger(getManager(), string(param));

        if (!purger) {
            HAGGLE_ERR("Could not instantiate replacement\n");
            return;
        }

        const Metadata *options = purgerConfig->getMetadata(param);
        if (!options) {
            delete purger;
            HAGGLE_ERR("Could not get configuration options for purger\n");
            return;
        }
        
        purger->onConfig(*options);
        purgerList.push_front(purger);
    }
}

bool 
CachePurgerParallel::isResponsibleForDataObject(
    DataObjectRef &dObj)
{
    List<CachePurger *>::iterator it = purgerList.begin();

    for (; it != purgerList.end(); it++) {
        CachePurger *purger = *it;
        if (purger->isResponsibleForDataObject(dObj)) {
            return true;
        }
    }

    return false; 
}

void
CachePurgerParallel::schedulePurge(
    DataObjectRef &dObj)
{
    List<CachePurger *>::iterator it = purgerList.begin();

    for (; it != purgerList.end(); it++) {
        CachePurger *purger = *it;
        if (purger->isResponsibleForDataObject(dObj)) {
            purger->schedulePurge(dObj);
        }
    }
}

void
CachePurgerParallel::initializationPurge()
{
    List<CachePurger *>::iterator it = purgerList.begin();

    for (; it != purgerList.end(); it++) {
        CachePurger *purger = *it;
        purger->initializationPurge();
    }
}
