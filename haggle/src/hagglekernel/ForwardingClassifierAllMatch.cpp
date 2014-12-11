/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ForwardingClassifierAllMatch.h"

ForwardingClassifierAllMatch::ForwardingClassifierAllMatch(
    ForwardingManager *m) :
        ForwardingClassifier(m, FORWARDING_CLASSIFIER_ALLMATCH_NAME),
        initialized(false),
        kernel(getManager()->getKernel())
{
}

ForwardingClassifierAllMatch::~ForwardingClassifierAllMatch()
{
}

const string
ForwardingClassifierAllMatch::getClassNameForDataObject(const DataObjectRef &dObj)
{
    if (!initialized) {
        HAGGLE_ERR("Classifier has not been fully initialized.\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj) {
        HAGGLE_ERR("Received null data object.\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    return className;
}

void
ForwardingClassifierAllMatch::onForwardingClassifierConfig(const Metadata &m)
{
    initialized = false;

    if (0 != strcmp(getName(), m.getName().c_str())) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *param = m.getParameter(FORWARDING_CLASSIFIER_ALLMATCH_OUT_TAG_NAME);

    if (NULL == param) {
        HAGGLE_ERR("No output tag specified.\n");
        return;
    }

    className = string(param);

    HAGGLE_DBG("Initialized all match forwarding classifier with class name: %s\n", className.c_str());

    initialized = true;
}
