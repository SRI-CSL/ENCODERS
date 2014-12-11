/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

/*
 * DEPRECATED: use ForwardingClassifierNodeDescription instead!
 */
#include "ForwardingClassifierBasic.h"

ForwardingClassifierBasic::ForwardingClassifierBasic(
    ForwardingManager *m)  :
    ForwardingClassifier(m, FORWARDING_CLASSIFIER_BASIC_NAME), 
    initialized(false),
    kernel(getManager()->getKernel())
{

}

ForwardingClassifierBasic::~ForwardingClassifierBasic() 
{
}

const string 
ForwardingClassifierBasic::getClassNameForDataObject(
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
    
    // only mark node descriptions as light-weight content
    if (dObj->isNodeDescription()) {
        return lwClassName;
    }

    return hwClassName;
}

void
ForwardingClassifierBasic::onForwardingClassifierConfig(
    const Metadata& m)
{
    initialized = false;

    if (strcmp(getName(), m.getName().c_str()) != 0) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *param;
    param = m.getParameter(FORWARDING_CLASSIFIER_BASIC_OPTION_LW_NAME);
    if (!param) {
        HAGGLE_ERR("No light weight class name specified.\n");
        return;
    }

    lwClassName = string(param);

    param = m.getParameter(FORWARDING_CLASSIFIER_BASIC_OPTION_HW_NAME);
    if (!param) {
        HAGGLE_ERR("No heavy weight class name specified.\n");
        return;
    }

    hwClassName = string(param); 

    HAGGLE_DBG("Initialized basic forwarding classifier with lw class: %s, hw class: %s\n", lwClassName.c_str(), hwClassName.c_str());

    initialized = true;
}
