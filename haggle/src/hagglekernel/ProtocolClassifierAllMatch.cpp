/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolClassifierAllMatch.h"

ProtocolClassifierAllMatch::ProtocolClassifierAllMatch(
    ProtocolManager *m) :
        ProtocolClassifier(m, PROTOCOL_CLASSIFIER_ALLMATCH_NAME),
        initialized(false),
        kernel(getManager()->getKernel())
{
}

ProtocolClassifierAllMatch::~ProtocolClassifierAllMatch()
{
}

string ProtocolClassifierAllMatch::getClassNameForDataObject(const DataObjectRef &dObj)
{
    if (!initialized) {
        HAGGLE_ERR("Classifier has not been fully initialized.\n");
        return PROTOCOL_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj) {
        HAGGLE_ERR("Received null data object.\n");
        return PROTOCOL_CLASSIFIER_INVALID_CLASS;
    }

    return outputTag;
}

void ProtocolClassifierAllMatch::onProtocolClassifierConfig(const Metadata &m)
{
    initialized = false;

    if (0 != strcmp(getName(), m.getName().c_str())) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *param = m.getParameter(PROTOCOL_CLASSIFIER_ALLMATCH_OUT_TAG_NAME);

    if (NULL == param) {
        HAGGLE_ERR("No output tag specified.\n");
        return;
    }

    outputTag = string(param);

    HAGGLE_DBG("Initialized all match protocol classifier with output tag: %s\n", outputTag.c_str());

    initialized = true;
}
