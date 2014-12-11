/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _PROTOCOLCLASSIFIER_H
#define _PROTOCOLCLASSIFIER_H

class ProtocolClassifier;

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

#include "ManagerModule.h"
#include "ProtocolManager.h"
#include "Protocol.h"

#include "ProtocolConfiguration.h"

typedef Map<string, ProtocolConfiguration *> class_to_config_t;

class ProtocolClassifier : public ManagerModule<ProtocolManager> {
#define PROTOCOL_CLASSIFIER_INVALID_CLASS (string(""))
protected:
    virtual void onProtocolClassifierConfig(const Metadata& m) { }
    class_to_config_t class_to_config;
public:
    ProtocolClassifier(
        ProtocolManager *m = NULL, 
        string name = "Unknown protocol classifier");
        
    ~ProtocolClassifier();

    virtual string getClassNameForDataObject(const DataObjectRef& dObj) { 
        return PROTOCOL_CLASSIFIER_INVALID_CLASS; 
    }

    virtual void quit() {}

    void onConfig(const Metadata& m);

    ProtocolConfiguration *
    getProtocolConfigurationForDataObject(const DataObjectRef& dObj);

    ProtocolConfiguration *getDefaultProtocolConfiguration();

    void registerConfigurationForClass(
	    ProtocolConfiguration *config, 
	    string className);

    ProtocolConfiguration *
    setDefaultProtocolConfiguration(ProtocolConfiguration *cfg);
};

#endif /* _PROTOCOLCLASSIFIER_H */
