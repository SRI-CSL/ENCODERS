/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _FORWARDINGCLASSIFIER_H
#define _FORWARDINGCLASSIFIER_H

class ForwardingClassifier;

#include "ManagerModule.h"
#include "ForwardingManager.h"
#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

typedef Map<string, Forwarder*> class_to_forwarder_t;

class ForwardingClassifier : public ManagerModule<ForwardingManager> {
#define FORWARDING_CLASSIFIER_INVALID_CLASS (string(""))
protected:
    virtual void onForwardingClassifierConfig(const Metadata& m) { }
    class_to_forwarder_t class_to_forwarder;
public:
    ForwardingClassifier(ForwardingManager *m = NULL,
        const string name = "Unknown forwarding classifier") :
        ManagerModule<ForwardingManager>(m, name) {}

    ~ForwardingClassifier() {} 

    virtual const string getClassNameForDataObject(const DataObjectRef& dObj) { 
        return FORWARDING_CLASSIFIER_INVALID_CLASS; 
    }

    // hooks for future modules
    virtual void quit() {}
    void onConfig(const Metadata& m);

    // key methods to allow forwarding manager to retrieve correct
    // forwarder.
    void registerForwarderForClass(Forwarder* f, string className);  
    Forwarder* getForwarderForDataObject(const DataObjectRef& dObj);
    Forwarder* getForwarderForNode(const NodeRef &neighbor);
};

#endif /* _FORWARDINGCLASSIFIER_H */
