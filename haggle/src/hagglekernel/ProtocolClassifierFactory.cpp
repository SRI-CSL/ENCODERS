/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolClassifierFactory.h"
#include "ProtocolClassifierBasic.h"
#include "ProtocolClassifierNodeDescription.h"
#include "ProtocolClassifierSizeRange.h"
#include "ProtocolClassifierAllMatch.h"
#include "ProtocolClassifierPriority.h"
#include "ProtocolClassifierAttribute.h"

ProtocolClassifier* ProtocolClassifierFactory::getClassifierForName(ProtocolManager *p, string name)
{
    if (name == PROTOCOL_CLASSIFIER_INVALID_CLASS) {
        return (ProtocolClassifier*) NULL;
    }

    if (name == PROTOCOL_CLASSIFIER_BASIC_NAME) {
        return new ProtocolClassifierBasic(p);
    }

    if (name == PROTOCOL_CLASSIFIER_ND_NAME) {
        return new ProtocolClassifierNodeDescription(p); 
    }

    if (name == PROTOCOL_CLASSIFIER_SIZERANGE_NAME) {
        return new ProtocolClassifierSizeRange(p);
    }

    if (name == PROTOCOL_CLASSIFIER_ALLMATCH_NAME) {
        return new ProtocolClassifierAllMatch(p);
    }

    if (name == PROTOCOL_CLASSIFIER_PRIORITY_NAME) {
        return new ProtocolClassifierPriority(p);
    }

    if (name == PROTOCOL_CLASSIFIER_ATTRIBUTE_NAME) {
        return new ProtocolClassifierAttribute(p);
    }

    HAGGLE_ERR("No classifier for name: %s\n", name.c_str());

    return (ProtocolClassifier*) NULL;
}
