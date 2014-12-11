/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

/*
 * DEPRECATED: use ForwardingClassifierNodeDescription instead!
 */
#ifndef _FORWARDINGCLASSIFIERBASIC_H
#define _FORWARDINGCLASSIFIERBASIC_H

class ForwardingClassifierBasic;

#include "ForwardingClassifier.h"

class ForwardingClassifierBasic : public ForwardingClassifier {
#define FORWARDING_CLASSIFIER_BASIC_NAME "ForwardingClassifierBasic"
#define FORWARDING_CLASSIFIER_BASIC_OPTION_LW_NAME "lightWeightClassName"
#define FORWARDING_CLASSIFIER_BASIC_OPTION_HW_NAME "heavyWeightClassName"
private:
    bool initialized;
    string lwClassName;
    string hwClassName;
    HaggleKernel *kernel;
protected:
    const string getClassNameForDataObject(const DataObjectRef& dObj);
    void onForwardingClassifierConfig(const Metadata& m);
public:
    ForwardingClassifierBasic(ForwardingManager *m = NULL); 
    ~ForwardingClassifierBasic(); 
};

#endif /* FORWARDINGCLASSIFIERBASIC */
