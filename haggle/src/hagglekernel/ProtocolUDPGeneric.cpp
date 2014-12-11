/* Copyright (c) 2014 SRI International and Suns-tech Incorporated and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Yu-Ting Yu (yty)
 */

#include <string.h>

#include <libcpphaggle/Platform.h>
#include <haggleutils.h>

#include "ProtocolUDPGeneric.h"
#include "ProtocolManager.h"
#include "Interface.h"

#define SOCKADDR_SIZE sizeof(struct sockaddr_in)
#define PROTOCOL_UDP_GENERIC_BUFSIZ (4096*16) // MOS - increased from 8K

// START: helper static functions
unsigned long ProtocolUDPGeneric::getSrcIPFromMsg(
    const char *buf)
{
    if (!buf) {
        HAGGLE_ERR("NULL buffer\n");
        return 0;
    }

    return ((udpmsg_t *)buf)->src_ip;
}

unsigned long ProtocolUDPGeneric::getDestIPFromMsg(
    const char *buf)
{
    if (!buf) {
        HAGGLE_ERR("NULL buffer\n");
        return 0;
    }

    return ((udpmsg_t *)buf)->dest_ip;
}

DataObjectId_t *ProtocolUDPGeneric::getSessionNoFromMsg(
    const char *buf)
{
    if (!buf) {
        HAGGLE_ERR("NULL buffer\n");
        return 0;
    }

    return &((udpmsg_t *)buf)->session_no;
}

int ProtocolUDPGeneric::getSeqNoFromMsg(
    const char *buf)
{
    if (!buf) {
        HAGGLE_ERR("NULL buffer\n");
        return 0;
    }

    return ((udpmsg_t *)buf)->seq_no;
}


unsigned long ProtocolUDPGeneric::interfaceToIP(
    const InterfaceRef peerIface)
{
    if (!peerIface) {
        HAGGLE_ERR("Null interface\n");
        return 0;
    }

    const IPv4Address *addr = peerIface->getAddress<IPv4Address>();
    if (NULL == addr) {
        HAGGLE_ERR("Could not get IP address\n");
        return 0;
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *peer_addr = (struct sockaddr *)buf;
    addr->fillInSockaddr((struct sockaddr_in *)peer_addr, 0);

    unsigned long ip = 
        ((struct sockaddr_in *)peer_addr)->sin_addr.s_addr;

    return ip;
}

// END: helper static functions

ProtocolUDPGeneric::ProtocolUDPGeneric(
    const ProtType_t _type,
    const string _name,
    const InterfaceRef& _localIface, 
    const InterfaceRef& _peerIface, 
    ProtocolManager * m,
    const short flags,
    EventType _deleteEventType,
    SocketWrapper *_sendSocket) : 
        Protocol(
            _type,
            _name,
            _localIface, 
            _peerIface, 
            flags, 
            m,
            PROTOCOL_UDP_GENERIC_BUFSIZ),
        writeEndOfReceiveSocket(NULL),
        readEndOfReceiveSocket(NULL),
        sendSocket(_sendSocket),
        nextSendSeqNo(0),
        lastReceivedSeqNo(0),
        lastValidReceivedSeqNo(-1),
        deleteEventType(_deleteEventType)
{
  memset(nextSendSessionNo, 0, sizeof(DataObjectId_t));
  memset(lastReceivedSessionNo, 0, sizeof(DataObjectId_t));
  memset(lastValidReceivedSessionNo, 0, sizeof(DataObjectId_t));
}

ProtocolUDPGeneric::~ProtocolUDPGeneric()
{
    if (!writeEndOfReceiveSocket) {
        HAGGLE_ERR("%s write end of receive socket somehow destroyed..\n", getName());
        return;
    }

    delete writeEndOfReceiveSocket;

    if (!readEndOfReceiveSocket) {
        HAGGLE_ERR("%s read end of receive socket somehow destroyed..\n", getName());
        return;
    }

    delete readEndOfReceiveSocket;
}

bool ProtocolUDPGeneric::init_derived()
{
    if (!peerIface) {
        HAGGLE_ERR("%s No peer interface\n", getName());
        return false;
    }

    if (!localIface) {
        HAGGLE_ERR("%s No local interface\n", getName());
        return false;
    }

    SOCKET sockets[2];
    if (0 > socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets)) {
        HAGGLE_ERR("%s Could not create socket pair for receiver: %s\n", getName(), STRERROR(ERRNO));
        HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n"); // MOS
	getKernel()->setFatalError();
	getKernel()->shutdown();
        return false;
    }
    else
      {
        HAGGLE_DBG2("%s Opening socketpair (%d,%d)\n", getName(), sockets[0], sockets[1]);
      }

    writeEndOfReceiveSocket =
        new SocketWrapper(getKernel(), getManager(), sockets[0]);

    if (!writeEndOfReceiveSocket) {
        HAGGLE_ERR("%s Could not allocate write end\n", getName());
        return false;
    }

    if (!writeEndOfReceiveSocket->multiplySndBufferSize(2)) {
        HAGGLE_ERR("%s Could not multiply buffer size.\n", getName());
        return false;
    }

    readEndOfReceiveSocket =
        new SocketWrapper(getKernel(), getManager(), sockets[1]);

    if (!readEndOfReceiveSocket) {
        HAGGLE_ERR("%s Could not allocate read end\n", getName());
        return false;
    }

    if (!readEndOfReceiveSocket->multiplyRcvBufferSize(2)) {
        HAGGLE_ERR("%s Could not multiply buffer size.\n", getName());
        return false;
    }

    srcIP = interfaceToIP(localIface);

    if (0 == srcIP) {
        HAGGLE_ERR("%s Could not get source IP.\n");
        return false;
    }

    destIP = interfaceToIP(peerIface);

    if (0 == destIP) {
        HAGGLE_ERR("%s Could not get dest IP.\n");
        return false;
    }

    return initbase();
}

bool ProtocolUDPGeneric::initbase()
{    
    return true;
}

SocketWrapper *ProtocolUDPGeneric::getWriteEndOfReceiveSocket() 
{
    return writeEndOfReceiveSocket;
}

SocketWrapper *ProtocolUDPGeneric::getReadEndOfReceiveSocket() 
{
    return readEndOfReceiveSocket;
}
   
ProtocolEvent ProtocolUDPGeneric::connectToPeer()
{
    setFlag(PROT_FLAG_CONNECTED);
    return PROT_EVENT_SUCCESS;
}

void ProtocolUDPGeneric::closeConnection() 
{
    unSetFlag(PROT_FLAG_CONNECTED);
}

void ProtocolUDPGeneric::hookCleanup()
{
    // SW: only make shutDown responsible for removing from cache
	//hookShutdown();
    closeConnection();
}

void ProtocolUDPGeneric::hookShutdown()
{
    closeConnection();
}

ProtocolError ProtocolUDPGeneric::getProtocolError()
{
    return SocketWrapper::getProtocolError();
}

const char *ProtocolUDPGeneric::getProtocolErrorStr()
{
    return SocketWrapper::getProtocolErrorStr();
}

ProtocolEvent ProtocolUDPGeneric::waitOnReceive(Timeval *timeout) 
{
    bool wasReadable = false; 
    bool wasWritable = false;

    SocketWrapper *sock = getReadEndOfReceiveSocket();
    if (NULL == sock) {
        HAGGLE_ERR("%s Null receive socket\n", getName());
        return PROT_EVENT_ERROR_FATAL;
    }

    int ret;
    sock->waitForEvent(
        timeout, 
        false, 
        &wasReadable,
        &wasWritable,
        &ret);

    if (Watch::TIMEOUT == ret) {
        HAGGLE_DBG("%s Waiting for event timeout\n", getName());
        return PROT_EVENT_TIMEOUT;
    }

    if (Watch::FAILED == ret) {
        HAGGLE_ERR("%s Waiting for event failed\n", getName());
        return PROT_EVENT_ERROR;
    }

    if (Watch::ABANDONED == ret) {
        // SW: this occurs when the protocol has been shutdown via some other
        // mechanism, such as interface down
        HAGGLE_DBG("%s Waiting for event abandoned\n", getName());
        return PROT_EVENT_SHOULD_EXIT;
    }

    if (!wasReadable) {
        HAGGLE_ERR("%s Waiting for even unknown error\n", getName());
        return PROT_EVENT_ERROR;
    }

    // SW: explicit synchronization to avoid waking up child before 
    // the entire message has been added to the socket
    //Mutex::AutoLocker l(sock->getProxyLock());
  
    return PROT_EVENT_INCOMING_DATA;
}

ProtocolEvent ProtocolUDPGeneric::waitForEvent(
    Timeval *timeout, 
    bool writeevent)
{
    if (writeevent) {
        return PROT_EVENT_WRITEABLE;
    } 

    return waitOnReceive(timeout);
}

ProtocolEvent ProtocolUDPGeneric::waitForEvent(
    DataObjectRef &dObj, 
    Timeval *timeout, 
    bool writeevent)
{
    QueueElement *qe = NULL;
    Queue *q = getQueue();

    if (!q) {
        return PROT_EVENT_ERROR;
    }

    SocketWrapper *sock = getReadEndOfReceiveSocket();

    if (NULL == sock) {
        HAGGLE_ERR("%s Null receive socket\n", getName());
        return PROT_EVENT_ERROR_FATAL;
    }

    HAGGLE_DBG("%s Waiting for queue element or timeout... %s\n", 
	       getName(), timeout->getAsString().c_str());

    QueueEvent_t qev = q->retrieve(&qe, 
		      sock->getSOCKET(), 
		      timeout, 
		      writeevent);

    if (QUEUE_ELEMENT == qev) {
        dObj = qe->getDataObject();
        delete qe;
        return PROT_EVENT_TXQ_NEW_DATAOBJECT;
    }

    if (QUEUE_EMPTY == qev) {
        return PROT_EVENT_TXQ_EMPTY;
    }

    if (QUEUE_WATCH_WRITE == qev) {
        return PROT_EVENT_WRITEABLE;
    }

    if (QUEUE_WATCH_ABANDONED == qev) {
        // SW: this occurs when the protocol has been shutdown via some other
        // mechanism, such as interface down
        HAGGLE_DBG("%s Waiting for event abandoned\n", getName());
        return PROT_EVENT_SHOULD_EXIT;
    }
    
    if (QUEUE_TIMEOUT == qev) {
        HAGGLE_DBG("%s Waiting for event timeout\n", getName());
        return  PROT_EVENT_TIMEOUT;
    }

    if (QUEUE_WATCH_READ == qev) {
        return PROT_EVENT_INCOMING_DATA;
    } 

    HAGGLE_ERR("%s Waiting for event unknown error\n", getName());
    return PROT_EVENT_ERROR;
}

ProtocolEvent ProtocolUDPGeneric::sendData(
    const void *buf, 
    size_t len, 
    const int flags, 
    size_t *bytes)
{
    *bytes = 0;

    int signed_size = static_cast<int>(len);
    if (signed_size <= 0) {
        HAGGLE_ERR("%s Cannot send file of invalid length %d\n", getName(), signed_size);
        return PROT_EVENT_ERROR_FATAL;
    }

    size_t newLength = len + sizeof(udpmsg_t);
    void *newBuff = malloc(newLength);
    bzero(newBuff, newLength);
    udpmsg_t *header = (udpmsg_t *)newBuff;
    memcpy(header->session_no, nextSendSessionNo, sizeof(DataObjectId_t));
    header->seq_no = nextSendSeqNo;
    header->src_ip = srcIP;
    header->dest_ip = destIP;

    if (NULL == memcpy((void *)&(((char *)newBuff)[sizeof(udpmsg_t)]), buf, len)) {
        HAGGLE_ERR("%s Could not create new buffer\n", getName());
        return PROT_EVENT_ERROR_FATAL;
    }

    HAGGLE_DBG2("%s Sending packet with session no %s, sequence no %d, and size %d\n",
        getName(), 
	DataObject::idString(nextSendSessionNo).c_str(), 
        nextSendSeqNo, 
        len);

    ProtocolEvent pEvent;
    ssize_t sbytes;

    pEvent = sendHook(newBuff, newLength, flags, &sbytes);

    if (sbytes < 0) {
        free(newBuff);
        return PROT_EVENT_ERROR;
    }

    *bytes = static_cast<size_t>(sbytes) - sizeof(udpmsg_t);
    free(newBuff);
    return pEvent;
}

ProtocolEvent ProtocolUDPGeneric::sendHook(
    const void *buf, 
    size_t len, 
    const int flags, 
    ssize_t *bytes)
{
    *bytes = 0;
    const IPv4Address *addr = getSendIP();
    if (NULL == addr) {
        HAGGLE_ERR("%s Could not get IP address\n", getName());
        return PROT_EVENT_ERROR_FATAL;
    }

    if (NULL == buf) {
        HAGGLE_ERR("%s NULL send buffer\n", getName());
        return PROT_EVENT_ERROR_FATAL;
    }

    char sockbuf[SOCKADDR_SIZE];
    bzero(sockbuf, SOCKADDR_SIZE);
    struct sockaddr *to = (struct sockaddr *)sockbuf;
    socklen_t tolen = sizeof(struct sockaddr);
    int port = getSendPort();

    if (0 == port) {
        HAGGLE_ERR("%s Invalid port\n", getName());
        return PROT_EVENT_ERROR;
    }
    addr->fillInSockaddr((struct sockaddr_in *)to, port);

    ssize_t wrote = sendSocket->sendTo(buf, len, flags, to, tolen);

    if (0 > wrote) {
        HAGGLE_ERR("%s Could not send data\n", getName());
        return PROT_EVENT_ERROR;
    }

    if (len != (size_t) wrote) {
        HAGGLE_ERR("%s Wrote invalid amount\n", getName());
        return PROT_EVENT_ERROR;
    }

    *bytes = wrote;

    return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolUDPGeneric::receiveData(
    void *buf, 
    size_t len, 
    const int flags, 
    size_t *bytes)
{
    *bytes = 0;

    ProtocolEvent pEvent;
    ssize_t sbytes;

    SocketWrapper *receiveSocket = getReadEndOfReceiveSocket();
    if (NULL == receiveSocket) {
        HAGGLE_ERR("%s Could not get receive socket\n", getName());
        return PROT_EVENT_ERROR_FATAL;
    }

    size_t newLength = len + sizeof(udpmsg_t);
    void *newBuff = malloc(newLength);
    bzero(newBuff, newLength);

    pEvent = receiveSocket->receiveData(newBuff, newLength, flags, &sbytes);
    if (sbytes < 0 || PROT_EVENT_SUCCESS != pEvent) {
        HAGGLE_ERR("%s Receive failed\n", getName());
        free(newBuff);
        return PROT_EVENT_ERROR;
    }
    *bytes = static_cast<size_t>(sbytes) - sizeof(udpmsg_t);

    if (*bytes <= 0) {
        HAGGLE_ERR("%s Receive failed (size was wrong)\n", getName());
        free(newBuff);
        return PROT_EVENT_ERROR;
    }

    memcpy(lastReceivedSessionNo, getSessionNoFromMsg((const char *)newBuff), sizeof(DataObjectId_t));
    lastReceivedSeqNo = getSeqNoFromMsg((const char *)newBuff);
    unsigned long rcvSrcIP = getSrcIPFromMsg((const char *)newBuff);
    //unsigned long rcvDestIP = getDestIPFromMsg((const char *)newBuff);

    HAGGLE_DBG2("%s Got packet with session no %s, sequence no %d, and size %d\n",
        getName(), 
	DataObject::idString(lastReceivedSessionNo).c_str(), 
        lastReceivedSeqNo, 
        *bytes);

    memcpy(buf, (void *)&(((char *)newBuff)[sizeof(udpmsg_t)]), len);

    free(newBuff);

    if (rcvSrcIP == srcIP) {
        HAGGLE_ERR("%s Somehow received our own UDP packet: %d\n", getName(), srcIP);
        return PROT_EVENT_ERROR;
    }

    // MOS: START SETTING PEER NODE
    Mutex::AutoLocker l(mutex); // MOS
    if (!peerIface) {
        HAGGLE_ERR("%s UDP peer interface was null\n", getName());
        return PROT_EVENT_ERROR;
    }

    // MOS - Protocol.cpp relies on peerNode for debugging
    NodeRef peer = getKernel()->getNodeStore()->retrieve(peerIface);
    if(peer) peerNode = peer;

    if (!peerNode) {
        peerNode = Node::create(Node::TYPE_UNDEFINED, "Peer node");      
        if (!peerNode) {      
	    HAGGLE_ERR("%s Could not create peer node\n", getName());
            return PROT_EVENT_ERROR;
        }
        peerNode->addInterface(peerIface);
    }
    // MOS: END SETTING PEER NODE

    return pEvent;
}

ProtocolEvent ProtocolUDPGeneric::sendDataObjectNowNoControl(
    const DataObjectRef& dObj)
{
    NodeRef currentPeer;
    { Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS

    NodeRef actualPeer = getManager()->getKernel()->getNodeStore()->retrieve(currentPeer, true);
    if (!actualPeer) {
        HAGGLE_ERR("%s Peer not in node store\n", getName());
        return PROT_EVENT_ERROR;
    }

    // check if already in peers bloomfilter
    if (actualPeer->hasThisOrParentDataObject(dObj)) { // MOS
        HAGGLE_DBG("%s Peer already had data object.\n", getName());
        return PROT_EVENT_SUCCESS;
    }
    // done

    // SW: we move the hook here to minimize race condition where we send
    // too many redundant node descriptions
    // TODO: we may want to synchronize on the dObj or have some serial
    // queue so this is not probabilistic
    sendDataObjectNowSuccessHook(dObj);

    HAGGLE_DBG("%s Sending data object [%s] to peer \'%s\'\n", 
        getName(), dObj->getIdStr(), peerDescription().c_str());

    DataObjectDataRetrieverRef retriever = dObj->getDataObjectDataRetriever();
    if (!retriever || !retriever->isValid()) {
        HAGGLE_ERR("%s unable to start reading data\n", getName());
        return PROT_EVENT_ERROR;
    }

    ProtocolEvent pEvent = PROT_EVENT_SUCCESS;

    ssize_t len = retriever->retrieve(buffer, bufferSize, false);

    if (0 == len) {
        HAGGLE_ERR("%s DataObject is empty\n", getName());
        return PROT_EVENT_ERROR;
    }

    if (len < 0) {
        HAGGLE_ERR("%s Could not retrieve data from data object\n", getName());
        return PROT_EVENT_ERROR;
    }

    if ((size_t) len == bufferSize) {
        HAGGLE_ERR("%s Buffer is too small for message\n", getName());
        return PROT_EVENT_ERROR;
    }

    Timeval t_start;
    t_start.setNow();

    setSessionNo(dObj->getId()); // MOS
    setSeqNo(0); // MOS

    size_t bytesSent = 0;

    // MOS - simple way to increase udp redundancy
    for(int i = 0; i <= getConfiguration()->getRedundancy(); i++) {

    pEvent = sendData(buffer, (size_t) len, 0, &bytesSent);
    if (bytesSent != (size_t) len) {
        pEvent = PROT_EVENT_ERROR;
    }

    if (pEvent != PROT_EVENT_SUCCESS) {
        HAGGLE_ERR("%s Broadcast - Error\n", getName());
        return pEvent;
    }
    }

#ifdef DEBUG
    Timeval tx_time = Timeval::now() - t_start;

    HAGGLE_DBG("%s Sent %lu bytes data in %.3lf seconds, average speed = %.2lf kB/s\n", 
        getName(), len, tx_time.getTimeAsSecondsDouble(), 
        (double)len / (1000*tx_time.getTimeAsSecondsDouble()));
#endif

    dataObjectsOutgoing += 1; // MOS
    dataObjectsSent += 1; // MOS
    if(!dObj->isControlMessage()) dataObjectsOutgoingNonControl += 1; // MOS
    dataObjectBytesOutgoing += bytesSent; // MOS
    dataObjectBytesSent += len; // MOS
    if(dObj->isNodeDescription()) { nodeDescSent += 1; nodeDescBytesSent += len; } // MOS

    return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolUDPGeneric::receiveDataObjectNoControl()
{
    ProtocolEvent pEvent;
    size_t len;
    pEvent = receiveData(buffer, bufferSize, MSG_DONTWAIT, &len);
    if (pEvent != PROT_EVENT_SUCCESS) {
        return pEvent;
    }

    buffer[bufferSize-1] = '\0';

    if (len == 0) {
        HAGGLE_DBG("%s Received zero-length message\n", getName());
        return PROT_EVENT_ERROR;
    }

    if(lastReceivedSessionNo == lastValidReceivedSessionNo && lastReceivedSeqNo == lastValidReceivedSeqNo) {
      HAGGLE_DBG("%s Ignoring duplicate message - session no %s sequence no %d\n", getName(), DataObject::idString(lastValidReceivedSessionNo).c_str(), lastReceivedSeqNo);
      return PROT_EVENT_SUCCESS;
    }

    memcpy(lastValidReceivedSessionNo, lastReceivedSessionNo, sizeof(DataObjectId_t)); 
    lastValidReceivedSeqNo = lastReceivedSeqNo; 

    // MOS - fastpath based on session id = data object id
    
    if (getKernel()->getThisNode()->getBloomfilter()->has(lastValidReceivedSessionNo)) {
      HAGGLE_DBG("%s Data object (session no %s) already in bloom filter - no event generated\n", getName(), DataObject::idString(lastValidReceivedSessionNo).c_str()); 
	dataObjectsNotReceived += 1; // MOS
        return PROT_EVENT_SUCCESS;
    }

    
    // MOS - quickly add to Bloom filter to reduce redundant processing in other procotols
    getKernel()->getThisNode()->getBloomfilter()->add(lastValidReceivedSessionNo);

    DataObjectRef dObj = DataObject::create_for_putting(localIface,
                                                        peerIface,  
                                                        getKernel()->getStoragePath());
    if (!dObj) {
        HAGGLE_DBG("%s Could not create data object\n", getName());
        return PROT_EVENT_ERROR;
    }

    size_t bytesRemaining = DATAOBJECT_METADATA_PENDING;
    ssize_t bytesPut = dObj->putData(buffer, len, &bytesRemaining, true);

    if (bytesPut < 0) {
        HAGGLE_ERR("%s Could not put data\n", getName());
        return PROT_EVENT_ERROR;
    }

    if(bytesRemaining != len - bytesPut) {
        HAGGLE_ERR("%s Received data object not complete - discarding\n", getName());
        return PROT_EVENT_ERROR;
    }

    HAGGLE_DBG("%s Metadata header received [%s].\n", getName(), dObj->getIdStr());
    
    dObj->setReceiveTime(Timeval::now());

    // MOS - the following was happening after posting INCOMING but that distorts the statistics
    HAGGLE_DBG("%s %ld bytes data received (including header), %ld bytes put\n", getName(), len, bytesPut);
    dataObjectsIncoming += 1; // MOS
    if(!dObj->isControlMessage()) dataObjectsIncomingNonControl += 1; // MOS
    dataObjectBytesIncoming += len; // MOS

    HAGGLE_DBG("%s Received data object [%s] from node %s\n", getName(), 
	       DataObject::idString(dObj).c_str(), peerDescription().c_str()); 
    // MOS - removed interface due to locking issue

    if (getKernel()->getThisNode()->getBloomfilter()->hasParentDataObject(dObj)) {
        HAGGLE_DBG("%s Data object [%s] already in bloom filter - no event generated\n", getName(), DataObject::idString(dObj).c_str()); 
	dataObjectsNotReceived += 1; // MOS
        return PROT_EVENT_SUCCESS;
    }

    NodeRef node = Node::create(dObj);
    if (node && (node == getKernel()->getThisNode())) {
        HAGGLE_DBG("%s Received own node description, discarding early.\n", getName());
	dataObjectsNotReceived += 1; // MOS
        return PROT_EVENT_SUCCESS;
    }

    // MOS - this now happens even before xml parsing
    // getKernel()->getThisNode()->getBloomfilter()->add(dObj);

    if(bytesRemaining > 0) {
      ssize_t bytesPut2 = dObj->putData(buffer + bytesPut, len - bytesPut, &bytesRemaining, false);
      HAGGLE_DBG("%s processing payload - %ld bytes put\n", getName(), bytesPut2);

      if (bytesPut2 < 0) {
        HAGGLE_ERR("%s Could not put data\n", getName());
        return PROT_EVENT_ERROR;
      }
      
      if(bytesRemaining != 0) {
        HAGGLE_ERR("%s Received data object not complete - discarding\n", getName());
        return PROT_EVENT_ERROR;
      }
    }

    NodeRef currentPeer;
    { Mutex::AutoLocker l(mutex); currentPeer = peerNode; } // MOS

    // Generate first an incoming event to conform with the base Protocol class
    getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_INCOMING, dObj, currentPeer));

    receiveDataObjectSuccessHook(dObj);

    // Since there is no data following, we generate the received event immediately 
    // following the incoming one
    getKernel()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_RECEIVED, dObj, currentPeer));

    dataObjectsReceived += 1; // MOS
    dataObjectBytesReceived += len; // MOS
    if(dObj->isNodeDescription()) { nodeDescReceived += 1; nodeDescBytesReceived += len; } // MOS

    return PROT_EVENT_SUCCESS;
}
