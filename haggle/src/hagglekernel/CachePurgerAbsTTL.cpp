/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CachePurgerAbsTTL.h"

class CachePurgerAbsTTLCallback : public EventHandler
{
private:
    DataManager *manager;
    HaggleKernel *kernel;
    CachePurgerAbsTTL *purger;
public:
    CachePurgerAbsTTLCallback(
        DataManager *_manager,
        HaggleKernel *_kernel,
        CachePurgerAbsTTL *_purger) :
            manager(_manager),
            kernel(_kernel),
            purger(_purger)
    {
    }

    ~CachePurgerAbsTTLCallback() 
    {
    }

    void deleteCallback(Event *e) 
    {
        if (!e || !purger) {
            HAGGLE_ERR("No event to delete callback\n");
            return;
        }
        
        DataObjectRef dObj = e->getDataObject();
        if (!dObj) {
            HAGGLE_ERR("No data object\n");
            return;
        }

        kernel->getDataStore()->deleteDataObject(dObj, purger->getKeepInBloomfilter());
    }

    void initializationCallback(Event *e) 
    {
        DataObjectRefList dObjs = e->getDataObjectList();
        
        for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
            DataObjectRef dObj = *it;
            if (!purger->isResponsibleForDataObject(dObj)) {
                continue;
            }
            double now = (Timeval::now()).getTimeAsSecondsDouble();
		    const Attribute *attr = dObj->getAttribute(purger->getMetricField(), "*", 1);
            double ttl = atof(attr->getValue().c_str()); 

            if (now < (ttl + purger->getMinDBTimeS())) {
                purger->schedulePurge(dObj);
            } 
            else {
                kernel->getDataStore()->deleteDataObject(dObj, purger->getKeepInBloomfilter());
            }
        }
    }
};

CachePurgerAbsTTL::CachePurgerAbsTTL(DataManager *m) :
    CachePurger(m, CACHE_PURGER_ABS_TTL_NAME),
    keepInBF(CACHE_PURGER_ABS_TTL_KEEP_IN_BF),
    minDBtimeS(CACHE_PURGER_ABS_TTL_MIN_DB_S),
    metricField(""),
    tagField(""),
    tagFieldValue(""),
    purgerCallback(NULL),
    initCallback(NULL),
    deleteCallback(NULL)
{
    purgerCallback = new CachePurgerAbsTTLCallback(getManager(), getKernel(), this);

    initCallback = (EventCallback<EventHandler> *)
        new EventCallback <CachePurgerAbsTTLCallback>(
            purgerCallback,
            &CachePurgerAbsTTLCallback::initializationCallback);

    deleteCallback = (EventCallback<EventHandler> *)
        new EventCallback <CachePurgerAbsTTLCallback>(
            purgerCallback,
            &CachePurgerAbsTTLCallback::deleteCallback);
}

CachePurgerAbsTTL::~CachePurgerAbsTTL() 
{
    if (purgerCallback) {
        delete purgerCallback;
    }

    if (initCallback) {
        delete initCallback;
    }

    if (deleteCallback) {
        delete deleteCallback;
    }
}

void 
CachePurgerAbsTTL::onConfig(
    const Metadata& m)
{
    if (m.getName() != getName()) {
        HAGGLE_ERR("Wrong config.\n");
        return;
    }

    const char *param;

    param = m.getParameter(CACHE_PURGER_ABS_TTL_METRIC);
    if (!param) {
        HAGGLE_ERR("No metric specified\n");
        return;
    }

    metricField = string(param);

    param = m.getParameter(CACHE_PURGER_ABS_TTL_TAG);
    if (!param) {
        HAGGLE_ERR("No tag specified\n");
        return;
    }

    tagField = string(param);

    param = m.getParameter(CACHE_PURGER_ABS_TTL_TAG_VALUE);
    if (!param) {
        HAGGLE_ERR("No tag value specified\n");
        return;
    }

    tagFieldValue = string(param);

    param = m.getParameter(CACHE_PURGER_ABS_TTL_KEEP_IN_BF_NAME);
    if (param) {
        if (0 == strcmp("true", param)) {
            keepInBF = true;
        }
        else if (0 == strcmp("false", param)) {
            keepInBF = false;
        }
        else {
            HAGGLE_ERR("Field must be true or false\n");
        }
    }

    param = m.getParameter(CACHE_PURGER_ABS_TTL_MIN_DB_TIME_S);
    if (param) {
        minDBtimeS = atof(param);
    }

    HAGGLE_DBG("Loaded absolute TTL purger with metric: %s, tag=%s:%s, keep in bf: %s, min db time: %f\n", 
        metricField.c_str(), tagField.c_str(), tagFieldValue.c_str(), keepInBF ? "true" : "false", minDBtimeS);
}

bool 
CachePurgerAbsTTL::isResponsibleForDataObject(
    DataObjectRef &dObj) 
{
    if (!dObj) {
        return false;
    }

    const Attribute *attr;
    attr = dObj->getAttribute (tagField, tagFieldValue, 1);
    
    if (!attr) {
        return false;
    }

    attr = dObj->getAttribute (metricField, "*", 1);

    if (!attr) {
        return false;
    }

    return true; 
}

void
CachePurgerAbsTTL::schedulePurge(
    DataObjectRef &dObj) 
{
    if (!isResponsibleForDataObject(dObj)) {
        HAGGLE_ERR("Cannot schedule purge for DO not responsible for\n");
        return;
    }

    const Attribute *attr = dObj->getAttribute(metricField, "*", 1);
    if (!attr) {
        HAGGLE_ERR("Attribute not specified\n");
        return;
    }

    double now = (Timeval::now()).getTimeAsSecondsDouble();
    double ttl = atof(attr->getValue().c_str());
    
    double expTime = ttl - now;
    expTime = expTime > minDBtimeS ? expTime : minDBtimeS;

    getKernel()->addEvent(new Event(deleteCallback, dObj, expTime));
}

void 
CachePurgerAbsTTL::initializationPurge()
{
    getKernel()->getDataStore()->
        doDataObjectQueryForTimedDelete(tagField.c_str(), tagFieldValue.c_str(), initCallback);
}
