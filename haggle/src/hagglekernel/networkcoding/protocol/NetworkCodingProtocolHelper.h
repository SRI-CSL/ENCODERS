/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef NETWORKCODINGPROTOCOLHELPER_H_
#define NETWORKCODINGPROTOCOLHELPER_H_

#include "Bloomfilter.h"
#include "Event.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"

class NetworkCodingProtocolHelper {
public:
    NetworkCodingProtocolHelper();
    virtual ~NetworkCodingProtocolHelper();

    bool containsOriginalDataObject(Bloomfilter* bloomFilter,DataObjectRef dataObject);
    /*
     * protocolmanager invokes this method to get the actual dataobject to send
     */
    Event* onSendDataObject(DataObjectRef& originalDataObjectRef, NodeRef& node, EventType send_data_object_actual_event);

private:
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
};

#endif /* NETWORKCODINGPROTOCOLHELPER_H_ */
