/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingDecoderManagerModuleProcessor.h"
#include "networkcoding/databitobject/DataObjectReceivedFlagsEnum.h"

NetworkCodingDecoderManagerModuleProcessor::NetworkCodingDecoderManagerModuleProcessor(
        NetworkCodingDecoderService* _networkCodingDecoderService,
        NetworkCodingConfiguration* _networkCodingConfiguration,
        HaggleKernel* _haggleKernel) {
    this->networkCodingDecoderService = _networkCodingDecoderService;
    this->networkCodingConfiguration = _networkCodingConfiguration;
    this->haggleKernel = _haggleKernel;
}

NetworkCodingDecoderManagerModuleProcessor::~NetworkCodingDecoderManagerModuleProcessor() {

}

void NetworkCodingDecoderManagerModuleProcessor::decode(
        NetworkCodingDecodingTaskRef networkCodingDecodingTask) {

    const DataObjectRef dObj = networkCodingDecodingTask->getDataObject();
    const NodeRef node; // = networkCodingDecodingTask->getNode(); // MOS - setting to NULL because 
    // there may not be a single source for the reconstructed data object (some optimization possible here)

    if (!dObj) {
      HAGGLE_ERR("Missing data object in task\n");
      return;
    }

    HAGGLE_DBG("Processing block %s to reconstruct parent data object\n", dObj->getIdStr());

    DataObjectRef dataObjectReceived =
            this->networkCodingDecoderService->decodeDataObject(dObj);

    if (dataObjectReceived) {
        HAGGLE_DBG("Sucessfully reconstructed coded data object %s, now raising events\n", 
		   DataObject::idString(dataObjectReceived).c_str());

        //add to bloom filter immediately
        this->haggleKernel->getThisNode()->getBloomfilter()->add(
                dataObjectReceived);

        DataObjectReceivedFlagsEnum dataObjectReceivedFlags = RECONSTRUCTED;
        double delay = 0.0;

	// MOS - changed order of the following two events
        Event* dataObjectIncomingEvent = new Event(EVENT_TYPE_DATAOBJECT_INCOMING,
                dataObjectReceived, node);
        this->haggleKernel->addEvent(dataObjectIncomingEvent);

        Event* dataObjectReceivedEvent = new Event(EVENT_TYPE_DATAOBJECT_RECEIVED,
                dataObjectReceived, node,dataObjectReceivedFlags,delay);
        this->haggleKernel->addEvent(dataObjectReceivedEvent);

	if(networkCodingConfiguration->isNodeDescUpdateOnReconstructionEnabled()) {
	  // MOS - immediately disseminate updated bloomfilter
	  this->haggleKernel->getThisNode()->setNodeDescriptionCreateTime();
	  this->haggleKernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));	
	}
    }
}
