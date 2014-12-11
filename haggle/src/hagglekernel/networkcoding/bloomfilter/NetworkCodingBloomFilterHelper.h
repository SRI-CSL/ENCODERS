/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGBLOOMFILTERHELPER_H_
#define NETWORKCODINGBLOOMFILTERHELPER_H_

#include "networkcoding/bloomfilter/NetworkCodingBloomFilterInterface.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"

class NetworkCodingBloomFilterHelper : public NetworkCodingBloomFilterInterface {
public:
    NetworkCodingBloomFilterHelper();
    virtual ~NetworkCodingBloomFilterHelper();

    virtual DataObjectIdRef getOriginalDataObjectId(DataObjectRef dataObject);

    virtual DataObjectIdRef getThisOrParentDataObject(DataObjectRef dataObject);

    virtual bool isNetworkCodedObject(DataObjectRef dataObjectRef);
private:
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
};

#endif /* NETWORKCODINGBLOOMFILTERHELPER_H_ */
