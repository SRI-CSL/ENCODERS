/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include "ProtocolUDPUnicast.h"

ProtocolConfiguration *ProtocolUDPUnicastClient::getConfiguration()
{
    if (NULL == cfg) {
        cfg = new ProtocolConfigurationUDPUnicast();
    }
    return cfg;
} 

ProtocolConfigurationUDPUnicast *ProtocolUDPUnicastClient::getUDPConfiguration() 
{
    ProtocolConfiguration *genCfg = getConfiguration();
    if (NULL == genCfg) {
        HAGGLE_ERR("NULL cfg!\n");
    }
    // SW: down cast to proper subclass, this looks dangerous but OK since
    // we overrode the other setters and getters
    return static_cast<ProtocolConfigurationUDPUnicast *>(genCfg);
}

const IPv4Address *ProtocolUDPUnicastClient::getSendIP()
{
    return peerIface->getAddress<IPv4Address>();
}

int ProtocolUDPUnicastSender::getSendPort()
{
    return getUDPConfiguration()->getControlPortB();
}

ProtocolUDPUnicastSender::~ProtocolUDPUnicastSender()
{
  if(!isDetached()) // MOS - no assumptions about other protocols in detached state
     proxy->removeCachedSender(this);
}


int ProtocolUDPUnicastSenderNoControl::getSendPort()
{
    return getUDPConfiguration()->getNonControlPortC();
}

int ProtocolUDPUnicastReceiver::getSendPort()
{
    return getUDPConfiguration()->getControlPortA();
}

ProtocolUDPUnicastReceiver::~ProtocolUDPUnicastReceiver()
{
  if(!isDetached()) // MOS - no assumptions about other protocols in detached state
     proxy->removeCachedReceiver(this);
}

int ProtocolUDPUnicastReceiverNoControl::getSendPort()
{
    return getUDPConfiguration()->getNonControlPortC();
}

ProtocolEvent 
ProtocolUDPUnicastSenderNoControl::sendDataObjectNowSuccessHook(
    const DataObjectRef& dObj)
{
    NodeRef actualNbr = getManager()->getKernel()->getNodeStore()->retrieve(peerNode, true);
    if (!actualNbr) {
        HAGGLE_ERR("Actual neighbor not in node store\n");
        return PROT_EVENT_ERROR;
    }
    actualNbr->getBloomfilter()->add(dObj);
    return PROT_EVENT_SUCCESS;
}
