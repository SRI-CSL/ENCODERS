/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef FRAGMENTATIONENCODERSTORAGE_H_
#define FRAGMENTATIONENCODERSTORAGE_H_

#include <stdio.h>
#include <stdlib.h>

#include "DataObject.h"
#include <libcpphaggle/Reference.h>

typedef Map<string,DataObjectRef> fragmentationdataobjectstorage_t;
typedef Reference<fragmentationdataobjectstorage_t> fragmentationdataobjectstorage_ref_t;

typedef Reference<Map<size_t,DataObjectRef> > fragmentationsequencedataobjectstorage_t;
typedef Map<string,fragmentationsequencedataobjectstorage_t> fragmentationsequencedataobjectstorageMapping_t;
typedef Reference<fragmentationsequencedataobjectstorageMapping_t> fragmentationsequencedataobjectstorageMapping_ref_t;

typedef Map<string,string> fragmentationSendEventTime_t;
typedef Reference<fragmentationSendEventTime_t> fragmentationSendEventTime_ref_t;

class FragmentationEncoderStorage {
public:
	FragmentationEncoderStorage();
	virtual ~FragmentationEncoderStorage();

    void addDataObject(string originalDataObjectId,const DataObjectRef originalDataObject);
    const DataObjectRef getDataObjectByOriginalDataObjectId(string originalDataObjectId);

    void addFragment(string originalDataObjectId,size_t sequenceNumber,const DataObjectRef fragmentDataObjectRef);
    const DataObjectRef getFragment(string originalDataObjectId,size_t sequenceNumber);

    void deleteFromStorage(DataObjectRef parentDataObjectRef);
    void deleteFromStorageByDataObjectId(string parentDataObjectId);

    void trackSendEvent(string parentDataObjectId);

    void ageOffFragments(double maxAgeFragment);

private:
    fragmentationdataobjectstorage_ref_t fragmentationdataobjectstorage;
    fragmentationsequencedataobjectstorageMapping_ref_t fragmentationsequencedataobjectstorageMapping;
    fragmentationSendEventTime_ref_t fragmentationSendEventTime;
};

#endif /* FRAGMENTATIONENCODERSTORAGE_H_ */
