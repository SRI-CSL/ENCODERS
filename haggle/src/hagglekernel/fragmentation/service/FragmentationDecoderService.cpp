/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationDecoderService.h"

FragmentationDecoderService::FragmentationDecoderService(
		FragmentationDecoderStorage* _fragmentationDecoderStorage,
		FragmentationDataObjectUtility* _fragmentationDataObjectUtility) {
	this->fragmentationDecoderStorage = _fragmentationDecoderStorage;
	this->fragmentationDataObjectUtility = _fragmentationDataObjectUtility;
}

FragmentationDecoderService::~FragmentationDecoderService() {

}

DataObjectRef FragmentationDecoderService::decode(
		DataObjectRef fragmentDataObjectRef, size_t fragmentSize) {
	FragmentParentDataObjectInfo fragmentParentDataObjectInfo =
			this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(
					fragmentDataObjectRef);

	string parentDataObjectId = fragmentParentDataObjectInfo.dataObjectId;

	bool isCompleted = this->fragmentationDecoderStorage->isCompleted(
			parentDataObjectId);
	if (isCompleted) {
		HAGGLE_DBG("Ignoring unnecessary fragment %s of %s - already completed decoding\n",
			   fragmentDataObjectRef->getIdStr(), parentDataObjectId.c_str());
		return NULL;
	}

	bool isAddFragment = this->fragmentationDecoderStorage->addFragment(
			fragmentDataObjectRef, fragmentSize);
	if(!isAddFragment) {
	    HAGGLE_ERR("Unable to add fragment to storage\n");
	    return NULL;
	}

	isCompleted = this->fragmentationDecoderStorage->isCompleted(
			parentDataObjectId);
	if (isCompleted) {
		HAGGLE_DBG("Decoding successfully completed for %s - last fragment was %s\n",
			   parentDataObjectId.c_str(), fragmentDataObjectRef->getIdStr());

		FragmentDecoderStructRef fragmentDecoderStructRef =
				this->fragmentationDecoderStorage->getFragmentDecoderStruct(
						parentDataObjectId);
		string filePathReconstructedDataObject =
				fragmentDecoderStructRef->filePath;

		DataObjectRef dataObjectReceived = DataObject::create(
				filePathReconstructedDataObject,
				fragmentParentDataObjectInfo.fileName);

		if(!dataObjectReceived) {
		  HAGGLE_ERR("Unable to create data object for file %s (%s)\n", filePathReconstructedDataObject.c_str(), fragmentParentDataObjectInfo.fileName.c_str());
		  return NULL;
		}

		dataObjectReceived->setPersistent(true);
		dataObjectReceived->setStored(true);

		bool isCopyAttributesToDataObject = this->fragmentationDataObjectUtility->copyAttributesToDataObject(
				dataObjectReceived, fragmentDataObjectRef);
		if(!isCopyAttributesToDataObject) {
		    HAGGLE_ERR("Unable to copy attributes to parent data object\n");
		    return NULL;
		}

		dataObjectReceived->restoreEncryptedDataObject(); // MOS

		this->fragmentationDecoderStorage->trackReconstructedDataObject(dataObjectReceived->getIdStr());

		HAGGLE_STAT("Reconstructed fragmented data object %s\n", dataObjectReceived->getIdStr());
		return dataObjectReceived;

	}

	return NULL;
}
