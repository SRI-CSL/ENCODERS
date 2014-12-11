/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#ifndef NETWORKCODINGCONFIGURATION_H_
#define NETWORKCODINGCONFIGURATION_H_

#include <vector>
#include <sstream>


#include "DataObject.h"
#include "Node.h"
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/Reference.h>
#include <libcpphaggle/Mutex.h>

typedef Reference<Node> NodeRef;
typedef HashMap<string, bool> contextawarecodingtracker_t;

/**
 * Want to avoid circular dependencies and also keep the configuration in 1 single place.
 * Any configuration parmeters can be read using this class.
 */
class NetworkCodingConfiguration {
public:
    NetworkCodingConfiguration();
    virtual ~NetworkCodingConfiguration();

    void turnOffNetworkCoding();
    void turnOnNetworkCoding();

    bool isNetworkCodingEnabled(DataObjectRef dataObject, NodeRef targetNodeToNetworkCodeFor);
    void turnOnNetworkCodingForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef);
    void turnOffNetworkCodingForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef);

    void turnOffForwarding();
    void turnOnForwarding();

    bool isForwardingEnabled();

    void turnOffNodeDescUpdateOnReconstruction();
    void turnOnNodeDescUpdateOnReconstruction();

    bool isNodeDescUpdateOnReconstructionEnabled();

    double getResendDelay();
    void setResendDelay(double _resendDelay);

    void setResendReconstructedDelay(double _resendReconstructedDelay);
    double getResendReconstructedDelay();

    double getDelayDeleteNetworkCodedBlocks();
    void setDelayDeleteNetworkCodedBlocks(double _delayDeleteNetworkCodedBlocks);

    size_t getBlockSize();
    void setBlockSize(size_t _blockSize);

    int getNumberOfBlockPerDataObject();
    void setNumberOfBlockPerDataObject(int _numberOfBlocksPerDataObject);

    void setDelayDeleteReconstructedNetworkCodedBlocks(double _delayDeleteReconstructedNetworkCodedBlocks);
    double getDelayDeleteReconstructedNetworkCodedBlocks();

    size_t getMinimumFileSize();
    void setMinimumFileSize(size_t _minimumFileSize);

    double getMaxAgeBlock();
    void setMaxAgeBlock(double _maxAgeBlock);

    double getMaxAgeDecoder();
    void setMaxAgeDecoder(double _maxAgeDecoder);

    std::vector<std::string> getWhitelistTargetNodeNames();
    void setWhiteListTargetNodeNames(std::vector<std::string> targetNodeNames);

    bool isEnabledForAllTargets();
    void setEnabledForAllTargets();
    void setDisabledForAllTargets();

    bool isSourceOnlyCodingEnabled();
    void setEnabledSourceOnlyCoding();
    void setDisabledSourceOnlyCoding(); 

    bool getDecodeOnlyIfTarget();
    void setDecodeOnlyIfTarget(size_t _decodeOnlyIfTarget);

    void setNodeIdStr(string _nodeIdStr);
    string getNodeIdStr();

private:
    //initialize to true in init
    static bool isNetworkCodingTurnedOn;
    static bool isForwardingTurnedOn;
    static bool isNodeDescUpdateOnReconstructionTurnedOn;
    static double resendDelay;
    static double resendReconstructedDelay;
    static double delayDeleteNetworkCodedBlocks;
    static double delayDeleteReconstructedNetworkCodedBlocks;
    static size_t blockSize;
    static size_t minimumFileSize;
    static int numberOfBlocksPerDataObject;
    static double maxAgeBlock;
    static double maxAgeDecoder;
    static std::vector<std::string> whitelistTargetNodeNames;
    static bool isNetworkCodingEnabledForAllTargets;
    static bool isSourceOnlyCoding;
    static string nodeIdStr;
    static bool decodeOnlyIfTarget;

    static contextawarecodingtracker_t contextawaretracker;
    static RecursiveMutex contextAwareMutex; 
};

#endif /* NETWORKCODINGCONFIGURATION_H_ */
