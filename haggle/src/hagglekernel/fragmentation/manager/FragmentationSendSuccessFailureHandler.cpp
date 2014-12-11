/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationSendSuccessFailureHandler.h"
#include "Trace.h"

FragmentationSendSuccessFailureHandler::FragmentationSendSuccessFailureHandler(
		FragmentationDataObjectUtility* _fragmentationDataObjectUtility,
		FragmentationEncoderStorage* _fragmentationEncoderStorage) {
	this->fragmentationDataObjectUtility = _fragmentationDataObjectUtility;
	this->fragmentationEncoderStorage = _fragmentationEncoderStorage;
}

FragmentationSendSuccessFailureHandler::~FragmentationSendSuccessFailureHandler() {

}

Event* FragmentationSendSuccessFailureHandler::retrieveOriginalDataObjectAndGenerateEvent(
		EventType type, DataObjectRef dataObjectRef, NodeRef node) {

	if (this->fragmentationDataObjectUtility->isFragmentationDataObject(
			dataObjectRef)) {

	        HAGGLE_DBG2("Processing fragment %s for raising event %d\n",
		   dataObjectRef->getIdStr(), type);

		//retrieve original data object
		FragmentParentDataObjectInfo fragmentParentDataObjectInfo =
				this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(
						dataObjectRef);
		string originalDataObjectId = fragmentParentDataObjectInfo.dataObjectId;
		const DataObjectRef originalDataObject =
				this->fragmentationEncoderStorage->getDataObjectByOriginalDataObjectId(originalDataObjectId);
		//this dataobject was not network encoded by this node
		if (!originalDataObject || originalDataObject.getObj() == NULL) {
			HAGGLE_DBG("Cannot find parent data object for fragment %s\n",
				   dataObjectRef->getIdStr());
			return NULL;
		}

		Event* newEvent = new Event(type, originalDataObject, node, 0);
		return newEvent;
	}
	return NULL;
}
