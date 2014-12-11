/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLUDPGENERIC_H
#define _PROTOCOLUDPGENERIC_H

class ProtocolUDPGeneric;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>

#include "Protocol.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Metadata.h"
#include "SocketWrapper.h"

class ProtocolUDPGeneric : public Protocol
{
protected:
    SocketWrapper *writeEndOfReceiveSocket;
    SocketWrapper *readEndOfReceiveSocket;

    SocketWrapper *sendSocket;
 
    unsigned long srcIP;
    unsigned long destIP;
    DataObjectId_t nextSendSessionNo;
    int nextSendSeqNo;
    DataObjectId_t lastReceivedSessionNo;
    int lastReceivedSeqNo;
    DataObjectId_t lastValidReceivedSessionNo;
    int lastValidReceivedSeqNo;

    EventType deleteEventType;
   
    SocketWrapper *getReadEndOfReceiveSocket();

    bool init_derived();

    virtual bool initbase();

public:

    typedef struct udpmsg {
        unsigned long src_ip;
        unsigned long dest_ip;
        DataObjectId_t session_no;
        int seq_no;
    } udpmsg_t;

    static unsigned long getSrcIPFromMsg(const char *buf);
    static unsigned long getDestIPFromMsg(const char *buf);
    static DataObjectId_t *getSessionNoFromMsg(const char *buf);
    static int getSeqNoFromMsg(const char *buf);

    static unsigned long interfaceToIP(const InterfaceRef peerIface);

    void setSessionNo(const DataObjectId_t session) { memcpy(nextSendSessionNo, session, sizeof(DataObjectId_t)); } // MOS
    bool getSessionNo(DataObjectId_t session) { memcpy(session, lastReceivedSessionNo, sizeof(DataObjectId_t)); return true; } // MOS
    void setSeqNo(int seq) { nextSendSeqNo = seq; }
    int getSeqNo() { return lastReceivedSeqNo; }

    ProtocolUDPGeneric(
        const ProtType_t _type,
        const string _name,
        const InterfaceRef& _localIface, 
        const InterfaceRef& _peerIface, 
        ProtocolManager * m,
        const short flags,
        EventType _deleteEventType,
        SocketWrapper *_sendSocket);

    virtual ~ProtocolUDPGeneric();

    ProtocolEvent connectToPeer();

    void closeConnection();

    virtual void hookShutdown();

    virtual void hookCleanup();

    virtual ProtocolError getProtocolError();

    virtual const char *getProtocolErrorStr();

    ProtocolEvent waitOnReceive(Timeval *timeout);

    ProtocolEvent waitForEvent(
        Timeval *timeout = NULL, 
        bool writeevent = false);

    ProtocolEvent waitForEvent(
        DataObjectRef &dObjRef, 
        Timeval *timeout = NULL, 
        bool writeevent = false);

    virtual ProtocolEvent sendData(
        const void *buf, 
        size_t len, 
        const int flags, 
        size_t *bytes);

    virtual ProtocolEvent sendHook(
        const void *buf, 
        size_t len, 
        const int flags, 
        ssize_t *bytes);

    virtual const IPv4Address *getSendIP() { return NULL; }

    virtual int getSendPort() { return 0; }

    virtual ProtocolEvent receiveData(
        void *buf, 
        size_t len, 
        const int flags, 
        size_t *bytes);
   
    SocketWrapper *getWriteEndOfReceiveSocket();

    virtual ProtocolEvent sendDataObjectNowSuccessHook(const DataObjectRef& dObj) {
        return PROT_EVENT_SUCCESS;
    };

    ProtocolEvent sendDataObjectNowNoControl(const DataObjectRef& dObj);

    ProtocolEvent receiveDataObjectNoControl();
};

#endif /* _PROTOCOLUDPGENERIC_H */
