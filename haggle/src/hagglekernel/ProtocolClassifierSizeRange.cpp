/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolClassifierSizeRange.h"

ProtocolClassifierSizeRange::ProtocolClassifierSizeRange(
    ProtocolManager *m) :
        ProtocolClassifier(m, PROTOCOL_CLASSIFIER_SIZERANGE_NAME),
        initialized(false),
        kernel(getManager()->getKernel())
{
}

ProtocolClassifierSizeRange::~ProtocolClassifierSizeRange()
{
}

string ProtocolClassifierSizeRange::getClassNameForDataObject(const DataObjectRef &dObj)
{
    if (!initialized) {
        HAGGLE_ERR("Classifier has not been fully initialized.\n");
        return PROTOCOL_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj) {
        HAGGLE_ERR("Received null data object.\n");
        return PROTOCOL_CLASSIFIER_INVALID_CLASS;
    }

    int dataSizeBytes = (int) dObj->getDataLen();
    if (minBytes <= dataSizeBytes && dataSizeBytes <= maxBytes) {
        return outputTag;
    }

    return PROTOCOL_CLASSIFIER_INVALID_CLASS;
}

void ProtocolClassifierSizeRange::onProtocolClassifierConfig(const Metadata &m)
{
    initialized = false;

    if (0 != strcmp(getName(), m.getName().c_str())) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *param = m.getParameter(PROTOCOL_CLASSIFIER_SIZERANGE_OUT_TAG_NAME);
    char *endptr = NULL;

    if (NULL == param) {
        HAGGLE_ERR("No output tag specified.\n");
        return;
    }

    outputTag = string(param);

    param = m.getParameter(PROTOCOL_CLASSIFIER_SIZERANGE_MINB_NAME);
    if (NULL == param) {
        HAGGLE_ERR("No min bytes size specified.\n");
        return;
    }

    endptr = NULL;
    int paramInt; 

    paramInt = (int) strtol(param, &endptr, 10);
    if (endptr && endptr != param && paramInt >= 0) {
        minBytes = paramInt;
    } 
    else {
        HAGGLE_ERR("Problems parsing min bytes.\n");
        return;
    }

    param = m.getParameter(PROTOCOL_CLASSIFIER_SIZERANGE_MAXB_NAME);
    if (NULL == param) {
        HAGGLE_ERR("No max bytes size specified.\n");
        return;
    }

    endptr = NULL;
    paramInt = (int) strtol(param, &endptr, 10);
    if (endptr && endptr != param && paramInt > 0) {
        maxBytes = paramInt;
    } 
    else {
        HAGGLE_ERR("Problems parsing max bytes.\n");
        return;
    }

    if (minBytes >= maxBytes) {
        HAGGLE_ERR("minBytes >= maxBytes\n");
        return;
    }

    HAGGLE_DBG("Initialized size range protocol classifier with output tag: %s, min bytes: %d, max bytes: %d\n", outputTag.c_str(), minBytes, maxBytes);

    initialized = true;
}
