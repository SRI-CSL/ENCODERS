/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationDecoderStorage.h"
#include "Trace.h"

FragmentationDecoderStorage::FragmentationDecoderStorage(HaggleKernel* _haggleKernel, DataStore* _dataStore,
        DataObjectTypeIdentifierUtility* _dataObjectTypeIdentifierUtility) {
	this->haggleKernel = _haggleKernel;
    this->dataStore = _dataStore;
    this->dataObjectTypeIdentifierUtility = _dataObjectTypeIdentifierUtility;
    this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
    this->timeStampUtility = new TimeStampUtility();

    this->fragmentDecoderStorage = new fragmentdecoderstorage_t();
    this->dataobjectsequencenumbertracking = new dataobjectsequencenumbertracking_t();
    this->fragmentsTracker = new fragments_t();
    this->fragmentationDecoderLastUsedTime = new fragmentationDecoderLastUsedTime_t();
    this->reconstructedDataObjectTracker = new reconstructedDataObjectTracker_t();
}

FragmentationDecoderStorage::~FragmentationDecoderStorage() {
    if (this->fragmentationDataObjectUtility) {
        delete this->fragmentationDataObjectUtility;
        this->fragmentationDataObjectUtility = NULL;
    }
    if (this->timeStampUtility) {
        delete this->timeStampUtility;
        this->timeStampUtility = NULL;
    }
}

string generateTimeStamp() {

    char *timestamp = (char *) malloc(sizeof(char) * 80);
    memset(timestamp, 0, sizeof(timestamp));
    time_t ltime;
    ltime = time(NULL);
    struct tm *tm;
    tm = localtime(&ltime);
    struct timeval tv;
    gettimeofday(&tv, NULL);

    sprintf(timestamp, "%04d%02d%02d%02d%02d%02d%d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec);
    string stringtimestamp = timestamp;
    free(timestamp);
    return stringtimestamp;
}

bool FragmentationDecoderStorage::addFragment(DataObjectRef fragmentDataObject,
        size_t fragmentSize) {
    FragmentParentDataObjectInfo fragmentParentDataObjectInfo =
            this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(
                    fragmentDataObject);

    string parentDataObjectId = fragmentParentDataObjectInfo.dataObjectId;
    //size_t sequenceNumber = fragmentParentDataObjectInfo.sequenceNumberList;
    //List<string> sequenceNumberList = fragmentParentDataObjectInfo.sequenceNumberList;
    size_t parentDataObjectFileSize = fragmentParentDataObjectInfo.fileSize;

    sequencenumbertracking_t sequencenumbertracking =
            dataobjectsequencenumbertracking->operator [](parentDataObjectId);

    /*
     if (sequencenumbertracking[sequenceNumber]) {
     HAGGLE_DBG("already processed this sequence number\n");
     return true;
     }
     */

    FragmentDecoderStructRef fragmentDecoderStructRef = this->getFragmentDecoderStruct(
            parentDataObjectId);

    if (!fragmentDecoderStructRef) {
        FragmentDecoderStruct* fragmentDecoderStructPtr = new FragmentDecoderStruct();
        string timestamp = generateTimeStamp();
        string storagePath = HAGGLE_DEFAULT_STORAGE_PATH;
        string reconstructedFile = storagePath + "/" + fragmentParentDataObjectInfo.fileName + "."
                + timestamp + ".recon";
        fragmentDecoderStructPtr->filePath = reconstructedFile;
        fragmentDecoderStructPtr->fileSize = fragmentParentDataObjectInfo.fileSize;
        fragmentDecoderStructPtr->segmentsRemaining =
                this->fragmentationDataObjectUtility->calculateTotalNumberOfFragments(
                        fragmentParentDataObjectInfo.fileSize, fragmentSize);
        string parentDataObjectId = fragmentParentDataObjectInfo.dataObjectId;

        this->fragmentDecoderStorage->operator [](parentDataObjectId) = fragmentDecoderStructPtr;
        this->trackDecoderLastUsedTime(parentDataObjectId);
        fragmentDecoderStructRef = fragmentDecoderStructPtr;
        bool isAllocated = this->fragmentationDataObjectUtility->allocateFile(
                fragmentDecoderStructPtr->filePath, fragmentDecoderStructPtr->fileSize);
        if (!isAllocated) {
            HAGGLE_ERR("Unable to allocate file\n");
            return false;
        }
    }

    string parentToReconstructDataObjectFilePath = fragmentDecoderStructRef->filePath;

    string listCsv = "";

    for (List<string>::iterator it = fragmentParentDataObjectInfo.sequenceNumberList.begin();
            it != fragmentParentDataObjectInfo.sequenceNumberList.end(); it++) {
        string sequenceNumberString = (*it);
        listCsv = listCsv + sequenceNumberString + ",";
    }

    size_t fragmentationStartPosition = 0;
    for (List<string>::iterator it = fragmentParentDataObjectInfo.sequenceNumberList.begin();
            it != fragmentParentDataObjectInfo.sequenceNumberList.end(); it++) {
        string sequenceNumberString = (*it);
        char* endptr = NULL;
        size_t sequenceNumber = strtol(sequenceNumberString.c_str(), &endptr, 10);
        HAGGLE_DBG2("Processing fragment sequenceNumber=%d\n", sequenceNumber);

        FragmentationPositionInfo fragmentationPositionInfo =
                this->fragmentationDataObjectUtility->calculateFragmentationPositionInfo(
                        sequenceNumber, fragmentSize, parentDataObjectFileSize);

        if (sequencenumbertracking[sequenceNumber]) {
            HAGGLE_DBG2("Already processed this sequenceNumber=%d\n", sequenceNumber);
            fragmentationStartPosition = fragmentationStartPosition
                    + fragmentationPositionInfo.actualFragmentSize;
            continue;
        }

        HAGGLE_DBG2(
                "Processing sequenceNumber=%d parentStartPosition=%d fragmentStartPosition=%d readFragmentSize=%d listCsv=%s fragmentFileName=%s\n",
                sequenceNumber, fragmentationPositionInfo.startPosition, fragmentationStartPosition, fragmentationPositionInfo.actualFragmentSize, listCsv.c_str(), fragmentDataObject->getFilePath().c_str());

        bool isWriteFragment = this->fragmentationDataObjectUtility->storeFragmentToDataObject(
                parentToReconstructDataObjectFilePath, fragmentDataObject->getFilePath(),
                fragmentationPositionInfo.startPosition,
                fragmentationPositionInfo.actualFragmentSize, fragmentationStartPosition);

        if (!isWriteFragment) {
            HAGGLE_ERR("Unable to store fragment\n");
            return false;
        }

        sequencenumbertracking[sequenceNumber] = true;
        dataobjectsequencenumbertracking->operator [](parentDataObjectId) = sequencenumbertracking;

        fragmentDecoderStructRef->segmentsRemaining--;
	HAGGLE_STAT("Fragmentation segmentsRemaining=%d for %s\n",fragmentDecoderStructRef->segmentsRemaining,parentDataObjectId.c_str());
        fragmentationStartPosition = fragmentationStartPosition
                + fragmentationPositionInfo.actualFragmentSize;
    }

    this->fragmentDecoderStorage->operator [](parentDataObjectId) = fragmentDecoderStructRef;
    this->trackDecoderLastUsedTime(parentDataObjectId);
    string fragmentDataObjectId = fragmentDataObject->getIdStr();

    this->trackFragment(parentDataObjectId, fragmentDataObjectId);

    return true;
}

FragmentDecoderStructRef FragmentationDecoderStorage::getFragmentDecoderStruct(
        string dataObjectId) {
    FragmentDecoderStructRef found = this->fragmentDecoderStorage->operator [](dataObjectId);
    if (!found) {
        HAGGLE_DBG2("Data object %s not found in fragmentation decoder storage\n", dataObjectId.c_str());
    }
    else {
        this->trackDecoderLastUsedTime(dataObjectId);
    }
    return found;
}

bool FragmentationDecoderStorage::isCompleted(string dataObjectId) {
    FragmentDecoderStructRef found = this->getFragmentDecoderStruct(dataObjectId);
    if (!found) {
        return false;
    }

    bool isCompleted = found->segmentsRemaining == 0;
    // HAGGLE_DBG2("Reconstruction completed: %d\n", isCompleted);
    return isCompleted;
}

void FragmentationDecoderStorage::cleanUpPartialReconstructedObject(string parentDataObjectId) {
    bool isCompleted = this->isCompleted(parentDataObjectId);
    //do nothing
    if( isCompleted == true ) {
        return;
    }

    FragmentDecoderStructRef found = this->getFragmentDecoderStruct(parentDataObjectId);
    if (!found) {
        HAGGLE_ERR("Some reason associated struct is not found for dataobjectid %s\n",parentDataObjectId.c_str());
        return;
    }


    if( remove( found->filePath.c_str() ) != 0 ) {
        HAGGLE_ERR("Error removing file %s for dataobjectid %s\n",found->filePath.c_str(),parentDataObjectId.c_str());
    }
    else {
        HAGGLE_DBG2("Removed file %s for dataobjectid %s\n",found->filePath.c_str(),parentDataObjectId.c_str());
    }


}

void FragmentationDecoderStorage::disposeDataObject(const unsigned char* dataObjectId) {
    string dataObjectIdHex = this->dataObjectTypeIdentifierUtility->convertDataObjectIdToHex(
            dataObjectId);
    HAGGLE_DBG("Requesting to delete data object %s\n", dataObjectIdHex.c_str());
    this->dataStore->deleteDataObject(dataObjectId, false);
}

void FragmentationDecoderStorage::disposeFragment(DataObjectRef fragmentDataObjectRef) {
    HAGGLE_DBG("Disposing fragment %s\n", fragmentDataObjectRef->getIdStr());
    string fragmentDataObjectId = fragmentDataObjectRef->getIdStr();
    DataObjectIdRef fragmentId =
            this->dataObjectTypeIdentifierUtility->convertDataObjectIdStringToDataObjectIdType(
                    fragmentDataObjectId);
    const unsigned char* fragmentIdPtr = fragmentId.c_str();
    string dataObjectIdHex = this->dataObjectTypeIdentifierUtility->convertDataObjectIdToHex(
            fragmentIdPtr);
    this->disposeDataObject(fragmentIdPtr);
}

void FragmentationDecoderStorage::disposeAssociatedFragments(const string originalDataObjectId) {
    fragmentsidlistref fragmentsList = this->getFragments(originalDataObjectId);
    if(fragmentsList->empty()) return;
    HAGGLE_DBG("Disposing fragments of parent data object %s\n", originalDataObjectId.c_str());

    for (List<string>::iterator it = fragmentsList->begin(); it != fragmentsList->end(); it++) {
        string fragmentDataObjectId = (*it);
        DataObjectIdRef fragmentId =
                this->dataObjectTypeIdentifierUtility->convertDataObjectIdStringToDataObjectIdType(
                        fragmentDataObjectId);
        this->disposeDataObject(fragmentId.c_str());
    }

    this->fragmentsTracker->erase(originalDataObjectId);
}

void FragmentationDecoderStorage::addCodedBlockId(string originalDataObjectId,
        const string fragmentDataObjectId) {
    fragmentsidlistref fragmentsList = this->getFragments(originalDataObjectId);
    size_t beforeSize = this->fragmentsTracker->operator [](originalDataObjectId)->size();
    fragmentsList->push_front(fragmentDataObjectId);
    this->fragmentsTracker->operator [](originalDataObjectId) = fragmentsList;
    size_t afterSize = this->fragmentsTracker->operator [](originalDataObjectId)->size();
    HAGGLE_DBG2("Added fragment for parent data object %s - beforesize=%d aftersize=%d\n",
		originalDataObjectId.c_str(), beforeSize, afterSize);
}

void FragmentationDecoderStorage::trackFragment(const string originalDataObjectId,
        const string fragmentDataObjectId) {
    HAGGLE_DBG2( "Tracking fragment %s for parent data object %s\n",
		 fragmentDataObjectId.c_str(), originalDataObjectId.c_str());

    this->addCodedBlockId(originalDataObjectId, fragmentDataObjectId);
}

fragmentsidlistref FragmentationDecoderStorage::getFragments(string originalDataObjectId) {
    if (!this->fragmentsTracker->operator [](originalDataObjectId)) {
        List<string>* fragmentList = new List<string>();
        this->fragmentsTracker->operator [](originalDataObjectId) = fragmentList;
    }
    HAGGLE_DBG2("Retrieving fragments for data object %s - listsize=%d\n",
            originalDataObjectId.c_str(), this->fragmentsTracker->operator [](originalDataObjectId)->size());
    return this->fragmentsTracker->operator [](originalDataObjectId);
}

void FragmentationDecoderStorage::trackReconstructedDataObject(const string dataObjectId) {
    Timeval t = Timeval::now();
    string timeNow = t.getAsString();
    HAGGLE_DBG2("Tracking reconstructed data object %s - timeReconstructed=%s\n", dataObjectId.c_str(), timeNow.c_str());
    this->reconstructedDataObjectTracker->operator [](dataObjectId) = timeNow;
}

double FragmentationDecoderStorage::getResendReconstructedDelay(const string dataObjectId,
        double doubleResendReconstructedDelay) {
    Timeval timeValTimeNow = Timeval::now();
    Timeval timeValResendReconstructedDelay(doubleResendReconstructedDelay);

    string stringReconstructedTime = reconstructedDataObjectTracker->operator[](dataObjectId);
    HAGGLE_DBG2("Computing resend delay for data object %s - timeReconstructed=%s\n",
            dataObjectId.c_str(), stringReconstructedTime.c_str());
    Timeval timeReconstructed(timeValTimeNow);
    if (stringReconstructedTime.size() > 0 && stringReconstructedTime.c_str() != NULL) {
        Timeval valToSet(stringReconstructedTime);
        timeReconstructed = valToSet;

        timeReconstructed += timeValResendReconstructedDelay;
        HAGGLE_DBG2("timeReconstructed+delay=%s\n", timeReconstructed.getAsString().c_str());
    } else {
        HAGGLE_DBG2("timeReconstructed=%s\n", timeReconstructed.getAsString().c_str());
    }

    double calculatedResendDelay = 0.0;

    if (timeReconstructed <= timeValTimeNow) {
        calculatedResendDelay = -100.0;
    } else {
        HAGGLE_DBG2("Using timeReconstructed=%s timeValTimeNow=%s\n",
                timeReconstructed.getAsString().c_str(), timeValTimeNow.getAsString().c_str());
        calculatedResendDelay = (timeReconstructed - timeValTimeNow).getTimeAsSecondsDouble();
    }

    HAGGLE_DBG2("Result: calculatedResendDelay=%f given timenow=%s doubleResendReconstructedDelay=%f\n",
            calculatedResendDelay, timeValTimeNow.getAsString().c_str(), doubleResendReconstructedDelay);
    return calculatedResendDelay;
}

void FragmentationDecoderStorage::deleteFromStorage(DataObjectRef parentDataObjectRef) {
    string dataObjectId = parentDataObjectRef->getIdStr();
    HAGGLE_DBG2("Considering to remove data object %s from fragmentation decoder storage\n",dataObjectId.c_str());

    this->disposeAssociatedFragments(dataObjectId);

    if(this->fragmentDecoderStorage->operator [](dataObjectId)) {
        HAGGLE_DBG2("Removing data object %s from fragmentation decoder storage\n",dataObjectId.c_str());
        this->fragmentDecoderStorage->erase(dataObjectId);
    }

    HAGGLE_DBG2("Removing data object %s from data object sequence number tracker\n",dataObjectId.c_str());
    this->dataobjectsequencenumbertracking->erase(dataObjectId);

    HAGGLE_DBG2("Removing data object %s from reconstructed data object tracker\n",dataObjectId.c_str());
    this->reconstructedDataObjectTracker->erase(dataObjectId);

}

void FragmentationDecoderStorage::ageOffDecoder(double maxAgeDecoder) {
    HAGGLE_DBG2("calling FragmentationDecoderStorage::ageOffDecoder\n");

    Timeval timeNow = Timeval::now();

    for (Map<string,string>::iterator it = this->fragmentationDecoderLastUsedTime->begin();
            it!=this->fragmentationDecoderLastUsedTime->end();it++) {
        string parentDataObjectId = it->first;
        string timeSendEvent = it->second;
        Timeval timevalTimeSendEvent(timeSendEvent);
        if( timevalTimeSendEvent + maxAgeDecoder < timeNow) {
            HAGGLE_DBG2("Expiration reached - timesendevent=%s timenow=%s\n",timevalTimeSendEvent.getAsString().c_str(),timeNow.getAsString().c_str());
            this->deleteDecoderFromStorageByDataObjectId(parentDataObjectId);
            this->disposeAssociatedFragments(parentDataObjectId);
        }
    }
}

void FragmentationDecoderStorage::deleteDecoderFromStorageByDataObjectId(string parentDataObjectId) {
    HAGGLE_DBG2("Cleaning up partial reconstructed dataobject for %s\n",parentDataObjectId.c_str());
    this->cleanUpPartialReconstructedObject(parentDataObjectId);

    HAGGLE_DBG2("Removing decoder for data object %s from networkcoding decoder storage\n",
            parentDataObjectId.c_str());
    this->fragmentDecoderStorage->erase(parentDataObjectId);

    HAGGLE_DBG2("Clean up last used time tracking for data object %s from networkcoding decoder storage\n",
            parentDataObjectId.c_str());
    this->fragmentationDecoderLastUsedTime->erase(parentDataObjectId);
}

void FragmentationDecoderStorage::trackDecoderLastUsedTime(string dataObjectId) {
    Timeval timeNow = Timeval::now();
    HAGGLE_DBG2("Tracking last used decoder time for data object %s - time=%s\n",
            dataObjectId.c_str(), timeNow.getAsString().c_str());
    this->fragmentationDecoderLastUsedTime->operator [](dataObjectId) = timeNow.getAsString();
}

void FragmentationDecoderStorage::onRetrievedFragmentFromDataStore(Event* event) {
    DataObjectRef dObj;
    if (!event || !event->hasData()) {
        // this can occur when the data object is no longer in the DB
        HAGGLE_DBG("No data in callback\n");
        return;
    }
    dObj = event->getDataObject();

    Event* incomingFragmentationToNetworkCodedBlock = new Event(EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION, dObj, 0, 0.01);
    this->haggleKernel->addEvent(incomingFragmentationToNetworkCodedBlock);
}

void FragmentationDecoderStorage::convertFragmentsToNetworkCodedBlocks(string parentDataObjectId) {
    fragmentsidlistref fragmentsList = this->getFragments(parentDataObjectId);
    if(fragmentsList->empty()) return;

    for (List<string>::iterator it = fragmentsList->begin(); it != fragmentsList->end(); it++) {
        string fragmentDataObjectId = (*it);
        DataObjectId_t id;
        DataObject::idStrToId(fragmentDataObjectId, id);

#define __CLASS__ FragmentationDecoderStorage

        this->dataStore->retrieveDataObject(id,newEventCallback(onRetrievedFragmentFromDataStore));
    }
}
