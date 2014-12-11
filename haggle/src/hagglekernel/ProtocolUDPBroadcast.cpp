/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include <string.h>

#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "ProtocolUDPBroadcast.h"

ProtocolConfiguration *ProtocolUDPBroadcastClient::getConfiguration()
{
    if (NULL == cfg) {
        cfg = new ProtocolConfigurationUDPBroadcast();
    }
    return cfg;
}

ProtocolConfigurationUDPBroadcast *ProtocolUDPBroadcastClient::getUDPConfiguration() 
{
    ProtocolConfiguration *genCfg = getConfiguration();
    if (NULL == genCfg) {
        HAGGLE_ERR("%s NULL cfg!\n", getName());
    }
    // SW: down cast to proper subclass, this looks dangerous but OK since
    // we overrode the other setters and getters
    return static_cast<ProtocolConfigurationUDPBroadcast *>(genCfg);
}

const IPv4Address *ProtocolUDPBroadcastClient::getSendIP() {
    return localIface->getAddress<IPv4BroadcastAddress>();
}

int ProtocolUDPBroadcastSender::getSendPort()
{
    return getUDPConfiguration()->getControlPortB();
}

ProtocolUDPBroadcastSender::~ProtocolUDPBroadcastSender()
{
  if(!isDetached()) // MOS - no assumptions about other protocols in detached state
     proxy->removeCachedSender(this);
}

int ProtocolUDPBroadcastSenderNoControl::getSendPort()
{
    return getUDPConfiguration()->getNonControlPortC();
}

int ProtocolUDPBroadcastReceiver::getSendPort()
{
    return getUDPConfiguration()->getControlPortA();
}

ProtocolUDPBroadcastReceiver::~ProtocolUDPBroadcastReceiver()
{
  if(unregisterFromProxy) { // MOS - this is false for passive receiver
   if(!isDetached()) // MOS - no assumptions about other protocols in detached state
      proxy->removeCachedReceiver(this);
  }
}

// MOS - passive receiver typically have shorter timeout
int ProtocolUDPBroadcastPassiveReceiver::getWaitTimeBeforeDoneMillis()
{
  return getConfiguration()->getPassiveWaitTimeBeforeDoneMillis();
}

ProtocolUDPBroadcastPassiveReceiver::~ProtocolUDPBroadcastPassiveReceiver()
{
    if(!isDetached()) // MOS - no assumptions about other protocols in detached state
      proxy->removeCachedSnoopedReceiver(this);
}

int ProtocolUDPBroadcastReceiverNoControl::getSendPort()
{
    return getUDPConfiguration()->getNonControlPortC();
}

ProtocolEvent
ProtocolUDPBroadcastSender::sendDataObjectNowSuccessHook(
    const DataObjectRef& dObj)
{
    // add to nbrs bloomfilters
    NodeRefList nbrs;
    int numNbrs = getManager()->getKernel()->getNodeStore()->retrieveNeighbors(nbrs);
    
    if (numNbrs <= 0) {
        HAGGLE_ERR("%s No neighbors in node store, yet sent broadcast...\n", getName());
        return PROT_EVENT_ERROR;
    }

    for (NodeRefList::iterator it = nbrs.begin(); it != nbrs.end(); it++) {
        NodeRef& nbr = *it;
        NodeRef actualNbr = getManager()->getKernel()->getNodeStore()->retrieve(nbr, true);
        if (!actualNbr) {
            HAGGLE_ERR("%s Actual neighbor not in node store\n", getName());
            continue;
        }
        actualNbr->getBloomfilter()->add(dObj);
	// HAGGLE_DBG2("%s Setting bloom filter optimistically for data object [%s] and neighbor %s, bloomfilter #objs=%lu\n", 
	//	    getName(), DataObject::idString(dObj).c_str(), actualNbr->getName().c_str(), actualNbr->getBloomfilter()->numObjects());
        //getManager()->getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dObj, nbr, 0));
    }
    // done adding to nbrs bloomfilters
    
    return PROT_EVENT_SUCCESS;
}

ProtocolEvent
ProtocolUDPBroadcastPassiveReceiver::sendHook(
        const void *buf, 
        size_t len, 
        const int flags, 
        ssize_t *bytes) 
{
    *bytes = static_cast<ssize_t>(len);
    return PROT_EVENT_SUCCESS;
}

ProtocolEvent
ProtocolUDPBroadcastReceiverNoControl::receiveDataObject() {
    return receiveDataObjectNoControl();
}

// SW: START: inspect 2-hop neighborhood  in ND metadata
void 
ProtocolUDPBroadcastReceiver::receiveDataObjectSuccessHook(
    const DataObjectRef& dObj)
{
    NodeRef currentPeer;
    { Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS

    if (!peerNode) {
        HAGGLE_ERR("%s No peer node set\n", getName());
        return;
    }

    if (Node::TYPE_PEER != peerNode->getType()) {
        HAGGLE_DBG("peer node is in undefined state\n", getName());
        return;
    }

    NodeRef actualNode = getKernel()->getNodeStore()->retrieve(peerNode);
    if (!actualNode) { // MOS
        HAGGLE_DBG("%s node %s not in node store\n", getName(), peerNode->getIdStr());
        return;
    }

    DataObjectRef peerDO = actualNode->getDataObject();
    Metadata *m = peerDO->getMetadata()->getMetadata("NodeManager");
    if (m) {
        m = m->getMetadata("Neighbors");
    }

    if (!m) {
        HAGGLE_DBG("%s No 2-hop neighborhood info available for node: %s\n", getName(), peerNode->getIdStr());
        return;
    }

    const Metadata *nbrMetadata = NULL; 

    int num_bf_updated = 0;
    int i = 0;
    while ((nbrMetadata = m->getMetadata("Neighbor", i++))) {
        string nbr_id = nbrMetadata->getContent();
        NodeRef twohop_nbr = getKernel()->getNodeStore()->retrieve(nbr_id);
        if (twohop_nbr) {
            twohop_nbr->getBloomfilter()->add(dObj);
            num_bf_updated++;
        }
    }

    HAGGLE_DBG("%s Used 2-hop info to set: %d bloomfilters\n", getName(), num_bf_updated);
}
// SW: END: inspect 2-hop neighborhood  in ND metadata.

