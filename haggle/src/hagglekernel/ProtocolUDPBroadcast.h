/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLUDP_BCAST_H
#define _PROTOCOLUDP_BCAST_H

class ProtocolUDPBroadcastPassiveReceiver;
class ProtocolUDPBroadcastClient;
class ProtocolUDPBroadcastSender;
class ProtocolUDPBroadcastSenderNoControl;
class ProtocolUDPBroadcastReceiver;
class ProtocolUDPBroadcastReceiverNoControl;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>

#include "Protocol.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Metadata.h"
#include "ProtocolUDPGeneric.h"
#include "ProtocolConfigurationUDPGeneric.h"

class ProtocolUDPBroadcastClient : public ProtocolUDPGeneric
{
protected:
    ProtocolUDPBroadcastProxy *proxy;      
public:
    ProtocolUDPBroadcastClient(
        ProtocolUDPBroadcastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket,
        string _name = "ProtocolUDPBroadcast") :
            ProtocolUDPGeneric(
                Protocol::TYPE_UDP_BROADCAST,
                _name,
                _localIface,
                _peerIface,
                m,
                PROT_FLAG_CLIENT,
                _deleteEventType,
                _sendSocket), 
            proxy(_proxy) {};

    virtual ~ProtocolUDPBroadcastClient() {};

    ProtocolConfiguration *getConfiguration();

    ProtocolConfigurationUDPBroadcast *getUDPConfiguration();

    const IPv4Address *getSendIP();
};

class ProtocolUDPBroadcastSender : public ProtocolUDPBroadcastClient
{
public:
    ProtocolUDPBroadcastSender(
        ProtocolUDPBroadcastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPBroadcastClient(
 	        _proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket,
                "ProtocolUDPBroadcastSender") {};
 
    virtual ~ProtocolUDPBroadcastSender();

    virtual int getSendPort();

    bool isSender() { return true; };

    ProtocolEvent sendDataObjectNowSuccessHook(const DataObjectRef& dObj);
};

class ProtocolUDPBroadcastSenderNoControl : public ProtocolUDPBroadcastSender
{
public:
    ProtocolUDPBroadcastSenderNoControl(
        ProtocolUDPBroadcastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPBroadcastSender(
                _proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket) {};

    ProtocolEvent sendDataObjectNow(const DataObjectRef& dObj) {
        return sendDataObjectNowNoControl(dObj);
    };

    int getSendPort();
};


class ProtocolUDPBroadcastReceiver : public ProtocolUDPBroadcastClient
{
protected:
    // SW: quick fix to avoid passive receiver to unsuccessfully remove
    // from proxy
    bool unregisterFromProxy;
public:
    ProtocolUDPBroadcastReceiver(
        ProtocolUDPBroadcastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket,
        string _name = "ProtocolUDPBroadcastReceiver",
        bool _unregisterFromProxy = true) :
            ProtocolUDPBroadcastClient(
                _proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket,
                _name),
            unregisterFromProxy(_unregisterFromProxy) {};

    virtual ~ProtocolUDPBroadcastReceiver();

    bool isReceiver() { return true; };

    virtual int getSendPort();

    void receiveDataObjectSuccessHook(const DataObjectRef& dObj);
};

class ProtocolUDPBroadcastPassiveReceiver : public ProtocolUDPBroadcastReceiver
{
protected:
    ProtocolUDPBroadcastProxy *proxy;      
public:
    ProtocolUDPBroadcastPassiveReceiver(
        ProtocolUDPBroadcastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType) :
            ProtocolUDPBroadcastReceiver(
                _proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                NULL,
                "ProtocolUDPBroadcastPassiveReceiver",
                false),
        proxy(_proxy) {};

    virtual ~ProtocolUDPBroadcastPassiveReceiver();

    virtual int getWaitTimeBeforeDoneMillis();

    ProtocolEvent sendHook(
        const void *buf, 
        size_t len, 
        const int flags, 
        ssize_t *bytes);
};

class ProtocolUDPBroadcastReceiverNoControl : public ProtocolUDPBroadcastReceiver
{
public:
    ProtocolUDPBroadcastReceiverNoControl(
        ProtocolUDPBroadcastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPBroadcastReceiver(
                _proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket) {};

    ProtocolEvent receiveDataObject();

    int getSendPort();
};

#endif /* _PROTOCOLUDP_BCAST_H */
