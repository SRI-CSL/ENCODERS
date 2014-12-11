/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _PROTOCOLCLASSIFIERFACTORY_H
#define _PROTOCOLCLASSIFIERFACTORY_H

class ProtocolClassifierFactory;

#include "ProtocolClassifier.h"

class ProtocolClassifierFactory {
public:
    static ProtocolClassifier *getClassifierForName(ProtocolManager *p, string name);
};

#endif /* PROTOCOLCLASSIFIERFACTORY */
