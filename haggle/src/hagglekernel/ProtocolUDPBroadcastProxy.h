/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLUDP_BCAST_PROXY_H
#define _PROTOCOLUDP_BCAST_PROXY_H

class ProtocolUDPBroadcastProxy;

#define UDP_BCAST_PROXY_PORT_A 8791
#define UDP_BCAST_PROXY_PORT_B 8792
#define UDP_BCAST_PROXY_PORT_C 8793

#define UDP_PROXY_MIN_BCAST_PERIOD_MS 400

#include "Manager.h"
#include "SocketWrapper.h"
#include "SocketWrapperUDP.h"
#include "ProtocolUDPBroadcast.h"
#include "ProtocolConfigurationUDPGeneric.h"

// MOS - use BasicMap instead of Map to allow multiple entries per ip
typedef BasicMap< unsigned long, ProtocolUDPBroadcastSender *> bcast_sender_registry_t;
typedef BasicMap< unsigned long, ProtocolUDPBroadcastReceiver *> bcast_receiver_registry_t;
typedef BasicMap< Pair<unsigned long, unsigned long>, ProtocolUDPBroadcastPassiveReceiver *> bcast_snooper_receiver_registry_t;

class ProtocolUDPBroadcastProxy {
private:
    HaggleKernel *k;
    ProtocolManager *m;
    const InterfaceRef& localIface;
    EventType deleteSenderEventType;
    EventType deleteReceiverEventType;
    EventType deleteSnoopedReceiverEventType;

    HaggleKernel *getKernel() { return k; };

    ProtocolManager *getManager() { return m; };

    bool getSrcIPDestIP(
        SocketWrapperUDP *wrappedSocket,
        unsigned long *src_ip, 
        unsigned long *dest_ip);

    bool validate();

public:

    void removeCachedSnoopedReceiver(ProtocolUDPGeneric *p);

    void removeCachedSender(ProtocolUDPGeneric *p);

    void removeCachedReceiver(ProtocolUDPGeneric *p);
 
protected:
    SocketWrapperUDP* sendCtrl;
    SocketWrapperUDP* sendData;
    SocketWrapperUDP* recvCtrl;
    SocketWrapperUDP* recvData;
    SocketWrapperUDP* sendNoCtrl;
    SocketWrapperUDP* recvNoCtrl;

    bcast_receiver_registry_t receiver_registry;
    bcast_sender_registry_t sender_registry;
    bcast_snooper_receiver_registry_t snooper_receiver_registry;

    ProtocolConfigurationUDPBroadcast *cfg;

    ProtocolUDPBroadcastReceiver *getCachedReceiver(const InterfaceRef peerIface, bool *o_nonFatalAbort);

    ProtocolUDPBroadcastSender *getCachedSender(const InterfaceRef peerIface, bool *o_nonFatalAbort);

    ProtocolUDPBroadcastReceiver *getCachedReceiver(struct sockaddr *peer_addr, bool *o_nonFatalAbort);

    ProtocolUDPBroadcastSender *getCachedSender(struct sockaddr *peer_addr, bool *o_nonFatalAbort);

    ProtocolUDPBroadcastPassiveReceiver *getCachedSnooperReceiver(
        const InterfaceRef peerIface,
        unsigned long src_ip,
        unsigned long dest_ip,
        bool *o_nonFatalAbort);

    ProtocolUDPBroadcastPassiveReceiver *getCachedSnooperReceiver(
        struct sockaddr *peer_addr,
        unsigned long src_ip,
        unsigned long dest_ip,
        bool *o_nonFatalAbort);

    IPv4Address *sockaddrToIPv4Address(struct sockaddr *peer_addr);

    InterfaceRef resolvePeerInterface(struct sockaddr *peer_addr);

    bool proxyReceive(SocketWrapperUDP* from, ProtocolUDPGeneric *p);

    bool isLocalAddress(unsigned long pip);
public:

    ProtocolUDPBroadcastProxy(
        HaggleKernel *kernel, 
        ProtocolManager *manager, 
        const InterfaceRef& localIface);

    ~ProtocolUDPBroadcastProxy();

    void handleWatchableEvent(const Watchable &wbl);

    bool hasWatchable(const Watchable &wbl);

    Protocol *getSenderProtocol(const InterfaceRef peerIface);

    void setConfiguration(ProtocolConfigurationUDPBroadcast *_cfg);

    ProtocolConfigurationUDPBroadcast *getConfiguration();

    bool init();

    bool doSnoopInterface(SocketWrapperUDP *wrappedSocket, sockaddr *o_peer_addr);
};

class ProtocolUDPBroadcastProxyServer : public Protocol {
private:
    ProtocolUDPBroadcastProxy *proxy;
public:
    ProtocolUDPBroadcastProxyServer(
        const InterfaceRef& _localIface,
        ProtocolManager *_m);

    ~ProtocolUDPBroadcastProxyServer();

    void handleWatchableEvent(const Watchable &wbl);

    bool hasWatchable(const Watchable &wbl);

    Protocol *getSenderProtocol(const InterfaceRef peerIface);

    ProtocolConfigurationUDPBroadcast *getUDPConfiguration();

    ProtocolConfiguration *getConfiguration();

    void setConfiguration(ProtocolConfiguration *_cfg);

    bool init();
};

#endif /* _PROTOCOLUDP_BCAST_PROXY_H */
