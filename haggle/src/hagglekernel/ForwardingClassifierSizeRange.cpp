/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ForwardingClassifierSizeRange.h"

ForwardingClassifierSizeRange::ForwardingClassifierSizeRange(
    ForwardingManager *m) :
        ForwardingClassifier(m, FORWARDING_CLASSIFIER_SIZERANGE_NAME),
        initialized(false),
        kernel(getManager()->getKernel())
{
}

ForwardingClassifierSizeRange::~ForwardingClassifierSizeRange()
{
}

const string
ForwardingClassifierSizeRange::getClassNameForDataObject(const DataObjectRef &dObj)
{
    if (!initialized) {
        HAGGLE_ERR("Classifier has not been fully initialized.\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj) {
        HAGGLE_ERR("Received null data object.\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    int dataSizeBytes = (int) dObj->getDataLen();
    if (minBytes <= dataSizeBytes && dataSizeBytes <= maxBytes) {
        return className;
    }

    return FORWARDING_CLASSIFIER_INVALID_CLASS;
}

void
ForwardingClassifierSizeRange::onForwardingClassifierConfig(const Metadata &m)
{
    initialized = false;

    if (0 != strcmp(getName(), m.getName().c_str())) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *param = m.getParameter(FORWARDING_CLASSIFIER_SIZERANGE_OPTION_NAME);
    char *endptr = NULL;

    if (NULL == param) {
        HAGGLE_ERR("No class name specified.\n");
        return;
    }

    className = string(param);

    param = m.getParameter(FORWARDING_CLASSIFIER_SIZERANGE_MINB_NAME);
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

    param = m.getParameter(FORWARDING_CLASSIFIER_SIZERANGE_MAXB_NAME);
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
        HAGGLE_ERR("min bytes >= max bytes\n");
        return;
    }

    HAGGLE_DBG("Initialized size range forwarding classifier with output tag: %s, min bytes: %d, max bytes: %d\n", className.c_str(), minBytes, maxBytes);

    initialized = true;
}
