/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLUDP_UCAST_PROXY_H
#define _PROTOCOLUDP_UCAST_PROXY_H

class ProtocolUDPUnicastProxy;

#define UDP_UCAST_PROXY_PORT_A 8788
#define UDP_UCAST_PROXY_PORT_B 8789
#define UDP_UCAST_PROXY_PORT_C 8790

#include "Manager.h"
#include "SocketWrapper.h"
#include "SocketWrapperUDP.h"
#include "ProtocolUDPUnicast.h"
#include "ProtocolConfigurationUDPGeneric.h"

// MOS - use BasicMap instead of Map to allow multiple entries per ip
typedef BasicMap<unsigned long, ProtocolUDPUnicastSender *> ucast_sender_registry_t;
typedef BasicMap<unsigned long, ProtocolUDPUnicastReceiver *> ucast_receiver_registry_t;

class ProtocolUDPUnicastProxy {
private:
    HaggleKernel *k;
    ProtocolManager *m;
    const InterfaceRef& localIface;
    EventType deleteSenderEventType;
    EventType deleteReceiverEventType;

    HaggleKernel *getKernel() { return k; };

    ProtocolManager *getManager() { return m; };

public:
    void removeCachedSender(ProtocolUDPGeneric *p);

    void removeCachedReceiver(ProtocolUDPGeneric *p);

protected:
    SocketWrapperUDP* sendCtrlRcvData;
    SocketWrapperUDP* sendDataRcvCtrl;
    SocketWrapperUDP* sendRecvNoCtrl;

    ucast_receiver_registry_t receiver_registry;
    ucast_sender_registry_t sender_registry;

    ProtocolConfigurationUDPUnicast *cfg;

    ProtocolUDPUnicastReceiver *getCachedReceiver(const InterfaceRef peerIface);

    ProtocolUDPUnicastSender *getCachedSender(const InterfaceRef peerIface);

    ProtocolUDPUnicastReceiver *getCachedReceiver(struct sockaddr *peer_addr); 

    ProtocolUDPUnicastSender *getCachedSender(struct sockaddr *peer_addr); 

    IPv4Address *sockaddrToIPv4Address(struct sockaddr *peer_addr);

    InterfaceRef resolvePeerInterface(struct sockaddr *peer_addr);

    bool proxyReceive(SocketWrapperUDP* from, ProtocolUDPGeneric *p);

    bool isLocalAddress(struct sockaddr *peer_addr);

    bool doSnoopInterface(
        SocketWrapperUDP *wrappedSocket, 
        sockaddr *o_peer_addr);
public:

    ProtocolUDPUnicastProxy(
        HaggleKernel *kernel, 
        ProtocolManager *manager, 
        const InterfaceRef& localIface);

    ~ProtocolUDPUnicastProxy();

    void handleWatchableEvent(const Watchable &wbl);

    bool hasWatchable(const Watchable &wbl);

    Protocol *getSenderProtocol(const InterfaceRef peerIface);

    void setConfiguration(ProtocolConfigurationUDPUnicast *_cfg);

    ProtocolConfigurationUDPUnicast *getConfiguration();

    bool init();

};

class ProtocolUDPUnicastProxyServer : public Protocol {
private:
    ProtocolUDPUnicastProxy *proxy;
public:
    ProtocolUDPUnicastProxyServer(
        const InterfaceRef& _localIface,
        ProtocolManager *_m);

    ~ProtocolUDPUnicastProxyServer();

    void handleWatchableEvent(const Watchable &wbl);

    bool hasWatchable(const Watchable &wbl);

    Protocol *getSenderProtocol(const InterfaceRef peerIface);

    ProtocolConfigurationUDPUnicast *getUDPConfiguration();

    ProtocolConfiguration *getConfiguration();

    void setConfiguration(ProtocolConfiguration *_cfg);

    bool init();
};

#endif /* _PROTOCOLUDP_UCAST_PROXY_H */
