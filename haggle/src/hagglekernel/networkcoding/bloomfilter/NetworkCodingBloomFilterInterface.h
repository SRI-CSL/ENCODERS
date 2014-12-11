/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGBLOOMFILTERINTERFACE_H_
#define NETWORKCODINGBLOOMFILTERINTERFACE_H_

#include "DataObject.h"
#include "networkcoding/databitobject/DataObjectIdRef.h"


class NetworkCodingBloomFilterInterface {
public:
    virtual DataObjectIdRef getOriginalDataObjectId(DataObjectRef dataObject) = 0;
    virtual DataObjectIdRef getThisOrParentDataObject(DataObjectRef dataObject) = 0;
    virtual bool isNetworkCodedObject(DataObjectRef dataObjectRef) =0;

    virtual ~NetworkCodingBloomFilterInterface() {};
};



#endif /* NETWORKCODINGBLOOMFILTERINTERFACE_H_ */
