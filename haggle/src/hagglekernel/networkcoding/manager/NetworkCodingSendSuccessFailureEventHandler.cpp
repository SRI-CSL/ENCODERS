/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingSendSuccessFailureEventHandler.h"
#include "Trace.h"

NetworkCodingSendSuccessFailureEventHandler::NetworkCodingSendSuccessFailureEventHandler(NetworkCodingEncoderStorage* _networkCodingEncoderStorage,
        NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility) {

    this->networkCodingEncoderStorage = _networkCodingEncoderStorage;
    this->networkCodingDataObjectUtility = _networkCodingDataObjectUtility;
}

NetworkCodingSendSuccessFailureEventHandler::~NetworkCodingSendSuccessFailureEventHandler() {

}

Event* NetworkCodingSendSuccessFailureEventHandler::retrieveOriginalDataObjectAndGenerateEvent(EventType type,
        DataObjectRef dataObjectRef, NodeRef nodeRef) {

    if (networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObjectRef)) {
        HAGGLE_DBG2("Processing block %s for raising event %d\n", dataObjectRef->getIdStr(), type);

        //retrieve original data object
        const DataObjectRef originalDataObject = this->getDataObjectForNetworkCodedDataObject(dataObjectRef);
        //this dataobject was not network encoded by this node
        if (!originalDataObject || originalDataObject.getObj() == NULL) {
            HAGGLE_DBG("Cannot find parent data object for block %s\n",
                    dataObjectRef->getIdStr());
            return NULL;
        }

        Event* newEvent = new Event(type, originalDataObject, nodeRef, 0);
        return newEvent;
    }
    return NULL;
}

const DataObjectRef NetworkCodingSendSuccessFailureEventHandler::getDataObjectForNetworkCodedDataObject(
        DataObjectRef& networkCodedDataObjectRef) {
    HAGGLE_DBG2("Processing block %s\n",
            networkCodedDataObjectRef->getIdStr());
    const string& originalDataObjectId =
            this->networkCodingDataObjectUtility->getOriginalDataObjectId(
                    networkCodedDataObjectRef.getObj());
    return this->networkCodingEncoderStorage->getDataObjectById(
            originalDataObjectId);
}
