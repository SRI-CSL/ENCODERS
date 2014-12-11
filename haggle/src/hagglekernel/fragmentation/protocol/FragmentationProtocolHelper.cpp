/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "Event.h"

#include "FragmentationProtocolHelper.h"

FragmentationProtocolHelper::FragmentationProtocolHelper() {
	this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
}

FragmentationProtocolHelper::~FragmentationProtocolHelper() {
	if (this->fragmentationDataObjectUtility) {
		delete this->fragmentationDataObjectUtility;
		this->fragmentationDataObjectUtility = NULL;
	}
}

Event* FragmentationProtocolHelper::onSendDataObject(DataObjectRef& originalDataObjectRef,
		NodeRef& node, EventType send_data_object_actual_event) {

        if(node->getType() == Node::TYPE_APPLICATION) {
        	//do nothing
        }
        else {
        	bool fragmentedForThisTarget = this->fragmentationDataObjectUtility->shouldBeFragmentedCheckTargetNode(originalDataObjectRef,node);
        	if(fragmentedForThisTarget) {
		  NodeRefList fragmentationTargetNodeRefList;
		  fragmentationTargetNodeRefList.add(node);
		  Event* newEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION, originalDataObjectRef, fragmentationTargetNodeRefList);
		  return newEvent;
		}
	}

    return NULL;
}
