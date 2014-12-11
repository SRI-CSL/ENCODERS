/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */


#include "FragmentationForwardingManagerHelper.h"
#include "Event.h"


FragmentationForwardingManagerHelper::FragmentationForwardingManagerHelper(HaggleKernel *_kernel) {
	this->kernel = _kernel;
	this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
	this->fragmentationConfiguration = new FragmentationConfiguration();
}

FragmentationForwardingManagerHelper::~FragmentationForwardingManagerHelper() {
	if(this->fragmentationDataObjectUtility) {
		delete this->fragmentationDataObjectUtility;
		this->fragmentationDataObjectUtility = NULL;
	}
	if(this->fragmentationConfiguration) {
	    delete this->fragmentationConfiguration;
	    this->fragmentationConfiguration = NULL;
	}
}

void FragmentationForwardingManagerHelper::addDataObjectEventWithDelay(DataObjectRef dataObjectRef,NodeRef nodeRef) {

    unsigned long flags = 0;
    double _delay = this->fragmentationConfiguration->getResendDelay();

    bool shouldBeFragmented = this->fragmentationDataObjectUtility->shouldBeFragmentedCheckTargetNode(dataObjectRef,nodeRef);

    if (!shouldBeFragmented) {
        HAGGLE_DBG("Generating event to send data object %s eligible for fragmentation\n", dataObjectRef->getIdStr());
        Event* repeatEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND, dataObjectRef, nodeRef, flags, _delay);
        kernel->addEvent(repeatEvent);
    }
    else {
        HAGGLE_DBG("Generating event to trigger sending of fragment for data object %s\n", dataObjectRef->getIdStr());
        Event* repeatEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION, dataObjectRef, nodeRef, flags, _delay);
        this->kernel->addEvent(repeatEvent);
    }
}
