/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _FORWARDINGCLASSIFIERFACTORY_H
#define _FORWARDINGCLASSIFIERFACTORY_H

class ForwardingClassifierFactory;

#include "ForwardingClassifier.h"

class ForwardingClassifierFactory {
public:
    static ForwardingClassifier* getClassifierForName(ForwardingManager* f, string name);
};

#endif /* FORWARDINGCLASSIFIERFACTORY */
