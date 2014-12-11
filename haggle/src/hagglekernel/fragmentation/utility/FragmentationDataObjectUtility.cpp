/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "FragmentationDataObjectUtility.h"
#include "Trace.h"

#include "networkcoding/NetworkCodingConstants.h"
#include "fragmentation/FragmentationConstants.h"

#include "networkcoding/NetworkCodingConstants.h"
#include "stringutils/CSVUtility.h"


FragmentationDataObjectUtility::FragmentationDataObjectUtility() {
    this->fragmentationConfiguration = new FragmentationConfiguration();
    this->dataObjectTypeIdentifierUtility = new DataObjectTypeIdentifierUtility();
    this->networkCodingDataObjectUtility = new NetworkCodingDataObjectUtility();
}

FragmentationDataObjectUtility::~FragmentationDataObjectUtility() {
    if(this->networkCodingDataObjectUtility) {
        delete this->networkCodingDataObjectUtility;
        this->networkCodingDataObjectUtility = NULL;
    }
    if (this->fragmentationConfiguration) {
        delete this->fragmentationConfiguration;
        this->fragmentationConfiguration = NULL;
    }
    if (this->dataObjectTypeIdentifierUtility) {
        delete this->dataObjectTypeIdentifierUtility;
        this->dataObjectTypeIdentifierUtility = NULL;
    }
}

bool FragmentationDataObjectUtility::isFragmentationDataObject(
		DataObjectRef dataObject) {

	if (!dataObject) {
		HAGGLE_DBG2("Unexpected null data object\n");
		return false;
	}

        HAGGLE_DBG2("Checking if data object %s is a fragment (but not a block)\n", dataObject->getIdStr());

	const Attribute* attributeIsFragmentation = new Attribute(
			HAGGLE_ATTR_FRAGMENTATION_NAME, "TRUE");
	bool hasFragmentationAttribute = dataObject->hasAttribute(
			*attributeIsFragmentation);
	delete attributeIsFragmentation;

	HAGGLE_DBG2("Data object %s has fragmentation attribute: %d\n",
		    dataObject->getIdStr(), hasFragmentationAttribute);

	if(!hasFragmentationAttribute) return false;

	// MOS
	const Attribute* attributeIsNetworkCoding = new Attribute(HAGGLE_ATTR_NETWORKCODING_NAME, "TRUE");
	bool hasNetworkCodingAttribute = dataObject->hasAttribute(*attributeIsNetworkCoding);
	delete attributeIsNetworkCoding;

	HAGGLE_DBG2("Data object %s has network coding attribute: %d\n",
		    dataObject->getIdStr(), hasNetworkCodingAttribute);

	if(hasNetworkCodingAttribute) return false;

	return true;
}

bool FragmentationDataObjectUtility::shouldBeFragmentedCheckTargetNode(DataObjectRef dataObject,NodeRef targetNode) {
	bool dataObjectShouldBeFragmented= this->shouldBeFragmented(dataObject,targetNode);
	//short circuit and return early
	if(!dataObjectShouldBeFragmented) {
		return dataObjectShouldBeFragmented;
	}

	bool isExists = false;

	if( this->fragmentationConfiguration->isEnabledForAllTargets()) {
		HAGGLE_DBG2("Fragmentation turned on for all targets\n");
		isExists = true;
	}
	else {
		std::vector<std::string> whitelistTargetNodes = this->fragmentationConfiguration->getWhitelistTargetNodeNames();
		std::string targetNodeName = targetNode->getName().c_str();
		isExists = itemInVector(targetNodeName,whitelistTargetNodes);
		HAGGLE_DBG2("IsTurning on fragmentation for node %s is %d\n",targetNodeName.c_str(),isExists);
	}

	return isExists;

}

bool FragmentationDataObjectUtility::shouldBeFragmented(DataObjectRef dataObject,NodeRef targetNode) {

    if (!dataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return false;
    }

    HAGGLE_DBG2("Checking if data object %s should be fragmented - isControlMessage=%d isNodeDescription=%d isThisNodeDescription=%d\n",
            dataObject->getIdStr(), dataObject->isControlMessage(), dataObject->isNodeDescription(), dataObject->isThisNodeDescription());
    if (!this->fragmentationConfiguration->isFragmentationEnabled(dataObject,targetNode)) {
        HAGGLE_DBG2( "Fragmentation not enabled, so not fragmenting data object\n");
        return false;
    }

    if (dataObject->isControlMessage() || dataObject->isNodeDescription()
            || dataObject->isThisNodeDescription()) {
        HAGGLE_DBG2("Data object %s should not be fragmented\n", dataObject->getIdStr());
        return false;
    }

    bool isFragmentationCodedObject = this->isFragmentationDataObject(dataObject);
    if (isFragmentationCodedObject) {
        HAGGLE_DBG2("Data object %s is already fragmented\n", dataObject->getIdStr());
        return false;
    }

    else if (this->dataObjectTypeIdentifierUtility->isHaggleIpc(dataObject)) {
        HAGGLE_DBG2("Haggle IPC data object %s should not be fragmentated\n", dataObject->getIdStr());
        return false;
    } 
    else if (this->dataObjectTypeIdentifierUtility->hasNoFileAssociated(dataObject)) {
        return false;
    }
    else if(this->networkCodingDataObjectUtility->isNetworkCodedDataObject(dataObject)) {
        HAGGLE_DBG2("Data object %s is already network coded shouldnt be fragmented\n",dataObject->getIdStr());
        return false;
    }

    //break down if statements as we make an io call to get the file size
    // FILE *filePointer = fopen(dataObject->getFilePath().c_str(), "rb");
    // fseek(filePointer, 0, SEEK_END);
    // long fileSize = ftell(filePointer);
    // fclose(filePointer);
    long fileSize = getFileLength(dataObject); // MOS - this is faster - also avoids possible error on fopen

    size_t minFileSize = this->fragmentationConfiguration->getMinimumFileSize();
    size_t fragmentSize = this->fragmentationConfiguration->getFragmentSize();
    if (fileSize < minFileSize) {
        HAGGLE_DBG2("No fragmentation - filesize=%d smaller than minFileSize=%d\n", fileSize, minFileSize);
        return false;
    } else if (fileSize < fragmentSize) {
        HAGGLE_DBG2("No fragmentation - filesize=%d smaller than fragmentSize=%d\n", fileSize, fragmentSize);
        return false;
    } else {
        HAGGLE_DBG2("Data object %s should be fragmented\n", dataObject->getIdStr());
        return true;
    }

}

const string FragmentationDataObjectUtility::getOriginalDataObjectId(
        DataObjectRef fragmentationDataObject) {
    if (!fragmentationDataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return "";
    }

    const Attribute* attributeDataObjectId = fragmentationDataObject->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_PARENT_DATAOBJECT_ID, "*", 1.0);
    if (NULL == attributeDataObjectId) {
        HAGGLE_DBG2("No parent data object id attribute found\n");
        return (char*) NULL;
    }
    const string& originalDataObjectId = attributeDataObjectId->getValue();
    HAGGLE_DBG2("Found parent data object id %s\n", originalDataObjectId.c_str());

    if (originalDataObjectId.c_str() == NULL || originalDataObjectId.empty()
            || originalDataObjectId.length() < 1) {
        HAGGLE_DBG2("Invalid parent data object for block %s\n",
                fragmentationDataObject->getIdStr());
        return (char*) NULL;
    }

    return originalDataObjectId;
}

const string FragmentationDataObjectUtility::checkAndGetOriginalDataObjectId(
        DataObjectRef fragmentationDataObject) {
    if (this->isFragmentationDataObject(fragmentationDataObject)) {
        const string& originalDataObjectId = this->getOriginalDataObjectId(fragmentationDataObject);
        if (originalDataObjectId.c_str() == NULL || originalDataObjectId.empty()
                || originalDataObjectId.length() < 1) {
	    HAGGLE_DBG2("returning null\n");
            return (char*) NULL;
        }
        return originalDataObjectId;
    } else {
        HAGGLE_DBG2("returning null\n");
        return (char*) NULL;
    }
}

DataObjectIdRef FragmentationDataObjectUtility::convertDataObjectIdStringToDataObjectIdType(
        string dataObjectId) {
    unsigned char* binaryDataObjectId = new unsigned char[DATAOBJECT_ID_LEN];
    memset(binaryDataObjectId, 0, DATAOBJECT_ID_LEN);
    unsigned char* binaryDataObjectId_ptr = binaryDataObjectId;

    size_t lenstring = dataObjectId.size();
    const char* dataObjectId_ptr = dataObjectId.c_str();
    for (unsigned int i = 0; i < lenstring; i += 2) {
        unsigned int number;
        sscanf(dataObjectId_ptr + i, "%02x", &number);
        binaryDataObjectId_ptr[0] = static_cast<unsigned char>(number);
        binaryDataObjectId_ptr = binaryDataObjectId_ptr + 1;
    }

    this->convertDataObjectIdToHex(binaryDataObjectId);

    DataObjectIdRef haggleDataObjectId(binaryDataObjectId, DATAOBJECT_ID_LEN);
    delete[] binaryDataObjectId;

    return haggleDataObjectId;
}

string FragmentationDataObjectUtility::convertDataObjectIdToHex(
        const unsigned char* dataObjectIdUnsignedChar) {
    string dataObjectIdhex;
    int len = 0;
    char idStr[MAX_DATAOBJECT_ID_STR_LEN];
    // Generate a readable string of the Id
    for (int i = 0; i < DATAOBJECT_ID_LEN; i++) {
        len += sprintf(idStr + len, "%02x", dataObjectIdUnsignedChar[i] & 0xff);
    }

    dataObjectIdhex = idStr;
    // HAGGLE_DBG2("converted hexid=%s\n", dataObjectIdhex.c_str());
    return dataObjectIdhex;
}

size_t FragmentationDataObjectUtility::calculateTotalNumberOfFragments(size_t dataLen,
        size_t fragmentSize) {

    double doubleDataLen = static_cast<double>(dataLen);
    double doubleFragmentSize = static_cast<double>(fragmentSize);
    double doubleNumberOfFragments = doubleDataLen / doubleFragmentSize;

    double roundedNumberOfFragments = ceil(doubleNumberOfFragments);
    size_t finalNumberOfFragments = static_cast<size_t>(roundedNumberOfFragments);
    if (finalNumberOfFragments == 0) {
        finalNumberOfFragments = 1;
    }

    HAGGLE_DBG2(
            "Calculating number of fragments: doubleDataLen=%f doubleFragmentSize=%f doubleNumberOfFragments=%f roundedNumberOfFragments=%f finalNumberOfFragments=%d\n",
            doubleDataLen, doubleFragmentSize, doubleNumberOfFragments, roundedNumberOfFragments, finalNumberOfFragments);

    return finalNumberOfFragments;
}

bool FragmentationDataObjectUtility::allocateFile(string filePath, size_t fileSize) {

    HAGGLE_DBG2("Allocating file %s - filesize=%d\n", filePath.c_str(), fileSize);
    /*
     * write fragment to dataobject file
     */
    FILE* allocateFilePointer = fopen(filePath.c_str(), "wb");
    if (NULL == allocateFilePointer) {
        HAGGLE_ERR("Error opening file %s\n", filePath.c_str());
        return false;
    }
    int fseekret = fseek(allocateFilePointer, 0, SEEK_SET);
    if (fseekret != 0) {
        HAGGLE_ERR("Unable to seek to start position\n");
        return false;
    }
    char* buffer = new char[fileSize];
    memset(buffer, 0, fileSize);
    fwrite(buffer, 1, fileSize, allocateFilePointer);
    if (ferror(allocateFilePointer)) {
        HAGGLE_ERR("Error writing to file %s\n", filePath.c_str());
        return false;
    }
    fclose(allocateFilePointer);
    delete[] buffer;

    return true;
}

// FIXME: better error/return value handling

FragmentParentDataObjectInfo FragmentationDataObjectUtility::getFragmentParentDataObjectInfo(
        DataObjectRef fragmentDataObjectRef) {
    FragmentParentDataObjectInfo fragmentParentDataObjectInfo;

    const Attribute* attributeDataObjectId = fragmentDataObjectRef->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_PARENT_DATAOBJECT_ID, "*", 1.0);
    if (attributeDataObjectId == NULL) {
        HAGGLE_DBG2("No original data object id attribute found\n");
	return fragmentParentDataObjectInfo;
    }
    fragmentParentDataObjectInfo.dataObjectId = attributeDataObjectId->getValue();

    const Attribute* attributeObjectLen = fragmentDataObjectRef->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_LEN, "*", 1.0);
    if (attributeObjectLen == NULL) {
        HAGGLE_ERR("No original data len attribute found\n");
	return fragmentParentDataObjectInfo;
    }
    string dataLen = attributeObjectLen->getValue();
    HAGGLE_DBG2("orignal data len is %s\n", dataLen.c_str());
    fragmentParentDataObjectInfo.fileSize = atol(dataLen.c_str());

    if (fragmentParentDataObjectInfo.fileSize == 0) {
        HAGGLE_ERR("Orignal data len is zero\n");
        return fragmentParentDataObjectInfo;
    }

    const Attribute* attributeSequenceNumber = fragmentDataObjectRef->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_SEQUENCE_NUMBER, "*", 1.0);
    if (attributeSequenceNumber == NULL) {
        HAGGLE_ERR("No fragment sequence number attribute found\n");
	return fragmentParentDataObjectInfo;
    }
    string sequenceNumber = attributeSequenceNumber->getValue();
    HAGGLE_DBG2("fragment sequence number is %s\n", sequenceNumber.c_str());
    List<string> sequenceNumberList;
    char* p = strtok((char*) sequenceNumber.c_str(), ",");
    while (p != NULL) {
        string sequenceNumberString = p;
        //sequenceNumberList.push_front(sequenceNumberString);
        sequenceNumberList.push_back(sequenceNumberString);
        p = strtok(NULL, ",");
        HAGGLE_DBG2("parsing sequence number %s\n", sequenceNumberList.front().c_str());
    }
    fragmentParentDataObjectInfo.sequenceNumberList = sequenceNumberList;
    //fragmentParentDataObjectInfo.sequenceNumber = atol(sequenceNumber.c_str());

    const Attribute* attributeFileName = fragmentDataObjectRef->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_NAME, "*", 1.0);
    if (attributeFileName == NULL) {
        HAGGLE_ERR("No original file name attribute found\n");
	return fragmentParentDataObjectInfo;
    }
    fragmentParentDataObjectInfo.fileName = attributeFileName->getValue();

    const Attribute* attributeCreateTime = fragmentDataObjectRef->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_PARENT_CREATION_TIME, "*", 1.0);
    if (attributeCreateTime == NULL) {
        HAGGLE_ERR("No original create time attribute found\n");
	return fragmentParentDataObjectInfo;
    }
    fragmentParentDataObjectInfo.createTime = attributeCreateTime->getValue();
    return fragmentParentDataObjectInfo;
}

FragmentationPositionInfo FragmentationDataObjectUtility::calculateFragmentPositionInfoEncoder(
        size_t sequenceNumber, size_t fragmentSize, size_t fileSize) {
    size_t startPosition = (sequenceNumber - 1) * fragmentSize;
    size_t endPosition = startPosition + fragmentSize - 1;
    size_t readFragmentSize = fragmentSize;
    if (endPosition > fileSize) {
        readFragmentSize = fileSize - startPosition;
        HAGGLE_DBG2( "End position is greater than filesize - readFragmentSize=%d\n",
                readFragmentSize);
    }

    HAGGLE_DBG2("Calculating fragment position - sequenceNumber=%d startPosition=%d fragmentSize=%d readFragmentSize=%d\n",
            sequenceNumber, startPosition, fragmentSize, readFragmentSize);

    FragmentationPositionInfo fragmentationPositionInfo;
    fragmentationPositionInfo.startPosition = startPosition;
    fragmentationPositionInfo.actualFragmentSize = readFragmentSize;

    return fragmentationPositionInfo;
}

FragmentationPositionInfo FragmentationDataObjectUtility::calculateFragmentationPositionInfo(
        size_t sequenceNumber, size_t fragmentSize, size_t fileSize) {

    FragmentationPositionInfo fragmentationPositionInfo;

    fragmentationPositionInfo.startPosition = (sequenceNumber - 1) * fragmentSize;
    HAGGLE_DBG2("Canculating fragment position info - fileSize=%d startPosition=%d fragmentSize=%d\n",
            fileSize, fragmentationPositionInfo.startPosition, fragmentSize);
    size_t endPosition = fragmentationPositionInfo.startPosition + fragmentSize - 1;
    fragmentationPositionInfo.actualFragmentSize = fragmentSize;

    if (endPosition > fileSize) {
        fragmentationPositionInfo.actualFragmentSize = fileSize
                - fragmentationPositionInfo.startPosition;
        HAGGLE_DBG2( "Endposition is greater than filesize readFragmentSize=%d.\n",
                fragmentationPositionInfo.actualFragmentSize);
    }

    size_t totalNumberOfFragments = this->calculateTotalNumberOfFragments(fileSize, fragmentSize);
    if (sequenceNumber == totalNumberOfFragments) {
        HAGGLE_DBG2("Last fragment encountered - sequenceNumber=%d endPosition=%d fileSize=%d startPosition=%d\n",
                sequenceNumber, endPosition, fileSize, fragmentationPositionInfo.startPosition);
        if (endPosition == fileSize) {
            HAGGLE_DBG2("Setting actual fragment size to 1\n");
            fragmentationPositionInfo.actualFragmentSize = fileSize
                    - fragmentationPositionInfo.startPosition;
        }

        if (endPosition < fileSize - 1) {
//			fragmentationPositionInfo.actualFragmentSize = endPosition
//					- fragmentationPositionInfo.startPosition;
            HAGGLE_ERR("Unexpected case - endPosition=%d fileSize=%d fragmentSize=%d startPosition=%d\n",
                    endPosition, fileSize, fragmentSize, fragmentationPositionInfo.actualFragmentSize, fragmentationPositionInfo.startPosition);
//			HAGGLE_DBG(
//					"startposition=%d endposition=%d is lesser than filesize readFragmentSize=%d.\n",
//					fragmentationPositionInfo.startPosition,endPosition,fragmentationPositionInfo.actualFragmentSize);
        }
    }

    HAGGLE_DBG2(
            "Result: sequenceNumber=%d startPosition=%d endPosition=%d fragmentSize=%d actualFragmentSize=%d fileSize=%d\n",
            sequenceNumber, fragmentationPositionInfo.startPosition, endPosition, fragmentSize, fragmentationPositionInfo.actualFragmentSize, fileSize);

    return fragmentationPositionInfo;
}

bool FragmentationDataObjectUtility::storeFragmentToDataObject(string originalDataObjectFilePath,
        string fragmentDataObjectFilePath, size_t parentDataObjectStartPosition,
        size_t fragmentSize, size_t fragmentStartPosition) {

    HAGGLE_DBG2(
            "Writing fragment file %s to parent file %s - parentDataObjectStartPosition=%d fragmentStartPosition=%d fragmentSize=%d\n",
            fragmentDataObjectFilePath.c_str(), originalDataObjectFilePath.c_str(), parentDataObjectStartPosition, fragmentStartPosition, fragmentSize);

    /*
     * read fragment into buffer
     */
    FILE* fragmentDataObjectFilePointer = fopen(fragmentDataObjectFilePath.c_str(), "rb");
    if (NULL == fragmentDataObjectFilePointer) {
        HAGGLE_ERR("Unable to open file %s\n", fragmentDataObjectFilePath.c_str());
        return false;
    }

    int retval = fseek(fragmentDataObjectFilePointer, fragmentStartPosition, SEEK_SET);
    if( retval != 0 ) {
        HAGGLE_ERR("Error seeking file %s\n",fragmentDataObjectFilePath.c_str());
   	return false;
    }

    char* buffer = new char[fragmentSize + 1];
    memset(buffer, 0, fragmentSize+1);
    size_t bytesRead = fread(buffer, 1, fragmentSize, fragmentDataObjectFilePointer);
    if (ferror(fragmentDataObjectFilePointer) || 0 == bytesRead) {
        HAGGLE_ERR("Error reading file %s - fragmentStartPosition=%d\n",
                fragmentDataObjectFilePath.c_str(), fragmentStartPosition);
        return false;
    }
    HAGGLE_DBG2("Bytes read: %d\n", bytesRead);
    fclose(fragmentDataObjectFilePointer);

    /*
     * write fragment to dataobject file
     */
    FILE* parentDataObjectFilePointer = fopen(originalDataObjectFilePath.c_str(), "r+b");
    if (NULL == parentDataObjectFilePointer) {
        HAGGLE_ERR("Error opening fragment file %s\n", originalDataObjectFilePath.c_str());
        return false;
    }
    int fseekret = fseek(parentDataObjectFilePointer, parentDataObjectStartPosition, SEEK_SET);
    if (fseekret != 0) {
        HAGGLE_ERR("Unable to seek to start position %d\n", parentDataObjectStartPosition);
        return false;
    }
    size_t currentPosition = ftell(parentDataObjectFilePointer);
    HAGGLE_DBG2("Positon in parent file: currentPosition=%d\n", currentPosition);
    fflush(parentDataObjectFilePointer);
    HAGGLE_DBG2("Writing fragment to parent file - startposition=%d fragmentSize=%d\n",
            parentDataObjectStartPosition, fragmentSize);
    fwrite(buffer, fragmentSize, 1, parentDataObjectFilePointer);
    delete[] buffer;
    if (ferror(parentDataObjectFilePointer)) {
        HAGGLE_ERR("Error writing file %s\n", originalDataObjectFilePath.c_str());
        return false;
    }
    fflush(parentDataObjectFilePointer);
    fclose(parentDataObjectFilePointer);

    return true;
}

bool FragmentationDataObjectUtility::copyAttributesToDataObject(DataObjectRef dataObjectOriginal,
        DataObjectRef fragmentationDataObjectRef) {
    HAGGLE_DBG2("Copying attributes from fragment to parent data object\n");

    string parentSignature = "";
    string parentSignee = "";


    const Attributes * attributes = fragmentationDataObjectRef->getAttributes();
    for (Attributes::const_iterator it = attributes->begin(); it != attributes->end(); it++) {
        const Attribute attribute = (*it).second;
        string name = attribute.getName();
        string value = attribute.getValue();
        unsigned long weight = attribute.getWeight();

        HAGGLE_DBG2("Attribute name %s\n", name.c_str());
        if (name == HAGGLE_ATTR_FRAGMENTATION_PARENT_CREATION_TIME) {
            HAGGLE_DBG2( "Previous create time was %s - setting create time to %s\n",
                    dataObjectOriginal->getCreateTime().getAsString().c_str(), value.c_str());
            dataObjectOriginal->setCreateTime(value);
            dataObjectOriginal->setReceiveTime(Timeval::now()); // MOS
            HAGGLE_DBG2("Parent create time is now %s\n",
                    dataObjectOriginal->getCreateTime().getAsString().c_str());
        }
        else if(name == HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_SIGNATURE) {
            HAGGLE_DBG2("Parent signature is %s\n",value.c_str());
            parentSignature = value;
        }
        else if(name == HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_SIGNEE) {
            HAGGLE_DBG2("Parent signee is %s\n",value.c_str());
            parentSignee = value;
        }
        else if (name != HAGGLE_ATTR_FRAGMENTATION_NAME
                && name != HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_NAME
                && name != HAGGLE_ATTR_FRAGMENTATION_PARENT_DATAOBJECT_ID
                && name != HAGGLE_ATTR_FRAGMENTATION_PARENT_ORIG_LEN
                && name != HAGGLE_ATTR_FRAGMENTATION_SEQUENCE_NUMBER) {
            HAGGLE_DBG2("Adding name=%s value=%s weight=%d\n",
                    name.c_str(), value.c_str(), weight);
            bool isAddAttribute = dataObjectOriginal->addAttribute(name, value, weight);
            if(!isAddAttribute) {
                HAGGLE_ERR("Unable to add attribute %s\n",name.c_str());
                return false;
            }
        } else {
            HAGGLE_DBG2("Skipping name=%s value=%s weight=%d\n",
                    name.c_str(), value.c_str(), weight);
        }

    }

    //setting signee and signature
    if(parentSignee.empty() || parentSignature.empty()) {
        HAGGLE_DBG2("Parent has no signature/signee - parentSignee=%s parentSignature=%s\n",
                parentSignee.c_str(),parentSignature.c_str());
        return true; // MOS - allow fragments without signature to go through (verified later)
    }


    dataObjectOriginal->setSignee(parentSignee);

    unsigned char* parentSignatureUnsignedChar = NULL;
    size_t signature_len = 0;
    base64_decode_context ctx;

    base64_decode_ctx_init(&ctx);

    if (!base64_decode_alloc(&ctx, parentSignature.c_str(), parentSignature.length(),
            (char **) &parentSignatureUnsignedChar, &signature_len)) {
        HAGGLE_ERR("Could not create signature from attribute\n");
        return false;
    }

    dataObjectOriginal->setSignature(parentSignee, parentSignatureUnsignedChar,
            signature_len);
    
    return true;
}

const string& FragmentationDataObjectUtility::getFilePath(DataObjectRef dataObjectRef) {
    if (dataObjectRef->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {
        return dataObjectRef->getEncryptedFilePath();
    }
    return dataObjectRef->getFilePath();
}

const string& FragmentationDataObjectUtility::getFileName(DataObjectRef dataObjectRef) {
    return dataObjectRef->getFileName();
}

size_t FragmentationDataObjectUtility::getFileLength(DataObjectRef dataObjectRef) {
    if (dataObjectRef->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {
        return dataObjectRef->getEncryptedFileLength();
    }
    return dataObjectRef->getDataLen();
}
