/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include "Event.h"
#include "FragmentationEncoderAsynchronousManagerModule.h"

FragmentationEncoderAsynchronousManagerModule::FragmentationEncoderAsynchronousManagerModule(
		FragmentationConfiguration* _fragmentationConfiguration,
		FragmentationDataObjectUtility* _fragmentationDataObjectUtility,
		FragmentationEncoderService* _fragmentationEncoderService,
		FragmentationManager* _fragmentationManager, const string name) :
		ManagerModule<FragmentationManager>(_fragmentationManager, name) {

    // MOS
    minEncoderDelayBase = 0;
    minEncoderDelayLinear = 0;
    minEncoderDelaySquare = 0;


	this->fragmentationConfiguration = _fragmentationConfiguration;
	this->fragmentationDataObjectUtility = _fragmentationDataObjectUtility;
	this->fragmentationEncoderService = _fragmentationEncoderService;
}

FragmentationEncoderAsynchronousManagerModule::~FragmentationEncoderAsynchronousManagerModule() {

}

bool FragmentationEncoderAsynchronousManagerModule::addTask(
		FragmentationEncodingTaskRef fragmentationEncodingTask) {
	HAGGLE_DBG2("Adding fragmentation task to task queue\n");
	taskQueue.insert(fragmentationEncodingTask);
	return true;
}

void FragmentationEncoderAsynchronousManagerModule::cleanup() {
	HAGGLE_DBG("Calling FragmentationEncoderAsynchronousManagerModule cleanup\n");
	this->taskQueue.close();
	this->stop();
}

bool FragmentationEncoderAsynchronousManagerModule::run() {
	HAGGLE_DBG("FragmentationEncoderAsynchronousManagerModule running...\n");

	while (!shouldExit()) {
		QueueEvent_t qe;
		FragmentationEncodingTaskRef task = NULL;

		qe = taskQueue.retrieve(&task);

		switch (qe) {
		case QUEUE_ELEMENT:
			doTask(task);

			// Delete task here or return it with result in private event?
			//delete task;
			break;
		case QUEUE_WATCH_ABANDONED:
			HAGGLE_DBG("FragmentationEncodingTaskRef instructed to exit...\n");
			return false;
		default:
			HAGGLE_ERR("Unknown FragmentationEncodingTask task queue return value\n");
			break;
		}

		// MOS - basic encoder rate limit
		int minEncoderDelay = minEncoderDelayBase;
		int numNeighbors = getKernel()->getNodeStore()->numNeighbors();
		minEncoderDelay += minEncoderDelayLinear * numNeighbors;
		minEncoderDelay += minEncoderDelaySquare * numNeighbors * numNeighbors;
		cancelableSleep(minEncoderDelay);
	}
	return false;
}

void FragmentationEncoderAsynchronousManagerModule::doTask(
		FragmentationEncodingTaskRef fragmentationEncodingTask) {

	switch (fragmentationEncodingTask->getType()) {
	case FRAGMENTATION_ENCODING_TASK_ENCODE:
		HAGGLE_DBG2("FRAGMENTATION_ENCODING_TASK_ENCODE\n");

		this->encode(fragmentationEncodingTask);
		break;

	}

	return;
}

void FragmentationEncoderAsynchronousManagerModule::encode(
		FragmentationEncodingTaskRef fragmentationEncodingTask) {

	DataObjectRef dataObject = fragmentationEncodingTask->getDataObject();
	NodeRefList nodeRefList = fragmentationEncodingTask->getNodeRefList();

	if (!dataObject) {
		HAGGLE_ERR("Missing data object in task\n");
		return;
	}

	for(NodeRefList::iterator iter = nodeRefList.begin();iter!=nodeRefList.end();iter++) {
	        NodeRef nodeRef = getKernel()->getNodeStore()->retrieve(*iter); // MOS - this is important because node may have been updated
		if(!nodeRef) {
		  HAGGLE_DBG("Destination node missing from node store\n");
		  continue;
		}

		HAGGLE_DBG("Perform fragmentation for data object %s - filepath=%s filename=%s\n",
			   dataObject->getIdStr(), fragmentationDataObjectUtility->getFilePath(dataObject).c_str(), 
			   fragmentationDataObjectUtility->getFileName(dataObject).c_str());

		size_t fragmentSize = this->fragmentationConfiguration->getFragmentSize();
		List<DataObjectRef> dataObjectRefList =
				this->fragmentationEncoderService->getAllFragmentsForDataObject(
						dataObject, fragmentSize,nodeRef);
		if(dataObjectRefList.size() > 0) {
		  for (List<DataObjectRef>::iterator iter = dataObjectRefList.begin();
				iter != dataObjectRefList.end(); iter++) {
			DataObjectRef fragmentDataObject = *iter;
			double resendDelay = this->fragmentationConfiguration->getResendDelay();
			HAGGLE_DBG("Generated fragment %s for data object %s\n",
				   fragmentDataObject->getIdStr(), dataObject->getIdStr());
			NodeRefList nodeRefListActual;
			nodeRefListActual.push_back(nodeRef);
			Event* sendEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND,
			    fragmentDataObject, nodeRefListActual, 0); // MOS - should be 0 not resendDelay
			this->addEvent(sendEvent);
		  }
		} else {
		  HAGGLE_DBG("Generated already all fragments for data object %s - raising EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL\n",
			     dataObject->getIdStr());
		  for (NodeRefList::iterator it = nodeRefList.begin(); it != nodeRefList.end(); it++) {
		    NodeRef& node = *it;
		    this->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dataObject, node));
		  }
		}
	}

}

bool FragmentationEncoderAsynchronousManagerModule::startup() {
	return this->start();
}

bool FragmentationEncoderAsynchronousManagerModule::shutdownAndClose() {
	this->cleanup();
	return true;
}
