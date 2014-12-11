/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationDecoderAsynchronousManagerModule.h"

FragmentationDecoderAsynchronousManagerModule::FragmentationDecoderAsynchronousManagerModule(
		FragmentationConfiguration* _fragmentationConfiguration,
		FragmentationDecoderService* _fragmentationDecoderService,
		FragmentationManager* _fragmentationManager, const string name) :
		ManagerModule<FragmentationManager>(_fragmentationManager, name), taskQueue(
				name) {

	this->fragmentationConfiguration = _fragmentationConfiguration;
	this->fragmentationDecoderService = _fragmentationDecoderService;
}

FragmentationDecoderAsynchronousManagerModule::~FragmentationDecoderAsynchronousManagerModule() {

}

bool FragmentationDecoderAsynchronousManagerModule::addTask(
		FragmentationDecodingTaskRef fragmentationDecodingTaskRef) {
	return taskQueue.insert(fragmentationDecodingTaskRef);
}

void FragmentationDecoderAsynchronousManagerModule::cleanup() {
	taskQueue.close();
	this->stop();
}

bool FragmentationDecoderAsynchronousManagerModule::run() {
	HAGGLE_DBG("FragmentationDecoderAsynchronousManagerModule running...\n");

	while (!shouldExit()) {
		QueueEvent_t qe;
		FragmentationDecodingTaskRef fragmentationDecodingTaskRef = NULL;

		qe = taskQueue.retrieve(&fragmentationDecodingTaskRef);

		switch (qe) {
		case QUEUE_ELEMENT:
			doTask(fragmentationDecodingTaskRef);

			// Delete task here or return it with result in private event?
			//delete task;
			break;
		case QUEUE_WATCH_ABANDONED:
			HAGGLE_DBG(
					"FragmentationDecoderAsynchronousManagerModule instructed to exit...\n");
			return false;
		default:
			HAGGLE_ERR(
					"Unknown FragmentationDecodingTaskRef task queue return value\n");
			break;
		}
	}
	return false;
}

void FragmentationDecoderAsynchronousManagerModule::doTask(
		FragmentationDecodingTaskRef fragmentationDecodingTaskRef) {

	switch (fragmentationDecodingTaskRef->getType()) {
	case FRAGMENTATION_DECODING_TASK_DECODE:
		HAGGLE_DBG2("FRAGMENTATION_DECODING_TASK_DECODE\n");

		this->decode(fragmentationDecodingTaskRef);
		break;

	}

	return;
}

void FragmentationDecoderAsynchronousManagerModule::decode(
		FragmentationDecodingTaskRef fragmentationDecodingTaskRef) {

	DataObjectRef dataObjectRef = fragmentationDecodingTaskRef->getDataObject();
	NodeRef nodeRef; // = fragmentationDecodingTaskRef->getNode(); // MOS - setting to NULL because 
	// there may not be a single source for the reconstructed data object (some optimization possible here)

	if (!dataObjectRef) {
		HAGGLE_ERR("Missing data object in task\n");
		return;
	}

	HAGGLE_DBG("Processing fragment %s to reconstruct parent data object\n", dataObjectRef->getIdStr());

	size_t fragmentationSize =
			this->fragmentationConfiguration->getFragmentSize();
	DataObjectRef reconstructedDataObjectRef =
			this->fragmentationDecoderService->decode(dataObjectRef,
					fragmentationSize);

	if (reconstructedDataObjectRef) {
	        HAGGLE_DBG("Sucessfully reconstructed fragmented data object %s - now raising events\n", 
			   DataObject::idString(reconstructedDataObjectRef).c_str());

                //add to bloom filter immediately
                this->getKernel()->getThisNode()->getBloomfilter()->add(
        		reconstructedDataObjectRef);

		Event* dataObjectIncomingEvent = new Event(
				EVENT_TYPE_DATAOBJECT_INCOMING, reconstructedDataObjectRef,
				nodeRef);
		this->addEvent(dataObjectIncomingEvent);

		Event* dataObjectReceivedEvent = new Event(
				EVENT_TYPE_DATAOBJECT_RECEIVED, reconstructedDataObjectRef,
				nodeRef, 1); // MOS - removed delay 1.0
		this->addEvent(dataObjectReceivedEvent);
		
		if(fragmentationConfiguration->isNodeDescUpdateOnReconstructionEnabled()) {
		  // MOS - immediately disseminate updated bloomfilter
		  this->getKernel()->getThisNode()->setNodeDescriptionCreateTime();
		  this->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));	
		}

		HAGGLE_DBG("Scheduling deletion of fragments associated with reconstructed data object %s\n",
				dataObjectRef->getIdStr());
		Event* deleteAssociatedFragmentsEvent =
				new Event(EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_FRAGMENTS,
						reconstructedDataObjectRef, nodeRef, 0,
						this->fragmentationConfiguration->getDelayDeleteReconstructedFragments());
		this->addEvent(deleteAssociatedFragmentsEvent);
	}

}

bool FragmentationDecoderAsynchronousManagerModule::startup() {
	return this->start();
}

bool FragmentationDecoderAsynchronousManagerModule::shutdownAndClose() {
	this->cleanup();
	return true;
}
