/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include "ProtocolConfiguration.h"

// NOTE: we default to 0
int ProtocolConfiguration::parseInt(
    const Metadata &m, 
    const char *paramName) 
{
    if (NULL == paramName) {
        HAGGLE_ERR("Bad parameter\n");
        return 0;
    }

    const char *param;
    param = m.getParameter(paramName);

    if (NULL == param) {
        return 0;
    }

    char *endptr = NULL;
    int paramInt = (int) strtol(param, &endptr, 10);
    if (endptr && endptr != param && paramInt >= 0) {
        return paramInt;
    } 

    HAGGLE_ERR("Problems parsing %s\n", paramName);
    return 0;
}

// NOTE: we default to false
bool ProtocolConfiguration::parseBool(
    const Metadata &m, 
    const char *paramName,
    bool *o_parsedBool) 
{
    *o_parsedBool = false;
    if (NULL == paramName) {
        HAGGLE_ERR("Bad parameter\n");
        return false;
    }

    const char *param;
    param = m.getParameter(paramName);

    if (NULL == param) {
        return false;
    }

    if (0 == strcmp("true", param)) {
        *o_parsedBool = true;
        return true;
    }

    if (0 == strcmp("false", param)) {
        *o_parsedBool = false;
        return true;
    }

    HAGGLE_ERR("Problems parsing arp hack option\n");
    return false;
}

void ProtocolConfiguration::onConfig(const Metadata &m)
{
    int param;

    if ((param = parseInt(m, PROT_WAIT_TIME_BEFORE_DONE_MS_NAME)) > 0) {
        waitTimeBeforeDoneMillis = param;
    }
        
    if ((param = parseInt(m, PROT_CONNECTION_ATTEMPTS_NAME)) > 0) {
        connectionAttempts = param;
    }

    if ((param = parseInt(m, PROT_BLOCK_TRY_MAX_NAME)) > 0) {
        maxBlockingTries = param;
    }

    if ((param = parseInt(m, PROT_BLOCK_SLEEP_TIME_MS_NAME)) > 0) {
        blockingSleepMillis = param;
    }

    if ((param = parseInt(m, PROT_MIN_SEND_DELAY_BASE_MS_NAME)) > 0) {
        minSendDelayBaseMillis = param;
    }

    if ((param = parseInt(m, PROT_MIN_SEND_DELAY_LINEAR_MS_NAME)) > 0) {
        minSendDelayLinearMillis = param;
    }

    if ((param = parseInt(m, PROT_MIN_SEND_DELAY_SQUARE_MS_NAME)) > 0) {
        minSendDelaySquareMillis = param;
    }

    if ((param = parseInt(m, PROT_RECVSEND_TIMEOUT_MS_NAME)) > 0) {
        connectionWaitMillis = param;
    }

    if ((param = parseInt(m, PROT_PASSIVE_WAIT_TIME_BEFORE_DONE_MS_NAME)) > 0) {
        passiveWaitTimeBeforeDoneMillis = param;
    }
    
    if ((param = parseInt(m, PROT_CONNNECTION_PAUSE_TIME_MS_NAME)) > 0) {
        connectionPauseMillis = param;
    }

    if ((param = parseInt(m, PROT_CONNNECTION_PAUSE_JITTER_MS_NAME)) > 0) {
        connectionPauseJitterMillis = param;
    }

    if ((param = parseInt(m, PROT_MAX_PROT_ERRORS_NAME)) > 0) {
        maxProtocolErrors = param;
    }

    if ((param = parseInt(m, PROT_MAX_SEND_TIMEOUTS_NAME)) > 0) {
        maxSendTimeouts = param;
    }

    if ((param = parseInt(m, PROT_LOAD_REDUCTION_MIN_QUEUE_SIZE_NAME)) > 0) {
        loadReductionMinQueueSize = param;
    }

    if ((param = parseInt(m, PROT_LOAD_REDUCTION_MAX_QUEUE_SIZE_NAME)) > 0) {
        loadReductionMaxQueueSize = param;
    }

    if ((param = parseInt(m, PROT_MAX_INSTANCES_NAME)) > 0) {
        maxInstances = param;
    }

    if ((param = parseInt(m, PROT_MAX_INSTANCES_PER_LINK_NAME)) > 0) {
        maxInstancesPerLink = param;
    }

    if ((param = parseInt(m, PROT_MAX_RECEIVER_INSTANCES_NAME)) > 0) {
        maxReceiverInstances = param;
    }

    if ((param = parseInt(m, PROT_MAX_RECEIVER_INSTANCES_PER_LINK_NAME)) > 0) {
        maxReceiverInstancesPerLink = param;
    }

    if ((param = parseInt(m, PROT_MAX_PASSIVE_RECEIVER_INSTANCES_NAME)) > 0) {
        maxPassiveReceiverInstances = param;
    }

    if ((param = parseInt(m, PROT_MAX_PASSIVE_RECEIVER_INSTANCES_PER_LINK_NAME)) > 0) {
        maxPassiveReceiverInstancesPerLink = param;
    }

    if ((param = parseInt(m, PROT_REDUNDANCY_NAME)) > 0) {
        redundancy = param;
    }
}

ProtocolConfiguration *ProtocolConfiguration::clone(
    ProtocolConfiguration *o_cfg) 
{
    if (NULL == o_cfg) {
        o_cfg = new ProtocolConfiguration(type);
    }

    o_cfg->waitTimeBeforeDoneMillis = waitTimeBeforeDoneMillis;
    o_cfg->passiveWaitTimeBeforeDoneMillis = passiveWaitTimeBeforeDoneMillis;
    o_cfg->connectionAttempts = connectionAttempts;
    o_cfg->maxBlockingTries = maxBlockingTries;
    o_cfg->blockingSleepMillis = blockingSleepMillis;
    o_cfg->minSendDelayBaseMillis = minSendDelayBaseMillis;
    o_cfg->minSendDelayLinearMillis = minSendDelayLinearMillis;
    o_cfg->minSendDelaySquareMillis = minSendDelaySquareMillis;
    o_cfg->connectionWaitMillis = connectionWaitMillis;
    o_cfg->passiveWaitMillis = passiveWaitMillis;
    o_cfg->connectionPauseMillis = connectionPauseMillis;
    o_cfg->connectionPauseJitterMillis = connectionPauseJitterMillis;
    o_cfg->maxProtocolErrors = maxProtocolErrors;
    o_cfg->maxSendTimeouts = maxSendTimeouts;
    o_cfg->loadReductionMinQueueSize = loadReductionMinQueueSize;
    o_cfg->loadReductionMaxQueueSize = loadReductionMaxQueueSize;
    o_cfg->maxInstances = maxInstances;
    o_cfg->maxInstancesPerLink = maxInstancesPerLink;
    o_cfg->maxReceiverInstances = maxReceiverInstances;
    o_cfg->maxReceiverInstancesPerLink = maxReceiverInstancesPerLink;
    o_cfg->maxPassiveReceiverInstances = maxPassiveReceiverInstances;
    o_cfg->maxPassiveReceiverInstancesPerLink = maxPassiveReceiverInstancesPerLink;
    o_cfg->redundancy = redundancy;

    return o_cfg;
}
