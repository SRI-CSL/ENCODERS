/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationEncoderStorage.h"
#include "Trace.h"

FragmentationEncoderStorage::FragmentationEncoderStorage() {
	this->fragmentationdataobjectstorage = new fragmentationdataobjectstorage_t();
	this->fragmentationsequencedataobjectstorageMapping = new fragmentationsequencedataobjectstorageMapping_t();
	this->fragmentationSendEventTime = new fragmentationSendEventTime_t();
}

FragmentationEncoderStorage::~FragmentationEncoderStorage() {

}

void FragmentationEncoderStorage::addDataObject(string originalDataObjectId,
        const DataObjectRef originalDataObject) {
    this->fragmentationdataobjectstorage->operator [](originalDataObjectId) = originalDataObject;
}

const DataObjectRef FragmentationEncoderStorage::getDataObjectByOriginalDataObjectId(
        string originalDataObjectId) {
    const DataObjectRef found = this->fragmentationdataobjectstorage->operator [](originalDataObjectId);
    if (found) {
        HAGGLE_DBG("Found data object %s\n", originalDataObjectId.c_str());
    } else {
        HAGGLE_DBG("Unable to locate data object %s\n", originalDataObjectId.c_str());
    }
    return found;
}

void FragmentationEncoderStorage::addFragment(string originalDataObjectId, size_t sequenceNumber,
        const DataObjectRef fragmentDataObjectRef) {
    
    //HAGGLE_DBG2("Adding fragment of %s - sequenceNumber=%d fragmentId=%s",
    //        originalDataObjectId.c_str(), sequenceNumber, fragmentDataObjectRef->getIdStr());

    fragmentationdataobjectstorage.lock();
    if (!this->fragmentationsequencedataobjectstorageMapping->operator [](originalDataObjectId)) {
        Map<size_t,DataObjectRef>* sequenceMap = new Map<size_t,DataObjectRef>();
        this->fragmentationsequencedataobjectstorageMapping->operator [](originalDataObjectId) = sequenceMap;
    }
    fragmentationsequencedataobjectstorage_t fragmentationsequencedataobjectstorage =
            fragmentationsequencedataobjectstorageMapping->operator [](originalDataObjectId);
    fragmentationsequencedataobjectstorage->operator [](sequenceNumber) = fragmentDataObjectRef;
    fragmentationsequencedataobjectstorageMapping->operator [](originalDataObjectId) =
            fragmentationsequencedataobjectstorage;
    fragmentationdataobjectstorage.unlock();
}

const DataObjectRef FragmentationEncoderStorage::getFragment(string originalDataObjectId,
        size_t sequenceNumber) {

    //HAGGLE_DBG2("looking up originalDataObjectId=%s sequenceNumber=%d\n",
    //        originalDataObjectId.c_str(), sequenceNumber);

    fragmentationdataobjectstorage.lock();
    if (!fragmentationsequencedataobjectstorageMapping->operator [](originalDataObjectId)) {
        fragmentationdataobjectstorage.unlock();
        return NULL;
    }
    fragmentationsequencedataobjectstorage_t fragmentationsequencedataobjectstorage =
            fragmentationsequencedataobjectstorageMapping->operator [](originalDataObjectId);
    const DataObjectRef res = fragmentationsequencedataobjectstorage->operator [](sequenceNumber);
    fragmentationdataobjectstorage.unlock();
    return res;
}

void FragmentationEncoderStorage::deleteFromStorage(DataObjectRef parentDataObjectRef) {
    string dataObjectId = parentDataObjectRef->getIdStr();
    this->deleteFromStorageByDataObjectId(dataObjectId);
}

void FragmentationEncoderStorage::deleteFromStorageByDataObjectId(string parentDataObjectId) {
    HAGGLE_DBG2("Considering to remove data object %s from fragmentation encoder storage\n",
            parentDataObjectId.c_str());

    fragmentationdataobjectstorage.lock();
    if (this->fragmentationdataobjectstorage->operator [](parentDataObjectId)) {
        HAGGLE_DBG2("Removing %s from fragmentation data object storage\n",
                parentDataObjectId.c_str());
        this->fragmentationdataobjectstorage->erase(parentDataObjectId);
    }

    if (this->fragmentationsequencedataobjectstorageMapping->operator [](parentDataObjectId)) {
        HAGGLE_DBG("Removing all associated fragments for %s\n",
                parentDataObjectId.c_str());
        fragmentationsequencedataobjectstorage_t fragmentationsequencedataobjectstorageRef =
                this->fragmentationsequencedataobjectstorageMapping->operator [](parentDataObjectId);

        for (Map<size_t,DataObjectRef>::iterator it = fragmentationsequencedataobjectstorageRef->begin();
        it != fragmentationsequencedataobjectstorageRef->end();it++) {
            DataObjectRef fragmentDataObjectRef = it->second;
	    if(fragmentDataObjectRef.refcount() == 0) {
	      HAGGLE_ERR("Ignoring fragment data object of parent %s with zero refcount\n", parentDataObjectId.c_str()); // MOS - fixed (should not happen)
	    }
	    else {
	      // HAGGLE_DBG2("Removing fragment data object %s of parent %s\n", DataObject::idString(fragmentDataObjectRef).c_str(), parentDataObjectId.c_str()); // MOS
	      fragmentDataObjectRef->setStored(false);
	    }
        }
        fragmentationsequencedataobjectstorageRef->clear();

        HAGGLE_DBG("Removing data object %s from fragmentation sequence data object storage mapping\n",
                parentDataObjectId.c_str());
        this->fragmentationsequencedataobjectstorageMapping->erase(parentDataObjectId);
    }
    fragmentationdataobjectstorage.unlock();
}

void FragmentationEncoderStorage::trackSendEvent(string parentDataObjectId) {
    Timeval timeNow = Timeval::now();
    HAGGLE_DBG2("Tracking data object %s - time=%s\n",
            parentDataObjectId.c_str(), timeNow.getAsString().c_str());
    this->fragmentationSendEventTime->operator [](parentDataObjectId) = timeNow.getAsString();
}

void FragmentationEncoderStorage::ageOffFragments(double maxAgeFragment) {
    Timeval timeNow = Timeval::now();

    List<string> deleteTrackerTimeAfterIterating;

    for (Map<string,string>::iterator it = this->fragmentationSendEventTime->begin();
            it!=this->fragmentationSendEventTime->end();it++) {
        string parentDataObjectId = it->first;
        string timeSendEvent = it->second;
        Timeval timevalTimeSendEvent(timeSendEvent);
        if( timevalTimeSendEvent + maxAgeFragment < timeNow) {
            HAGGLE_DBG("Expiration reached - timesendevent=%s timenow=%s\n",timevalTimeSendEvent.getAsString().c_str(),timeNow.getAsString().c_str());
            this->deleteFromStorageByDataObjectId(parentDataObjectId);
            deleteTrackerTimeAfterIterating.push_back(parentDataObjectId);
        }
    }

    for(List<string>::iterator it=deleteTrackerTimeAfterIterating.begin();it!=deleteTrackerTimeAfterIterating.end();it++) {
    	string parentDataObjectId = (*it);
    	HAGGLE_DBG2("Deleting data object %s from tracker\n",parentDataObjectId.c_str());
    	this->fragmentationSendEventTime->erase(parentDataObjectId);
    }

}
