/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include "FragmentationManager.h"
#include "Event.h"
#include "stringutils/CSVUtility.h"

FragmentationManager::FragmentationManager(HaggleKernel *_haggle) :
		Manager("FragmentationManager", _haggle) {

	this->dataObjectTypeIdentifierUtility =
			new DataObjectTypeIdentifierUtility();
	this->fragmentationConfiguration = new FragmentationConfiguration();
	this->fragmentationFileUtility = new FragmentationFileUtility();
	this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();

	this->fragmentationEncoderStorage = new FragmentationEncoderStorage();

	this->fragmentationEncoderService = new FragmentationEncoderService(
			this->fragmentationFileUtility, this->fragmentationConfiguration,
			this->fragmentationEncoderStorage);
	this->fragmentationDecoderStorage = new FragmentationDecoderStorage(
			this->getKernel(),
			this->getKernel()->getDataStore(),
			this->dataObjectTypeIdentifierUtility);

	this->fragmentationSendSuccessFailureHandler =
			new FragmentationSendSuccessFailureHandler(
					this->fragmentationDataObjectUtility,
					this->fragmentationEncoderStorage);

	this->fragmentationDecoderService = new FragmentationDecoderService(
			this->fragmentationDecoderStorage,
			this->fragmentationDataObjectUtility);

	string nameFragmentationEncoderManagerModule =
			"Fragmentation Encoding Manager Module";
	this->fragmentationEncoderAsynchronousManagerModule =
			new FragmentationEncoderAsynchronousManagerModule(
					this->fragmentationConfiguration,
					this->fragmentationDataObjectUtility,
					this->fragmentationEncoderService, this,
					nameFragmentationEncoderManagerModule);

	string nameFragmentationDecoderManagerModule =
			"Fragmentation Decoding Manager Module";
	this->fragmentationDecoderAsynchronousManagerModule =
			new FragmentationDecoderAsynchronousManagerModule(
					this->fragmentationConfiguration,
					this->fragmentationDecoderService, this,
					nameFragmentationDecoderManagerModule);
}

FragmentationManager::~FragmentationManager() {
    if(this->fragmentationSendSuccessFailureHandler) {
        delete this->fragmentationSendSuccessFailureHandler;
        this->fragmentationSendSuccessFailureHandler = NULL;
    }
	if (this->fragmentationDecoderAsynchronousManagerModule) {
		delete this->fragmentationDecoderAsynchronousManagerModule;
		this->fragmentationDecoderAsynchronousManagerModule = NULL;
	}
	if (this->fragmentationEncoderAsynchronousManagerModule) {
		delete this->fragmentationEncoderAsynchronousManagerModule;
		this->fragmentationEncoderAsynchronousManagerModule = NULL;
	}
	if (this->fragmentationEncoderService) {
		delete this->fragmentationEncoderService;
		this->fragmentationEncoderService = NULL;
	}
	if (this->fragmentationDecoderStorage) {
		delete this->fragmentationDecoderStorage;
		this->fragmentationDecoderStorage = NULL;
	}
	if (this->fragmentationDecoderService) {
		delete this->fragmentationDecoderService;
		this->fragmentationDecoderService = NULL;
	}
	if (this->fragmentationConfiguration) {
		delete this->fragmentationConfiguration;
		this->fragmentationConfiguration = NULL;
	}
	if (this->fragmentationFileUtility) {
		delete this->fragmentationFileUtility;
		this->fragmentationFileUtility = NULL;
	}
	if (this->fragmentationDataObjectUtility) {
		delete this->fragmentationDataObjectUtility;
		this->fragmentationDataObjectUtility = NULL;
	}
	if (this->dataObjectTypeIdentifierUtility) {
		delete this->dataObjectTypeIdentifierUtility;
		this->dataObjectTypeIdentifierUtility = NULL;
	}
	if (this->fragmentationEncoderStorage) {
		delete this->fragmentationEncoderStorage;
		this->fragmentationEncoderStorage = NULL;
	}
}

void FragmentationManager::onDataObjectSendFragmentation(Event *e) {
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring send data object\n");
          return;
        }

	DataObjectRef dataObjectRef = e->getDataObject();
	NodeRefList nodeRefList = e->getNodeList();

	if (!dataObjectRef) {
		HAGGLE_ERR("Missing data object in event\n");
		return;
	}

	if (!this->fragmentationDataObjectUtility->shouldBeFragmented(
			dataObjectRef,NULL)) {
	        HAGGLE_ERR("Unexpected case - data object %s should not be fragmented\n", dataObjectRef->getIdStr());
		return;
	}

    NodeRefList fragmentationTargetNodeRefList;

    for (NodeRefList::iterator it = nodeRefList.begin(); it != nodeRefList.end(); it++) {
        NodeRef& node = *it;
        if(node->getType() == Node::TYPE_APPLICATION) {
        	//do nothing
        }
        else {
        	bool fragmentedForThisTarget = this->fragmentationDataObjectUtility->shouldBeFragmentedCheckTargetNode(dataObjectRef,node);
        	if(fragmentedForThisTarget) {
        		fragmentationTargetNodeRefList.add(node);
        	}
        }
    }

    if(fragmentationTargetNodeRefList.empty()) {
    	return;
    }

	HAGGLE_DBG("Decided to send data object %s with fragmentation - filepath=%s filename=%s\n",
		    dataObjectRef->getIdStr(), fragmentationDataObjectUtility->getFilePath(dataObjectRef).c_str(), 
		    fragmentationDataObjectUtility->getFileName(dataObjectRef).c_str());

	string parentDataObjectId = dataObjectRef->getIdStr();
	double resendReconstructedDelay = this->fragmentationConfiguration->getResendReconstructedDelay();
	double resendReconstructedDelayCalculated = this->fragmentationDecoderStorage->getResendReconstructedDelay(parentDataObjectId,resendReconstructedDelay);
	if( resendReconstructedDelayCalculated > 0.0 ) {
		HAGGLE_DBG("Reschedule sending of parent data object %s with delay=%f\n",parentDataObjectId.c_str(),resendReconstructedDelayCalculated);
		kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dataObjectRef, fragmentationTargetNodeRefList, resendReconstructedDelayCalculated)); // MOS
		// MOS - using EVENT_TYPE_DATAOBJECT_SEND instead of EVENT_TYPE_DATAOBJECT_FRAGMENTATION_SEND to have another Bloom filter check
		return;
	}

	this->fragmentationEncoderStorage->trackSendEvent(parentDataObjectId);

	FragmentationEncodingTask* fragmentationEncodingTask =
			new FragmentationEncodingTask(FRAGMENTATION_ENCODING_TASK_ENCODE,
					dataObjectRef, fragmentationTargetNodeRefList);

	bool addTaskReturn =
			this->fragmentationEncoderAsynchronousManagerModule->addTask(
					fragmentationEncodingTask);
	if (!addTaskReturn) {
		HAGGLE_ERR("Error adding task to task queue\n");
	}
}

void FragmentationManager::onDataObjectForReceived(Event* e) {

        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring data object received\n");
          return;
        }

	DataObjectRef dataObjectRef = e->getDataObject();

	if (!dataObjectRef) {
		HAGGLE_ERR("Missing data object in event\n");
		return;
	}

	HAGGLE_DBG2("Received data object %s\n", dataObjectRef->getIdStr());

	NodeRef nodeRef = e->getNode(); // MOS - not available for EVENT_TYPE_DATAOBJECT_NEW events

        /* MOS - nodeRef is not used anymore and is NULL for reconstructed objects

	if(!nodeRef) {
	  InterfaceRef remoteIface = dataObjectRef->getRemoteInterface();
	  if(!remoteIface) {
	    HAGGLE_DBG2("Missing remote interface for data object %s\n", dataObjectRef->getIdStr());
	    return;
	  }      
	  nodeRef = kernel->getNodeStore()->retrieve(remoteIface,false); // MOS - fixed, node may not be a neighbor anymore
	  if(!nodeRef) {
	    HAGGLE_DBG2("Cannot find peer node for data object %s\n", dataObjectRef->getIdStr());
	    return;
	  }
	}
        */

	bool isFragmentationObject =
			this->fragmentationDataObjectUtility->isFragmentationDataObject(
					dataObjectRef);

	if (isFragmentationObject) {
		HAGGLE_DBG("Creating reconstruction task for fragment %s\n", dataObjectRef->getIdStr());

		size_t fragmentationSize =
				this->fragmentationConfiguration->getFragmentSize();

		if(fragmentationSize == 0) {
		  HAGGLE_ERR("Fragmentation (fragment size) not properly configured - ignoring fragment\n");
		} else {
		  FragmentationDecodingTask* fragmentationDecodingTask =
				new FragmentationDecodingTask(
						FRAGMENTATION_DECODING_TASK_DECODE, dataObjectRef,
						nodeRef);
		  this->fragmentationDecoderAsynchronousManagerModule->addTask(
				fragmentationDecodingTask);
		}

		double delayDeleteFragments =
				this->fragmentationConfiguration->getDelayDeleteFragments();
		HAGGLE_DBG("Scheduling fragment %s for deletion\n",
				dataObjectRef->getIdStr());
		Event* deleteFragmentEvent = new Event(
				EVENT_TYPE_DATAOBJECT_DELETE_FRAGMENT, dataObjectRef, nodeRef,
				0, delayDeleteFragments);
		kernel->addEvent(deleteFragmentEvent);
	}

}

void FragmentationManager::onDataObjectSendSuccessful(Event *e) {
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring send data object results\n");
          return;
        }

	DataObjectRef dataObjectRef = e->getDataObject();
	NodeRef node = e->getNode();
	unsigned long flags = e->getFlags();

	/* MOS - need to find a clean way so that following SEND_SUCCESSFUL event for the top-level parent object
                 is not only triggered by rejected fragements but also by rejected blocks (if network coded)
	*/

	bool isFragmentationObject = this->fragmentationDataObjectUtility->isFragmentationDataObject(dataObjectRef);
	if(!isFragmentationObject) {
		HAGGLE_DBG2("Not a fragmentation data object\n");
		return;
	}
	Event* newEvent = NULL;

	HAGGLE_DBG("Data object %s successfully sent - flags=%d\n", dataObjectRef->getIdStr(), flags);

	//send reject parentdataobject so already have
	if (flags == 2) {
		HAGGLE_DBG("Preparing send-successful event for parent data object of %s - after reject\n",dataObjectRef->getIdStr());
		newEvent =
				this->fragmentationSendSuccessFailureHandler->retrieveOriginalDataObjectAndGenerateEvent(
						EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dataObjectRef,
						node);
	} else {
	        if(dataObjectRef->getRemoteInterface()) return; // MOS - no event translation for forwarded fragments
		HAGGLE_DBG("Preparing send-successful event to trigger next fragment of parent data object of %s\n",dataObjectRef->getIdStr());
		newEvent =
				this->fragmentationSendSuccessFailureHandler->retrieveOriginalDataObjectAndGenerateEvent(
						EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL,
						dataObjectRef, node);
	}

	if (newEvent != NULL) {
		HAGGLE_DBG("Raising event %d for parent object %s of fragment %s\n",
			   newEvent->getType(),
			   DataObject::idString(newEvent->getDataObject()).c_str(),
			   DataObject::idString(dataObjectRef).c_str());
		this->kernel->addEvent(newEvent);
	}
}

void FragmentationManager::onDataObjectSendFailure(Event *e) {
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring send data object results\n");
          return;
        }

	DataObjectRef dataObjectRef = e->getDataObject();
	NodeRef nodeRef = e->getNode();

	HAGGLE_DBG("Received send-failure event for data object %s\n", dataObjectRef->getIdStr());

	Event* newEvent =
			this->fragmentationSendSuccessFailureHandler->retrieveOriginalDataObjectAndGenerateEvent(
					EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dataObjectRef, nodeRef);

	if (newEvent != NULL) {
		this->kernel->addEvent(newEvent);
	}
}

void FragmentationManager::onDeleteFragment(Event* e) {
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring event\n");
          return;
        }

	DataObjectRef fragmentDataObjectRef = e->getDataObject();
	HAGGLE_DBG("Received event to dispose fragment %s\n",
			fragmentDataObjectRef->getIdStr());
	this->fragmentationDecoderStorage->disposeFragment(fragmentDataObjectRef);
}

void FragmentationManager::onDeleteAssociatedFragments(Event* e) {
        if (getState() > MANAGER_STATE_RUNNING) {
          HAGGLE_DBG("In shutdown, ignoring event\n");
          return;
        }

	DataObjectRef dataObjectRef = e->getDataObject();

	HAGGLE_DBG("Received event to delete associated fragments of data object %s\n",
			dataObjectRef->getIdStr());

	string originalDataObjectId = dataObjectRef->getIdStr();
	this->fragmentationDecoderStorage->disposeAssociatedFragments(
			originalDataObjectId);
}

void FragmentationManager::onDeletedDataObject(Event* e) {
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
            HAGGLE_DBG2("Removing deleted data object %s from fragmentation storage - flags=%d\n", dataObjectRef->getIdStr(),e->getFlags());
            this->fragmentationDecoderStorage->deleteFromStorage(dataObjectRef);
            this->fragmentationEncoderStorage->deleteFromStorage(dataObjectRef);
	}
    }
}

void FragmentationManager::onDataObjectAgingFragmentation(Event* e) {
    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    HAGGLE_DBG("Received event to age fragment data objects\n");

    double maxAgeFragment = this->fragmentationConfiguration->getMaxAgeFragment();
    this->fragmentationEncoderStorage->ageOffFragments(maxAgeFragment);

    //reschedule
    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION, NULL, 0, maxAgeFragment/10));
}

void FragmentationManager::onDecoderAgingFragmentation(Event* e) {
    HAGGLE_DBG("Received event to age fragment decoder\n");

    if (getState() > MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring event\n");
      return;
    }

    double maxAgeDecoder = this->fragmentationConfiguration->getMaxAgeDecoder();
    this->fragmentationDecoderStorage->ageOffDecoder(maxAgeDecoder);

    //reschedule
    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_AGING_DECODER_FRAGMENTATION, NULL, 0, maxAgeDecoder/10));
}

void FragmentationManager::onConvertFragmentationToNetworkcoding(Event* e) {
    if (!e || !e->hasData()) {
        HAGGLE_ERR("No data.\n");
        return;
    }
    char* parentDataObjectId= (char*)e->getData();

    // CBMEN, HL - Actually do something here
    this->fragmentationDecoderStorage->convertFragmentsToNetworkCodedBlocks(parentDataObjectId);

    //delete char* allocated memory when completed
    delete[] parentDataObjectId;
}

void FragmentationManager::onConfig(Metadata *m) {
	HAGGLE_DBG("Fragmentation manager configuration\n");

	const char *param;

	param = m->getParameter("enable_fragmentation");
	if (param) {
		if (strstr(param, "true") != NULL) {
			this->fragmentationConfiguration->turnOnFragmentation();
			HAGGLE_DBG("Enable fragmentation\n");
		}
		else {
		  this->fragmentationConfiguration->turnOffFragmentation();
		  HAGGLE_DBG("Disable fragmentation\n");
		}		  
	}

	param = m->getParameter("enable_forwarding");
	if (param) {
		if (strstr(param, "true") != NULL) {
			this->fragmentationConfiguration->turnOnForwarding();
			HAGGLE_DBG("Enable forwarding of fragments\n");
		}
		else {
		  this->fragmentationConfiguration->turnOffForwarding();
		  HAGGLE_DBG("Disable forwarding of fragments\n");
		}		  
	}

	param = m->getParameter("node_desc_update_on_reconstruction");
	if (param) {
	  if (strstr(param, "true") != NULL) {
	    fragmentationConfiguration->turnOnNodeDescUpdateOnReconstruction();
	    HAGGLE_DBG("Enable node description update on reconstruction\n");
	  }
	  else {
	    fragmentationConfiguration->turnOffNodeDescUpdateOnReconstruction();
	    HAGGLE_DBG("Disable node description update on reconstruction\n");
	  }
	}

	param = m->getParameter("fragment_size");
	if (param) {
		char* endptr = NULL;
		size_t fragmentSize = strtol(param, &endptr, 10);
		HAGGLE_DBG("Setting fragment_size=%d\n", fragmentSize);
		this->fragmentationConfiguration->setFragmentSize(fragmentSize);
	} else {
		size_t DEFAULT_FRAGMENT_SIZE = 32768;
		HAGGLE_ERR("Setting fragment_size=%d - default\n",
				DEFAULT_FRAGMENT_SIZE);
		this->fragmentationConfiguration->setFragmentSize(
				DEFAULT_FRAGMENT_SIZE);
	}

	param = m->getParameter("min_fragmentation_file_size");
	if (param) {
		char* endptr = NULL;
		size_t minimumFileSize = strtol(param, &endptr, 10);
		HAGGLE_DBG("Setting min_fragmentation_file_size=%d\n", minimumFileSize);
		this->fragmentationConfiguration->setMinimumFileSize(minimumFileSize);
	} else {
		size_t DEFAULT_MINIMUM_FILE_SIZE = 32769;
		HAGGLE_ERR("Setting min_fragmentation_file_size=%d - default\n",
				DEFAULT_MINIMUM_FILE_SIZE);
		this->fragmentationConfiguration->setMinimumFileSize(
				DEFAULT_MINIMUM_FILE_SIZE);
	}

	param = m->getParameter("resend_delay");
	if (param) {
		char *endptr = NULL;
		double resendDelay = strtod(param, &endptr);
		HAGGLE_DBG("Setting resend_delay=%f\n", resendDelay);
		this->fragmentationConfiguration->setResendDelay(resendDelay);
	} else {
		double DEFAULT_RESEND_DELAY = 0.1;
		HAGGLE_ERR("Setting resend_delay=%f - default\n", DEFAULT_RESEND_DELAY);
		this->fragmentationConfiguration->setResendDelay(DEFAULT_RESEND_DELAY);
	}

	param = m->getParameter("resend_reconstructed_delay");
	if (param) {
		char *endptr = NULL;
		double resendReconstructedDelay = strtod(param, &endptr);
		HAGGLE_DBG("Setting resend_reconstructed_delay=%f\n", resendReconstructedDelay);
		this->fragmentationConfiguration->setResendReconstructedDelay(
				resendReconstructedDelay);
	} else {
		double DEFAULT_RESEND_RECONSTRUCTED_DELAY = 100.0;
		HAGGLE_ERR("Setting resend_reconstructed_delay=%f\n",
				DEFAULT_RESEND_RECONSTRUCTED_DELAY);
		this->fragmentationConfiguration->setResendReconstructedDelay(
				DEFAULT_RESEND_RECONSTRUCTED_DELAY);
	}

	param = m->getParameter("delay_delete_fragments");
	if (param) {
		char* endptr = NULL;
		double delayDeleteFragments = strtod(param, &endptr);
		HAGGLE_DBG("Setting delay_delete_fragments=%f\n", delayDeleteFragments);
		this->fragmentationConfiguration->setDelayDeleteFragments(
				delayDeleteFragments);
	} else {
		double delayDeleteFragments = 0.0;
		HAGGLE_DBG("Setting delay_delete_fragments=%f - default\n", delayDeleteFragments);
		this->fragmentationConfiguration->setDelayDeleteFragments(
				delayDeleteFragments);
	}

	param = m->getParameter("delay_delete_reconstructed_fragments");
	if (param) {
		char* endptr = NULL;
		double delayDeleteReconstructedFragments = strtod(param, &endptr);
		HAGGLE_DBG("Setting delay_delete_reconstructed_fragments=%f\n",
				delayDeleteReconstructedFragments);
		this->fragmentationConfiguration->setDelayDeleteReconstructedFragments(
				delayDeleteReconstructedFragments);
	} else {
		double delayDeleteReconstructedFragments = 0.0;
		HAGGLE_DBG("Setting delay_delete_reconstructed_fragments=%f - default\n",
				delayDeleteReconstructedFragments);
		this->fragmentationConfiguration->setDelayDeleteReconstructedFragments(
				delayDeleteReconstructedFragments);
	}

	param = m->getParameter("number_fragments_per_dataobject");
	if (param) {
		char* endptr = NULL;
		size_t numberFragmentsPerDataobject = strtol(param, &endptr, 10);
		HAGGLE_DBG("Setting number_fragments_per_dataobject=%d\n",
				numberFragmentsPerDataobject);
		this->fragmentationConfiguration->setNumberFragmentsPerDataObject(
				numberFragmentsPerDataobject);
	} else {
		size_t DEFAULT_FRAGMENTS_PER_DATAOBJECT = 1;
		HAGGLE_ERR("Setting number_fragments_per_dataobject=%d - default\n",
				DEFAULT_FRAGMENTS_PER_DATAOBJECT);
		this->fragmentationConfiguration->setNumberFragmentsPerDataObject(
				DEFAULT_FRAGMENTS_PER_DATAOBJECT);
	}

    param = m->getParameter("max_age_fragment");
    if (param) {
        char* endptr = NULL;
        double maxAgeFragment = strtod(param, &endptr);
        HAGGLE_DBG("Setting max_age_fragment=%f\n",
                maxAgeFragment);
        this->fragmentationConfiguration->setMaxAgeFragment(maxAgeFragment);
    } else {
        double DEFAULT_MAX_AGE_FRAGMENT = 86400;
        HAGGLE_DBG("Setting max_age_fragment=%f - default\n",
                DEFAULT_MAX_AGE_FRAGMENT);
        this->fragmentationConfiguration->setMaxAgeFragment(DEFAULT_MAX_AGE_FRAGMENT);
    }

    param = m->getParameter("max_age_decoder");
    if (param) {
        char* endptr = NULL;
        double maxAgeDecoder = strtod(param, &endptr);
        HAGGLE_DBG("Setting max_age_decoder=%f\n",
                maxAgeDecoder);
        this->fragmentationConfiguration->setMaxAgeDecoder(maxAgeDecoder);
    } else {
        double DEFAULT_MAX_AGE_DECODER = 86400;
        HAGGLE_DBG("Setting max_age_decoder=%f - default\n",
                DEFAULT_MAX_AGE_DECODER);
        this->fragmentationConfiguration->setMaxAgeDecoder(DEFAULT_MAX_AGE_DECODER);
    }

    param = m->getParameter("source_fragmentation_whitelist");
    if (param) {
        std::string thisNodeName = this->getKernel()->getThisNode()->getName().c_str();
        std::string stdstringparam = param;
        std::vector<std::string> elems = split(stdstringparam, ',');

        bool isExists = itemInVector(thisNodeName,elems);

        HAGGLE_DBG("fragmentation_whitelist=%s nodename=%s\n",param,thisNodeName.c_str());

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
            this->fragmentationConfiguration->turnOffFragmentation();
        }
    }

    param = m->getParameter("target_fragmentation_whitelist");
    if (param) {
        std::string thisNodeName = this->getKernel()->getThisNode()->getName().c_str();
        std::string stdstringparam = param;
        std::vector<std::string> targetNodeNames = split(stdstringparam, ',');

	this->fragmentationConfiguration->setDisabledForAllTargets();
        this->fragmentationConfiguration->setWhiteListTargetNodeNames(targetNodeNames);

        HAGGLE_DBG("target_fragmentation_whitelist=%s nodename=%s\n",param,thisNodeName.c_str());

        if( stdstringparam == "*") {
        	this->fragmentationConfiguration->setEnabledForAllTargets();
            HAGGLE_DBG("All nodes included in target whitelist\n");
        }
    }

    if(this->fragmentationConfiguration->isFragmentationEnabled(NULL,NULL)) {
      //schedule first event EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION
      double maxAgeFragment = this->fragmentationConfiguration->getMaxAgeFragment();
      kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION, NULL, 0, maxAgeFragment/10));

      double maxAgeDecoder = this->fragmentationConfiguration->getMaxAgeDecoder();
      kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_AGING_DECODER_FRAGMENTATION, NULL, 0, maxAgeDecoder/10));
    }

    param = m->getParameter("min_encoder_delay_base");
    if(param) {
        fragmentationEncoderAsynchronousManagerModule->minEncoderDelayBase = atoi(param);
        HAGGLE_DBG("Setting min_encoder_delay_base=%d\n",fragmentationEncoderAsynchronousManagerModule->minEncoderDelayBase);
    }
    param = m->getParameter("min_encoder_delay_linear");
    if(param) {
        fragmentationEncoderAsynchronousManagerModule->minEncoderDelayLinear = atoi(param);
        HAGGLE_DBG("Setting min_encoder_delay_linear=%d\n",fragmentationEncoderAsynchronousManagerModule->minEncoderDelayLinear);
    }
    param = m->getParameter("min_encoder_delay_square");
    if(param) {
        fragmentationEncoderAsynchronousManagerModule->minEncoderDelaySquare = atoi(param);
        HAGGLE_DBG("Setting min_encoder_delay_square=%d\n",fragmentationEncoderAsynchronousManagerModule->minEncoderDelaySquare);
    }

}

bool FragmentationManager::init_derived() {
	HAGGLE_DBG("Starting fragmentation manager\n");

	int ret = 1;
#define __CLASS__ FragmentationManager

	ret =
			setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION, onDataObjectSendFragmentation);
	if (ret < 0) {
		HAGGLE_ERR(
				"Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION - result=%d\n",
				ret);
		return false;
	}

	ret =
			setEventHandler(EVENT_TYPE_DATAOBJECT_NEW, onDataObjectForReceived);
	if (ret < 0) {
		HAGGLE_ERR(
				"Could not register event handler EVENT_TYPE_DATAOBJECT_NEW - result=%d\n",
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
			setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL, onDataObjectSendSuccessful);
	if (ret < 0) {
		HAGGLE_ERR(
				"Could not register event handler EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL - result=%d\n",
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
			setEventHandler(EVENT_TYPE_DATAOBJECT_DELETE_FRAGMENT, onDeleteFragment);
	if (ret < 0) {
		HAGGLE_ERR(
				"Could not register event handler EVENT_TYPE_DATAOBJECT_DELETE_FRAGMENT - result=%d\n",
				ret);
		return false;
	}

	ret =
			setEventHandler(EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_FRAGMENTS, onDeleteAssociatedFragments);
	if (ret < 0) {
		HAGGLE_ERR(
				"Could not register event handler EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_FRAGMENTS - result=%d\n",
				ret);
		return false;
	}

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_DELETED, onDeletedDataObject);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event EVENT_TYPE_DATAOBJECT_DELETED\n");
        return false;
    }


    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION, onDataObjectAgingFragmentation);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_AGING_DECODER_FRAGMENTATION, onDecoderAgingFragmentation);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event EVENT_TYPE_DATAOBJECT_AGING_DECODER_FRAGMENTATION\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_CONVERT_FRAGMENTATION_NETWORKCODING, onConvertFragmentationToNetworkcoding);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event EVENT_TYPE_DATAOBJECT_CONVERT_FRAGMENTATION_NETWORKCODING\n");
        return false;
    }



	if (!this->fragmentationEncoderAsynchronousManagerModule
			|| !this->fragmentationEncoderAsynchronousManagerModule->startup()) {
		HAGGLE_ERR(
				"Could not create or start fragmentationEncoderAsynchronousManagerModule\n");
		return false;
	}

	if (!this->fragmentationDecoderAsynchronousManagerModule
			|| !this->fragmentationDecoderAsynchronousManagerModule->startup()) {
		HAGGLE_ERR(
				"Could not create or start fragmentationDecoderAsynchronousManagerModule\n");
		return false;
	}


	return true;
}

void FragmentationManager::onShutdown() {
	HAGGLE_DBG("Shutting down fragmentation manager\n");
	this->fragmentationDecoderAsynchronousManagerModule->shutdownAndClose();
	this->fragmentationEncoderAsynchronousManagerModule->shutdownAndClose();
	this->unregisterWithKernel();
}
