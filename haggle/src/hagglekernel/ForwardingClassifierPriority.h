/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _FORWARDINGCLASSIFIERPRIOR_H
#define _FORWARDINGCLASSIFIERPRIOR_H

// TODO: NOTE: this is modeled after ProtocolClassifierPriority, and shoud be merged
// with this code in the future, to avoid code duplication.

class ForwardingClassifierPriority;

#include "ForwardingClassifier.h"

class ForwardingClassifierPriority : public ForwardingClassifier {
#define FORWARDING_CLASSIFIER_PRIORITY_NAME "ForwardingClassifierPriority"
#define FORWARDING_CLASSIFIER_PRIORITY_CHILD_NAME "name"
#define FORWARDING_CLASSIFIER_PRIORITY_CHILD_PRIORITY_NAME "priority"
private:
    bool initialized;
    HaggleKernel *kernel;
    typedef Pair<ForwardingClassifier *, int> PriorityPairType; 
    typedef List<PriorityPairType> ClassifierListType;
    ClassifierListType classifierList;
protected:
    const string getClassNameForDataObject(const DataObjectRef& dObj);
    void onForwardingClassifierConfig(const Metadata& m);
public:
    ForwardingClassifierPriority(ForwardingManager *m = NULL); 
    ~ForwardingClassifierPriority(); 
};

#endif /* FORWARDINGCLASSIFIERPRIOR */
