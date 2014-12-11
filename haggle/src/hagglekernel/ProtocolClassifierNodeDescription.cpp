/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolClassifierNodeDescription.h"

ProtocolClassifierNodeDescription::ProtocolClassifierNodeDescription(
    ProtocolManager *m) :
        ProtocolClassifier(m, PROTOCOL_CLASSIFIER_ND_NAME),
        initialized(false),
        kernel(getManager()->getKernel())
{
}

ProtocolClassifierNodeDescription::~ProtocolClassifierNodeDescription()
{
}

string ProtocolClassifierNodeDescription::getClassNameForDataObject(const DataObjectRef &dObj)
{
    if (!initialized) {
        HAGGLE_ERR("Classifier has not been fully initialized.\n");
        return PROTOCOL_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj) {
        HAGGLE_ERR("Received null data object.\n");
        return PROTOCOL_CLASSIFIER_INVALID_CLASS;
    }

    if (dObj->isNodeDescription() || dObj->isThisNodeDescription()) {
        return outputTag;
    }

    return PROTOCOL_CLASSIFIER_INVALID_CLASS;
}

void ProtocolClassifierNodeDescription::onProtocolClassifierConfig(const Metadata &m)
{
    initialized = false;

    if (0 != strcmp(getName(), m.getName().c_str())) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *_outputTag = m.getParameter(PROTOCOL_CLASSIFIER_ND_OUT_TAG_NAME);
    if (NULL == _outputTag) {
        HAGGLE_ERR("No output tag name specified.\n");
        return;
    }

    outputTag = string(_outputTag);

    HAGGLE_DBG("Initialized node description protocol classifier with output tag: %s\n", outputTag.c_str());

    initialized = true;
}
