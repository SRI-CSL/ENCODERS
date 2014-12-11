/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#ifndef FRAGMENTATIONCONFIGURATION_H_
#define FRAGMENTATIONCONFIGURATION_H_

#include <stdlib.h>

#include <vector>
#include <sstream>


#include "DataObject.h"
#include "Node.h"
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/Reference.h>
#include <libcpphaggle/Mutex.h>

typedef Reference<Node> NodeRef;

typedef HashMap<string, bool> contextawarefragmentationtracker_t; // CBMEN, HL

class FragmentationConfiguration {
public:
    FragmentationConfiguration();
    virtual ~FragmentationConfiguration();

    void turnOffFragmentation();
    void turnOnFragmentation();
    bool isFragmentationEnabled(DataObjectRef dataObject, NodeRef targetNodeToFragmentCodeFor);

    void turnOffFragmentationForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef);
    void turnOnFragmentationForDataObjectandTargetNode(string parentDataObjectId, NodeRef targetNodeRef);

    void turnOffForwarding();
    void turnOnForwarding();
    bool isForwardingEnabled();

    void turnOffNodeDescUpdateOnReconstruction();
    void turnOnNodeDescUpdateOnReconstruction();

    bool isNodeDescUpdateOnReconstructionEnabled();

    void setFragmentSize(size_t _fragmentSize);
    size_t getFragmentSize();

    size_t getMinimumFileSize();
    void setMinimumFileSize(size_t _minimumFileSize);

    void setResendDelay(double _resendDelay);
    double getResendDelay();

    void setDelayDeleteFragments(double _delayDeleteFragments);
    double getDelayDeleteFragments();

    void setDelayDeleteReconstructedFragments(double _delayDeleteReconstructedFragments);
    double getDelayDeleteReconstructedFragments();

    void setNumberFragmentsPerDataObject(size_t _numberFragmentsPerDataObject);
    size_t getNumberFragmentsPerDataObject();

    void setResendReconstructedDelay(double _resendReconstructedDelay);
    double getResendReconstructedDelay();

    void setMaxAgeFragment(double _maxAgeFragment);
    double getMaxAgeFragment();

    void setMaxAgeDecoder(double _maxAgeDecoder);
    double getMaxAgeDecoder();

    std::vector<std::string> getWhitelistTargetNodeNames();
    void setWhiteListTargetNodeNames(std::vector<std::string> targetNodeNames);

    bool isEnabledForAllTargets();
    void setEnabledForAllTargets();
    void setDisabledForAllTargets();


private:
    static bool isFragmentationTurnedOn;
    static bool isForwardingTurnedOn;
    static bool isNodeDescUpdateOnReconstructionTurnedOn;
    static size_t fragmentSize;
    static size_t minimumFileSize;
    static double resendDelay;
    static double resendReconstructedDelay;
    static double delayDeleteFragments;
    static double delayDeleteReconstructedFragments;
    static double maxAgeFragment;
    static double maxAgeDecoder;
    static size_t numberFragmentsPerDataObject;
    static std::vector<std::string> whitelistTargetNodeNames;
    static bool isFragmentationEnabledForAllTargets;
    static contextawarefragmentationtracker_t contextawaretracker;
    static RecursiveMutex contextAwareMutex;
};

#endif /* FRAGMENTATIONCONFIGURATION_H_ */
