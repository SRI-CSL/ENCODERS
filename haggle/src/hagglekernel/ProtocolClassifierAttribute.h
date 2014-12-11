/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _PROTOCOLCLASSIFIERATTRIBUTE_H
#define _PROTOCOLCLASSIFIERATTRIBUTE_H

class ProtocolClassifierAttribute;

#include "ProtocolClassifier.h"

class ProtocolClassifierAttribute : public ProtocolClassifier {
#define PROTOCOL_CLASSIFIER_ATTRIBUTE_NAME "ProtocolClassifierAttribute"
#define PROTOCOL_CLASSIFIER_ATTRIBUTE_NAME_OPTION "attribute_name"
#define PROTOCOL_CLASSIFIER_ATTRIBUTE_VALUE_OPTION "attribute_value"
#define PROTOCOL_CLASSIFIER_ATTRIBUTE_CLASS_NAME "outputTag"
private:
    bool initialized;
    string attrName;
    string attrValue;
    string className;
    HaggleKernel *kernel;
protected:
    string getClassNameForDataObject(const DataObjectRef& dObj);
    void onProtocolClassifierConfig(const Metadata& m);
public:
    ProtocolClassifierAttribute(ProtocolManager *m = NULL); 
    ~ProtocolClassifierAttribute(); 
};

#endif /* PROTOCOLCLASSIFIERATTRIBUTE */
