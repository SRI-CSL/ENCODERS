/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include <stdio.h>
#include <stdlib.h>

#include "HaggleKernel.h"
#include "NetworkCodingManager.h"
#include "networkcoding/codetorrentencoder.h"
#include "networkcoding/concurrent/NetworkCodingDecodingTaskType.h"
#include "networkcoding/concurrent/NetworkCodingDecodingTask.h"
#include "networkcoding/managermodule/NetworkCodingDecoderAsynchronousManagerModule.h"
#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTaskType.h"
#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTask.h"
#include "networkcoding/managermodule/encoder/NetworkCodingEncoderAsynchronousManagerModule.h"
#include "networkcoding/databitobject/DataObjectReceivedFlagsEnum.h"
#include "stringutils/CSVUtility.h"

NetworkCodingManager::NetworkCodingManager(HaggleKernel *_haggle) :
        Manager("NetworkCodingManager", _haggle), minTimeBetweenToggles(10.0) {

    this->networkCodingEncoderStorage = new NetworkCodingEncoderStorage();
    this->networkCodingFileUtility = new NetworkCodingFileUtility();
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
    this->networkCodingConfiguration = new NetworkCodingConfiguration();
    this->codeTorrentUtility = new CodeTorrentUtility();
    this->fragmentationConfiguration = new FragmentationConfiguration();

    this->networkCodingDecoderStorage = new NetworkCodingDecoderStorage(
    		this->getKernel(),
            this->getKernel()->getDataStore(),
            this->networkCodingDataObjectUtility,
            this->networkCodingConfiguration);

    this->networkCodingDecoderService = new NetworkCodingDecoderService(
            this->networkCodingDecoderStorage, this->networkCodingFileUtility,
            this->networkCodingConfiguration,
            this->networkCodingDataObjectUtility);

    this->networkCodingSendSuccessFailureEventHandler =
            new NetworkCodingSendSuccessFailureEventHandler(
                    this->networkCodingEncoderStorage,
                    this->networkCodingDataObjectUtility);

    this->networkCodingEncoderService = new NetworkCodingEncoderService(
            this->networkCodingFileUtility,
            this->networkCodingDataObjectUtility,
            this->networkCodingEncoderStorage, this->codeTorrentUtility,
            this->networkCodingConfiguration);
}

NetworkCodingManager::~NetworkCodingManager() {
	if (this->fragmentationConfiguration) {
		delete this->fragmentationConfiguration;
		this->fragmentationConfiguration = NULL;
	}
    if (this->networkCodingEncoderService) {
        delete this->networkCodingEncoderService;
        this->networkCodingEncoderService = NULL;
    }
    if (this->networkCodingDecoderService) {
        delete this->networkCodingDecoderService;
        this->networkCodingDecoderService = NULL;
    }
    if (this->networkCodingSendSuccessFailureEventHandler) {
        delete this->networkCodingSendSuccessFailureEventHandler;
        this->networkCodingSendSuccessFailureEventHandler = NULL;
    }
    if (this->networkCodingConfiguration) {
        delete this->networkCodingConfiguration;
        this->networkCodingConfiguration = NULL;
    }
    if (this->networkCodingDataObjectUtility) {
        delete this->networkCodingDataObjectUtility;
        this->networkCodingDataObjectUtility = NULL;
    }
    if (this->networkCodingFileUtility) {
        delete this->networkCodingFileUtility;
        this->networkCodingFileUtility = NULL;
    }
    if (this->networkCodingDecoderStorage) {
        delete this->networkCodingDecoderStorage;
        this->networkCodingDecoderStorage = NULL;
    }
    if (this->networkCodingEncoderStorage) {
        delete this->networkCodingEncoderStorage;
        this->networkCodingEncoderStorage = NULL;
    }
    if (this->codeTorrentUtility) {
        delete this->codeTorrentUtility;
        this->codeTorrentUtility = NULL;
    }
    if (this->networkCodingDecoderManagerModule) {
        delete this->networkCodingDecoderManagerModule;
        this->networkCodingDecoderManagerModule = NULL;
    }
    if (this->networkCodingEncoderManagerModule) {
        delete this->networkCodingEncoderManagerModule;
        this->networkCodingEncoderManagerModule = NULL;
    }
    if (this->nodeQueryCallback) {
        delete this->nodeQueryCallback;
        this->nodeQueryCallback = NULL;
    }
}

bool NetworkCodingManager::processOriginalDataObject(
        DataObjectRef& dataObjectRef, NodeRefList nodeRefList) {

    string parentDataObjectId = dataObjectRef->getIdStr();
    this->networkCodingEncoderStorage->trackSendEvent(parentDataObjectId);

    NetworkCodingEncoderTask* networkCodingEncoderTask =
            new NetworkCodingEncoderTask(NETWORKCODING_ENCODING_TASK_ENCODE,
                    dataObjectRef, nodeRefList);
    NetworkCodingEncoderTaskRef networkCodingEncoderTaskRef =
            networkCodingEncoderTask;
    bool addTask = this->networkCodingEncoderManagerModule->addTask(
            networkCodingEncoderTaskRef);
    return addTask;
}

void NetworkCodingManager::onDataObjectForSendNetworkCodedBlock(Event *e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring send data object\n");
      return;
    }

    DataObjectRef dataObject = e->getDataObject();
    NodeRefList nodeRefList = e->getNodeList();

    if (!dataObject) {
        HAGGLE_ERR("Missing data object in event\n");
        return;
    }

    HAGGLE_DBG("Decided to send data object %s with network coding - filepath=%s filename=%s\n",
            dataObject->getIdStr(), networkCodingDataObjectUtility->getFilePath(dataObject).c_str(), 
            networkCodingDataObjectUtility->getFileName(dataObject).c_str());

    NodeRefList networkCodingTargetNodeRefList;

    if (networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObject)) {
        HAGGLE_DBG("Already networkcoded, so just generating send event\n");
        Event* sendEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND, dataObject,
                nodeRefList);
        this->getKernel()->addEvent(sendEvent);
        return;
    }

    for (NodeRefList::iterator it = nodeRefList.begin(); it != nodeRefList.end(); it++) {
        NodeRef& node = *it;
        if(node->getType() == Node::TYPE_APPLICATION) {
        	//do nothing
        }
        else {
        	bool networkCodedForThisTarget = this->networkCodingDataObjectUtility->shouldBeNetworkCodedCheckTargetNode(dataObject,node);
        	if(networkCodedForThisTarget) {
        		networkCodingTargetNodeRefList.add(node);
        	}
        }
    }

    if(!networkCodingTargetNodeRefList.empty()) {
        HAGGLE_DBG("Not already networkcoded, so need to generate block\n");

        string parentDataObjectId = dataObject->getIdStr();
        double resendReconstructedDelay = this->networkCodingConfiguration->getResendReconstructedDelay();
        double resendReconstructedDelayCalculated = this->networkCodingDecoderStorage->getResendReconstructedDelay(parentDataObjectId,resendReconstructedDelay);
        if( resendReconstructedDelayCalculated > 0.0 ) {
            HAGGLE_DBG("Reschedule sending of parent data object %s with delay=%f\n",parentDataObjectId.c_str(),resendReconstructedDelayCalculated);
            kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dataObject, networkCodingTargetNodeRefList, resendReconstructedDelayCalculated)); // MOS
            // MOS - using EVENT_TYPE_DATAOBJECT_SEND instead of EVENT_TYPE_DATAOBJECT_FRAGMENTATION_SEND to have another Bloom filter check
            return;
        }

        bool addTask = this->processOriginalDataObject(dataObject, networkCodingTargetNodeRefList);
		if (!addTask) {
			HAGGLE_ERR("Error adding task to task queue\n");
		}
	}
}

void NetworkCodingManager::onDataObjectForReceiveNetworkCodedBlock(Event *e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring data object received\n");
      return;
    }

    unsigned long int flag = e->getFlags();
    DataObjectRef dataObjectRef = e->getDataObject();

    if (!dataObjectRef) {
      HAGGLE_ERR("Missing data object in event\n");
      return;
    }

    HAGGLE_DBG2("Received data object %s - event flag=%d\n", dataObjectRef->getIdStr(), flag);

    NodeRef nodeRef = e->getNode(); // MOS - not available for EVENT_TYPE_DATAOBJECT_NEW events

    /* MOS - nodeRef is not used anymore and is NULL for reconstructed objects

    if(!nodeRef) {
      InterfaceRef remoteIface = dataObjectRef->getRemoteInterface();
      if(!remoteIface) {
	HAGGLE_DBG("Missing remote interface for data object %s\n", dataObjectRef->getIdStr());
	return;
      }      
      nodeRef = kernel->getNodeStore()->retrieve(remoteIface,false); // MOS - fixed, node may not be a neighbor anymore

      if(!nodeRef) {
	HAGGLE_DBG("Cannot find peer node for data object %s\n", dataObjectRef->getIdStr());
	return;
      }
    }
    */

    //check to make sure is network coded block since this is EVENT_TYPE_DATAOBJECT_RECEIVED
    bool isNetworkCodedObject = this->networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObjectRef);
    bool isFragDataObject = this->networkCodingDataObjectUtility->isFragmentationDataObject(dataObjectRef); // CBMEN, HL

    if( e->getType() == EVENT_TYPE_DATAOBJECT_NEW && NONE == flag && isNetworkCodedObject) {
        HAGGLE_DBG("Creating reconstruction task for block %s\n", dataObjectRef->getIdStr());

	if(this->networkCodingConfiguration->getBlockSize() == 0) {
	  HAGGLE_ERR("Network coding (block size) not properly configured - ignoring network-coded block\n");
	} else {
        
        if (this->networkCodingConfiguration->getDecodeOnlyIfTarget()) {
            HAGGLE_STAT("Deferring decoder creation of data object %s\n", dataObjectRef->getIdStr());
            kernel->getDataStore()->doNodeQuery(dataObjectRef, 5, 1, nodeQueryCallback, true); 
        } else {
            NetworkCodingDecodingTask* networkCodingDecodingTask =
                new NetworkCodingDecodingTask(NETWORKCODING_DECODING_TASK_DECODE,
                        dataObjectRef, nodeRef);
            this->networkCodingDecoderManagerModule->addTask(networkCodingDecodingTask);
        }
      
	}

        double delayDeleteNetworkCodedBlocks =
                this->networkCodingConfiguration->getDelayDeleteNetworkCodedBlocks();
        if (0.0 != delayDeleteNetworkCodedBlocks) {
            HAGGLE_DBG("Scheduling deletion of block %s\n", dataObjectRef->getIdStr());
            Event* deleteNetworkCodedBlock =
                    new Event(EVENT_TYPE_DATAOBJECT_DELETE_NETWORKCODEDBLOCK,
                            dataObjectRef, nodeRef, 0,
                            this->networkCodingConfiguration->getDelayDeleteNetworkCodedBlocks());
            kernel->addEvent(deleteNetworkCodedBlock);
        }
    }
    else if( e->getType() == EVENT_TYPE_DATAOBJECT_RECEIVED && RECONSTRUCTED == flag ){
        HAGGLE_DBG("Scheduling deletion of associated blocks for data object %s - flag = RECONSTRUCTED\n",dataObjectRef->getIdStr());

        Event* deleteAssociatedNetworkCodedBlocks =
                new Event(EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_NETWORKCODEDBLOCKS,
			  dataObjectRef,nodeRef,0,this->networkCodingConfiguration->getDelayDeleteReconstructedNetworkCodedBlocks());
        kernel->addEvent(deleteAssociatedNetworkCodedBlocks);

        if (dataObjectRef) {
            this->networkCodingDecoderStorage->deleteDecoderFromStorageByDataObjectId(dataObjectRef->getIdStr());
        }
    // CBMEN, HL, Begin - frag to nc
    } else if (e->getType() == EVENT_TYPE_DATAOBJECT_NEW && isFragDataObject && 
               this->networkCodingDecoderStorage->haveDecoder(this->networkCodingDataObjectUtility->getFragmentationOriginalDataObjectId(dataObjectRef))) {
        HAGGLE_DBG("Scheduling fragment to block conversion for data object %s\n", dataObjectRef->getIdStr());
        Event* incomingFragmentationToNetworkCodedBlock = new Event(EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION, dataObjectRef, 0, 1);
        kernel->addEvent(incomingFragmentationToNetworkCodedBlock);
    }
    // CBMEN, HL, End

}

void NetworkCodingManager::onDataObjectSendSuccessful(Event* e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring send data object results\n");
      return;
    }

    DataObjectRef dataObjectRef = e->getDataObject();

    NodeRef node = e->getNode();

    unsigned long flags = e->getFlags();

    Event* newEvent = NULL;

    bool isNetworkCodedObject = this->networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObjectRef);

    if(!isNetworkCodedObject) {
    	HAGGLE_DBG2("Not a networkcoded data object %s\n",dataObjectRef->getIdStr());
    	return;
    }

    HAGGLE_DBG("Data object %s sucessfully sent - flags=%d\n", dataObjectRef->getIdStr(), flags);

    //send reject so already have
    // Protocol.h enum ProtocolEvent
    if (flags == 2) {
        HAGGLE_DBG("Preparing send-sucessful event for parent data object of %s - after reject\n", dataObjectRef->getIdStr());
        newEvent =
                this->networkCodingSendSuccessFailureEventHandler->retrieveOriginalDataObjectAndGenerateEvent(
                        EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dataObjectRef,
                        node);
    }
    else {
        if(dataObjectRef->getRemoteInterface()) return; // MOS - no event translation for forwarded blocks
        HAGGLE_DBG("Preparing send-sucessful event to trigger next block of parent data object of %s\n", dataObjectRef->getIdStr());
        newEvent =
                this->networkCodingSendSuccessFailureEventHandler->retrieveOriginalDataObjectAndGenerateEvent(
                        EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL,
                        dataObjectRef, node);
    }

    if (newEvent != NULL) {
        HAGGLE_DBG("Raising event %d for parent object %s of block %s\n",
		   newEvent->getType(),
		   DataObject::idString(newEvent->getDataObject()).c_str(),
		   DataObject::idString(dataObjectRef).c_str());
        this->kernel->addEvent(newEvent);
    }

}

void NetworkCodingManager::onDataObjectSendFailure(Event* e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring send data object results\n");
      return;
    }

    DataObjectRef dataObjectRef = e->getDataObject();
    NodeRef nodeRef = e->getNode();

    HAGGLE_DBG("Received send-failure event for data object %s\n", dataObjectRef->getIdStr());

    Event* newEvent =
            this->networkCodingSendSuccessFailureEventHandler->retrieveOriginalDataObjectAndGenerateEvent(
                    EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dataObjectRef, nodeRef);

    if (newEvent != NULL) {
        this->kernel->addEvent(newEvent);
    }
}

void NetworkCodingManager::onDeleteNetworkCodedBlock(Event* e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    DataObjectRef networkCodedBlock = e->getDataObject();
    HAGGLE_DBG("Received event to dispose block %s\n",
            networkCodedBlock->getIdStr());
    this->networkCodingDecoderStorage->disposeNetworkCodedBlock(
            networkCodedBlock);
}

void NetworkCodingManager::onDeleteAssociatedNetworkCodedBlocks(Event* e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    DataObjectRef dataObjectRef = e->getDataObject();

    HAGGLE_DBG("Received event to delete associated blocks of data objects %s\n",
	       dataObjectRef->getIdStr());

    string parentDataObjectId = dataObjectRef->getIdStr();
    this->networkCodingDecoderStorage->disposeNetworkCodedBlocks(parentDataObjectId);
    this->networkCodingDecoderStorage->deleteDecoderFromStorageByDataObjectId(parentDataObjectId);
}

// CBMEN, HL, Begin
void NetworkCodingManager::onDeletedDataObject(Event* e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    if (!e || !e->hasData()) {
        HAGGLE_DBG2("Missing data object in event\n");
        return;
    }

    DataObjectRefList dObjs = e->getDataObjectList();

    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        DataObjectRef dataObjectRef = (*it);

        if (!dataObjectRef->isNodeDescription() && !dataObjectRef->isControlMessage()) {
            HAGGLE_DBG2("Removing deleted data object %s from network coding storage - flags=%d\n", dataObjectRef->getIdStr(),e->getFlags());
            string parentDataObjectId = dataObjectRef->getIdStr();
            this->networkCodingDecoderStorage->disposeNetworkCodedBlocks(parentDataObjectId);
            this->networkCodingDecoderStorage->deleteDecoderFromStorageByDataObjectId(parentDataObjectId);
            this->networkCodingEncoderStorage->deleteFromStorageByDataObjectId(parentDataObjectId);
            this->networkCodingEncoderStorage->deleteTrackingInfoByDataObjectId(parentDataObjectId); 
        }
    }
}
// CBMEN, HL

void NetworkCodingManager::onDataObjectAgingNetworkCodedBlock(Event* e) {
    if (kernel->isShuttingDown()) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    HAGGLE_DBG("Received event to age block data objects\n");

    double maxAgeBlock = this->networkCodingConfiguration->getMaxAgeBlock();
    this->networkCodingEncoderStorage->ageOffBlocks(maxAgeBlock);

    //reschedule
    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK, NULL, 0, maxAgeBlock/10));
}

void NetworkCodingManager::onAgingDecoderNetworkCoding(Event* e) {
    if (kernel->isShuttingDown()) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    HAGGLE_DBG("Received event to age network coding decoder\n");

    double maxAgeDecoder = this->networkCodingConfiguration->getMaxAgeDecoder();
    this->networkCodingDecoderStorage->ageOffDecoder(maxAgeDecoder);

    //reschedule
    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_AGING_DECODER_NETWORKCODING, NULL, 0, maxAgeDecoder/10));
}

void NetworkCodingManager::onDisableNetworkCoding(Event* e) {
    if (kernel->isShuttingDown()) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    HAGGLE_DBG("Received event to disable network coding\n");

    // dataobject is optional which means encode all objects destined for target
    bool hasDataObject = false;
    DataObjectRef dataObjectToNetworkCode = NULL;

    NodeRefList targetNodeRefListToDisableNetworkCode = e->getNodeList();

    for (NodeRefList::iterator it = targetNodeRefListToDisableNetworkCode.begin(); it != targetNodeRefListToDisableNetworkCode.end(); it++) {
        NodeRef& nodeRef = *it;
        if(nodeRef->getType() == Node::TYPE_APPLICATION || nodeRef->getType() == Node::TYPE_GATEWAY ||
        		nodeRef->getType() == Node::TYPE_LOCAL_DEVICE) {
        	//do nothing
        }
        else if(hasDataObject) {
        	string parentDataObjectId = dataObjectToNetworkCode->getIdStr();
        	HAGGLE_DBG("Disable network coding for dataobjectid=%s noderefid=%s\n",
        			parentDataObjectId.c_str(),nodeRef->getName().c_str());
            this->fragmentationConfiguration->turnOnFragmentationForDataObjectandTargetNode(parentDataObjectId,nodeRef); // CBMEN, HL
        	this->networkCodingConfiguration->turnOffNetworkCodingForDataObjectandTargetNode(parentDataObjectId,nodeRef);
        }
        else {
            // CBMEN, HL, Begin
            string nid = nodeRef->getName();
            if (!this->networkCodingConfiguration->isNetworkCodingEnabled(NULL, nodeRef)) {
                HAGGLE_DBG("Network coding already disabled for target node %s! ignoring.\n", nid.c_str());
            } else {
                Timeval now = Timeval::now();
                bool toToggle = true;
                HashMap<string, Timeval>::iterator lit = lastToggleTimes.find(nid);
                if (lit != lastToggleTimes.end()) {
                    Timeval then = (*lit).second;
                    if ((now - then).getTimeAsSecondsDouble() < minTimeBetweenToggles) {
                        toToggle = false;
                    } else {
                        lastToggleTimes.erase(lit);
                    }
                }
                if (toToggle) {
                	HAGGLE_DBG("Disable network coding for all objects for nodename=%s\n",nid.c_str());
                    lastToggleTimes.insert(make_pair(nid, now));
                	this->fragmentationConfiguration->turnOnFragmentationForDataObjectandTargetNode("",nodeRef);
                	this->networkCodingConfiguration->turnOffNetworkCodingForDataObjectandTargetNode("",nodeRef);
                } else {
                    HAGGLE_DBG("Trying to disable network coding for %s less than %fs after enabled! ignoring.\n", nid.c_str(), minTimeBetweenToggles);
                }
            }
            // CBMEN, HL, End
        }
    }


}

void NetworkCodingManager::onEnableNetworkCoding(Event* e) {
    if (kernel->isShuttingDown()) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    HAGGLE_DBG("Received event to enable network coding\n");

    // dataobject is optional which means encode all objects destined for target
    bool hasDataObject = false;
    DataObjectRef dataObjectToNetworkCode = NULL;
    /*
    if(e->hasData()) {
    	hasDataObject = true;
    	dataObjectToNetworkCode = e->getDataObject();
    }
    */

    NodeRefList targetNodeRefListToNetworkCode = e->getNodeList();

    for (NodeRefList::iterator it = targetNodeRefListToNetworkCode.begin(); it != targetNodeRefListToNetworkCode.end(); it++) {
        NodeRef& nodeRef = *it;
        if(nodeRef->getType() == Node::TYPE_APPLICATION || nodeRef->getType() == Node::TYPE_GATEWAY ||
        		nodeRef->getType() == Node::TYPE_LOCAL_DEVICE) {
        	//do nothing
        }
        else if(hasDataObject) {
        	string parentDataObjectId = dataObjectToNetworkCode->getIdStr();
        	HAGGLE_DBG("Enabling network coding for dataobjectid=%s nodename=%s\n",
        			parentDataObjectId.c_str(),nodeRef->getName().c_str());
            this->fragmentationConfiguration->turnOffFragmentationForDataObjectandTargetNode(parentDataObjectId,nodeRef); // CBMEN, HL
        	this->networkCodingConfiguration->turnOnNetworkCodingForDataObjectandTargetNode(parentDataObjectId,nodeRef);
        }
        else {
            // CBMEN, HL, Begin
            string nid = nodeRef->getName();
            if (this->networkCodingConfiguration->isNetworkCodingEnabled(NULL, nodeRef)) {
                HAGGLE_DBG("Network coding already enabled for target node %s! ignoring.\n", nid.c_str());
            } else {
                Timeval now = Timeval::now();
                bool toToggle = true;
                HashMap<string, Timeval>::iterator lit = lastToggleTimes.find(nid);
                if (lit != lastToggleTimes.end()) {
                    Timeval then = (*lit).second;
                    if ((now - then).getTimeAsSecondsDouble() < minTimeBetweenToggles) {
                        toToggle = false;
                    } else {
                        lastToggleTimes.erase(lit);
                    }
                }
                if (toToggle) {
                    HAGGLE_DBG("Enabling network coding for all objects for nodename=%s\n",nid.c_str());
                    lastToggleTimes.insert(make_pair(nid, now));
                    this->fragmentationConfiguration->turnOffFragmentationForDataObjectandTargetNode("",nodeRef);
                    this->networkCodingConfiguration->turnOnNetworkCodingForDataObjectandTargetNode("",nodeRef);
                } else {
                    HAGGLE_DBG("Trying to enable network coding for %s less than %fs after disabled! ignoring.\n", nid.c_str(), minTimeBetweenToggles);
                }
            }
            // CBMEN, HL, End
        }
    }
}

// CBMEN, HL, Begin
void NetworkCodingManager::onNodeQueryResult(Event *e) {

    if (!e || !e->hasData())
        return;

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring node query results\n");
        return;
    }
    
    DataStoreQueryResult *qr = static_cast < DataStoreQueryResult * >(e->getData());
    DataObjectRef dObj = qr->detachFirstDataObject();
    const NodeRefList *targets = qr->getNodeList();

    if (!dObj) {
        HAGGLE_DBG("No dataobject in query result\n");
        delete qr;
        return;
    }

    if (!targets || targets->empty()) {
        HAGGLE_ERR("No nodes in query result\n");
        delete qr;
        return;
    }

    for (NodeRefList::const_iterator it = targets->begin(); it != targets->end(); it++) {
        const NodeRef& target = *it;
        if (target && (target->getType() == Node::TYPE_APPLICATION)) {
            HAGGLE_STAT("Issuing NETWORKCODING_DECODING_TASK_DECODE for DataObject %s as local node is interested.\n", dObj->getIdStr());
            NetworkCodingDecodingTask* networkCodingDecodingTask =
                new NetworkCodingDecodingTask(NETWORKCODING_DECODING_TASK_DECODE, dObj, NULL);
            this->networkCodingDecoderManagerModule->addTask(networkCodingDecodingTask);
            delete qr;
            return;
        }
    }

    HAGGLE_STAT("Local node is not interested in DataObject %s, not creating decoder!\n", dObj->getIdStr());
        
    delete qr;
}
// CBMEN, HL, End

void NetworkCodingManager::onIncomingFragmentationToNetworkCodingConversion(Event* e) {
    if (kernel->isShuttingDown()) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }
    DataObjectRef fragmentDataObject = e->getDataObject();

    if (!fragmentDataObject)
        return;

    HAGGLE_DBG("Received event to convert fragment %s to network coded block\n", fragmentDataObject->getIdStr());
    this->networkCodingEncoderService->encodeFragment(fragmentDataObject,fragmentDataObject->getDataLen());
}

bool NetworkCodingManager::init_derived() {
    HAGGLE_DBG("Starting networkcoding manager\n");

    int ret = 1;
#define __CLASS__ NetworkCodingManager

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING, onDataObjectForSendNetworkCodedBlock);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_RECEIVED, onDataObjectForReceiveNetworkCodedBlock);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_RECEIVED - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_NEW, onDataObjectForReceiveNetworkCodedBlock);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_NEW - result =%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onDataObjectSendSuccessful);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, onDataObjectSendFailure);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_FAILURE - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_NETWORKCODEDBLOCKS, onDeleteAssociatedNetworkCodedBlocks);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENTY_TYPE_DATAOBJECT_DELETE_ASSOCIATED_NETWORKCODEDBLOCKS - result=%d\n",
                ret);
        return false;
    }

    // CBMEN, HL, Begin
    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_DELETED, onDeletedDataObject);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event EVENT_TYPE_DATAOBJECT_DELETED\n");
        return false;
    }
    // CBMEN, HL, End

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK, onDataObjectAgingNetworkCodedBlock);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_AGING_DECODER_NETWORKCODING, onAgingDecoderNetworkCoding);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_AGING_DECODER_NETWORKCODING - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_ENABLE_NETWORKCODING, onEnableNetworkCoding);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_ENABLE_NETWORKCODING - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_DISABLE_NETWORKCODING, onDisableNetworkCoding);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_DISABLE_NETWORKCODING - result=%d\n",
                ret);
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION, onIncomingFragmentationToNetworkCodingConversion);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION - result=%d\n",
                ret);
        return false;
    }

    string nameNetworkCodingDecoderManagerModule =
            "Network Coding Decoding Manager Module";
    this->networkCodingDecoderManagerModule =
            new NetworkCodingDecoderAsynchronousManagerModule(
		    this->networkCodingDecoderService, this->networkCodingConfiguration, this,
                    nameNetworkCodingDecoderManagerModule);
    if (!this->networkCodingDecoderManagerModule
            || !this->networkCodingDecoderManagerModule->startup()) {
        HAGGLE_ERR(
                "Could not create or start networkCodingDecoderManagerModule\n");
        return false;
    }

    string nameNetworkCodingEncoderManagerModule =
            "Network Coding Encoder Manager Module";
    this->networkCodingEncoderManagerModule =
            new NetworkCodingEncoderAsynchronousManagerModule(
                    this->networkCodingEncoderService, this,
                    nameNetworkCodingEncoderManagerModule);

    if (!this->networkCodingEncoderManagerModule
            || !this->networkCodingEncoderManagerModule->startup()) {
        HAGGLE_ERR(
                "Could not create or start networkCodingEncoderManagerModule\n");
        return false;
    }

    ret =
            setEventHandler(EVENT_TYPE_DATAOBJECT_DELETE_NETWORKCODEDBLOCK, onDeleteNetworkCodedBlock);
    if (ret < 0) {
        HAGGLE_ERR(
                "Could not register event handler EVENT_TYPE_DATAOBJECT_DELETE_NETWORKCODEDBLOCK - result=%d\n",
                ret);
        return false;
    }

    this->nodeQueryCallback = newEventCallback(onNodeQueryResult); // CBMEN, HL

    return true;
}

void NetworkCodingManager::onConfig(Metadata *m) {
    HAGGLE_DBG("Networkcoding manager configuration\n");
    const char *param;

    param = m->getParameter("resend_delay");
    if (param) {
        char *endptr = NULL;
        double resendDelay = strtod(param, &endptr);
        HAGGLE_DBG("Setting resend_delay=%f\n", resendDelay);
        networkCodingConfiguration->setResendDelay(resendDelay);
    }
    else {
        double DEFAULT_RESEND_DELAY = 0.1;
        HAGGLE_ERR("Setting resend_delay=%f - default\n",DEFAULT_RESEND_DELAY);
        networkCodingConfiguration->setResendDelay(DEFAULT_RESEND_DELAY);
    }

    param = m->getParameter("resend_reconstructed_delay");
    if (param) {
        char *endptr = NULL;
        double resendReconstructedDelay = strtod(param, &endptr);
        HAGGLE_DBG("Setting resend_reconstructed_delay=%f\n", resendReconstructedDelay);
        networkCodingConfiguration->setResendReconstructedDelay(resendReconstructedDelay);
    }
    else {
        double DEFAULT_RECONSTRUCTED_RESEND_DELAY = 0.1;
        HAGGLE_ERR("Setting resend_reconstructed_relay=%f - default\n",DEFAULT_RECONSTRUCTED_RESEND_DELAY);
        networkCodingConfiguration->setResendReconstructedDelay(DEFAULT_RECONSTRUCTED_RESEND_DELAY);
    }


    param = m->getParameter("delay_delete_networkcodedblocks");
    if (param) {
        char* endptr = NULL;
        double delayDeleteNetworkCodedBlocks = strtod(param, &endptr);
        HAGGLE_DBG("Setting delay_delete_networkcodedblocks=%f\n",
                delayDeleteNetworkCodedBlocks);
        networkCodingConfiguration->setDelayDeleteNetworkCodedBlocks(
                delayDeleteNetworkCodedBlocks);
    }
    else {
        double delayDeleteNetworkCodedBlocks = 0.0;
        HAGGLE_DBG("Setting delay_delete_networkcodedblocks=%f - default\n",
                delayDeleteNetworkCodedBlocks);
        networkCodingConfiguration->setDelayDeleteNetworkCodedBlocks(delayDeleteNetworkCodedBlocks);
    }

    param = m->getParameter("delay_delete_reconstructed_networkcodedblocks");
    if (param) {
        char* endptr = NULL;
        double delayDeleteReconstructedNetworkCodedBlocks = strtod(param, &endptr);
        HAGGLE_DBG("Setting delay_delete_reconstructed_networkcodedblocks=%f\n",
                delayDeleteReconstructedNetworkCodedBlocks);
        networkCodingConfiguration->setDelayDeleteReconstructedNetworkCodedBlocks(delayDeleteReconstructedNetworkCodedBlocks);
    }
    else {
        double delayDeleteReconstructedNetworkCodedBlocks = 0.0;
        HAGGLE_DBG("Setting delay_delete_reconstructed_networkcodedblocks=%f - default\n",
                delayDeleteReconstructedNetworkCodedBlocks);
        networkCodingConfiguration->setDelayDeleteReconstructedNetworkCodedBlocks(delayDeleteReconstructedNetworkCodedBlocks);
    }

    param = m->getParameter("block_size");
    if (param) {
        char* endptr = NULL;
        size_t blockSize = strtol(param, &endptr, 10);
        HAGGLE_DBG("Setting block_size=%d\n", blockSize);
        networkCodingConfiguration->setBlockSize(blockSize);
    }
    else {
        size_t DEFAULT_BLOCK_SIZE = 32768;
        HAGGLE_ERR("Setting block_size=%d - default\n",DEFAULT_BLOCK_SIZE);
        networkCodingConfiguration->setBlockSize(DEFAULT_BLOCK_SIZE);
    }

    param = m->getParameter("min_network_coding_file_size");
    if (param) {
        char* endptr = NULL;
        size_t minimumFileSize = strtol(param, &endptr, 10);
        HAGGLE_DBG("Seting min_network_coding_file_size=%d\n", minimumFileSize);
        networkCodingConfiguration->setMinimumFileSize(minimumFileSize);
    }
    else {
        size_t DEFAULT_MINIMUM_FILE_SIZE = 32769;
        HAGGLE_ERR("Setting min_network_coding_file_size=%d - default\n",DEFAULT_MINIMUM_FILE_SIZE);
        networkCodingConfiguration->setMinimumFileSize(DEFAULT_MINIMUM_FILE_SIZE);
    }

    param = m->getParameter("enable_network_coding");
    if (param) {
        if (strstr(param, "true") != NULL) {
            networkCodingConfiguration->turnOnNetworkCoding();
            HAGGLE_DBG("Enable network coding\n");
        }
        else {
            networkCodingConfiguration->turnOffNetworkCoding();
            HAGGLE_DBG("Disable network coding\n");
        }
    }

    string nodeIdStr = kernel->getThisNode()->getIdStr();
    networkCodingConfiguration->setNodeIdStr(nodeIdStr);
    HAGGLE_DBG("set nodeIdStr=%s\n",networkCodingConfiguration->getNodeIdStr().c_str());
    param = m->getParameter("enable_source_coding");
    if (param) {
        if (strstr(param, "true") != NULL) {
            networkCodingConfiguration->setEnabledSourceOnlyCoding();
            HAGGLE_DBG("Enable source coding\n");
        }
        else {
            networkCodingConfiguration->setDisabledSourceOnlyCoding();
            HAGGLE_DBG("Disable source coding\n");
        }
    }


    param = m->getParameter("enable_forwarding");
    if (param) {
        if (strstr(param, "true") != NULL) {
            networkCodingConfiguration->turnOnForwarding();
            HAGGLE_DBG("Enable forwarding of blocks\n");
        }
        else {
            networkCodingConfiguration->turnOffForwarding();
            HAGGLE_DBG("Disable forwarding of blocks\n");
        }
    }

    param = m->getParameter("node_desc_update_on_reconstruction");
    if (param) {
        if (strstr(param, "true") != NULL) {
            networkCodingConfiguration->turnOnNodeDescUpdateOnReconstruction();
            HAGGLE_DBG("Enable node description update on reconstruction\n");
        }
        else {
            networkCodingConfiguration->turnOffNodeDescUpdateOnReconstruction();
            HAGGLE_DBG("Disable node description update on reconstruction\n");
        }
    }

    param = m->getParameter("number_blocks_per_dataobject");
    if(param) {
        int numberOfBlocksPerDataObject = atoi(param);
        HAGGLE_DBG("Setting number_blocks_per_dataobject=%d\n",numberOfBlocksPerDataObject);
        this->networkCodingConfiguration->setNumberOfBlockPerDataObject(numberOfBlocksPerDataObject);
    }
    else {
        int DEFAULT_NUMBEROFBLOCKSPERDATAOBJECT = 1;
        HAGGLE_DBG("Setting number_blocks_per_dataobject=%d - default\n",DEFAULT_NUMBEROFBLOCKSPERDATAOBJECT);
        this->networkCodingConfiguration->setNumberOfBlockPerDataObject(DEFAULT_NUMBEROFBLOCKSPERDATAOBJECT);
    }

    param = m->getParameter("max_age_block");
    if (param) {
        char* endptr = NULL;
        double maxAgeBlock = strtod(param, &endptr);
        HAGGLE_DBG("Setting max_age_block=%f\n",
                maxAgeBlock);
        this->networkCodingConfiguration->setMaxAgeBlock(maxAgeBlock);
    } else {
        double DEFAULT_MAX_AGE_BLOCK= 86400;
        HAGGLE_DBG("Setting max_age_block=%f - default\n",
                DEFAULT_MAX_AGE_BLOCK);
        this->networkCodingConfiguration->setMaxAgeBlock(DEFAULT_MAX_AGE_BLOCK);
    }

    param = m->getParameter("max_age_decoder");
    if (param) {
        char* endptr = NULL;
        double maxAgeDecoder = strtod(param, &endptr);
        HAGGLE_DBG("Setting max_age_decoder=%f\n",
                maxAgeDecoder);
        this->networkCodingConfiguration->setMaxAgeDecoder(maxAgeDecoder);
    } else {
        double DEFAULT_MAX_AGE_DECODER= 86400;
        HAGGLE_DBG("Setting max_age_decoder=%f - default\n",
                DEFAULT_MAX_AGE_DECODER);
        this->networkCodingConfiguration->setMaxAgeDecoder(DEFAULT_MAX_AGE_DECODER);
    }

    param = m->getParameter("source_encoding_whitelist");
    if (param) {
        std::string thisNodeName = this->getKernel()->getThisNode()->getName().c_str();
        std::string stdstringparam = param;
        std::vector<std::string> elems = split(stdstringparam, ',');

        bool isExists = itemInVector(thisNodeName,elems);


        HAGGLE_DBG("source_encoding_whitelist=%s nodename=%s\n",param,thisNodeName.c_str());

        if( stdstringparam == "*") {
            //do nothing
            HAGGLE_DBG("All nodes included in whitelist\n");
        }
        else if(isExists) {
            //do nothing
            HAGGLE_DBG("This node included in whitelist\n");
        }
        else {
            HAGGLE_DBG("This node NOT included in whitelist\n");
            this->networkCodingConfiguration->turnOffNetworkCoding();
        }
    }

    param = m->getParameter("target_encoding_whitelist");
    if (param) {
        std::string thisNodeName = this->getKernel()->getThisNode()->getName().c_str();
        std::string stdstringparam = param;
        std::vector<std::string> targetNodeNames = split(stdstringparam, ',');

	this->networkCodingConfiguration->setDisabledForAllTargets();
        this->networkCodingConfiguration->setWhiteListTargetNodeNames(targetNodeNames);

        HAGGLE_DBG("target_encoding_whitelist=%s nodename=%s\n",param,thisNodeName.c_str());

        if( stdstringparam == "*") {
        	this->networkCodingConfiguration->setEnabledForAllTargets();
            HAGGLE_DBG("All nodes included in target whitelist\n");
        }
    }


    if(this->networkCodingConfiguration->isNetworkCodingEnabled(NULL,NULL)) {
      //schedule first event EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK
      double maxAgeBlock = this->networkCodingConfiguration->getMaxAgeBlock();
      Event* agingNetworkCodedBlockEvent =
              new Event(EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK, NULL, 0, maxAgeBlock/10);
      kernel->addEvent(agingNetworkCodedBlockEvent);

      double maxAgeDecoder = this->networkCodingConfiguration->getMaxAgeDecoder();
      Event* agingNetworkCodedDecoderEvent =
              new Event(EVENT_TYPE_DATAOBJECT_AGING_DECODER_NETWORKCODING, NULL, 0, maxAgeDecoder/10);
      kernel->addEvent(agingNetworkCodedDecoderEvent);
    }

    param = m->getParameter("min_encoder_delay_base");
    if(param) {
        networkCodingEncoderManagerModule->minEncoderDelayBase = atoi(param);
        HAGGLE_DBG("Setting min_encoder_delay_base=%d\n",networkCodingEncoderManagerModule->minEncoderDelayBase);
    }
    param = m->getParameter("min_encoder_delay_linear");
    if(param) {
        networkCodingEncoderManagerModule->minEncoderDelayLinear = atoi(param);
        HAGGLE_DBG("Setting min_encoder_delay_linear=%d\n",networkCodingEncoderManagerModule->minEncoderDelayLinear);
    }
    param = m->getParameter("min_encoder_delay_square");
    if(param) {
        networkCodingEncoderManagerModule->minEncoderDelaySquare = atoi(param);
        HAGGLE_DBG("Setting min_encoder_delay_square=%d\n",networkCodingEncoderManagerModule->minEncoderDelaySquare);
    }

    // CBMEN, HL, Begin
    param = m->getParameter("min_time_between_toggles");
    if (param) {
        char *endptr = NULL;
        double mtime = strtod(param, &endptr);
        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting min_time_between_toggles=%f\n", mtime);
            minTimeBetweenToggles = mtime;                              
        }
    }               

    param = m->getParameter("decode_only_if_target");
    if (param) {
        if (strstr(param, "true") != NULL) {
            networkCodingConfiguration->setDecodeOnlyIfTarget(true);
            HAGGLE_DBG("Enable decode only if target!\n");
        }
        else {
            networkCodingConfiguration->setDecodeOnlyIfTarget(false);
            HAGGLE_DBG("Disable decode only if target!\n");
        }
    }
    // CBMEN, HL, End
}

void NetworkCodingManager::onShutdown() {
    HAGGLE_DBG("Shutting down network coding manager\n");
    this->networkCodingDecoderManagerModule->shutdownAndClose();
    this->networkCodingEncoderManagerModule->shutdownAndClose();
    this->unregisterWithKernel();
}
