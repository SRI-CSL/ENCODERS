/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Mark-Oliver Stehr (MOS)
 *   Jihwa Lee (JL)
 */

#include "CacheReplacementTotalOrder.h"

CacheReplacementTotalOrder::CacheReplacementTotalOrder(
    DataManager *m,
    string _metricField,
    string _idField,
    string _tagField,
    string _tagFieldValue)
   : CacheReplacement(m, CACHE_REPLACEMENT_TOTAL_ORDER_NAME),
    metricField(_metricField),
    idField(_idField),
    tagField(_tagField),
    tagFieldValue(_tagFieldValue)
{
}

CacheReplacementTotalOrder::~CacheReplacementTotalOrder() 
{
}

void 
CacheReplacementTotalOrder::onConfig(
    const Metadata& m)
{
    if (m.getName() != getName()) {
        HAGGLE_ERR("Wrong config.\n");
        return;
    }

    const char *param;

    param = m.getParameter(CACHE_REPLACEMENT_TOTAL_ORDER_METRIC);
    if (!param) {
        HAGGLE_ERR("No metric specified\n");
        return;
    }

    metricField = string(param);

    param = m.getParameter(CACHE_REPLACEMENT_TOTAL_ORDER_ID);
    if (!param) {
        HAGGLE_ERR("No id specified\n");
        return;
    }

    idField = string(param);

    param = m.getParameter(CACHE_REPLACEMENT_TOTAL_ORDER_TAG);
    if (!param) {
        HAGGLE_ERR("No tag specified\n");
        return;
    }

    tagField = string(param);

    param = m.getParameter(CACHE_REPLACEMENT_TOTAL_ORDER_TAG_VALUE);
    if (!param) {
        HAGGLE_ERR("No id specified\n");
        return;
    }

    tagFieldValue = string(param);

    HAGGLE_DBG("Loaded total order replacement with metric: %s, id: %s, tag: %s:%s\n",
        metricField.c_str(), idField.c_str(), tagField.c_str(), tagFieldValue.c_str());
}

bool 
CacheReplacementTotalOrder::isResponsibleForDataObject(
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

    attr = dObj->getAttribute (idField, "*", 1);

    if (!attr) {
        return false;
    }

    return true; 
}

class CacheReplacementTotalOrderCallback : public EventHandler
{
private:
    DataManager *manager;
    HaggleKernel *kernel;
    List<Pair<DataObjectRef, unsigned long> > *relatedDataObjects;
    EventCallback <EventHandler> *wrapperCallback;
    Mutex lock;
    Condition cond;
protected:
    void signal() {
        cond.signal(); 
    }
public:
    CacheReplacementTotalOrderCallback(
        DataManager *_manager,
        HaggleKernel *_kernel) :
            manager(_manager),
            kernel(_kernel),
            relatedDataObjects(NULL),
            wrapperCallback(NULL) {}

    ~CacheReplacementTotalOrderCallback() {
        if (wrapperCallback) {
            delete wrapperCallback;
        } 
    }

    bool wait() {
        lock.lock();
        struct timeval max_wait = manager->getDatabaseTimeout();
        return cond.timedWait(&lock, &max_wait);
    }

    void callback (Event *e) {
        if (!e || !e->hasData()) {
            HAGGLE_ERR("Callback had NULL args.\n");
        }

        DataStoreReplacementTotalOrderQueryResult *qr =
            static_cast <DataStoreReplacementTotalOrderQueryResult * >
                (e->getData());

        relatedDataObjects = qr->getOrderedObjects();
        delete qr;
        signal();
    }

    void setWrapperCallback(EventCallback <EventHandler> *_wrapperCallback) {
        wrapperCallback = _wrapperCallback;
    }

    List<Pair<DataObjectRef, unsigned long> > *
    getSortedRelatedDataObjects() {
        return relatedDataObjects;
    }
};

List<Pair<DataObjectRef, unsigned long> >*
CacheReplacementTotalOrder::getRelatedDataObjects(
    DataObjectRef newDataObject, bool &isDatabaseTimeout)
{
    const Attribute *attr = newDataObject->getAttribute(idField, "*", 1);
    if (!attr) {
        HAGGLE_ERR("Could not get id field\n");
        return NULL;
    }

    string idFieldValue = attr->getValue();

    CacheReplacementTotalOrderCallback *totalOrderCallback = 
        new CacheReplacementTotalOrderCallback(getManager(), getKernel());

    EventCallback <EventHandler> *callback =
        (EventCallback<EventHandler> *) 
            new EventCallback<CacheReplacementTotalOrderCallback> (
                totalOrderCallback,
                &CacheReplacementTotalOrderCallback::callback);

    totalOrderCallback->setWrapperCallback(callback);

    getManager()->getKernel()->getDataStore()->doDataObjectQueryForTotalOrderReplacement(
        tagField,
        tagFieldValue,
        metricField,
        idField,
        idFieldValue,
        callback);

    if (!getKernel()->isShuttingDown()) {
        if (!totalOrderCallback->wait()) {
            HAGGLE_ERR("Timed out waiting for data store callback.\n");
            isDatabaseTimeout = true;
            return NULL;
        }
    }

    List<Pair<DataObjectRef, unsigned long> > * sorteddObjs = totalOrderCallback->getSortedRelatedDataObjects();

    // delete totalOrderCallback; // MOS - this is unsafe

    return sorteddObjs;
}

void
CacheReplacementTotalOrder::getOrganizedDataObjectsByNewDataObject(
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

    DataObjectRefList subObjs = DataObjectRefList();
    DataObjectRefList equivObjs = DataObjectRefList();
    DataObjectRefList nonSubObjs = DataObjectRefList();

    const Attribute *attr;
    attr = dObj->getAttribute (metricField, "*", 1);
    if (!attr) {
        HAGGLE_ERR("No metric field\n");
        return;
    }
    
    unsigned long newDOMetric = atol(attr->getValue().c_str());

    List<Pair<DataObjectRef, unsigned long> > *relatedDataObjects =
        getRelatedDataObjects(dObj, isDatabaseTimeout);

    if (!relatedDataObjects) {
        HAGGLE_ERR("Could not get related data objects\n");
        return;
    }

    List<Pair<DataObjectRef, unsigned long> >::iterator it = relatedDataObjects->begin();

    for (; it != relatedDataObjects->end(); it++) {
        if (newDOMetric > (*it).second) {
            subObjs.push_front((*it).first);
        }
        else if (newDOMetric == (*it).second) {
            equivObjs.push_front((*it).first);
        }
        else {
            nonSubObjs.push_front((*it).first);
        }
    }

    delete relatedDataObjects;

    (*o_subsumed) = subObjs;
    (*o_equiv) = equivObjs;
    (*o_nonsubsumed) = nonSubObjs;
}
