/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLUDP_UCAST_H
#define _PROTOCOLUDP_UCAST_H

class ProtocolUDPUnicastClient;
class ProtocolUDPUnicastSender;
class ProtocolUDPUnicastSenderNoControl;
class ProtocolUDPUnicastReceiver;
class ProtocolUDPUnicastReceiverNoControl;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>

#include "Protocol.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Metadata.h"
#include "ProtocolUDPGeneric.h"
#include "ProtocolConfigurationUDPGeneric.h"

class ProtocolUDPUnicastClient : public ProtocolUDPGeneric
{
protected:
    ProtocolUDPUnicastProxy *proxy;      
public:
    ProtocolUDPUnicastClient(
        ProtocolUDPUnicastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) : 
            ProtocolUDPGeneric(
                Protocol::TYPE_UDP_UNICAST,
                "ProtocolUDPUnicast",
                _localIface,
                _peerIface,
                m,
                PROT_FLAG_CLIENT,
                _deleteEventType,
                _sendSocket),
         proxy(_proxy) {};

    virtual ~ProtocolUDPUnicastClient() {};

    ProtocolConfiguration *getConfiguration();

    ProtocolConfigurationUDPUnicast *getUDPConfiguration();

    const IPv4Address *getSendIP();

    ProtocolEvent sendHook(
        const void *buf, 
        size_t len, 
        const int flags, 
        size_t *bytes);
};

class ProtocolUDPUnicastSender : public ProtocolUDPUnicastClient
{
public:
    ProtocolUDPUnicastSender(
        ProtocolUDPUnicastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager *m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPUnicastClient(
 	        _proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket) {};

    virtual ~ProtocolUDPUnicastSender();

    bool isSender() { return true; };

    virtual int getSendPort();
};

class ProtocolUDPUnicastSenderNoControl : public ProtocolUDPUnicastSender
{
public:
    ProtocolUDPUnicastSenderNoControl(
        ProtocolUDPUnicastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager *m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPUnicastSender(
		_proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket) {};

    ProtocolEvent sendDataObjectNow(const DataObjectRef& dObj) {
        return sendDataObjectNowNoControl(dObj);
    };
        
    ProtocolEvent sendDataObjectNowSuccessHook(const DataObjectRef& dObj);

    int getSendPort();
};

class ProtocolUDPUnicastReceiver : public ProtocolUDPUnicastClient
{
public:
    ProtocolUDPUnicastReceiver(
        ProtocolUDPUnicastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager *m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPUnicastClient(
		_proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket) {};

    virtual ~ProtocolUDPUnicastReceiver();

    bool isReceiver() { return true; };

    virtual int getSendPort();
};

class ProtocolUDPUnicastReceiverNoControl : public ProtocolUDPUnicastReceiver
{
public:
    ProtocolUDPUnicastReceiverNoControl(
        ProtocolUDPUnicastProxy *_proxy,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager *m,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket) :
            ProtocolUDPUnicastReceiver(
		_proxy,
                _localIface,
                _peerIface,
                m,
                _deleteEventType,
                _sendSocket) {};

    ProtocolEvent receiveDataObject() {
        return receiveDataObjectNoControl();
    }

    int getSendPort();
};

#endif /* _PROTOCOLUDP_UCAST_H */
