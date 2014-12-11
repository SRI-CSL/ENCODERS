/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ForwardingClassifierNodeDescription.h"

ForwardingClassifierNodeDescription::ForwardingClassifierNodeDescription(
    ForwardingManager *m)  :
    ForwardingClassifier(m, FORWARDING_CLASSIFIER_ND_NAME), 
    initialized(false),
    kernel(getManager()->getKernel()),
    deviceEnabled(true),
    appEnabled(true)
{

}

ForwardingClassifierNodeDescription::~ForwardingClassifierNodeDescription() 
{
}

const string 
ForwardingClassifierNodeDescription::getClassNameForDataObject(
    const DataObjectRef& dObj)
{
    if (!initialized) {
        HAGGLE_ERR("Classifier has not been fully initialized.\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj) {
        HAGGLE_ERR("Received null data object.\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    if (!dObj->isNodeDescription()) {
        HAGGLE_DBG2("Data object is not a node description\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    NodeRef node = Node::create(dObj);
    if (!node) {
        HAGGLE_ERR("Could not create node from dobj\n");
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    Node::Type_t type = node->getType();

    if (deviceEnabled && (type == Node::TYPE_PEER || type == Node::TYPE_LOCAL_DEVICE)) {
        return nodeDescriptionClassName;
    }

    if (appEnabled && (type == Node::TYPE_APPLICATION)) {
        // only push nodes with attributes
        const Attributes *attrs = node->getAttributes();
        if (attrs && attrs->size() > 0) {
            return nodeDescriptionClassName;
        }
    }

    return FORWARDING_CLASSIFIER_INVALID_CLASS;
}

void
ForwardingClassifierNodeDescription::onForwardingClassifierConfig(
    const Metadata& m)
{
    initialized = false;

    if (strcmp(getName(), m.getName().c_str()) != 0) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *param = m.getParameter(FORWARDING_CLASSIFIER_ND_OPTION_NAME);
    if (!param) {
        HAGGLE_ERR("No node description class name specified.\n");
        return;
    }    

    nodeDescriptionClassName = string(param);    

    param = m.getParameter(FORWARDING_CLASSIFIER_DEVICE_OPTION_NAME);
    if (param) {
        string param_str = string(param);
        if (param_str == "true") {
            deviceEnabled = true; 
        }
        else if (param_str == "false") {
            deviceEnabled = false; 
        }
        else {
            HAGGLE_ERR("Invalid option for device\n");
        }
    }

    param = m.getParameter(FORWARDING_CLASSIFIER_APP_OPTION_NAME);
    if (param) {
        string param_str = string(param);
        if (param_str == "true") {
            appEnabled = true; 
        }
        else if (param_str == "false") {
            appEnabled = false; 
        }
        else {
            HAGGLE_ERR("Invalid option for app\n");
        }
    }

    HAGGLE_DBG("Initialized forwarding classifier with node description class: %s, device enabled: %s, app enabled: %s\n", nodeDescriptionClassName.c_str(), deviceEnabled ? "yes" : "no", appEnabled ? "yes" : "no" );

    initialized = true;
}
