/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ForwardingClassifierFactory.h"

#include "ForwardingClassifierNodeDescription.h"
#include "ForwardingClassifierAttribute.h"
#include "ForwardingClassifierPriority.h"
#include "ForwardingClassifierSizeRange.h"
#include "ForwardingClassifierAllMatch.h"

#include "ForwardingClassifierBasic.h" // SW: DEPRECATED, for backwards compat.

ForwardingClassifier* ForwardingClassifierFactory::getClassifierForName(ForwardingManager* f, string name) 
{
    if (name == FORWARDING_CLASSIFIER_INVALID_CLASS) {
        return (ForwardingClassifier*) NULL;
    }

    if (name == FORWARDING_CLASSIFIER_ND_NAME) {
        return new ForwardingClassifierNodeDescription(f);
    }

    if (name == FORWARDING_CLASSIFIER_ATTRIBUTE_NAME) {
        return new ForwardingClassifierAttribute(f);
    }

    if (name == FORWARDING_CLASSIFIER_PRIORITY_NAME) {
        return new ForwardingClassifierPriority(f);
    }

    if (name == FORWARDING_CLASSIFIER_SIZERANGE_NAME) {
        return new ForwardingClassifierSizeRange(f);
    }
    
    if (name == FORWARDING_CLASSIFIER_ALLMATCH_NAME) {
        return new ForwardingClassifierAllMatch(f);
    }

    // SW: NOTE: DEPRECATED, for backwards compat. only
    if (name == FORWARDING_CLASSIFIER_BASIC_NAME) {
        return new ForwardingClassifierBasic(f);
    }

    return (ForwardingClassifier*) NULL;
}
