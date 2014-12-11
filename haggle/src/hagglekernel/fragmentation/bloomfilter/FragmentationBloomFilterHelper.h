/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONBLOOMFILTERHELPER_H_
#define FRAGMENTATIONBLOOMFILTERHELPER_H_

#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/bloomfilter/FragmentationBloomFilterInterface.h"
#include "dataobject/DataObjectTypeIdentifierUtility.h"

class FragmentationBloomFilterHelper : public FragmentationBloomFilterInterface {
public:
	FragmentationBloomFilterHelper();
	virtual ~FragmentationBloomFilterHelper();
    virtual DataObjectIdRef getOriginalDataObjectId(DataObjectRef dataObject);

    //virtual DataObjectIdRef getThisOrParentDataObject(DataObjectRef dataObject);

    virtual bool isFragmentationObject(DataObjectRef dataObjectRef);
private:
    FragmentationDataObjectUtility* fragmentationDataObjectUtility;
    DataObjectTypeIdentifierUtility* dataObjectTypeIdentifierUtility;
};

#endif /* FRAGMENTATIONBLOOMFILTERHELPER_H_ */
