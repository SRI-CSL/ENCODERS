/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "FragmentationEncoderService.h"
#include "Trace.h"

#include "networkcoding/NetworkCodingConstants.h"
#include "fragmentation/FragmentationConstants.h"

FragmentationEncoderService::FragmentationEncoderService(FragmentationFileUtility *_fragmentationFileUtility,
        FragmentationConfiguration* _fragmentationConfiguration,
        FragmentationEncoderStorage* _fragmentationEncoderStorage) {
    this->fragmentationFileUtility = _fragmentationFileUtility;
    this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
    this->fragmentationConfiguration = _fragmentationConfiguration;
    this->fragmentationEncoderStorage = _fragmentationEncoderStorage;
}

FragmentationEncoderService::~FragmentationEncoderService() {
    if (this->fragmentationDataObjectUtility) {
        delete this->fragmentationDataObjectUtility;
        this->fragmentationDataObjectUtility = NULL;
    }
}

DataObjectRef FragmentationEncoderService::encodeDataObject(const DataObjectRef originalDataObject, size_t startIndex,
        size_t fragmentSize, NodeRef nodeRef) {

    const char* filePathOriginalDataObject = fragmentationDataObjectUtility->getFilePath(originalDataObject).c_str();

    string originalFileName = fragmentationDataObjectUtility->getFileName(originalDataObject);

    string fragmentFile = this->fragmentationFileUtility->createFragmentFileName(originalFileName.c_str());
    HAGGLE_DBG2("Creating fragment file name %s\n", fragmentFile.c_str());

    FILE* originaldataObjectFilePointer = fopen(filePathOriginalDataObject, "rb");
    if (NULL == originaldataObjectFilePointer) {
        HAGGLE_ERR("Unable to open file %s\n", filePathOriginalDataObject);
        return NULL;
    }

    FILE* fragmentFilePointer = fopen(fragmentFile.c_str(), "wb");
    if (NULL == fragmentFilePointer) {
        HAGGLE_ERR("Error opening fragment file %s\n", fragmentFile.c_str());
	fclose(originaldataObjectFilePointer);
        return NULL;
    }

    this->fragmentationEncoderStorage->addDataObject(originalDataObject->getIdStr(), originalDataObject);

    fseek(originaldataObjectFilePointer, 0, SEEK_END);
    size_t fileSize = ftell(originaldataObjectFilePointer);

    size_t dataLen = fragmentationDataObjectUtility->getFileLength(originalDataObject);
    size_t totalNumberOfFragments = this->fragmentationDataObjectUtility->calculateTotalNumberOfFragments(dataLen,
            fragmentSize);
    int* fragmentSequenceNumberList = this->getIndexesOfFragmentsToCreate(totalNumberOfFragments);

    string sequenceNumberListCsv = "";

    size_t numberOfFragmentsToWrite = this->fragmentationConfiguration->getNumberFragmentsPerDataObject();
    if (numberOfFragmentsToWrite > totalNumberOfFragments) {
        HAGGLE_DBG2( "numberOfFragmentsToWrite=%d > totalNumberOfFragments=%d\n",
                numberOfFragmentsToWrite, totalNumberOfFragments);
        numberOfFragmentsToWrite = totalNumberOfFragments;
    }
    for (int count = 0; count < numberOfFragmentsToWrite && startIndex < totalNumberOfFragments;
            count++, startIndex++) {
        size_t sequenceNumberToWrite = fragmentSequenceNumberList[startIndex];
        HAGGLE_DBG2( "sequenceNumberToWrite=%d startIndex=%d totalNumberOfFragments=%d\n",
                sequenceNumberToWrite, startIndex, totalNumberOfFragments);
        FragmentationPositionInfo fragmentationPositionInfo =
                this->fragmentationDataObjectUtility->calculateFragmentationPositionInfo(sequenceNumberToWrite,
                        fragmentSize, fileSize);
        char intbuffer[33];
        memset(intbuffer, 0, sizeof(intbuffer));
        sprintf(intbuffer, "%d", sequenceNumberToWrite);
        sequenceNumberListCsv = sequenceNumberListCsv + intbuffer + ",";

        fseek(originaldataObjectFilePointer, fragmentationPositionInfo.startPosition, SEEK_SET);
        char* buffer = new char[fragmentationPositionInfo.actualFragmentSize + 1];
        memset(buffer, 0, fragmentationPositionInfo.actualFragmentSize + 1);
        size_t bytesRead = fread(buffer, 1, fragmentationPositionInfo.actualFragmentSize,
                originaldataObjectFilePointer);
        if (ferror(originaldataObjectFilePointer)) {
            HAGGLE_ERR("Error reading file %s\n", filePathOriginalDataObject);
	    delete[] buffer;
	    delete[] fragmentSequenceNumberList;
	    fclose(fragmentFilePointer);
	    fclose(originaldataObjectFilePointer);
            return NULL;
        }
        HAGGLE_DBG2( "bytesRead=%d read for sequenceNumber=%d actualFragmentSize=%d startPosition=%d\n",
                bytesRead, sequenceNumberToWrite, fragmentationPositionInfo.actualFragmentSize, fragmentationPositionInfo.startPosition);

        /*write to fragment*/
        fwrite(buffer, fragmentationPositionInfo.actualFragmentSize, 1, fragmentFilePointer);
        if (ferror(fragmentFilePointer)) {
            HAGGLE_ERR("Error writing file %s\n", fragmentFile.c_str());
	    delete[] buffer;
	    delete[] fragmentSequenceNumberList;
	    fclose(fragmentFilePointer);
	    fclose(originaldataObjectFilePointer);
            return NULL;
        }
        HAGGLE_DBG2("wrote fragment file %s - sequenceNumber=%d\n", fragmentFile.c_str(), sequenceNumberToWrite);

        delete[] buffer;
    }
    delete[] fragmentSequenceNumberList;
    fclose(fragmentFilePointer);
    fclose(originaldataObjectFilePointer);

    sequenceNumberListCsv = sequenceNumberListCsv.substr(0, sequenceNumberListCsv.size() - 1);
    HAGGLE_DBG2("sequenceNumberListCsv=%s\n", sequenceNumberListCsv.c_str());

    DataObjectRef fragmentDataObjectRef = DataObject::create(fragmentFile, fragmentationDataObjectUtility->getFileName(originalDataObject));

    if(!fragmentDataObjectRef) {
      HAGGLE_ERR("Unable to create data object for file %s (%s)\n", fragmentFile.c_str(), originalDataObject->getFileName().c_str());
      return NULL;
    }

    bool addedAttributes = this->addAttributes(originalDataObject, fragmentDataObjectRef, sequenceNumberListCsv);

    if (!addedAttributes) {
        HAGGLE_ERR("Unable to add fragment attributes\n");
        return NULL;
    }

    fragmentDataObjectRef->setStored(true);

    HAGGLE_DBG2("Created fragment file %s for parent data object %s\n",
            fragmentDataObjectRef->getFilePath().c_str(), originalDataObject->getIdStr());

    return fragmentDataObjectRef;
}

bool FragmentationEncoderService::addAttributes(DataObjectRef originalDataObject, DataObjectRef fragmentDataObject,
        string sequenceNumberListCsv) {

    //copy attributes. though eventually will use rich metadata?
    const Attributes* originalAttributes = originalDataObject->getAttributes();
    for (Attributes::const_iterator it = originalAttributes->begin(); it != originalAttributes->end(); it++) {
        const Attribute attr = (*it).second;
        bool addAttribute = fragmentDataObject->addAttribute(attr);
        if (!addAttribute) {
            HAGGLE_ERR("unable to add attribute\n");
            return false;
        }
    }

    //add sequence number attribute
//	char sequenceBuffer[33];
//	memset(sequenceBuffer, 0, sizeof(sequenceBuffer));
//	sprintf(sequenceBuffer, "%d", sequenceNumber);
//	HAGGLE_DBG("stringSequenceNumber=%s\n", sequenceBuffer);
//	bool addedSequenceNUmber = fragmentDataObject->addAttribute(
//			HAGGLE_ATTR_FRAGMENTATION_SEQUENCE_NUMBER, sequenceBuffer, 0);
//	if (!addedSequenceNUmber) {
//		HAGGLE_ERR("unable to add addedSequenceNUmber attribute\n");
//		return false;
//	}

    HAGGLE_DBG2("stringSequenceNumber=%s\n", sequenceNumberListCsv.c_str());
    bool addedSequenceNumber = fragmentDataObject->addAttribute(HAGGLE_ATTR_FRAGMENTATION_SEQUENCE_NUMBER,
            sequenceNumberListCsv, 0);
    if (!addedSequenceNumber) {
        HAGGLE_ERR("Unable to add sequence number attribute\n");
        return false;
    }

    //add attribute to indicate data object is fragmentation block
    bool addedIsFragmentationCodedAttribute = fragmentDataObject->addAttribute(HAGGLE_ATTR_FRAGMENTATION_NAME, "TRUE",
            0);
    if (!addedIsFragmentationCodedAttribute) {
        HAGGLE_ERR("Unable to add fragmentation attribute\n");
        return false;
    }

    //add original data len attribute
    char lenBuffer[33];
    memset(lenBuffer, 0, sizeof(lenBuffer));
    int len = fragmentationDataObjectUtility->getFileLength(originalDataObject);
    if(len == 0) {
        HAGGLE_ERR("Orignal data len is zero - file already deleted\n");
        return false;
    }
    sprintf(lenBuffer, "%d", len);
    bool addedDataLenAttribute = fragmentDataObject->addAttribute(HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_LEN, lenBuffer,
            0);
    if (!addedDataLenAttribute) {
        HAGGLE_ERR("Unable to add original data len attribute\n");
        return false;
    }

    //add dataobject id
    const char* originalId = originalDataObject->getIdStr();
    string originalStringId = originalId;
    bool addedIdAttribute = fragmentDataObject->addAttribute(HAGGLE_ATTR_FRAGMENTATION_PARENT_DATAOBJECT_ID,
            originalStringId, 0);
    if (!addedIdAttribute) {
        HAGGLE_ERR("Unable to add original data object id attribute\n");
        return false;
    }

    //add dataobject name
    string originalName = fragmentationDataObjectUtility->getFileName(originalDataObject);
    HAGGLE_DBG2("Add original name %s as attribute\n", originalName.c_str());
    bool addedNameAttribute = fragmentDataObject->addAttribute(HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_NAME, originalName,
            0);
    if (!addedNameAttribute) {
        HAGGLE_ERR("Unable to add original name attribute\n");
        return false;
    }

    //add create time
    string originalCreateTime = originalDataObject->getCreateTime().getAsString();
    HAGGLE_DBG2("Add original create time %s as attribute\n", originalCreateTime.c_str());
    bool addedCreatedTimeAttribute = fragmentDataObject->addAttribute(HAGGLE_ATTR_FRAGMENTATION_PARENT_CREATION_TIME,
            originalCreateTime, 0);
    if (!addedCreatedTimeAttribute) {
        HAGGLE_ERR("Unable to add original create time attribute\n");
        return false;
    }

    //set create time of fragment to same create time as parent so fragment data object ids can match up
    Timeval createTime(originalCreateTime);
    fragmentDataObject->setCreateTime(createTime);

    if(originalDataObject->getSignature()) { // MOS
      //add signee
      string parentSignee = originalDataObject->getSignee();
      HAGGLE_DBG2("Add original signee %s as attribute\n",parentSignee.c_str());
      bool addedSigneeAttribute = fragmentDataObject->
	addAttribute(HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_SIGNEE,parentSignee,0);
      if(!addedSigneeAttribute) {
        HAGGLE_ERR("Unable to add original signee attribute\n");
        return false;
      }
      
      //add signature
      char *base64_signature = NULL;
      if (base64_encode_alloc((char *)originalDataObject->getSignature(), originalDataObject->getSignatureLength(), &base64_signature) <= 0) {
        HAGGLE_ERR("Unable to generate base64 encoded signature\n");
        return false;
      }
      string parentSignature = base64_signature;
      HAGGLE_DBG2("Add original signature %s as attribute\n",parentSignature.c_str());
      bool addedSignatureAttribute = fragmentDataObject->
	addAttribute(HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_SIGNATURE,parentSignature,0);
      if(!addedSignatureAttribute) {
        HAGGLE_ERR("Unable to add original signature attribute\n");
        return false;
      }
      if(base64_signature) {
        free(base64_signature);
        base64_signature = NULL;
      }
    }

    return true;
}

List<DataObjectRef> FragmentationEncoderService::getAllFragmentsForDataObject(DataObjectRef originalDataObject,
        size_t fragmentSize, NodeRef nodeRef) {

    size_t dataLen = fragmentationDataObjectUtility->getFileLength(originalDataObject);
    size_t totalNumberOfFragments = this->fragmentationDataObjectUtility->calculateTotalNumberOfFragments(dataLen,
            fragmentSize);
    size_t numberOfFragmentsPerDataObject = this->fragmentationConfiguration->getNumberFragmentsPerDataObject();

    List<DataObjectRef> dataObjectRefList;
    List<DataObjectRef> dataObjectsMissing;

    string originalDataObjectId = originalDataObject->getIdStr();

    for (int startIndex = 0; startIndex < totalNumberOfFragments; startIndex += numberOfFragmentsPerDataObject) {
        HAGGLE_DBG2("Fragmention startIndex=%d\n", startIndex);
        DataObjectRef fragmentDataObjectRef = this->fragmentationEncoderStorage->getFragment(
        		originalDataObjectId.c_str(), startIndex);
        if (!fragmentDataObjectRef) {
            HAGGLE_DBG2("Generating fragment of parent data object %s with startIndex=%d\n", originalDataObjectId.c_str(), startIndex);
            fragmentDataObjectRef = this->encodeDataObject(originalDataObject, startIndex, fragmentSize, nodeRef);
	    if(fragmentDataObjectRef) // MOS
	      this->fragmentationEncoderStorage->addFragment(originalDataObjectId,startIndex,fragmentDataObjectRef);
        }
	if(fragmentDataObjectRef) { //MOS
	  bool alreadySent = nodeRef->has(fragmentDataObjectRef);
	  if (!alreadySent) {
            HAGGLE_DBG2("Fragment with startIndex=%d has not been sent/received yet\n", startIndex);
            dataObjectsMissing.push_front(fragmentDataObjectRef);
	  }
	  else {
	    HAGGLE_DBG2("Fragment with startIndex=%d has already been sent/received\n", startIndex);
	  }
	}
    }

    if (dataObjectsMissing.size() > 0) {
        int whichToSend = rand() % dataObjectsMissing.size();
        HAGGLE_DBG("Selecting random fragment data object %d out of %d\n", whichToSend, dataObjectsMissing.size());
        for (List<DataObjectRef>::const_iterator it = dataObjectsMissing.begin(); it != dataObjectsMissing.end();
                it++) {
            if (whichToSend == 0) {
                dataObjectRefList.push_front(*it);
                break;
            }
            whichToSend--;
        }
    }

    /*
     int* listIndex = this->getIndexesOfFragmentsToCreate(
     totalNumberOfFragments);
     for (int startIndex = 0; startIndex < totalNumberOfFragments; startIndex +=
     numberOfFragmentsPerDataObject) {
     HAGGLE_DBG("startIndex=%d\n", startIndex);
     DataObjectRef fragmentDataObjectRef = this->encodeDataObject(
     originalDataObject, startIndex, fragmentSize, nodeRef);
     bool alreadySent = nodeRef->getBloomfilter()->has(
     fragmentDataObjectRef);
     if (!alreadySent) {
     HAGGLE_DBG("not already sent startIndex=%d\n", startIndex);
     dataObjectRefList.push_front(fragmentDataObjectRef);
     delete[] listIndex;
     return dataObjectRefList;
     }
     HAGGLE_DBG("already sent startIndex=%d\n");
     }
     delete[] listIndex;
     */

    return dataObjectRefList;
}

int* FragmentationEncoderService::getIndexesOfFragmentsToCreate(size_t totalNumberOfFragments) {
    int* indexes = new int[totalNumberOfFragments];
    memset(indexes, 0, sizeof(int)*totalNumberOfFragments);
    for (int x = 0; x < totalNumberOfFragments; x++) {
        indexes[x] = x + 1;
    }

    //using fisher-yates shuffle
    /* disable shuffle
     for (int i = totalNumberOfFragments - 1; i > 0; i--) {
     //0 <= j <= i
     int upperBound = i;
     int j = rand() % upperBound + 1;
     int tmp = indexes[i];
     indexes[i] = indexes[j];
     indexes[j] = tmp;
     }
     */
    return indexes;
}

