/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include "FragmentationConfiguration.h"

#include "Trace.h"

bool FragmentationConfiguration::isFragmentationTurnedOn = false;
bool FragmentationConfiguration::isForwardingTurnedOn = true;
bool FragmentationConfiguration::isNodeDescUpdateOnReconstructionTurnedOn = false;
size_t FragmentationConfiguration::fragmentSize;
size_t FragmentationConfiguration::minimumFileSize;
double FragmentationConfiguration::resendDelay;
double FragmentationConfiguration::resendReconstructedDelay;
double FragmentationConfiguration::delayDeleteFragments;
double FragmentationConfiguration::delayDeleteReconstructedFragments;
size_t FragmentationConfiguration::numberFragmentsPerDataObject;
double FragmentationConfiguration::maxAgeFragment;
double FragmentationConfiguration::maxAgeDecoder;
std::vector<std::string> FragmentationConfiguration::whitelistTargetNodeNames;
bool FragmentationConfiguration::isFragmentationEnabledForAllTargets = true;
contextawarefragmentationtracker_t FragmentationConfiguration::contextawaretracker;
RecursiveMutex FragmentationConfiguration::contextAwareMutex;

FragmentationConfiguration::FragmentationConfiguration() {

}

FragmentationConfiguration::~FragmentationConfiguration() {

}

void FragmentationConfiguration::turnOffFragmentation() {
        //HAGGLE_DBG("turning off fragmentation\n");
	FragmentationConfiguration::isFragmentationTurnedOn=false;
}

void FragmentationConfiguration::turnOnFragmentation() {
        //HAGGLE_DBG("turning on fragmentation\n");
	FragmentationConfiguration::isFragmentationTurnedOn=true;
}

void FragmentationConfiguration::
	turnOffFragmentationForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef) {
	//some reason empty target node so just return
	if(!targetNodeRef) {
		return;
	}

	string targetNodeRefIdStr = targetNodeRef->getName();
	HAGGLE_DBG("disable fragmentation for for targetnoderefid = %s\n",targetNodeRefIdStr.c_str());
    string key = parentDataObjectId + "|" + targetNodeRefIdStr;

    {
        Mutex::AutoLocker l(FragmentationConfiguration::contextAwareMutex); // needs to be fine grained
        contextawarefragmentationtracker_t::iterator it = FragmentationConfiguration::contextawaretracker.find(key);
        if (it == FragmentationConfiguration::contextawaretracker.end())
            FragmentationConfiguration::contextawaretracker.insert(make_pair(key, true));
    }

}

void FragmentationConfiguration::
	turnOnFragmentationForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef) {
	//some reason empty target node so just return
	if(!targetNodeRef) {
		return;
	}

	string targetNodeRefIdStr = targetNodeRef->getName();
	HAGGLE_DBG("enable fragmentation for for targetnoderefid = %s\n",targetNodeRefIdStr.c_str());
    if (!parentDataObjectId.empty()) {
        HAGGLE_DBG("Not enabling fragmentation for data object id = %s as we may have started coding\n", parentDataObjectId.c_str());
    } else {
        Mutex::AutoLocker l(FragmentationConfiguration::contextAwareMutex); // needs to be fine grained
        HAGGLE_DBG("Enabling fragmentation for all data objects for targetnoderefid = %s\n", targetNodeRefIdStr.c_str());
        string key = parentDataObjectId + "|" + targetNodeRefIdStr;
        contextawarefragmentationtracker_t::iterator it = FragmentationConfiguration::contextawaretracker.find(key);
        if (it != FragmentationConfiguration::contextawaretracker.end())
            FragmentationConfiguration::contextawaretracker.erase(it);
    }

}


bool FragmentationConfiguration::isFragmentationEnabled(DataObjectRef dataObject, NodeRef targetNodeToFragmentCodeFor) {
	// fragmentation is not enabled at all, so just return false and let nc checks run
	if(!FragmentationConfiguration::isFragmentationTurnedOn) {
		return false;
	}

	// no target node and already passed the is fragemtnation turned on, so return true
	if( !targetNodeToFragmentCodeFor ) {
		return true;
	}

	string targetNodeId = targetNodeToFragmentCodeFor->getName();
    string dataObjectId;
    if (dataObject)
        dataObjectId = dataObject->getIdStr();

    string key = dataObjectId + "|" + targetNodeId;
    {
        Mutex::AutoLocker l(FragmentationConfiguration::contextAwareMutex); // needs to be fine grained
        contextawarefragmentationtracker_t::iterator it = FragmentationConfiguration::contextawaretracker.find(key);
        if (it != FragmentationConfiguration::contextawaretracker.end()) {
            return false;
        }

        key = "|" + targetNodeId;
        it = FragmentationConfiguration::contextawaretracker.find(key);
        if (it != FragmentationConfiguration::contextawaretracker.end()) {
            HAGGLE_DBG("context aware coding is enabled for targetnoderefid=%s, saving status for dataobject=%s\n", targetNodeId.c_str(), dataObjectId.c_str());
            FragmentationConfiguration::contextawaretracker.insert(make_pair(dataObjectId + "|" + targetNodeId, true));
            return false;
        }
    }

	return true;

}

void FragmentationConfiguration::turnOffForwarding() {
	FragmentationConfiguration::isForwardingTurnedOn=false;
}

void FragmentationConfiguration::turnOnForwarding() {
	FragmentationConfiguration::isForwardingTurnedOn=true;
}

bool FragmentationConfiguration::isForwardingEnabled() {
	return FragmentationConfiguration::isForwardingTurnedOn;
}

void FragmentationConfiguration::turnOffNodeDescUpdateOnReconstruction() {
    FragmentationConfiguration::isNodeDescUpdateOnReconstructionTurnedOn = false;
}

void FragmentationConfiguration::turnOnNodeDescUpdateOnReconstruction() {
    FragmentationConfiguration::isNodeDescUpdateOnReconstructionTurnedOn = true;
}

bool FragmentationConfiguration::isNodeDescUpdateOnReconstructionEnabled() {
    return FragmentationConfiguration::isNodeDescUpdateOnReconstructionTurnedOn;
}

void FragmentationConfiguration::setFragmentSize(size_t _fragmentSize) {
        //HAGGLE_DBG("setting fragmentSize=%d\n",_fragmentSize);
	FragmentationConfiguration::fragmentSize = _fragmentSize;
}

size_t FragmentationConfiguration::getFragmentSize() {
	return FragmentationConfiguration::fragmentSize;
}

size_t FragmentationConfiguration::getMinimumFileSize() {
    return FragmentationConfiguration::minimumFileSize;
}

void FragmentationConfiguration::setMinimumFileSize(size_t _minimumFileSize) {
    //HAGGLE_DBG("setting minimumFileSize=%d\n",_minimumFileSize);
    FragmentationConfiguration::minimumFileSize=_minimumFileSize;
}

void FragmentationConfiguration::setResendDelay(double _resendDelay) {
        //HAGGLE_DBG("setting resenddelay=%f\n",_resendDelay);
	FragmentationConfiguration::resendDelay=_resendDelay;
}

double FragmentationConfiguration::getResendDelay() {
	return FragmentationConfiguration::resendDelay;
}

void FragmentationConfiguration::setDelayDeleteFragments(double _delayDeleteFragments) {
        //HAGGLE_DBG("setting delayDeleteFragments=%f\n",_delayDeleteFragments);
	FragmentationConfiguration::delayDeleteFragments=_delayDeleteFragments;
}

double FragmentationConfiguration::getDelayDeleteFragments() {
	return FragmentationConfiguration::delayDeleteFragments;
}

void FragmentationConfiguration::setDelayDeleteReconstructedFragments(double _delayDeleteReconstructedFragments) {
        //HAGGLE_DBG("setting delayDeleteReconstructedFragments=%f\n",_delayDeleteReconstructedFragments);
	FragmentationConfiguration::delayDeleteReconstructedFragments=_delayDeleteReconstructedFragments;
}

double FragmentationConfiguration::getDelayDeleteReconstructedFragments() {
	return FragmentationConfiguration::delayDeleteReconstructedFragments;
}

void FragmentationConfiguration::setNumberFragmentsPerDataObject(size_t _numberFragmentsPerDataObject) {
        //HAGGLE_DBG("setting numberFragmentsPerDataObject=%d\n",_numberFragmentsPerDataObject);
	FragmentationConfiguration::numberFragmentsPerDataObject = _numberFragmentsPerDataObject;
}

size_t FragmentationConfiguration::getNumberFragmentsPerDataObject() {
	return FragmentationConfiguration::numberFragmentsPerDataObject;
}

void FragmentationConfiguration::setResendReconstructedDelay(double _resendReconstructedDelay) {
        //HAGGLE_DBG("setting resendReconstructedDelay=%f\n",_resendReconstructedDelay);
	FragmentationConfiguration::resendReconstructedDelay = _resendReconstructedDelay;
}

double FragmentationConfiguration::getResendReconstructedDelay() {
	return FragmentationConfiguration::resendReconstructedDelay;
}

void FragmentationConfiguration::setMaxAgeFragment(double _maxAgeFragment) {
    //HAGGLE_DBG("setting maxAgeFragment=%f\n",_maxAgeFragment);
    FragmentationConfiguration::maxAgeFragment = _maxAgeFragment;
}

double FragmentationConfiguration::getMaxAgeFragment() {
    return FragmentationConfiguration::maxAgeFragment;
}

void FragmentationConfiguration::setMaxAgeDecoder(double _maxAgeDecoder) {
    FragmentationConfiguration::maxAgeDecoder = _maxAgeDecoder;
}

double FragmentationConfiguration::getMaxAgeDecoder() {
    return FragmentationConfiguration::maxAgeDecoder;
}

std::vector<std::string> FragmentationConfiguration::getWhitelistTargetNodeNames() {
	return FragmentationConfiguration::whitelistTargetNodeNames;
}

void FragmentationConfiguration::setWhiteListTargetNodeNames(std::vector<std::string> targetNodeNames) {
	FragmentationConfiguration::whitelistTargetNodeNames = targetNodeNames;
}

bool FragmentationConfiguration::isEnabledForAllTargets() {
	return FragmentationConfiguration::isFragmentationEnabledForAllTargets;
}

void FragmentationConfiguration::setEnabledForAllTargets() {
	FragmentationConfiguration::isFragmentationEnabledForAllTargets = true;
}

void FragmentationConfiguration::setDisabledForAllTargets() {
	FragmentationConfiguration::isFragmentationEnabledForAllTargets = false;
}
