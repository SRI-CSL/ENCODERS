/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include <string.h>
#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "ProtocolUDPBroadcastProxy.h"
#include "ProtocolManager.h"
#include "Interface.h"
#include "ProtocolUDPGeneric.h"

#define SOCKADDR_SIZE sizeof(struct sockaddr_in)

ProtocolUDPBroadcastProxy::ProtocolUDPBroadcastProxy(
    HaggleKernel *_k,
    ProtocolManager *_m,
    const InterfaceRef& _localIface) :
        k(_k), 
        m(_m), 
        localIface(_localIface),
        deleteSenderEventType(-1),
        deleteReceiverEventType(-1),
        deleteSnoopedReceiverEventType(-1),
        cfg(NULL)
{

    if (!localIface) {
        HAGGLE_ERR("Could not initialize UPD proxy without local interface\n");
        return;
    }

    // used by protocol sender
    sendData = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    recvCtrl = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    // used by protocol receiver 
    sendCtrl = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    recvData = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    // bypass control protocol
    sendNoCtrl = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    recvNoCtrl = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));
}

ProtocolUDPBroadcastProxy::~ProtocolUDPBroadcastProxy() 
{
    if (sendData) {
        delete sendData;
    }

    if (recvCtrl) {
        delete recvCtrl;
    }

    if (sendCtrl) {
        delete sendCtrl;
    }

    if (recvData) {
        delete recvData;
    }

    if (sendNoCtrl) {
        delete sendNoCtrl;
    }

    if (recvNoCtrl) {
        delete recvNoCtrl;
    }

    if (deleteSenderEventType > 0) {
        Event::unregisterType(deleteSenderEventType);
    }
    
    if (deleteReceiverEventType > 0) {
        Event::unregisterType(deleteReceiverEventType);
    }

    if (deleteSnoopedReceiverEventType > 0) {
        Event::unregisterType(deleteSnoopedReceiverEventType);
    }

    if (cfg) {
        delete cfg;
    }
}

void ProtocolUDPBroadcastProxy::setConfiguration(
    ProtocolConfigurationUDPBroadcast *_cfg)
{
    if (NULL != cfg){
        delete cfg;
    }
    cfg = _cfg;
}

ProtocolConfigurationUDPBroadcast *
ProtocolUDPBroadcastProxy::getConfiguration()
{
    if (NULL == cfg) {
        cfg = new ProtocolConfigurationUDPBroadcast();
    }
    return cfg;
}

bool ProtocolUDPBroadcastProxy::validate() 
{
    if (NULL == sendCtrl || NULL == recvCtrl || NULL == sendData 
     || NULL == recvData || NULL == sendNoCtrl || NULL == recvNoCtrl) {
        return false;
    }
    return true;
}

bool ProtocolUDPBroadcastProxy::init() 
{
#define __CLASS__ ProtocolUDPBroadcastProxy

    if (!validate()) {
        HAGGLE_ERR("Broadcast in bad state\n");
        return false;
    }

    sendData->setUseArpHack(getConfiguration()->getUseArpHack());
    recvData->setUseArpHack(getConfiguration()->getUseArpHack());
    sendCtrl->setUseArpHack(getConfiguration()->getUseArpHack());
    recvCtrl->setUseArpHack(getConfiguration()->getUseArpHack());
    sendNoCtrl->setUseArpHack(getConfiguration()->getUseArpHack());
    recvNoCtrl->setUseArpHack(getConfiguration()->getUseArpHack());

    const IPv4Address *addr = localIface->getAddress<IPv4Address>();
    if (NULL == addr) {
        HAGGLE_ERR("Could not get local interface address\n");
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *local_addr = (struct sockaddr *)buf;
    ssize_t addrlen = sizeof(struct sockaddr_in);
    bzero(buf, addrlen);
    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getControlPortB());

    struct sockaddr_in *sa = (struct sockaddr_in *)local_addr;
    if (AF_INET != sa->sin_family) {
        HAGGLE_DBG("IPv6 unsupported\n");
    }
    sa->sin_family = AF_INET;

    bool ret = false;

    ret = sendCtrl->openLocalSocket(local_addr, addrlen, false, false);
    sendCtrl->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create sendCtrl\n");
        return false;
    }

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getControlPortA());
    
    ret = sendData->openLocalSocket(local_addr, addrlen, false, false);
    sendData->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create sendData\n");
        return false;
    }

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getNonControlPortC());

    ret = sendNoCtrl->openLocalSocket(local_addr, addrlen, false, false);
    sendNoCtrl->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create sendNoCtrl\n");
        return false;
    }

    // NOTE: we have to have a separate receiver to connect to the 
    // broadcast address.
    addr = localIface->getAddress<IPv4BroadcastAddress>();

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getControlPortB());

    ret = recvData->openLocalSocket(local_addr, addrlen, true, false);
    recvData->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create recvData\n");
        return false;
    }

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getControlPortA());

    ret = recvCtrl->openLocalSocket(local_addr, addrlen, true, false);
    recvCtrl->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create recvCtrl\n");
        return false;
    }

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getNonControlPortC());

    ret = recvNoCtrl->openLocalSocket(local_addr, addrlen, true, false);
    recvNoCtrl->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create recvNoCtrl\n");
        return false;
    }

    return true;
}

void ProtocolUDPBroadcastProxy::removeCachedSnoopedReceiver(
    ProtocolUDPGeneric *p)
{
    if (!p) {
        HAGGLE_ERR("NULL protocol\n");
        return;
    }

    for (bcast_snooper_receiver_registry_t::iterator it = snooper_receiver_registry.begin(); 
         it != snooper_receiver_registry.end(); it++) {
        ProtocolUDPGeneric *i = (*it).second;
        if (i == p) {
            snooper_receiver_registry.erase(it);
            return;
        }
    }

    HAGGLE_ERR("Could not uncache (not in registry)\n");
}

void ProtocolUDPBroadcastProxy::removeCachedReceiver(
    ProtocolUDPGeneric *p)
{
    if (!p) {
        HAGGLE_ERR("NULL protocol\n");
        return;
    }

    for (bcast_receiver_registry_t::iterator it = receiver_registry.begin(); 
         it != receiver_registry.end(); it++) {
        ProtocolUDPGeneric *i = (*it).second;
        if (i == p) {
            receiver_registry.erase(it);
            return;
        }
    }

    HAGGLE_ERR("Could not uncache (not in registry)\n");
}

void ProtocolUDPBroadcastProxy::removeCachedSender(
    ProtocolUDPGeneric *p)
{
    if (!p) {
        HAGGLE_ERR("NULL protocol\n");
        return;
    }

    for (bcast_sender_registry_t::iterator it = sender_registry.begin(); 
         it != sender_registry.end(); it++) {
        ProtocolUDPGeneric *i = (*it).second;
        if (i == p) {
            sender_registry.erase(it);
            return;
        }
    }

    HAGGLE_ERR("Could not uncache (not in registry)\n");
}

ProtocolUDPBroadcastSender *ProtocolUDPBroadcastProxy::getCachedSender(
    const InterfaceRef peerIface,
    bool *o_nonFatalAbort)
{
    if (!peerIface) {
        HAGGLE_ERR("Null interface\n");
        return NULL;
    }

    if (!o_nonFatalAbort) {
        HAGGLE_ERR("NULL boolean pointer arg.\n");
        return NULL;
    }

    *o_nonFatalAbort = false;

    unsigned long remote_ip = ProtocolUDPGeneric::interfaceToIP(peerIface);

    if (0 == remote_ip) {
        HAGGLE_ERR("Could not get remote IP\n");
        return NULL;
    }

    bcast_sender_registry_t::iterator it = sender_registry.find(remote_ip);

    // MOS
    int activeProtocols = 0;
    while (it != sender_registry.end() && (*it).first == remote_ip) {
        ProtocolUDPBroadcastSender *cachedProto = (*it).second;
        if (NULL != cachedProto) {
	  if(!cachedProto->isGarbage() && !cachedProto->isDone()) return cachedProto;
	  activeProtocols++;
        }
        it++;
    }

    HAGGLE_DBG2("Number of active UDP broadcast sender protocols: %d (%d for this link)\n", sender_registry.size(), activeProtocols);

    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not instantiating new protocols\n");
      return NULL;
    }

    // MOS - without the following there is a danger of file descriptor overflow
    if(activeProtocols >= getConfiguration()->getMaxInstancesPerLink()) {
        HAGGLE_DBG("Not instantiating new sender protocol - %d matching protocols still running or shutting down\n", activeProtocols);
        *o_nonFatalAbort = true;
        return NULL;
    }
    if(sender_registry.size() >= getConfiguration()->getMaxInstances()) {
        HAGGLE_DBG("Not instantiating new sender protocol - %d protocols still running or shutting down\n", sender_registry.size());
        *o_nonFatalAbort = true;
        return NULL;
    }    

    ProtocolUDPBroadcastSender *p = NULL;

    if (getConfiguration()->getUseControlProtocol()) {
        p = new ProtocolUDPBroadcastSender(
            this,
            localIface,
            peerIface,
            getManager(),
            deleteSenderEventType,
            sendData);
    } 
    else {
        p = new ProtocolUDPBroadcastSenderNoControl(
            this,
            localIface,
            peerIface,
            getManager(),
            deleteSenderEventType,
            sendNoCtrl);
    }

    if (NULL == p) {
        HAGGLE_ERR("Could not construct protocol\n");
        return NULL;
    }

    p->setConfiguration(getConfiguration());

    if (!p->init()) {
        HAGGLE_ERR("Could not initialize protocol %s\n", p->getName());
        delete p;
        return NULL;
    }

    sender_registry.insert(make_pair(remote_ip, p));

    p->registerWithManager(); // MOS - do not use registerProtocol directly

    return p;
}

ProtocolUDPBroadcastSender *ProtocolUDPBroadcastProxy::getCachedSender(
    struct sockaddr *peer_addr,
    bool *o_nonFatalAbort) 
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("No peer interface\n");
        return NULL;
    }

    if (!o_nonFatalAbort) {
        HAGGLE_ERR("NULL boolean pointer arg.\n");
        return NULL;
    }

    struct sockaddr_in *sa = (struct sockaddr_in *)peer_addr;
    if (AF_INET != sa->sin_family) {
        if (AF_INET6 != sa->sin_family) {
            HAGGLE_ERR("Unknown family type\n");
            return NULL;
        }
        HAGGLE_DBG("IPv6 unsupported\n");
    }
    unsigned long remote_ip = sa->sin_addr.s_addr;
    if (0 == remote_ip) {
        HAGGLE_ERR("Remote IP is 0\n"); 
        return NULL;
    }
    sa->sin_family = AF_INET;

    InterfaceRef peerIface = resolvePeerInterface(peer_addr);

    if (!peerIface) {
        HAGGLE_ERR("Could not resolve peer interface\n");
        return NULL;
    }
   
    return getCachedSender(peerIface, o_nonFatalAbort);
}

ProtocolUDPBroadcastReceiver *ProtocolUDPBroadcastProxy::getCachedReceiver(
    const InterfaceRef peerIface,
    bool *o_nonFatalAbort)
{
    if (!peerIface) {
        HAGGLE_ERR("Null interface\n");
        return NULL;
    }

    if (!o_nonFatalAbort) {
        HAGGLE_ERR("NULL boolean pointer arg.\n");
        return NULL;
    }

    *o_nonFatalAbort = false;

    unsigned long remote_ip = ProtocolUDPGeneric::interfaceToIP(peerIface);

    if (0 == remote_ip) {
        HAGGLE_ERR("Could not get remote IP\n");
        return NULL;
    }

    bcast_receiver_registry_t::iterator it = receiver_registry.find(remote_ip);

    // MOS
    int activeProtocols = 0;
    while (it != receiver_registry.end() && (*it).first == remote_ip) {
        ProtocolUDPBroadcastReceiver *cachedProto = (*it).second;
        if (NULL != cachedProto) {
	  if(!cachedProto->isGarbage() && !cachedProto->isDone()) return cachedProto;
	  activeProtocols++;
        }
	it++;
    }

    HAGGLE_DBG2("Number of active UDP broadcast receiver protocols: %d (%d for this link)\n", receiver_registry.size(), activeProtocols);

    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not instantiating new protocols\n");
      return NULL;
    }

    // MOS - without the following there is a danger of file descriptor overflow
    if(activeProtocols >= getConfiguration()->getMaxReceiverInstancesPerLink()) {
        HAGGLE_DBG("Not instantiating new receiver protocol - %d matching protocols still running or shutting down\n", activeProtocols);
        *o_nonFatalAbort = true;
        return NULL;
    }
    if(receiver_registry.size() >= getConfiguration()->getMaxReceiverInstances()) {
        HAGGLE_DBG("Not instantiating new receiver protocol - %d protocols still running or shutting down\n", receiver_registry.size());
        *o_nonFatalAbort = true;
        return NULL;
    }    

    // create a new receiver
    ProtocolUDPBroadcastReceiver *p = NULL;
    if (getConfiguration()->getUseControlProtocol()) {
        p = new ProtocolUDPBroadcastReceiver(
	    this,
            localIface,
            peerIface,
            getManager(),
            deleteReceiverEventType,
            sendCtrl);
    }
    else {
       p = new ProtocolUDPBroadcastReceiverNoControl(
            this,
            localIface,
            peerIface,
            getManager(),
            deleteReceiverEventType,
            sendNoCtrl);
    }
    
    if (NULL == p) {
        HAGGLE_ERR("Could not construct protocol\n");
        return NULL;
    }

    p->setConfiguration(getConfiguration());

    // not in cache
    if (!p->init()) {
        HAGGLE_ERR("Could not initialize protocol %s\n", p->getName());
        delete p;
        return NULL;
    }

    receiver_registry.insert(make_pair(remote_ip, p));

    p->registerWithManager(); // MOS - do not use registerProtocol directly
    
    if (PROT_EVENT_SUCCESS != p->startTxRx()) {
        HAGGLE_ERR("Could not start receiver\n");
        return NULL;
    }

    return p;
}

ProtocolUDPBroadcastReceiver *ProtocolUDPBroadcastProxy::getCachedReceiver(
    struct sockaddr *peer_addr,
    bool *o_nonFatalAbort) 
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("No peer interface\n");
        return NULL;
    }

    if (!o_nonFatalAbort) {
        HAGGLE_ERR("NULL boolean pointer arg.\n");
        return NULL;
    }

    struct sockaddr_in *sa = (struct sockaddr_in *)peer_addr;
    if (AF_INET != sa->sin_family) {
        if (AF_INET6 != sa->sin_family) {
            HAGGLE_ERR("Unknown family type\n");
            return NULL;
        }
        HAGGLE_DBG("IPv6 unsupported\n");
    }
    unsigned long remote_ip = sa->sin_addr.s_addr;
    if (0 == remote_ip) {
        HAGGLE_ERR("Remote IP is 0\n"); 
        return NULL;
    }
    sa->sin_family = AF_INET;

    InterfaceRef peerIface = resolvePeerInterface(peer_addr);

    if (!peerIface) {
        HAGGLE_DBG("Could not resolve peer interface: this is non-fatal, we may be able to snoop node description.\n");
        return NULL;
    }

    return getCachedReceiver(peerIface, o_nonFatalAbort);
}

ProtocolUDPBroadcastPassiveReceiver *
ProtocolUDPBroadcastProxy::getCachedSnooperReceiver(
    const InterfaceRef peerIface,
    unsigned long src_ip,
    unsigned long dest_ip,
    bool *o_nonFatalAbort)
{
    if (!peerIface) {
        HAGGLE_ERR("Null interface\n");
        return NULL;
    }

    if (!o_nonFatalAbort) {
        HAGGLE_ERR("NULL boolean pointer arg.\n");
        return NULL;
    }

    *o_nonFatalAbort = false;

    Pair<unsigned long, unsigned long> ip_pair = make_pair(src_ip, dest_ip);
    bcast_snooper_receiver_registry_t::iterator it = snooper_receiver_registry.find(ip_pair);

    // MOS
    int activeProtocols = 0;
    while (it != snooper_receiver_registry.end() && (*it).first == ip_pair) {
        ProtocolUDPBroadcastPassiveReceiver *cachedProto = (*it).second;
        if (NULL != cachedProto) {
	  if(!cachedProto->isGarbage() && !cachedProto->isDone()) return cachedProto;
	  activeProtocols++;
        }
	it++;
    }

    HAGGLE_DBG2("Number of active UDP broadcast passive receiver protocols: %d (%d for this link)\n", snooper_receiver_registry.size(), activeProtocols);

    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not instantiating new protocols\n");
      return NULL;
    }

    // MOS - without the following there is a danger of file descriptor overflow
    if(activeProtocols >= getConfiguration()->getMaxPassiveReceiverInstancesPerLink()) {
        HAGGLE_DBG("Not instantiating new passive receiver protocol - %d matching protocols still running or shutting down\n", activeProtocols);
        *o_nonFatalAbort = true;
        return NULL;
    }
    if(snooper_receiver_registry.size() >= getConfiguration()->getMaxPassiveReceiverInstances()) {
        HAGGLE_DBG("Not instantiating new passive receiver protocol - %d protocols still running or shutting down\n", snooper_receiver_registry.size());
        *o_nonFatalAbort = true;
        return NULL;
    }    

    // create a new receiver
    ProtocolUDPBroadcastPassiveReceiver *p = new ProtocolUDPBroadcastPassiveReceiver(
            this,
            localIface,
            peerIface,
            getManager(),
            deleteSnoopedReceiverEventType);
    
    if (NULL == p) {
        HAGGLE_ERR("Could not construct protocol\n");
        return NULL;
    }

    p->setConfiguration(getConfiguration());

    // not in cache
    if (!p->init()) {
        HAGGLE_ERR("Could not initialize protocol %s\n", p->getName());
        delete p;
        return NULL;
    }

    snooper_receiver_registry.insert(make_pair(make_pair(src_ip, dest_ip), p));

    p->registerWithManager(); // MOS - do not use registerProtocol directly

    if (PROT_EVENT_SUCCESS != p->startTxRx()) {
        HAGGLE_ERR("Could not start snooper receiver\n");
        return NULL;
    }

    return p;
}

ProtocolUDPBroadcastPassiveReceiver *
ProtocolUDPBroadcastProxy::getCachedSnooperReceiver(
    struct sockaddr *peer_addr,
    unsigned long src_ip,
    unsigned long dest_ip,
    bool *o_nonFatalAbort) 
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("No peer interface\n");
        return NULL;
    }

    if (!o_nonFatalAbort) {
        HAGGLE_ERR("NULL boolean pointer arg.\n");
        return NULL;
    }

    struct sockaddr_in *sa = (struct sockaddr_in *)peer_addr;
    if (AF_INET != sa->sin_family) {
        if (AF_INET6 != sa->sin_family) {
            HAGGLE_ERR("Unknown family type\n");
            return NULL;
        }
        HAGGLE_DBG("IPv6 unsupported\n");
    }
    unsigned long remote_ip = sa->sin_addr.s_addr;
    if (0 == remote_ip) {
        HAGGLE_ERR("Remote IP is 0\n"); 
        return NULL;
    }
    sa->sin_family = AF_INET;

    InterfaceRef peerIface = resolvePeerInterface(peer_addr);
    if (!peerIface) {
        HAGGLE_DBG("Could not resolve peer interface: this is non-fatal, we may be able to snoop node description.\n");
        return NULL;
    }

    return getCachedSnooperReceiver(peerIface, src_ip, dest_ip, o_nonFatalAbort);
}

bool ProtocolUDPBroadcastProxy::proxyReceive(
    SocketWrapperUDP* from, 
    ProtocolUDPGeneric *p)
{
    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, ignoring received messages\n");
      return false;
    }

    if (NULL == from) {
        HAGGLE_ERR("Could not proxy to protocol\n");
        return false;
    }

    if (NULL == p) {
        HAGGLE_DBG("Discarding message from socket\n");
        from->clearMessage(); 
        return false;
    }

    // SW: NOTE: There is still a race condition here, where the protocol is not
    // yet marked as done, but will be when it comes time to splice...

    SocketWrapper *writeEndOfReceiveSocket = p->getWriteEndOfReceiveSocket();
    if (!SocketWrapper::spliceSockets(from, writeEndOfReceiveSocket)) {
        HAGGLE_ERR("Error splicing, protocol done: %s\n", p->isDone() ? "true" : "false");
        from->clearMessage(); 
        return false;
    }

    return true;
}

bool ProtocolUDPBroadcastProxy::isLocalAddress(unsigned long pip) 
{
    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *local_addr = (struct sockaddr *)buf;
    const IPv4Address *addr = localIface->getAddress<IPv4Address>();
    addr->fillInSockaddr((struct sockaddr_in *)local_addr, 0);
    unsigned long lip = ((struct sockaddr_in *)local_addr)->sin_addr.s_addr;

    if (lip == pip) {
        return true;
    }

    addr = localIface->getAddress<IPv4BroadcastAddress>();
    addr->fillInSockaddr((struct sockaddr_in *)local_addr, 0);
    lip = ((struct sockaddr_in *)local_addr)->sin_addr.s_addr;

    if (lip == pip) {
        return true;
    }
    
    return false;
}

bool ProtocolUDPBroadcastProxy::doSnoopInterface(
    SocketWrapperUDP* wrappedSocket, 
    sockaddr *o_peer_addr)
{
    HAGGLE_DBG("Attempting to snoop interface from message.\n");
    if (NULL == wrappedSocket || NULL == o_peer_addr || !localIface) {
        HAGGLE_ERR("Null args\n");
        return false;
    }

    // grab mac address
    unsigned char snoop_mac[6];
    bool snooped = wrappedSocket->doSnoopMacAddress(snoop_mac);
    if (!snooped) {
        HAGGLE_ERR("Could not snoop MAC address\n");
        return false;
    }
    
    char mac_str[BUFSIZ];
    if (0 >= sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", 
        snoop_mac[0],
        snoop_mac[1],
        snoop_mac[2],
        snoop_mac[3],
        snoop_mac[4],
        snoop_mac[5])) {
        HAGGLE_ERR("Could not construct mac string\n");
        return false;
    }

    snooped = wrappedSocket->doSnoopIPAddress(o_peer_addr);

    if (!snooped) {
        HAGGLE_ERR("Could not snoop IP address\n");
        return false;
    }

    return SocketWrapperUDP::doManualArpInsertion(
        getConfiguration()->getArpInsertionPathString().c_str(),
        localIface->getName(),
        inet_ntoa(((struct sockaddr_in *)o_peer_addr)->sin_addr),
        mac_str);
}

bool ProtocolUDPBroadcastProxy::getSrcIPDestIP(
    SocketWrapperUDP *wrappedSocket,
    unsigned long *src_ip, 
    unsigned long *dest_ip)
{
    if (NULL == wrappedSocket || NULL == src_ip || NULL == dest_ip) {
        HAGGLE_ERR("NULL args\n");
        return false;
    }

    char buf[sizeof(ProtocolUDPGeneric::udpmsg_t)];
    bzero(buf, sizeof(ProtocolUDPGeneric::udpmsg_t));
    int bytes_read = wrappedSocket->peek(buf, sizeof(ProtocolUDPGeneric::udpmsg_t));

    if (bytes_read != sizeof(ProtocolUDPGeneric::udpmsg_t)) {
        HAGGLE_ERR("Could not peek into socket for udp header\n");
        return false;
    }

    *src_ip = ProtocolUDPGeneric::getSrcIPFromMsg(buf);
    *dest_ip = ProtocolUDPGeneric::getDestIPFromMsg(buf);

    return true;
}

// this function is called to proxy receiver data
void ProtocolUDPBroadcastProxy::handleWatchableEvent(const Watchable &wbl)
{
    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, not communicating with peers\n");
        return;
    }

    if (!validate()) {
        HAGGLE_ERR("Broadcast in bad state\n");
        return;
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *peer_addr = (struct sockaddr *)buf;
    socklen_t peer_addr_len = sizeof(*peer_addr);

    if (recvData->hasWatchable(wbl)) {
        recvData->peek(peer_addr, &peer_addr_len);  
        unsigned long src_ip = 0;
        unsigned long dest_ip = 0;
        if (getSrcIPDestIP(recvData, &src_ip, &dest_ip)) {
            if (isLocalAddress(src_ip))  {
                recvData->clearMessage();
            }
            else if (isLocalAddress(dest_ip)) {
                bool nonFatalAbort = false;
                ProtocolUDPBroadcastReceiver *receiver = getCachedReceiver(peer_addr, &nonFatalAbort);
                if (nonFatalAbort) {

                }
                else if (!receiver && !doSnoopInterface(recvData, peer_addr)) {
                    HAGGLE_ERR("Problems snooping addresses\n");
                }
                else {
                    // SW: NOTE: this uses the replaced peer_addr from doSnoop
                    receiver = getCachedReceiver(peer_addr, &nonFatalAbort);
                }
                if (nonFatalAbort) {
                    recvData->clearMessage();
                }
                else if (!proxyReceive(recvData, receiver)) {
                    HAGGLE_ERR("Could not properly receive message.\n");
                    recvData->clearMessage();
                }
            }
            else {
                // SW: NOTE: this is the case where we snoop diff sender/receiver pair
                bool nonFatalAbort = false;
                ProtocolUDPBroadcastPassiveReceiver *receiver = 
                    getCachedSnooperReceiver(peer_addr, src_ip, dest_ip, &nonFatalAbort);
                if (nonFatalAbort) {

                }
                else if (!receiver && !doSnoopInterface(recvData, peer_addr)) {
                    HAGGLE_ERR("Problems snooping addresses\n");
                }
                else {
                    // SW: NOTE: this uses the replaced peer_addr from doSnoop
                    receiver = getCachedSnooperReceiver(peer_addr, src_ip, dest_ip, &nonFatalAbort);
                }
                if (nonFatalAbort) {
                    recvData->clearMessage();
                }
                else if (!proxyReceive(recvData, receiver)) {
                    HAGGLE_ERR("Could not properly receive (snooped) message.\n");
                    recvData->clearMessage();
                }
            }
        }
        else {
            HAGGLE_ERR("Could not get source and dest ip of packet\n");
        }
    }

    if (recvCtrl->hasWatchable(wbl)) {
        recvCtrl->peek(peer_addr, &peer_addr_len);  
        unsigned long src_ip = 0;
        unsigned long dest_ip = 0;
        if (getSrcIPDestIP(recvCtrl, &src_ip, &dest_ip)) {
            if (isLocalAddress(src_ip))  {
                recvCtrl->clearMessage();
            }
            else if (isLocalAddress(dest_ip)) {
                bool nonFatalAbort = false;
                ProtocolUDPBroadcastSender *sender = getCachedSender(peer_addr, &nonFatalAbort);
                if (nonFatalAbort) {
                    recvCtrl->clearMessage();
                }
                else if (!proxyReceive(recvCtrl, sender)) {
                    HAGGLE_ERR("Could not properly receive message.\n");
                    recvCtrl->clearMessage();
                }
            }
            else {
                /*
                // SW: NOTE: this is the case where snoop a control message from
                // another session. Right now we do not do anything, but future
                // optimizations could use this information to set blomofilters (for
                // example, due to an ACK or REJECT). 
                //HAGGLE_DBG2("Snooped another receiver's ctrl message: src ip: %d\n", src_ip);
                ProtocolUDPBroadcastPassiveReceiver *receiver = 
                    getCachedSnooperReceiver(peer_addr, src_ip, dest_ip);
                if (!proxyReceive(recvCtrl, receiver)) {
                    HAGGLE_ERR("Could not properly receive (snooped) message.\n");
                }
                */
                recvCtrl->clearMessage();
            }
        }
        else {
            HAGGLE_ERR("Could not get source and dest ip of packet\n");
        }
    }

    if (recvNoCtrl->hasWatchable(wbl)) {
        recvNoCtrl->peek(peer_addr, &peer_addr_len);  
        unsigned long src_ip = 0;
        unsigned long dest_ip = 0;
        if (getSrcIPDestIP(recvNoCtrl, &src_ip, &dest_ip)) {
            if (isLocalAddress(src_ip))  {
                recvNoCtrl->clearMessage();
            }
            else {
                bool nonFatalAbort = false;
                ProtocolUDPBroadcastReceiver *receiver = getCachedReceiver(peer_addr, &nonFatalAbort);
                if (nonFatalAbort) {

                }
                else if (!receiver && !doSnoopInterface(recvNoCtrl, peer_addr)) {
                    HAGGLE_ERR("Problems snooping addresses\n");
                }
                else {
                    // SW: NOTE: this uses the replaced peer_addr from doSnoop
                    receiver = getCachedReceiver(peer_addr, &nonFatalAbort);
                }
                if (nonFatalAbort) {
                    recvData->clearMessage();
                }
                else if (!proxyReceive(recvNoCtrl, receiver)) {
                    HAGGLE_ERR("Could not properly receive message.\n");
                    recvNoCtrl->clearMessage();
                }
            }
        }
        else {
            HAGGLE_ERR("Could not get source and dest ip of packet\n");
        }
    }
}

bool ProtocolUDPBroadcastProxy::hasWatchable(const Watchable &wbl) 
{
    if (!validate()) {
        HAGGLE_ERR("Broadcast in bad state\n");
        return false;
    }

    if (recvData->hasWatchable(wbl)) {
        return true;
    }

    if (recvCtrl->hasWatchable(wbl)) {
        return true;
    }

    if (sendNoCtrl->hasWatchable(wbl)) {
        return true;
    }

    if (recvNoCtrl->hasWatchable(wbl)) {
        return true;
    }

    return false;
}

IPv4Address *ProtocolUDPBroadcastProxy::sockaddrToIPv4Address(
    struct sockaddr *peer_addr)
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("NULL peer_addr\n");
        return NULL;
    }

    struct sockaddr_in *saddr_in = (struct sockaddr_in *)peer_addr;
    // unsigned short port = ntohs(saddr_in->sin_port); // MOS - too specific
    // return new IPv4Address(*saddr_in, TransportUDP(port)); // MOS - too specific
    return new IPv4Address(*saddr_in, TransportNone());
}

InterfaceRef  
ProtocolUDPBroadcastProxy::resolvePeerInterface(struct sockaddr *peer_addr)
{
    IPv4Address *ipv4Address = sockaddrToIPv4Address(peer_addr);
    if (NULL == ipv4Address) {
        HAGGLE_ERR("Could not get ipv4 address\n");
        return NULL;
    }

    InterfaceRef pIface 
        = getKernel()->getInterfaceStore()->retrieve(*ipv4Address);

    if (pIface) {
        HAGGLE_DBG("Peer interface for peer %s is [%s]\n", ipv4Address->getStr(), pIface->getIdentifierStr());
        delete ipv4Address;
        return pIface;
    } 

    HAGGLE_DBG("No interface found for peer %s\n", ipv4Address->getStr());

    unsigned char mac[6];
    const char *ifname = localIface->getName();
    int res = get_peer_mac_address(peer_addr, ifname, mac, 6);

    if (0 > res) {
        HAGGLE_ERR("Could not get peer mac address, error: %d\n", res);
        delete ipv4Address;
        return pIface;
    } 

    SocketWrapperUDP *sendSocket = getConfiguration()->getUseControlProtocol() ? sendData : sendNoCtrl;

    if (0 == res) {
        HAGGLE_DBG("No mac address for peer %s\n", ipv4Address->getStr());
        if (sendSocket->getUseArpHack()) {
            HAGGLE_DBG("Sending ARP request for peer: %s\n", ipv4Address->getStr());
            sendSocket->sendArpRequest(peer_addr);
            res = get_peer_mac_address(peer_addr, ifname, mac, 6);
        }

        if (res <= 0) {
            HAGGLE_DBG("ARP cache entry was missing, this is non-fatal and can occur when a node receives a ND before a hello.\n");
            delete ipv4Address;
            return pIface;
        }
    }

    EthernetAddress eth_addr(mac);

    InterfaceRef newIface = Interface::create<EthernetInterface>(mac, 
                "UDP broadcast peer",
                eth_addr, 
                IFFLAG_UP);

    if (newIface) {
        newIface->addAddress(eth_addr);
        newIface->addAddress(*ipv4Address);
        HAGGLE_DBG("Peer interface is [%s]\n", newIface->getIdentifierStr());
        if (0 == strcmp(newIface->getIdentifierStr(), "00:00:00:00:00:00")) {
            HAGGLE_ERR("Resolved peer interface has illegal mac address: %s\n", newIface->getIdentifierStr());
        } else {
            pIface = newIface;
        }
    }

    delete ipv4Address;
    return pIface;
}

Protocol *ProtocolUDPBroadcastProxy::getSenderProtocol(
    const InterfaceRef peerIface)
{
    bool bogusNonFatal = false;
    return getCachedSender(peerIface, &bogusNonFatal);    
}

/*
 * Proxy wrapper to interface with ProtocolManager.
 */
ProtocolUDPBroadcastProxyServer::ProtocolUDPBroadcastProxyServer(
        const InterfaceRef& _localIface,
        ProtocolManager *_m) :
            Protocol(
                Protocol::TYPE_UDP_BROADCAST,
                "ProtocolUDPBroadcastProxy",
                _localIface,
                NULL,
                PROT_FLAG_SERVER,
                _m) 
{
    proxy = new ProtocolUDPBroadcastProxy(getKernel(), getManager(), localIface);    
}


ProtocolUDPBroadcastProxyServer::~ProtocolUDPBroadcastProxyServer()
{
    if (proxy) {
        delete proxy;
    }
}

bool ProtocolUDPBroadcastProxyServer::init()
{
    if (NULL == proxy) {
        return false;
    }

    return proxy->init();
}

void ProtocolUDPBroadcastProxyServer::handleWatchableEvent(const Watchable &wbl)
{
    if (NULL == proxy) {
        HAGGLE_ERR("Proxy is not set\n");
        return;
    }

    proxy->handleWatchableEvent(wbl);
}

bool ProtocolUDPBroadcastProxyServer::hasWatchable(const Watchable &wbl)
{
    if (NULL == proxy) {
        HAGGLE_ERR("Proxy is not set\n");
        return false;
    }

    return proxy->hasWatchable(wbl);
}

Protocol *ProtocolUDPBroadcastProxyServer::getSenderProtocol(
    const InterfaceRef peerIface)
{
    if (NULL == proxy) {
        HAGGLE_ERR("Proxy is not set\n");
        return NULL;
    }

    return proxy->getSenderProtocol(peerIface);
}

ProtocolConfigurationUDPBroadcast *
ProtocolUDPBroadcastProxyServer::getUDPConfiguration() 
{
    ProtocolConfiguration *genCfg = getConfiguration();
    // SW: down cast to proper subclass, this looks dangerous but OK since
    // we overrode the other setters and getters
    return static_cast<ProtocolConfigurationUDPBroadcast *>(genCfg);
}

ProtocolConfiguration *ProtocolUDPBroadcastProxyServer::getConfiguration() 
{ 
    if (NULL == cfg) {
        cfg = new ProtocolConfigurationUDPBroadcast();
    }
    return cfg;
}

void ProtocolUDPBroadcastProxyServer::setConfiguration(
    ProtocolConfiguration *_cfg)
{
    if (NULL == _cfg) {
        HAGGLE_ERR("Cannot set NULL configuration\n");
        return;
    }

    if (Protocol::TYPE_UDP_BROADCAST != _cfg->getType()) {
        HAGGLE_ERR("Setting wrong configuration type\n");
        return;
    }

    if (cfg) {
        delete cfg;
    }

    cfg = _cfg->clone();

    proxy->setConfiguration(static_cast<ProtocolConfigurationUDPBroadcast*>(cfg->clone()));
}
