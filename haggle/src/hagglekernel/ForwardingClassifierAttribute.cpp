/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ForwardingClassifierAttribute.h"

ForwardingClassifierAttribute::ForwardingClassifierAttribute(
    ForwardingManager *m)  :
    ForwardingClassifier(m, FORWARDING_CLASSIFIER_ATTRIBUTE_NAME), 
    initialized(false),
    kernel(getManager()->getKernel())
{

}

ForwardingClassifierAttribute::~ForwardingClassifierAttribute() 
{
}

const string 
ForwardingClassifierAttribute::getClassNameForDataObject(
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
    
    const Attribute *attr = dObj->getAttribute(attrName, attrValue);
    
    // only mark node descriptions as light-weight content
    if (!attr) {
        return FORWARDING_CLASSIFIER_INVALID_CLASS;
    }

    return className;
}

void
ForwardingClassifierAttribute::onForwardingClassifierConfig(
    const Metadata& m)
{
    initialized = false;

    if (strcmp(getName(), m.getName().c_str()) != 0) {
        HAGGLE_ERR("Invalid metadata to config handler.\n");
        return;
    }

    const char *_attrName = m.getParameter(FORWARDING_CLASSIFIER_ATTRIBUTE_NAME_OPTION);
    if (!_attrName) {
        HAGGLE_ERR("No attribute name specified.\n");
        return;
    }    

    const char *_attrValue = m.getParameter(FORWARDING_CLASSIFIER_ATTRIBUTE_VALUE_OPTION);
    if (!_attrName) {
        HAGGLE_ERR("No attribute value specified.\n");
        return;
    }    


    const char *_className = m.getParameter(FORWARDING_CLASSIFIER_ATTRIBUTE_CLASS_NAME);
    if (!_className) {
        HAGGLE_ERR("No class name specified.\n");
        return;
    }

    attrName = string(_attrName);
    attrValue = string(_attrValue);
    className = string(_className);

    HAGGLE_DBG("Initialized forwarding classifier with attr name: %s, attr value: %s, class name: %s.\n",
        attrName.c_str(),
        attrValue.c_str(),
        className.c_str());

    initialized = true;
}
