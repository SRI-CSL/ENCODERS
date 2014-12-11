/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "ApplicationManagerHelper.h"

ApplicationManagerHelper::ApplicationManagerHelper() {
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
    this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
}

ApplicationManagerHelper::~ApplicationManagerHelper() {
    if(this->networkCodingDataObjectUtility) {
        delete this->networkCodingDataObjectUtility;
        this->networkCodingDataObjectUtility = NULL;
    }
    if(this->fragmentationDataObjectUtility) {
    	delete this->fragmentationDataObjectUtility;
    	this->fragmentationDataObjectUtility = NULL;
    }
}

bool ApplicationManagerHelper::shouldNotSendToApplication(DataObjectRef dataObjectRef) {
    if( this->networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObjectRef)) {
        HAGGLE_DBG2("Not passing block %s to application\n",dataObjectRef->getIdStr());
        return true;
    }
    if(this->fragmentationDataObjectUtility->isFragmentationDataObject(dataObjectRef)) {
    	HAGGLE_DBG2("Not passing fragment %s to application\n",dataObjectRef->getIdStr());
    	return true;
    }
    HAGGLE_DBG2("Data object %s can be passed to application\n",dataObjectRef->getIdStr());
    return false;
}
