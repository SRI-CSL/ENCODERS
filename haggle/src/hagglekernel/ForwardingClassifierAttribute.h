/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _FORWARDINGCLASSIFIERATTRIBUTE_H
#define _FORWARDINGCLASSIFIERATTRIBUTE_H

class ForwardingClassifierAttribute;

#include "ForwardingClassifier.h"

class ForwardingClassifierAttribute : public ForwardingClassifier {
#define FORWARDING_CLASSIFIER_ATTRIBUTE_NAME "ForwardingClassifierAttribute"
#define FORWARDING_CLASSIFIER_ATTRIBUTE_NAME_OPTION "attribute_name"
#define FORWARDING_CLASSIFIER_ATTRIBUTE_VALUE_OPTION "attribute_value"
#define FORWARDING_CLASSIFIER_ATTRIBUTE_CLASS_NAME "class_name"
private:
    bool initialized;
    string attrName;
    string attrValue;
    string className;
    HaggleKernel *kernel;
protected:
    const string getClassNameForDataObject(const DataObjectRef& dObj);
    void onForwardingClassifierConfig(const Metadata& m);
public:
    ForwardingClassifierAttribute(ForwardingManager *m = NULL); 
    ~ForwardingClassifierAttribute(); 
};

#endif /* FORWARDINGCLASSIFIERATTRIBUTE */
