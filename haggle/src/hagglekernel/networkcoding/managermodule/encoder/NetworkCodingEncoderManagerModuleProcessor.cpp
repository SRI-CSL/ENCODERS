/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingEncoderManagerModuleProcessor.h"

NetworkCodingEncoderManagerModuleProcessor::NetworkCodingEncoderManagerModuleProcessor(
        NetworkCodingEncoderService* _networkCodingEncoderService,
        HaggleKernel* _haggleKernel) {
    this->networkCodingEncoderService = _networkCodingEncoderService;
    this->haggleKernel = _haggleKernel;
}

NetworkCodingEncoderManagerModuleProcessor::~NetworkCodingEncoderManagerModuleProcessor() {

}

void NetworkCodingEncoderManagerModuleProcessor::encode(NetworkCodingEncoderTaskRef networkCodingEncoderTask) {

    const DataObjectRef originalDataObjectRef =
            networkCodingEncoderTask->getDataObject();
    const NodeRefList nodeRefList = networkCodingEncoderTask->getNodeRefList();

    HAGGLE_DBG("Perform network coding for data object %s\n", originalDataObjectRef->getIdStr());

    DataObjectRef networkCodedDataObject =
            this->networkCodingEncoderService->encodeDataObject(originalDataObjectRef);
    if(networkCodedDataObject) { // MOS
      HAGGLE_DBG("Generated block %s for data object %s\n", networkCodedDataObject->getIdStr(), originalDataObjectRef->getIdStr());

      Event* sendEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND,
				   networkCodedDataObject, nodeRefList);
      this->haggleKernel->addEvent(sendEvent);
    }
}
