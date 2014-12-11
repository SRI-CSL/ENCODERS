/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLCFG_H
#define _PROTOCOLCFG_H

class ProtocolConfiguration;

// START: old defines from Protocol.h:
#define PROT_WAIT_TIME_BEFORE_DONE_MS (60*1000)
#define PROT_WAIT_TIME_BEFORE_DONE_MS_NAME "waitTimeBeforeDoneMillis"

#define PROT_CONNECTION_ATTEMPTS 4
#define PROT_CONNECTION_ATTEMPTS_NAME "connectionAttempts"

#define PROT_BLOCK_TRY_MAX 5
#define PROT_BLOCK_TRY_MAX_NAME "maxBlockingTries"

#define PROT_BLOCK_SLEEP_TIME_MS 400
#define PROT_BLOCK_SLEEP_TIME_MS_NAME "blockingSleepMillis"

#define PROT_MIN_SEND_DELAY_BASE_MS 0
#define PROT_MIN_SEND_DELAY_BASE_MS_NAME "minSendDelayBaseMillis"

#define PROT_MIN_SEND_DELAY_LINEAR_MS 0
#define PROT_MIN_SEND_DELAY_LINEAR_MS_NAME "minSendDelayLinearMillis"

#define PROT_MIN_SEND_DELAY_SQUARE_MS 0
#define PROT_MIN_SEND_DELAY_SQUARE_MS_NAME "minSendDelaySquareMillis"

#define PROT_MAX_RANDOM_SEND_DELAY_MS 100
#define PROT_MAX_RANDOM_SEND_DELAY_MS_NAME "maxRandomSendDelayMillis"

#define PROT_RECVSEND_TIMEOUT_MS (20*1000)
#define PROT_RECVSEND_TIMEOUT_MS_NAME "connectionWaitMillis"

#define PROT_PASSIVE_WAIT_TIME_BEFORE_DONE_MS (10*1000)
#define PROT_PASSIVE_WAIT_TIME_BEFORE_DONE_MS_NAME "passiveWaitTimeBeforeDoneMillis"

#define PROT_CONNNECTION_PAUSE_TIME_MS (5*1000)
#define PROT_CONNNECTION_PAUSE_TIME_MS_NAME "connectionPauseMillis"

#define PROT_CONNNECTION_PAUSE_JITTER_MS (20*1000)
#define PROT_CONNNECTION_PAUSE_JITTER_MS_NAME "connectionPauseJitterMillis"

#define PROT_MAX_PROT_ERRORS 0 // MOS - don't use with TCP
#define PROT_MAX_PROT_ERRORS_NAME "maxProtocolErrors"

#define PROT_MAX_SEND_TIMEOUTS 0 // MOS - don't use with TCP
#define PROT_MAX_SEND_TIMEOUTS_NAME "maxSendTimeouts"

#define PROT_LOAD_REDUCTION_MIN_QUEUE_SIZE INT_MAX
#define PROT_LOAD_REDUCTION_MIN_QUEUE_SIZE_NAME "loadReductionMinQueueSize"

#define PROT_LOAD_REDUCTION_MAX_QUEUE_SIZE INT_MAX
#define PROT_LOAD_REDUCTION_MAX_QUEUE_SIZE_NAME "loadReductionMaxQueueSize"

#define PROT_MAX_INSTANCES 100
#define PROT_MAX_INSTANCES_NAME "maxInstances"

#define PROT_MAX_INSTANCES_PER_LINK 3
#define PROT_MAX_INSTANCES_PER_LINK_NAME "maxInstancesPerLink"

#define PROT_MAX_RECEIVER_INSTANCES 100
#define PROT_MAX_RECEIVER_INSTANCES_NAME "maxReceiverInstances"

#define PROT_MAX_RECEIVER_INSTANCES_PER_LINK 1
#define PROT_MAX_RECEIVER_INSTANCES_PER_LINK_NAME "maxReceiverInstancesPerLink"

#define PROT_MAX_PASSIVE_RECEIVER_INSTANCES 40
#define PROT_MAX_PASSIVE_RECEIVER_INSTANCES_NAME "maxPassiveReceiverInstances"

#define PROT_MAX_PASSIVE_RECEIVER_INSTANCES_PER_LINK 1
#define PROT_MAX_PASSIVE_RECEIVER_INSTANCES_PER_LINK_NAME "maxPassiveReceiverInstancesPerLink"

#define PROT_REDUNDANCY 0
#define PROT_REDUNDANCY_NAME "redundancy"

// END: old defines from Protocol.h:

#include "Manager.h"
#include "Protocol.h"
#include <haggleutils.h>

class ProtocolConfiguration
{
protected:
    ProtType_t type;
private: 
    int waitTimeBeforeDoneMillis;
    int passiveWaitTimeBeforeDoneMillis; // MOS
    int connectionAttempts;
    int maxBlockingTries;
    int blockingSleepMillis;
    int minSendDelayBaseMillis; // MOS
    int minSendDelayLinearMillis; // MOS
    int minSendDelaySquareMillis; // MOS
    int maxRandomSendDelayMillis; // MOS
    int connectionWaitMillis;
    int passiveWaitMillis; // MOS
    int connectionPauseMillis;
    int connectionPauseJitterMillis;
    int maxProtocolErrors;
    int maxSendTimeouts;
    int loadReductionMinQueueSize; // MOS
    int loadReductionMaxQueueSize; // MOS
    int maxInstances; // MOS
    int maxInstancesPerLink; // MOS
    int maxReceiverInstances; // MOS
    int maxReceiverInstancesPerLink; // MOS
    int maxPassiveReceiverInstances; // MOS
    int maxPassiveReceiverInstancesPerLink; // MOS
    int redundancy; // MOS
public:
    // used for clone():
    ProtocolConfiguration(
        ProtType_t _type,
        int _waitTimeBeforeDoneMillis, 
	int _passiveWaitTimeBeforeDoneMillis,
        int _connectionAttempts,
        int _maxBlockingTries,
        int _blockingSleepMillis,
        int _minSendDelayBaseMillis,
        int _minSendDelayLinearMillis,
        int _minSendDelaySquareMillis,
	int _maxRandomSendDelayMillis,
        int _connectionWaitMillis,
	int _passiveWaitMillis,
        int _connectionPauseMillis,
        int _connectionPauseJitterMillis,
        int _maxProtocolErrors,
        int _maxSendTimeouts,
	int _loadReductionMinQueueSize,
	int _loadReductionMaxQueueSize,
	int _maxInstances,
	int _maxInstancesPerLink,
	int _maxReceiverInstances,
	int _maxReceiverInstancesPerLink,
        int _maxPassiveReceiverInstances,
        int _maxPassiveReceiverInstancesPerLink,
	int _redundancy) :
            type(_type),
            waitTimeBeforeDoneMillis(_waitTimeBeforeDoneMillis),
            passiveWaitTimeBeforeDoneMillis(_passiveWaitTimeBeforeDoneMillis),
            connectionAttempts(_connectionAttempts),
            maxBlockingTries(_maxBlockingTries),
            blockingSleepMillis(_blockingSleepMillis),
            minSendDelayBaseMillis(_minSendDelayBaseMillis),
            minSendDelayLinearMillis(_minSendDelayLinearMillis),
            minSendDelaySquareMillis(_minSendDelaySquareMillis),
            maxRandomSendDelayMillis(_maxRandomSendDelayMillis),
            connectionWaitMillis(_connectionWaitMillis),
            connectionPauseMillis(_connectionPauseMillis),
            connectionPauseJitterMillis(_connectionPauseJitterMillis),
            maxProtocolErrors(_maxProtocolErrors),
	    maxSendTimeouts(_maxSendTimeouts),
	    loadReductionMinQueueSize(_loadReductionMinQueueSize),
            loadReductionMaxQueueSize(_loadReductionMaxQueueSize),
	    maxInstances(_maxInstances), maxInstancesPerLink(_maxInstancesPerLink),
	    maxReceiverInstances(_maxReceiverInstances), maxReceiverInstancesPerLink(_maxReceiverInstancesPerLink),
	    maxPassiveReceiverInstances(_maxPassiveReceiverInstances), maxPassiveReceiverInstancesPerLink(_maxPassiveReceiverInstancesPerLink),
	    redundancy(_redundancy)  {};
            
    ProtocolConfiguration(ProtType_t _type) :
        type(_type),
        waitTimeBeforeDoneMillis(PROT_WAIT_TIME_BEFORE_DONE_MS),
        passiveWaitTimeBeforeDoneMillis(PROT_PASSIVE_WAIT_TIME_BEFORE_DONE_MS), // MOS
        connectionAttempts(PROT_CONNECTION_ATTEMPTS),
        maxBlockingTries(PROT_BLOCK_TRY_MAX),
        blockingSleepMillis(PROT_BLOCK_SLEEP_TIME_MS),
        minSendDelayBaseMillis(PROT_MIN_SEND_DELAY_BASE_MS),
        minSendDelayLinearMillis(PROT_MIN_SEND_DELAY_LINEAR_MS),
        minSendDelaySquareMillis(PROT_MIN_SEND_DELAY_SQUARE_MS),
        maxRandomSendDelayMillis(PROT_MAX_RANDOM_SEND_DELAY_MS),
        connectionWaitMillis(PROT_RECVSEND_TIMEOUT_MS),
        connectionPauseMillis(PROT_CONNNECTION_PAUSE_TIME_MS),
        connectionPauseJitterMillis(PROT_CONNNECTION_PAUSE_JITTER_MS),
        maxProtocolErrors(PROT_MAX_PROT_ERRORS),
	maxSendTimeouts(PROT_MAX_SEND_TIMEOUTS),
	loadReductionMinQueueSize(PROT_LOAD_REDUCTION_MIN_QUEUE_SIZE),
	loadReductionMaxQueueSize(PROT_LOAD_REDUCTION_MAX_QUEUE_SIZE),
	maxInstances(PROT_MAX_INSTANCES),
	maxInstancesPerLink(PROT_MAX_INSTANCES_PER_LINK),
	maxReceiverInstances(PROT_MAX_RECEIVER_INSTANCES),
	maxReceiverInstancesPerLink(PROT_MAX_RECEIVER_INSTANCES_PER_LINK),
	maxPassiveReceiverInstances(PROT_MAX_PASSIVE_RECEIVER_INSTANCES),
	maxPassiveReceiverInstancesPerLink(PROT_MAX_PASSIVE_RECEIVER_INSTANCES_PER_LINK),
	redundancy(PROT_REDUNDANCY) {};

    virtual ~ProtocolConfiguration() { }

    virtual void onConfig(const Metadata &m);

    int parseInt(const Metadata &m, const char *param);

    bool parseBool(const Metadata &m, const char *paramName, bool *o_parsedBool); 

    // Wait time before closing connection and setting PROT_FLAG_DONE (seconds)
    // This should probably be quite short since it will keep a connection open and
    // potentially block the channel. This might be improved if it is possible to
    int getWaitTimeBeforeDoneMillis() { return waitTimeBeforeDoneMillis; }

    int getConnectionAttempts() { return connectionAttempts; }

    // The number of attempts to try to send or receive something when a "would block"
    // error is returned from e.g., a socket
    int getMaxBlockingTries() { return maxBlockingTries; }

    // The number of milli seconds to sleep between each try to send or receive when
    // a block error occurs
    int getBlockingSleepMillis() { return blockingSleepMillis; }

    // min delay to limit sending rate (can be function of number of neighbors)
    int getMinSendDelayBaseMillis() { return minSendDelayBaseMillis; }
    int getMinSendDelayLinearMillis() { return minSendDelayLinearMillis; }
    int getMinSendDelaySquareMillis() { return minSendDelaySquareMillis; }

    // max delay to randomize send (especially for broadcast)
    int getMaxRandomSendDelayMillis() { return maxRandomSendDelayMillis; }

    // Seconds to wait for read or write status on a connection
    int getConnectionWaitMillis() { return connectionWaitMillis; }

    // Seconds to wait for read or write status on a connection for passive/snooper protocols
    int getPassiveWaitTimeBeforeDoneMillis() { return passiveWaitTimeBeforeDoneMillis; }
    void setPassiveWaitTimeBeforeDoneMillis(int _passiveWaitTimeBeforeDoneMillis) { passiveWaitTimeBeforeDoneMillis = _passiveWaitTimeBeforeDoneMillis; }

    ProtType_t getType() { return type; }

    // pause and jitter on a connection error
    int getConnectionPauseMillis() { return connectionPauseMillis; }

    int getConnectionPauseJitterMillis() { return connectionPauseJitterMillis; }

    int getMaxProtocolErrors() { return maxProtocolErrors; }

    int getMaxSendTimeouts() { return maxSendTimeouts; }

    int getLoadReductionMinQueueSize() { return loadReductionMinQueueSize; }
    int getLoadReductionMaxQueueSize() { return loadReductionMaxQueueSize; }

    int getMaxInstances() { return maxInstances; }
    int getMaxInstancesPerLink() { return maxInstancesPerLink; }
    int getMaxReceiverInstances() { return maxReceiverInstances; }
    int getMaxReceiverInstancesPerLink() { return maxReceiverInstancesPerLink; }
    int getMaxPassiveReceiverInstances() { return maxPassiveReceiverInstances; }
    int getMaxPassiveReceiverInstancesPerLink() { return maxPassiveReceiverInstancesPerLink; }

    int getRedundancy() { return redundancy; }

    virtual ProtocolConfiguration *clone(ProtocolConfiguration *o_cfg = NULL);  
};

#endif /* _PROTOCOLCFG_H */
