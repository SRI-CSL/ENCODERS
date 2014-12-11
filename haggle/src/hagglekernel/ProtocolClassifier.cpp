/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolClassifier.h"

ProtocolClassifier::ProtocolClassifier( 
        ProtocolManager *m, 
        string name) :
            ManagerModule<ProtocolManager>(m, name)
{
}

ProtocolClassifier::~ProtocolClassifier() 
{
    for (class_to_config_t::iterator it = class_to_config.begin();
         it != class_to_config.end(); it++) {

        ProtocolConfiguration *cfg = (*it).second;
        
        if (NULL == cfg) {
            HAGGLE_ERR("Null config in registry\n");
        }

        class_to_config.erase(it);

        // TODO: this cfg object is shared in protofactory maps, so
        // we cannot delete here..
        //delete cfg;
    }
}

void ProtocolClassifier::onConfig(
    const Metadata& m)
{
    if (0 != m.getName().compare("ProtocolClassifier")) {
        HAGGLE_ERR("Config in wrong format\n");
        return;
    }

    const Metadata *md = m.getMetadata(getName());

    if (!md) {
        HAGGLE_ERR("Missing options (for name: %s)\n", getName());
        return;
    }

    onProtocolClassifierConfig(*md);
}

void ProtocolClassifier::registerConfigurationForClass(
    ProtocolConfiguration *config,
    string className)
{
    if (PROTOCOL_CLASSIFIER_INVALID_CLASS == className) {
        HAGGLE_ERR("No protocol for invalid class name.\n");
        return;
    }

    if (!config) {
        HAGGLE_ERR("No configuration for class name.\n");
        return;
    }

    if (class_to_config[className]) {
        HAGGLE_ERR("Protocol cfg already registered for class\n");
        return;
    }
    
    class_to_config[className] = config;
}

ProtocolConfiguration *
ProtocolClassifier::getProtocolConfigurationForDataObject(
    const DataObjectRef& dObj)
{
    if (!dObj) {
        HAGGLE_ERR("Null data object.\n");
        return NULL;
    }
  
    string className = getClassNameForDataObject(dObj);   

    if (PROTOCOL_CLASSIFIER_INVALID_CLASS == className) {
        HAGGLE_DBG("Could not categorize the content, using default\n"); 
    }

    if (!class_to_config[className]) {
        HAGGLE_ERR("No protocol registered for class: %s\n", className.c_str());
        return NULL;
    }
   
    return class_to_config[className];
}

ProtocolConfiguration *
ProtocolClassifier::getDefaultProtocolConfiguration()
{
    return class_to_config[PROTOCOL_CLASSIFIER_INVALID_CLASS];
}

ProtocolConfiguration *
ProtocolClassifier::setDefaultProtocolConfiguration(ProtocolConfiguration *cfg)
{
    if (class_to_config[PROTOCOL_CLASSIFIER_INVALID_CLASS]) {
        ProtocolConfiguration *old_cfg = class_to_config[PROTOCOL_CLASSIFIER_INVALID_CLASS];
        //delete old_cfg;
        class_to_config.erase(PROTOCOL_CLASSIFIER_INVALID_CLASS);
    }

    class_to_config[PROTOCOL_CLASSIFIER_INVALID_CLASS] = cfg;
    return cfg;
}
