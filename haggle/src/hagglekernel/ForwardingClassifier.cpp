/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include "ForwardingClassifier.h"

void ForwardingClassifier::onConfig(const Metadata& m) 
{
    if (0 != m.getName().compare("ForwardingClassifier")) {
        return;
    }

    const Metadata *md = m.getMetadata(getName());

    if (!md) {
        HAGGLE_DBG("No configuration for classifier\n");
        return;
    }

    onForwardingClassifierConfig(*md);
}

void ForwardingClassifier::registerForwarderForClass(Forwarder* f, string className)  
{
    if (NULL == f) {
        HAGGLE_ERR("Null forwarder.\n");
        return;
    }

    if (FORWARDING_CLASSIFIER_INVALID_CLASS == className) {
        HAGGLE_ERR("No forwarder for invalid class name.\n");
        return;
    }

    Forwarder* o = class_to_forwarder[className];      
    
    if (o) {
        HAGGLE_ERR("Forwarder already registered for content class\n");
        return;
    }        
    
    class_to_forwarder[className] = f;
}

Forwarder* ForwardingClassifier::getForwarderForDataObject(const DataObjectRef& dObj)
{
    if (!dObj) {
        HAGGLE_ERR("Null data object.\n");
        return (Forwarder*) NULL;
    }

    string className = getClassNameForDataObject(dObj);   

    if (FORWARDING_CLASSIFIER_INVALID_CLASS == className) {
        HAGGLE_DBG2("Could not categorize the content.\n"); 
        return (Forwarder*) NULL;
    }
    
    Forwarder* f = class_to_forwarder[className];

    if (!f) {
        HAGGLE_DBG("Could not get forwarder for class name (%s).\n", className.c_str());
        return (Forwarder*) NULL;
    }

    return f;
}

Forwarder* ForwardingClassifier::getForwarderForNode(const NodeRef &neighbor)
{
    // TODO: Unimplemented.
    HAGGLE_ERR("Not currently implemented.\n");
    return (Forwarder*) NULL;
}
