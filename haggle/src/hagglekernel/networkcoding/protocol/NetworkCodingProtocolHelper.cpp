/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingProtocolHelper.h"
#include "DataObject.h"

NetworkCodingProtocolHelper::NetworkCodingProtocolHelper() {
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
}

NetworkCodingProtocolHelper::~NetworkCodingProtocolHelper() {
    if( this->networkCodingDataObjectUtility ) {
        delete this->networkCodingDataObjectUtility;
        this->networkCodingDataObjectUtility = NULL;
    }
}

bool NetworkCodingProtocolHelper::containsOriginalDataObject(Bloomfilter* bloomFilter,DataObjectRef dataObject) {
    const string originalDataObjectId = this->networkCodingDataObjectUtility->checkAndGetOriginalDataObjectId(dataObject);
    if( originalDataObjectId.c_str() == NULL || originalDataObjectId.empty() ||  originalDataObjectId.length() < 1 ) {
        return false;
    }

    string dataObjectIdString = originalDataObjectId;

    DataObjectIdRef dataObjectIdRef = this->networkCodingDataObjectUtility->convertDataObjectIdStringToDataObjectIdType(dataObjectIdString);
    const unsigned char * dataObjectIdPtr = dataObjectIdRef.c_str();
    bool contains =  bloomFilter->has(dataObjectIdPtr,DATAOBJECT_ID_LEN);
    HAGGLE_DBG("bloom filter contains=%d id=%s\n",contains,originalDataObjectId.c_str());
    return contains;
}

Event* NetworkCodingProtocolHelper::onSendDataObject(DataObjectRef& originalDataObjectRef,NodeRef& node, EventType send_data_object_actual_event) {

        if(node->getType() == Node::TYPE_APPLICATION) {
        	//do nothing
        }
        else {
        	bool networkCodedForThisTarget = this->networkCodingDataObjectUtility->shouldBeNetworkCodedCheckTargetNode(originalDataObjectRef,node);
        	if(networkCodedForThisTarget) {
		  NodeRefList networkCodingTargetNodeRefList;
		  networkCodingTargetNodeRefList.add(node);
		  Event* newEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING, originalDataObjectRef, networkCodingTargetNodeRefList);
		  return newEvent;
        	}
        }

    return NULL;
}
