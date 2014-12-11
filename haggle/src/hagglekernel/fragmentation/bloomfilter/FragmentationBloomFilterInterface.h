/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONBLOOMFILTERINTERFACE_H_
#define FRAGMENTATIONBLOOMFILTERINTERFACE_H_

#include "DataObject.h"
#include "networkcoding/databitobject/DataObjectIdRef.h"

class FragmentationBloomFilterInterface {
public:
    virtual DataObjectIdRef getOriginalDataObjectId(DataObjectRef dataObject) = 0;
    //virtual DataObjectIdRef getThisOrParentDataObject(DataObjectRef dataObject) = 0;
    virtual bool isFragmentationObject(DataObjectRef dataObjectRef) =0;

    virtual ~FragmentationBloomFilterInterface() {};
};



#endif /* FRAGMENTATIONBLOOMFILTERINTERFACE_H_ */
