/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Jihwa Lee (JL)
 */

#ifndef _CACHE_REPLACEMENT_TO_H
#define _CACHE_REPLACEMENT_TO_H

class CacheReplacementTotalOrder;

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"
#include "CacheReplacement.h"

#define CACHE_REPLACEMENT_TOTAL_ORDER_NAME "CacheReplacementTotalOrder"
#define CACHE_REPLACEMENT_TOTAL_ORDER_METRIC "metric_field"
#define CACHE_REPLACEMENT_TOTAL_ORDER_ID "id_field"
#define CACHE_REPLACEMENT_TOTAL_ORDER_TAG "tag_field"
#define CACHE_REPLACEMENT_TOTAL_ORDER_TAG_VALUE "tag_field_value"

class CacheReplacementTotalOrder : public CacheReplacement {
private:
    string metricField;
    string idField;
    string tagField;
    string tagFieldValue;
protected:
    List<Pair<DataObjectRef, unsigned long> >* getRelatedDataObjects(DataObjectRef newDataObject, bool &isDatabaseTimeout);
public:
    CacheReplacementTotalOrder(
        DataManager *m = NULL,
        string _metricField = "",
        string _idField = "",
        string _tagField = "",
        string _tagFieldValue = "");

    ~CacheReplacementTotalOrder();

    void onConfig(const Metadata& m);

    bool isResponsibleForDataObject(DataObjectRef &dObj);

    void getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout);
};

#endif /* _CACHE_REPLACEMENT_TO_H */
