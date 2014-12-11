/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationBloomFilterHelper.h"

FragmentationBloomFilterHelper::FragmentationBloomFilterHelper() {
	this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
	this->dataObjectTypeIdentifierUtility =
			new DataObjectTypeIdentifierUtility();

}

FragmentationBloomFilterHelper::~FragmentationBloomFilterHelper() {
	if (this->fragmentationDataObjectUtility) {
		delete this->fragmentationDataObjectUtility;
		this->fragmentationDataObjectUtility = NULL;
	}
	if (this->dataObjectTypeIdentifierUtility) {
		delete this->dataObjectTypeIdentifierUtility;
		this->dataObjectTypeIdentifierUtility = NULL;
	}
}

DataObjectIdRef FragmentationBloomFilterHelper::getOriginalDataObjectId(DataObjectRef dataObject) {
    const string originalDataObjectId = this->fragmentationDataObjectUtility->checkAndGetOriginalDataObjectId(dataObject);
    if( originalDataObjectId.c_str() == NULL || originalDataObjectId.empty() ||  originalDataObjectId.length() < 1 ) {
        HAGGLE_DBG("no parent for data object %s found\n",dataObject->getIdStr());
        DataObjectIdRef empty;
        return empty;
    }

    DataObjectIdRef dataObjectIdRef = this->fragmentationDataObjectUtility->convertDataObjectIdStringToDataObjectIdType(originalDataObjectId);
    return dataObjectIdRef;
}

#if 0

DataObjectIdRef FragmentationBloomFilterHelper::getOriginalDataObjectId(
		DataObjectRef dataObject) {

/* MOS - allow access to original data object for Bloomfilter check even if this is
         not a fragment but the block of a fragment

	if (!this->fragmentationDataObjectUtility->isFragmentationDataObject(
			dataObject)) {
		DataObjectIdRef empty;
		return empty;
	}
*/
	FragmentParentDataObjectInfo fragmentParentDataObjectInfo =
			this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(
					dataObject);

	const string originalDataObjectId = fragmentParentDataObjectInfo.dataObjectId;
	if (originalDataObjectId.c_str() == NULL || originalDataObjectId.empty()
			|| originalDataObjectId.length() < 1) {
		HAGGLE_DBG2("no parent for data object %s found\n", dataObject->getIdStr());
		DataObjectIdRef empty;
		return empty;
	}

	DataObjectIdRef dataObjectIdRef =
			this->dataObjectTypeIdentifierUtility->convertDataObjectIdStringToDataObjectIdType(
					originalDataObjectId);
	return dataObjectIdRef;

}
#endif

bool FragmentationBloomFilterHelper::isFragmentationObject(
		DataObjectRef dataObjectRef) {
	return this->fragmentationDataObjectUtility->isFragmentationDataObject(
			dataObjectRef);
}
