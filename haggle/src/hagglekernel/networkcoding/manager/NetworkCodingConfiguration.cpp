/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include "Trace.h"
#include "NetworkCodingConfiguration.h"

bool NetworkCodingConfiguration::isNetworkCodingTurnedOn = false;
bool NetworkCodingConfiguration::isForwardingTurnedOn = true;
bool NetworkCodingConfiguration::isNodeDescUpdateOnReconstructionTurnedOn = false;
double NetworkCodingConfiguration::resendDelay;
double NetworkCodingConfiguration::resendReconstructedDelay;
double NetworkCodingConfiguration::delayDeleteNetworkCodedBlocks;
size_t NetworkCodingConfiguration::blockSize = 0; // CBMEN, HL - So we don't network code when not configured
int NetworkCodingConfiguration::numberOfBlocksPerDataObject;
double NetworkCodingConfiguration::delayDeleteReconstructedNetworkCodedBlocks;
size_t NetworkCodingConfiguration::minimumFileSize;
double NetworkCodingConfiguration::maxAgeBlock;
double NetworkCodingConfiguration::maxAgeDecoder;
std::vector<std::string> NetworkCodingConfiguration::whitelistTargetNodeNames;
bool NetworkCodingConfiguration::isNetworkCodingEnabledForAllTargets = true;
bool NetworkCodingConfiguration::isSourceOnlyCoding = false;
string NetworkCodingConfiguration::nodeIdStr;
bool NetworkCodingConfiguration::decodeOnlyIfTarget = false;
contextawarecodingtracker_t NetworkCodingConfiguration::contextawaretracker; 
RecursiveMutex NetworkCodingConfiguration::contextAwareMutex;

NetworkCodingConfiguration::NetworkCodingConfiguration() {
    //should already be initialized to 0 which is false
    //do not override may have already been init
    //NetworkCodingConfiguration::isNetworkCodingTurnedOn = false;
    //HAGGLE_DBG2("NetworkCodingConfiguration initialvalues isNetworkCodingTurnedOn=%d resendDelay=%f\n",isNetworkCodingTurnedOn,resendDelay);
}

NetworkCodingConfiguration::~NetworkCodingConfiguration() {

}

void NetworkCodingConfiguration::turnOffNetworkCoding() {
    //HAGGLE_DBG("turn off networkcoding\n");
    NetworkCodingConfiguration::isNetworkCodingTurnedOn = false;
}

void NetworkCodingConfiguration::turnOnNetworkCoding() {
    //HAGGLE_DBG("turn on networkcoding\n");
    NetworkCodingConfiguration::isNetworkCodingTurnedOn = true;
}

void NetworkCodingConfiguration::
	turnOffNetworkCodingForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef) {
	string targetNodeRefIdStr = targetNodeRef->getName();
	HAGGLE_DBG("context aware coding disable for dataobject id = %s targetnoderefid = %s\n",
			parentDataObjectId.c_str(),targetNodeRefIdStr.c_str());
    if (!parentDataObjectId.empty()) {
        HAGGLE_DBG("Not disabling context aware for data object id = %s as we may have started coding\n", parentDataObjectId.c_str());
    } else {
        Mutex::AutoLocker l(NetworkCodingConfiguration::contextAwareMutex); // needs to be fine grained
        HAGGLE_DBG("Disabling context aware for all data objects for targetnoderefid = %s\n", targetNodeRefIdStr.c_str());
        string key = parentDataObjectId + "|" + targetNodeRefIdStr;
        contextawarecodingtracker_t::iterator it = NetworkCodingConfiguration::contextawaretracker.find(key);
        if (it != NetworkCodingConfiguration::contextawaretracker.end())
            NetworkCodingConfiguration::contextawaretracker.erase(it);
    }
}


void NetworkCodingConfiguration::
	turnOnNetworkCodingForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef) {
	string targetNodeRefIdStr = targetNodeRef->getName();
	HAGGLE_DBG("context aware coding enable for dataobject id = %s targetnoderefid = %s\n",
			parentDataObjectId.c_str(),targetNodeRefIdStr.c_str());
    string key = parentDataObjectId + "|" + targetNodeRefIdStr;

    {
        Mutex::AutoLocker l(NetworkCodingConfiguration::contextAwareMutex); // needs to be fine grained
        contextawarecodingtracker_t::iterator it = NetworkCodingConfiguration::contextawaretracker.find(key);
        if (it == NetworkCodingConfiguration::contextawaretracker.end())
            NetworkCodingConfiguration::contextawaretracker.insert(make_pair(key, true));
    }
}

bool NetworkCodingConfiguration::isNetworkCodingEnabled(DataObjectRef dataObject, NodeRef targetNodeToNetworkCodeFor) {
	// if network coding turned on doesn't matter about dataobjects or targetnoderefids
	if( NetworkCodingConfiguration::isNetworkCodingTurnedOn ) {
		return true;
	}

	if( !targetNodeToNetworkCodeFor ) {
		return false;
	}

	string targetNodeId = targetNodeToNetworkCodeFor->getName();
    string dataObjectId;
    if (dataObject)
        dataObjectId = dataObject->getIdStr();

    string key = dataObjectId + "|" + targetNodeId;
    {
        Mutex::AutoLocker l(NetworkCodingConfiguration::contextAwareMutex); // needs to be fine grained
        contextawarecodingtracker_t::iterator it = NetworkCodingConfiguration::contextawaretracker.find(key);
        if (it != NetworkCodingConfiguration::contextawaretracker.end()) {
            return true;
        }

        key = "|" + targetNodeId;
        it = NetworkCodingConfiguration::contextawaretracker.find(key);
        if (it != NetworkCodingConfiguration::contextawaretracker.end()) {
            HAGGLE_DBG("context aware coding is enabled for targetnoderefid=%s, saving status for dataobject=%s\n", targetNodeId.c_str(), dataObjectId.c_str());
            NetworkCodingConfiguration::contextawaretracker.insert(make_pair(dataObjectId + "|" + targetNodeId, true));
            return true;
        }
    }

	return false;
}

void NetworkCodingConfiguration::turnOffForwarding() {
    NetworkCodingConfiguration::isForwardingTurnedOn = false;
}

void NetworkCodingConfiguration::turnOnForwarding() {
    NetworkCodingConfiguration::isForwardingTurnedOn = true;
}

bool NetworkCodingConfiguration::isForwardingEnabled() {
    return NetworkCodingConfiguration::isForwardingTurnedOn;
}

void NetworkCodingConfiguration::turnOffNodeDescUpdateOnReconstruction() {
    NetworkCodingConfiguration::isNodeDescUpdateOnReconstructionTurnedOn = false;
}

void NetworkCodingConfiguration::turnOnNodeDescUpdateOnReconstruction() {
    NetworkCodingConfiguration::isNodeDescUpdateOnReconstructionTurnedOn = true;
}

bool NetworkCodingConfiguration::isNodeDescUpdateOnReconstructionEnabled() {
    return NetworkCodingConfiguration::isNodeDescUpdateOnReconstructionTurnedOn;
}

double NetworkCodingConfiguration::getResendDelay() {
    return NetworkCodingConfiguration::resendDelay;
}

void NetworkCodingConfiguration::setResendDelay(double _resendDelay) {
    //HAGGLE_DBG("setting resenddelay=%f currentalue=%f\n",_resendDelay,resendDelay);
    NetworkCodingConfiguration::resendDelay = _resendDelay;
}

double NetworkCodingConfiguration::getDelayDeleteNetworkCodedBlocks() {
    return NetworkCodingConfiguration::delayDeleteNetworkCodedBlocks;
}

void NetworkCodingConfiguration::setDelayDeleteNetworkCodedBlocks(double _delayDeleteNetworkCodedBlocks) {
    //HAGGLE_DBG("setting delayDeleteNetworkCodedBlocks=%f\n",_delayDeleteNetworkCodedBlocks);
    NetworkCodingConfiguration::delayDeleteNetworkCodedBlocks = _delayDeleteNetworkCodedBlocks;
}

size_t NetworkCodingConfiguration::getBlockSize() {
    return NetworkCodingConfiguration::blockSize;
}

void NetworkCodingConfiguration::setBlockSize(size_t _blockSize) {
    //HAGGLE_DBG("setting blockSize=%d\n",_blockSize);
    NetworkCodingConfiguration::blockSize = _blockSize;
}

int NetworkCodingConfiguration::getNumberOfBlockPerDataObject() {
    return NetworkCodingConfiguration::numberOfBlocksPerDataObject;
}

void NetworkCodingConfiguration::setNumberOfBlockPerDataObject(int _numberOfBlocksPerDataObject) {
    //HAGGLE_DBG("setting numberOfBlocksPerDataObject=%d\n",_numberOfBlocksPerDataObject);
    NetworkCodingConfiguration::numberOfBlocksPerDataObject = _numberOfBlocksPerDataObject;
}

void NetworkCodingConfiguration::setDelayDeleteReconstructedNetworkCodedBlocks(double _delayDeleteReconstructedNetworkCodedBlocks) {
    //HAGGLE_DBG("setting delayDeleteReconstructedNetworkCodedBlocks=%f\n",_delayDeleteReconstructedNetworkCodedBlocks);
    NetworkCodingConfiguration::delayDeleteReconstructedNetworkCodedBlocks = _delayDeleteReconstructedNetworkCodedBlocks;
}

double NetworkCodingConfiguration::getDelayDeleteReconstructedNetworkCodedBlocks() {
    return NetworkCodingConfiguration::delayDeleteReconstructedNetworkCodedBlocks;
}

size_t NetworkCodingConfiguration::getMinimumFileSize() {
    return NetworkCodingConfiguration::minimumFileSize;
}

void NetworkCodingConfiguration::setMinimumFileSize(size_t _minimumFileSize) {
    //HAGGLE_DBG("setting minimumFileSize=%d\n",_minimumFileSize);
    NetworkCodingConfiguration::minimumFileSize=_minimumFileSize;
}

void NetworkCodingConfiguration::setResendReconstructedDelay(double _resendReconstructedDelay) {
    //HAGGLE_DBG("setting resendReconstructedDelay=%f\n",_resendReconstructedDelay);
    NetworkCodingConfiguration::resendReconstructedDelay = _resendReconstructedDelay;
}

double NetworkCodingConfiguration::getResendReconstructedDelay() {
    return NetworkCodingConfiguration::resendReconstructedDelay;
}

double NetworkCodingConfiguration::getMaxAgeBlock() {
    return NetworkCodingConfiguration::maxAgeBlock;
}

void NetworkCodingConfiguration::setMaxAgeBlock(double _maxAgeBlock) {
    //HAGGLE_DBG("setting maxAgeBlock=%f\n",_maxAgeBlock);
    NetworkCodingConfiguration::maxAgeBlock = _maxAgeBlock;
}

double NetworkCodingConfiguration::getMaxAgeDecoder() {
    return NetworkCodingConfiguration::maxAgeDecoder;
}

void NetworkCodingConfiguration::setMaxAgeDecoder(double _maxAgeDecoder) {
    NetworkCodingConfiguration::maxAgeDecoder = _maxAgeDecoder;
}

std::vector<std::string> NetworkCodingConfiguration::getWhitelistTargetNodeNames() {
	return NetworkCodingConfiguration::whitelistTargetNodeNames;
}

void NetworkCodingConfiguration::setWhiteListTargetNodeNames(std::vector<std::string> targetNodeNames) {
	NetworkCodingConfiguration::whitelistTargetNodeNames = targetNodeNames;
}

bool NetworkCodingConfiguration::isEnabledForAllTargets() {
	return NetworkCodingConfiguration::isNetworkCodingEnabledForAllTargets;
}

void NetworkCodingConfiguration::setEnabledForAllTargets() {
	NetworkCodingConfiguration::isNetworkCodingEnabledForAllTargets = true;
}

void NetworkCodingConfiguration::setDisabledForAllTargets() {
	NetworkCodingConfiguration::isNetworkCodingEnabledForAllTargets = false;
}

bool NetworkCodingConfiguration::isSourceOnlyCodingEnabled() {
	return NetworkCodingConfiguration::isSourceOnlyCoding; 
}

void NetworkCodingConfiguration::setEnabledSourceOnlyCoding() {
	NetworkCodingConfiguration::isSourceOnlyCoding = true;
}

void NetworkCodingConfiguration::setDisabledSourceOnlyCoding() {
	NetworkCodingConfiguration::isSourceOnlyCoding = false;
}

bool NetworkCodingConfiguration::getDecodeOnlyIfTarget() {
    return NetworkCodingConfiguration::decodeOnlyIfTarget;
}

void NetworkCodingConfiguration::setDecodeOnlyIfTarget(size_t _decodeOnlyIfTarget) {
    NetworkCodingConfiguration::decodeOnlyIfTarget = _decodeOnlyIfTarget;
}

void NetworkCodingConfiguration::setNodeIdStr(string _nodeIdStr) {
	HAGGLE_DBG2("setting nodeIdStr %s\n",_nodeIdStr.c_str());
	NetworkCodingConfiguration::nodeIdStr = _nodeIdStr;
	HAGGLE_DBG2("nodeIdStr value %s\n",nodeIdStr.c_str());
}

string NetworkCodingConfiguration::getNodeIdStr() {
	return NetworkCodingConfiguration::nodeIdStr;
}
