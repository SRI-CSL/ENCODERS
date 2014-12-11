/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _FORWARDINGCLASSIFIERND_H
#define _FORWARDINGCLASSIFIERND_H

class ForwardingClassifierNodeDescription;

#include "ForwardingClassifier.h"

class ForwardingClassifierNodeDescription : public ForwardingClassifier {
#define FORWARDING_CLASSIFIER_ND_NAME "ForwardingClassifierNodeDescription"
#define FORWARDING_CLASSIFIER_ND_OPTION_NAME "class_name"
#define FORWARDING_CLASSIFIER_DEVICE_OPTION_NAME "device_enabled"
#define FORWARDING_CLASSIFIER_APP_OPTION_NAME "app_enabled"
private:
    bool initialized;
    string nodeDescriptionClassName;
    HaggleKernel *kernel;
    bool deviceEnabled;
    bool appEnabled;
protected:
    const string getClassNameForDataObject(const DataObjectRef& dObj);
    void onForwardingClassifierConfig(const Metadata& m);
public:
    ForwardingClassifierNodeDescription(ForwardingManager *m = NULL); 
    ~ForwardingClassifierNodeDescription(); 
};

#endif /* FORWARDINGCLASSIFIERND */
