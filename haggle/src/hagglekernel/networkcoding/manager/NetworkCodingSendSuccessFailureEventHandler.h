/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGSENDSUCCESSFAILUREEVENTHANDLER_H_
#define NETWORKCODINGSENDSUCCESSFAILUREEVENTHANDLER_H_

#include "Event.h"
#include "networkcoding/storage/NetworkCodingEncoderStorage.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"

class NetworkCodingSendSuccessFailureEventHandler {
public:
    NetworkCodingSendSuccessFailureEventHandler(
            NetworkCodingEncoderStorage* _networkCodingEncoderStorage,
            NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility);
    virtual ~NetworkCodingSendSuccessFailureEventHandler();

    Event* retrieveOriginalDataObjectAndGenerateEvent(EventType type,
            DataObjectRef dataObjectRef, NodeRef node);
private:
    const DataObjectRef getDataObjectForNetworkCodedDataObject(
            DataObjectRef& networkCodedDataObjectRef);

    NetworkCodingEncoderStorage* networkCodingEncoderStorage;
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
};

#endif /* NETWORKCODINGSENDSUCCESSFAILUREEVENTHANDLER_H_ */
