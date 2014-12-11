/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _FORWARDINGCLASSIFIER_ALLMATCH_H
#define _FORWARDINGCLASSIFIER_ALLMATCH_H

class ForwardingClassifierAllMatch;

#include "ForwardingClassifier.h"

class ForwardingClassifierAllMatch : public ForwardingClassifier {
#define FORWARDING_CLASSIFIER_ALLMATCH_NAME "ForwardingClassifierAllMatch"
#define FORWARDING_CLASSIFIER_ALLMATCH_OUT_TAG_NAME "class_name"
private:
    bool initialized;
    string className;
    HaggleKernel *kernel;
protected:
    const string getClassNameForDataObject(const DataObjectRef &dObj);
    void onForwardingClassifierConfig(const Metadata &m);
public:
    ForwardingClassifierAllMatch(ForwardingManager *m = NULL);
    ~ForwardingClassifierAllMatch();
};

#endif /* FORWARDINGCLASSIFIER_ALLMATCH */
