/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingForwardingManagerHelper.h"

ForwardingManagerHelper::ForwardingManagerHelper(HaggleKernel *_kernel) {
    this->kernel = _kernel;
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
    this->networkCodingConfiguration = new NetworkCodingConfiguration();
}

ForwardingManagerHelper::~ForwardingManagerHelper() {
    if( this->networkCodingDataObjectUtility) {
        delete this->networkCodingDataObjectUtility;
        this->networkCodingDataObjectUtility = NULL;
    }
    if( this->networkCodingConfiguration ) {
        delete this->networkCodingConfiguration;
        this->networkCodingConfiguration = NULL;
    }
}

void ForwardingManagerHelper::addDataObjectEventWithDelay(DataObjectRef dataObjectRef, NodeRef nodeRef) {
    unsigned long flags = 0;
    double _delay = networkCodingConfiguration->getResendDelay();

    bool shouldBeNetworkCoded = this->networkCodingDataObjectUtility->shouldBeNetworkCodedCheckTargetNode(dataObjectRef,nodeRef);

    if (!shouldBeNetworkCoded) {
        HAGGLE_DBG("Generating event to send data object %s eligible for network coding\n", dataObjectRef->getIdStr());
        Event* repeatEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND, dataObjectRef, nodeRef, flags, _delay);
        kernel->addEvent(repeatEvent);
    }
    else {
        HAGGLE_DBG("Generating event to trigger sending of block for data object %s\n", dataObjectRef->getIdStr());
        Event* repeatEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING, dataObjectRef, nodeRef, flags, _delay);
        kernel->addEvent(repeatEvent);
    }
}



