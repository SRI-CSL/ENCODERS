/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include <string.h>
#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "ProtocolUDPUnicastProxy.h"
#include "ProtocolManager.h"
#include "Interface.h"

#define SOCKADDR_SIZE sizeof(struct sockaddr_in)

ProtocolUDPUnicastProxy::ProtocolUDPUnicastProxy(
    HaggleKernel *_k,
    ProtocolManager *_m,
    const InterfaceRef& _localIface) :
        k(_k), 
        m(_m), 
        localIface(_localIface),
        deleteSenderEventType(-1),
        deleteReceiverEventType(-1),
        cfg(NULL)
{

    if (!localIface) {
        HAGGLE_ERR("Could not initialize UPD proxy without local interface\n");
        return;
    }

    sendCtrlRcvData = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    sendDataRcvCtrl = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

    sendRecvNoCtrl = new SocketWrapperUDP(
                            getKernel(), 
                            getManager(),
                            string(localIface->getName()));

}

ProtocolUDPUnicastProxy::~ProtocolUDPUnicastProxy() 
{
    if (sendCtrlRcvData) {
        delete sendCtrlRcvData;
    }
    
    if (sendDataRcvCtrl) {
        delete sendDataRcvCtrl;
    }

    if (sendRecvNoCtrl) {
        delete sendRecvNoCtrl;
    }

    if (deleteSenderEventType > 0) {
        Event::unregisterType(deleteSenderEventType);
    }
    
    if (deleteReceiverEventType > 0) {
        Event::unregisterType(deleteReceiverEventType);
    }

    if (cfg) {
        delete cfg;
    }
}

void ProtocolUDPUnicastProxy::setConfiguration(
    ProtocolConfigurationUDPUnicast *_cfg)
{
    if (NULL != cfg) {
        delete cfg;
    }
    cfg = _cfg; 
}

ProtocolConfigurationUDPUnicast *ProtocolUDPUnicastProxy::getConfiguration()
{
    if (NULL == cfg) {
        cfg = new ProtocolConfigurationUDPUnicast();
    }
    return cfg;
}

bool ProtocolUDPUnicastProxy::init() 
{
#define __CLASS__ ProtocolUDPUnicastProxy

    if (NULL == sendCtrlRcvData || NULL == sendDataRcvCtrl || NULL == sendRecvNoCtrl) {
        HAGGLE_ERR("In bad state.\n");
        return false;
    } 

    sendCtrlRcvData->setUseArpHack(getConfiguration()->getUseArpHack()); 
    sendDataRcvCtrl->setUseArpHack(getConfiguration()->getUseArpHack()); 
    sendRecvNoCtrl->setUseArpHack(getConfiguration()->getUseArpHack()); 

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

    ret = sendCtrlRcvData->openLocalSocket(local_addr, addrlen, true, false);
    sendCtrlRcvData->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create sendCtrlRcvData\n");
        return false;
    }

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getControlPortA());
    
    ret = sendDataRcvCtrl->openLocalSocket(local_addr, addrlen, true, false);
    sendDataRcvCtrl->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create sendDataRcvCtrl\n");
        return false;
    }

    addr->fillInSockaddr((struct sockaddr_in *)local_addr, getConfiguration()->getNonControlPortC());
    
    ret = sendRecvNoCtrl->openLocalSocket(local_addr, addrlen, true, false);
    sendRecvNoCtrl->setConnected(true);

    if (!ret) {
        HAGGLE_ERR("Could not create sendRecvNoCtrl\n");
        return false;
    }
    
    return true;
}

void ProtocolUDPUnicastProxy::removeCachedReceiver(
    ProtocolUDPGeneric *p)
{    
    if (!p) {
        HAGGLE_ERR("NULL protocol\n");
        return;
    }

    for (ucast_receiver_registry_t::iterator it = receiver_registry.begin(); 
         it != receiver_registry.end(); it++) {
        ProtocolUDPGeneric *i = (*it).second;
        if (i == p) {
            receiver_registry.erase(it);
            return;
        }
    }

    HAGGLE_ERR("Could not uncache (not in registry)\n");
}

void ProtocolUDPUnicastProxy::removeCachedSender(
    ProtocolUDPGeneric *p)
{
  if (!p) {
        HAGGLE_ERR("NULL protocol\n");
        return;
    }

    for (ucast_sender_registry_t::iterator it = sender_registry.begin(); 
         it != sender_registry.end(); it++) {
        ProtocolUDPGeneric *i = (*it).second;
        if (i == p) {
            sender_registry.erase(it);
            return;
        }
    }

    HAGGLE_ERR("Could not uncache (not in registry)\n");
}

ProtocolUDPUnicastReceiver *ProtocolUDPUnicastProxy::getCachedReceiver(
    const InterfaceRef peerIface)
{
    if (!peerIface) {
        HAGGLE_ERR("Null interface\n");
        return NULL;
    }

    unsigned long remote_ip = ProtocolUDPGeneric::interfaceToIP(peerIface);

    if (0 == remote_ip) {
        HAGGLE_ERR("Could not get remote IP\n");
        return NULL;
    }

    ucast_receiver_registry_t::iterator it = receiver_registry.find(remote_ip);

    // MOS
    int activeProtocols = 0;
    while (it != receiver_registry.end() && (*it).first == remote_ip) {
        ProtocolUDPUnicastReceiver *cachedProto = (*it).second;
        if (NULL != cachedProto) {
	  if(!cachedProto->isGarbage() && !cachedProto->isDone()) return cachedProto;
	  activeProtocols++;
        }
	it++;
    }

    HAGGLE_DBG2("Number of active UDP unicast receiver protocols: %d (%d for this link)\n", receiver_registry.size(), activeProtocols);

    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not instantiating new protocols\n");
      return NULL;
    }

    // MOS - without the following there is a danger of file descriptor overflow
    if(activeProtocols >= getConfiguration()->getMaxReceiverInstancesPerLink()) {
        HAGGLE_DBG("Not instantiating new receiver protocol - %d matching protocols still running or shutting down\n", activeProtocols);
        return NULL;
    }
    if(receiver_registry.size() >= getConfiguration()->getMaxReceiverInstances()) {
        HAGGLE_DBG("Not instantiating new receiver protocol - %d protocols still running or shutting down\n", receiver_registry.size());
        return NULL;
    }    

    // create a new receiver
    ProtocolUDPUnicastReceiver *p = NULL;
    if (getConfiguration()->getUseControlProtocol()) {
       p = new ProtocolUDPUnicastReceiver(this,
            localIface,
            peerIface,
            getManager(),
            deleteReceiverEventType,
            sendCtrlRcvData);
    }
    else {
       p = new ProtocolUDPUnicastReceiverNoControl(this,
            localIface,
            peerIface,
            getManager(),
            deleteReceiverEventType,
            sendRecvNoCtrl);
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

ProtocolUDPUnicastSender *ProtocolUDPUnicastProxy::getCachedSender(
    const InterfaceRef peerIface)
{
    if (!peerIface) {
        HAGGLE_ERR("Null interface\n");
        return NULL;
    }

    unsigned long remote_ip = ProtocolUDPGeneric::interfaceToIP(peerIface);

    if (0 == remote_ip) {
        HAGGLE_ERR("Could not get remote IP\n");
        return NULL;
    }

    ucast_sender_registry_t::iterator it = sender_registry.find(remote_ip);

    // MOS
    int activeProtocols = 0;
    while (it != sender_registry.end() && (*it).first == remote_ip) {
        ProtocolUDPUnicastSender *cachedProto = (*it).second;
        if (NULL != cachedProto) {
	  if(!cachedProto->isGarbage() && !cachedProto->isDone()) return cachedProto;
	  activeProtocols++;
        }
	it++;
    }

    HAGGLE_DBG2("Number of active UDP unicast sender protocols: %d (%d for this link)\n", sender_registry.size(), activeProtocols);

    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
      HAGGLE_DBG("In shutdown, not instantiating new protocols\n");
      return NULL;
    }

    // MOS - without the following there is a danger of file descriptor overflow
    if(activeProtocols >= getConfiguration()->getMaxInstancesPerLink()) {
        HAGGLE_DBG("Not instantiating new sender protocol - %d matching protocols still running or shutting down\n", activeProtocols);
        return NULL;
    }
    if(sender_registry.size() >= getConfiguration()->getMaxInstances()) {
        HAGGLE_DBG("Not instantiating new sender protocol - %d protocols still running or shutting down\n", sender_registry.size());
        return NULL;
    }    

    ProtocolUDPUnicastSender *p = NULL;

    if (getConfiguration()->getUseControlProtocol()) {
       p = new ProtocolUDPUnicastSender(this,
            localIface,
            peerIface,
            getManager(),
            deleteSenderEventType,
            sendDataRcvCtrl);
    } 
    else {
       p = new ProtocolUDPUnicastSenderNoControl(this,
            localIface,
            peerIface,
            getManager(),
            deleteSenderEventType,
            sendRecvNoCtrl);
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

ProtocolUDPUnicastReceiver *ProtocolUDPUnicastProxy::getCachedReceiver(
    struct sockaddr *peer_addr) 
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("No peer interface\n");
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

    return getCachedReceiver(peerIface);
}

ProtocolUDPUnicastSender *ProtocolUDPUnicastProxy::getCachedSender(
    struct sockaddr *peer_addr) 
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("No peer interface\n");
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
   
    return getCachedSender(peerIface);
}

bool ProtocolUDPUnicastProxy::proxyReceive(
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

bool ProtocolUDPUnicastProxy::isLocalAddress(struct sockaddr *peer_addr) 
{
    if (NULL == peer_addr) {
        HAGGLE_ERR("Peer ip is NULL\n");
        return false;
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *local_addr = (struct sockaddr *)buf;
    const IPv4Address *addr = localIface->getAddress<IPv4Address>();
    addr->fillInSockaddr((struct sockaddr_in *)local_addr, 0);
    unsigned long lip = ((struct sockaddr_in *)local_addr)->sin_addr.s_addr;
    unsigned long pip = ((struct sockaddr_in *)peer_addr)->sin_addr.s_addr;
    return lip == pip;
}

bool ProtocolUDPUnicastProxy::doSnoopInterface(
    SocketWrapperUDP *wrappedSocket, 
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

void ProtocolUDPUnicastProxy::handleWatchableEvent(const Watchable &wbl)
{
    if (getManager()->getState() > Manager::MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, not communicating with peers\n");
        return;
    }

    // watchable only occurs on receive
    if (NULL == sendCtrlRcvData || NULL == sendDataRcvCtrl || NULL == sendRecvNoCtrl) {
        HAGGLE_ERR("In bad state.\n");
        return;
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *peer_addr = (struct sockaddr *)buf;
    socklen_t peer_addr_len = sizeof(*peer_addr);

    if (sendCtrlRcvData->hasWatchable(wbl)) {
        sendCtrlRcvData->peek(peer_addr, &peer_addr_len);  
        if (isLocalAddress(peer_addr)) {
            sendCtrlRcvData->clearMessage();
        }
        else {
            ProtocolUDPUnicastReceiver *receiver = getCachedReceiver(peer_addr);
            if (!receiver && !doSnoopInterface(sendCtrlRcvData, peer_addr)) {
                HAGGLE_ERR("Problems snooping addresses\n");
            }
            else {
                // SW: NOTE: this uses the replaced peer_addr from doSnoop
                receiver = getCachedReceiver(peer_addr);
            }
            if (!proxyReceive(sendCtrlRcvData, receiver)) {
                HAGGLE_ERR("Could not properly receive message.\n");
                sendCtrlRcvData->clearMessage();
            }
        }
    }

    if (sendDataRcvCtrl->hasWatchable(wbl)) {
        sendDataRcvCtrl->peek(peer_addr, &peer_addr_len);  
        if (isLocalAddress(peer_addr)) {
            sendDataRcvCtrl->clearMessage();
        }
        else {
            ProtocolUDPUnicastSender *sender = getCachedSender(peer_addr);
            // NOTE: we don't bother trying to snoop here b/c receivers
            // don't send data objects
            if (!proxyReceive(sendDataRcvCtrl, sender)) {
                HAGGLE_ERR("Could not properly receive message.\n");
                sendDataRcvCtrl->clearMessage();
            }
        }
    }

    if (sendRecvNoCtrl->hasWatchable(wbl)) {
        sendRecvNoCtrl->peek(peer_addr, &peer_addr_len);  
        if (isLocalAddress(peer_addr)) {
            sendRecvNoCtrl->clearMessage();
        }
        else {
            ProtocolUDPUnicastReceiver *receiver = getCachedReceiver(peer_addr);
            if (!receiver && !doSnoopInterface(sendRecvNoCtrl, peer_addr)) {
                HAGGLE_ERR("Problems snooping addresses\n");
            }
            else {
                // SW: NOTE: this uses the replaced peer_addr from doSnoop
                receiver = getCachedReceiver(peer_addr);
            }
            if (!proxyReceive(sendRecvNoCtrl, receiver)) {
                HAGGLE_ERR("Could not properly receive message.\n");
                sendRecvNoCtrl->clearMessage();
            }
        }
    }
}

bool ProtocolUDPUnicastProxy::hasWatchable(const Watchable &wbl) 
{
    if (NULL == sendCtrlRcvData || NULL == sendDataRcvCtrl || NULL == sendRecvNoCtrl) {
        HAGGLE_ERR("In bad state.\n");
        return false;
    }

    if (sendCtrlRcvData->hasWatchable(wbl)) {
        return true;
    }    

    if (sendDataRcvCtrl->hasWatchable(wbl)) {
        return true;
    }

    if (sendRecvNoCtrl->hasWatchable(wbl)) {
        return true;
    }

    return false;
}

Protocol *ProtocolUDPUnicastProxy::getSenderProtocol(
    const InterfaceRef peerIface)
{
    return getCachedSender(peerIface);    
}

IPv4Address *ProtocolUDPUnicastProxy::sockaddrToIPv4Address(
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
ProtocolUDPUnicastProxy::resolvePeerInterface(struct sockaddr *peer_addr)
{
   
    IPv4Address *ipv4Address = sockaddrToIPv4Address(peer_addr);
    if (NULL == ipv4Address) {
        HAGGLE_ERR("Could not get ipv4 address\n");
        return NULL;
    }


    InterfaceRef pIface 
        = getKernel()->getInterfaceStore()->retrieve(*ipv4Address);


    if (pIface) {
        HAGGLE_DBG("Peer interface is [%s]\n", pIface->getIdentifierStr());
        delete ipv4Address;
        return pIface;
    } 

    unsigned char mac[6];
    const char *ifname = localIface->getName();
    int res = get_peer_mac_address(peer_addr, ifname, mac, 6);

    if (0 > res) {
        HAGGLE_ERR("Could not get peer mac address, error: %d\n", res);
        delete ipv4Address;
        return pIface;
    } 

    SocketWrapperUDP *sendSocket = getConfiguration()->getUseControlProtocol() ? sendRecvNoCtrl : sendDataRcvCtrl;

    if (!sendSocket) {
        HAGGLE_ERR("Could not get send socket\n");
        delete ipv4Address;
        return pIface;
    }
    
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
                "UDP unicast peer",
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


/*
 * Proxy wrapper to interface with ProtocolManager.
 */

ProtocolUDPUnicastProxyServer::ProtocolUDPUnicastProxyServer(
        const InterfaceRef& _localIface,
        ProtocolManager *_m) :
            Protocol(
                Protocol::TYPE_UDP_UNICAST,
                "ProtocolUDPUnicastProxy",
                _localIface,
                NULL,
                PROT_FLAG_SERVER,
                _m) 
{
    proxy = new ProtocolUDPUnicastProxy(getKernel(), getManager(), localIface);    
}


ProtocolUDPUnicastProxyServer::~ProtocolUDPUnicastProxyServer()
{
    if (proxy) {
        delete proxy;
    }
}

bool ProtocolUDPUnicastProxyServer::init()
{
    if (NULL == proxy) {
        return false;
    }

    return proxy->init();
}

void ProtocolUDPUnicastProxyServer::handleWatchableEvent(const Watchable &wbl)
{
    if (NULL == proxy) {
        HAGGLE_ERR("Proxy is not set\n");
        return;
    }

    proxy->handleWatchableEvent(wbl);
}

bool ProtocolUDPUnicastProxyServer::hasWatchable(const Watchable &wbl)
{
    if (NULL == proxy) {
        HAGGLE_ERR("Proxy is not set\n");
        return false;
    }

    return proxy->hasWatchable(wbl);
}

Protocol *ProtocolUDPUnicastProxyServer::getSenderProtocol(
    const InterfaceRef peerIface) 
{
    if (NULL == proxy) {
        HAGGLE_ERR("Proxy is not set\n");
        return NULL;
    }

    return proxy->getSenderProtocol(peerIface);
}

ProtocolConfigurationUDPUnicast *ProtocolUDPUnicastProxyServer::getUDPConfiguration() 
{
    ProtocolConfiguration *genCfg = getConfiguration();
    // SW: down cast to proper subclass, this looks dangerous but OK since
    // we overrode the other setters and getters
    return static_cast<ProtocolConfigurationUDPUnicast *>(genCfg);
}

ProtocolConfiguration *ProtocolUDPUnicastProxyServer::getConfiguration() 
{ 
    if (NULL == cfg) {
        cfg = new ProtocolConfigurationUDPUnicast();
    }
    return cfg;
}

void ProtocolUDPUnicastProxyServer::setConfiguration(ProtocolConfiguration *_cfg)
{
    if (NULL == _cfg) {
        HAGGLE_ERR("Cannot set NULL configuration\n");
        return;
    }

    if (Protocol::TYPE_UDP_UNICAST != _cfg->getType()) {
        HAGGLE_ERR("Setting wrong configuration type\n");
        return;
    }

    ProtocolConfiguration *oldCfg = cfg;
    cfg = _cfg->clone();
    if (NULL != oldCfg) {
        delete oldCfg;
    }

    proxy->setConfiguration(static_cast<ProtocolConfigurationUDPUnicast*>(cfg->clone()));
}
