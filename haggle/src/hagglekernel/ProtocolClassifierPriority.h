/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _PROTOCOLCLASSIFIER_PRIORITY_H
#define _PROTOCOLCLASSIFIER_PRIORITY_H

class ProtocolClassifierPriority;

#include "ProtocolClassifier.h"

class ProtocolClassifierPriority : public ProtocolClassifier {
#define PROTOCOL_CLASSIFIER_PRIORITY_NAME "ProtocolClassifierPriority"
#define PROTOCOL_CLASSIFIER_PRIORITY_CHILD_NAME "name"
#define PROTOCOL_CLASSIFIER_PRIORITY_CHILD_PRIORITY_NAME "priority"
private:
    bool initialized;
    HaggleKernel *kernel;
    typedef Pair<ProtocolClassifier *, int> PriorityPairType; 
    typedef List<PriorityPairType> ClassifierListType;
    ClassifierListType classifierList;
protected:
    string getClassNameForDataObject(const DataObjectRef &dObj);
    void onProtocolClassifierConfig(const Metadata &m);
public:
    ProtocolClassifierPriority(ProtocolManager *m = NULL);
    ~ProtocolClassifierPriority();
};

#endif /* PROTOCOLCLASSIFIER_PRIORITY */
