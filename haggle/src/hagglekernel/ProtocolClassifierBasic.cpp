/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolClassifierBasic.h"

#include "ProtocolConfigurationTCP.h"
#include "ProtocolConfigurationUDPGeneric.h"

ProtocolClassifierBasic::ProtocolClassifierBasic(
    ProtocolManager *m) :
        ProtocolClassifier(m, PROTOCOL_CLASSIFIER_BASIC_NAME),
        initialized(false),
        kernel(getManager()->getKernel())
{
}

ProtocolClassifierBasic::~ProtocolClassifierBasic()
{
}

string ProtocolClassifierBasic::getClassNameForDataObject(const DataObjectRef &dObj)
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
        return lightWeightClassName;
    }

    return heavyWeightClassName;
}

void ProtocolClassifierBasic::onProtocolClassifierConfig(const Metadata &m)
{
    initialized = false;

    if (0 != strcmp(getName(), m.getName().c_str())) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *lightWeightName = m.getParameter(PROTOCOL_CLASSIFIER_BASIC_LIGHTWEIGHT);
    if (!lightWeightName) {
        HAGGLE_ERR("No light weight class name specified.\n");
        return;
    }

    const char *heavyWeightName = m.getParameter(PROTOCOL_CLASSIFIER_BASIC_HEAVYWEIGHT);
    if (!heavyWeightName) {
        HAGGLE_ERR("No heavy weight class name specified.\n");
        return;
    }

    if (0 == strcmp(lightWeightName, heavyWeightName)) {
        HAGGLE_ERR("Light weight and heavy weight class names must be different.\n");
        return;
    }

    lightWeightClassName = string(lightWeightName);
    heavyWeightClassName = string(heavyWeightName);

    HAGGLE_DBG("Initialized protocol classifier with light weight class: %s, heavy weight class: %s.\n", lightWeightName, heavyWeightName);

    // NOTE: right now we use TCP for heavyweight, UDP unicast for lightweight

    registerConfigurationForClass(new ProtocolConfigurationUDPUnicast(), lightWeightClassName);
    registerConfigurationForClass(new ProtocolConfigurationTCP(), heavyWeightClassName);
    
    initialized = true;
}
