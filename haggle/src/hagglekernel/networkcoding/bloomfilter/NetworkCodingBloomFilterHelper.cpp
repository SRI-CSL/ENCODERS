/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingBloomFilterHelper.h"
#include "Trace.h"

NetworkCodingBloomFilterHelper::NetworkCodingBloomFilterHelper() {
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
}

NetworkCodingBloomFilterHelper::~NetworkCodingBloomFilterHelper() {
    if(this->networkCodingDataObjectUtility) {
        delete this->networkCodingDataObjectUtility;
        this->networkCodingDataObjectUtility = NULL;
    }
}

DataObjectIdRef NetworkCodingBloomFilterHelper::getOriginalDataObjectId(DataObjectRef dataObject) {
    const string originalDataObjectId = this->networkCodingDataObjectUtility->checkAndGetOriginalDataObjectId(dataObject);
    if( originalDataObjectId.c_str() == NULL || originalDataObjectId.empty() ||  originalDataObjectId.length() < 1 ) {
        HAGGLE_DBG("no parent for data object %s found\n",dataObject->getIdStr());
        DataObjectIdRef empty;
        return empty;
    }

    DataObjectIdRef dataObjectIdRef = this->networkCodingDataObjectUtility->convertDataObjectIdStringToDataObjectIdType(originalDataObjectId);
    return dataObjectIdRef;
}

DataObjectIdRef NetworkCodingBloomFilterHelper::getThisOrParentDataObject(DataObjectRef dataObject) {
    const string originalDataObjectId = this->networkCodingDataObjectUtility->checkAndGetOriginalDataObjectId(dataObject);
    if( originalDataObjectId.c_str() == NULL || originalDataObjectId.empty() ||  originalDataObjectId.length() < 1 ) {
        HAGGLE_DBG("no parent objectid for id=%s\n",dataObject->getIdStr());
        return dataObject->getId();
    }
    DataObjectIdRef dataObjectIdRef = this->networkCodingDataObjectUtility->convertDataObjectIdStringToDataObjectIdType(originalDataObjectId);
    return dataObjectIdRef;
}

bool NetworkCodingBloomFilterHelper::isNetworkCodedObject(DataObjectRef dataObjectRef) {
	return this->networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObjectRef);
}
