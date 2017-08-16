/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 *   Yu-Ting Yu (yty)
 */

#include "NetworkCodingEncoderService.h"
#include "../../fragmentation/FragmentationConstants.h"

NetworkCodingEncoderService::NetworkCodingEncoderService(
        NetworkCodingFileUtility* _networkCodingFileUtility,
        NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility,
        NetworkCodingEncoderStorage* _networkCodingEncoderStorage,
        CodeTorrentUtility* _codeTorrentUtility,
        NetworkCodingConfiguration* _networkCodingConfiguration) {
    this->networkCodingFileUtility = _networkCodingFileUtility;
    this->networkCodingDataObjectUtility = _networkCodingDataObjectUtility;
    this->networkCodingEncoderStorage = _networkCodingEncoderStorage;
    this->codeTorrentUtility = _codeTorrentUtility;
    this->networkCodingConfiguration = _networkCodingConfiguration;
    this->fragmentationDataObjectUtility = new FragmentationDataObjectUtility();
}

NetworkCodingEncoderService::~NetworkCodingEncoderService() {
    if (this->fragmentationDataObjectUtility) {
        delete this->fragmentationDataObjectUtility;
        this->fragmentationDataObjectUtility = NULL;
    }
}

bool NetworkCodingEncoderService::writeCodedBlocks(FILE* _pnetworkCodedBlockFile,
        codetorrentencoderref& _encoderref, int numberOfBlocks) {
    size_t totalBytesWritten = 0;
    for (int cnt = 0; cnt < numberOfBlocks; cnt++) {
        int coeffsSize = _encoderref->getBlocksPerGeneration();
        HAGGLE_DBG2("Encoding block with coefficients size: %d\n", coeffsSize);

        BlockyPacket* block = new BlockyPacket();
        _encoderref->encode(*block, cnt);
	if(block == NULL) {
	  HAGGLE_DBG2("Encoding failure\n");
	  return false;
	}

        totalBytesWritten = fwrite(block->coeffs, 1, coeffsSize, _pnetworkCodedBlockFile);
        totalBytesWritten += fwrite(block->data, 1,
                this->networkCodingConfiguration->getBlockSize(), _pnetworkCodedBlockFile);
        this->codeTorrentUtility->freeCodedBlock(&block);
    }
    HAGGLE_DBG2("Encoding completed - wrote %d bytes\n", totalBytesWritten);
    return true;
}

DataObjectRef NetworkCodingEncoderService::encodeDataObject(
        const DataObjectRef originalDataObject) {
    string fileName = originalDataObject->getFileName();
    string parentDataObjectId = originalDataObject->getIdStr();
    codetorrentencoderref encoderref = this->networkCodingEncoderStorage->getEncoder(parentDataObjectId,
	  networkCodingDataObjectUtility->getFilePath(originalDataObject), this->networkCodingConfiguration->getBlockSize());
    if (!encoderref) {
        HAGGLE_ERR("Unable create encoder for file %s\n", networkCodingDataObjectUtility->getFilePath(originalDataObject).c_str());
        return NULL;
    }

    string networkCodedBlockFile = networkCodingFileUtility->createNetworkCodedBlockFileName(
            fileName);
    HAGGLE_DBG2("File name for network coded block: %s\n", networkCodedBlockFile.c_str());
    FILE* pnetworkCodedBlockFile = fopen(networkCodedBlockFile.c_str(), "wb");
    if(pnetworkCodedBlockFile) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", networkCodedBlockFile.c_str(), fileno(pnetworkCodedBlockFile));
    if (!pnetworkCodedBlockFile) {
        HAGGLE_ERR("Unable create networkcoded block file %s\n", networkCodedBlockFile.c_str());
        return NULL;
    }

    bool ok = this->writeCodedBlocks(pnetworkCodedBlockFile, encoderref,
            this->networkCodingConfiguration->getNumberOfBlockPerDataObject());

    fclose(pnetworkCodedBlockFile);

    if(!ok) {
        HAGGLE_ERR("Unable to encode data for file %s (%s)\n", networkCodedBlockFile.c_str(), originalDataObject->getFileName().c_str());
        return NULL;
    }

    DataObjectRef networkCodedDataObjectRef = DataObject::create(networkCodedBlockFile,
            networkCodingDataObjectUtility->getFileName(originalDataObject));

    if(!networkCodedDataObjectRef) {
        HAGGLE_ERR("Unable to create data object for file %s (%s)\n", networkCodedBlockFile.c_str(), originalDataObject->getFileName().c_str());
        return NULL;
    }

    bool addedAttributes = this->addAttributes(originalDataObject, networkCodedDataObjectRef);
    if (!addedAttributes) {
        HAGGLE_ERR("Unable to add attributes\n");
        return NULL;
    }

    //DEBUG to true want to look at files
    networkCodedDataObjectRef->setStored(false);

    const char* originalId = originalDataObject->getIdStr();
    string originalStringId = originalId;

    //HAGGLE_DBG("Adding send event for data object %s\n", originalId);
    HAGGLE_DBG2("Added network coded data object file %s\n",
            networkCodedDataObjectRef->getFilePath().c_str());
    HAGGLE_DBG2("Adding parent data object %s to data object storage\n", originalStringId.c_str());
    this->networkCodingEncoderStorage->addDataObject(originalStringId, originalDataObject);

    return networkCodedDataObjectRef;
}

DataObjectRef NetworkCodingEncoderService::encodeFragment(
        const DataObjectRef fragDataObject, const size_t fragmentSize) {

    //parse fragDataObject
    string parentDataObjectId = fragDataObject->getIdStr();

	const Attribute* attributeSequenceNumber = fragDataObject->getAttribute(HAGGLE_ATTR_FRAGMENTATION_SEQUENCE_NUMBER, "*", 1.0);
	if(attributeSequenceNumber == NULL){
		HAGGLE_ERR("No fragment sequence number found");
	}

	string sn = attributeSequenceNumber->getValue();
	char* endptr = NULL;
	size_t sequenceNumber = strtol(sn.c_str(), &endptr, 10);
	HAGGLE_DBG2("frag seq number is %s\n", sn.c_str());

	size_t parentDataObjectFileSize = this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(fragDataObject).fileSize;
	HAGGLE_DBG2("parent object size is %d\n", parentDataObjectFileSize);


	//get fragfile name
	string fileName = fragDataObject->getFileName();
	string fragmentDataObjectFilePath = fragDataObject->getFilePath();

    //read fragfile
	FragmentationPositionInfo fragmentationPositionInfo =
		this->fragmentationDataObjectUtility->calculateFragmentationPositionInfo(
				sequenceNumber, fragmentSize, parentDataObjectFileSize);

	//size_t fragmentStartPosition = fragmentationPositionInfo.startPosition;
	size_t fragmentStartPosition = 0;
	size_t actualFragmentSize = fragmentationPositionInfo.actualFragmentSize;

	HAGGLE_DBG2("frag start position is %d\n", fragmentStartPosition);
	HAGGLE_DBG2("frag actual size is %d\n", actualFragmentSize);

	FILE* fragmentDataObjectFilePointer = fopen(fragmentDataObjectFilePath.c_str(), "rb");
	if (NULL == fragmentDataObjectFilePointer) {
		HAGGLE_ERR("Unable to open file %s\n", fragmentDataObjectFilePath.c_str());
		return NULL;
	}

	int retval = fseek(fragmentDataObjectFilePointer, fragmentStartPosition, SEEK_SET);
	if( retval != 0 ) {
		HAGGLE_ERR("Error seeking file %s\n",fragmentDataObjectFilePath.c_str());
		return NULL;
	}
	char* buffer = new char[fragmentSize + 1];
	memset(buffer, 0, fragmentSize+1);
	size_t bytesRead = fread(buffer, 1, actualFragmentSize, fragmentDataObjectFilePointer);

	HAGGLE_DBG2("Bytes read: %d\n", bytesRead);
	fclose(fragmentDataObjectFilePointer);

	
	/*codetorrentencoderref encoderref = this->networkCodingEncoderStorage->getEncoder(parentDataObjectId,
		networkCodingDataObjectUtility->getFilePath(fragDataObject), this->networkCodingConfiguration->getBlockSize());*/
	

	//create a NC data object name based on the frag data object name
    string originalName = this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(fragDataObject).fileName;

	string ncFileName = networkCodingFileUtility->createNetworkCodedBlockFileName(
            originalName);


	FILE* pnetworkCodedBlockFile = fopen(ncFileName.c_str(), "wb");
	if(pnetworkCodedBlockFile) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", ncFileName.c_str(), fileno(pnetworkCodedBlockFile));

	if (!pnetworkCodedBlockFile) {
		HAGGLE_ERR("Unable create networkcoded block file %s\n", ncFileName.c_str());
		return NULL;
	}


	//calculate number of blocks
	double doubleFileSize = static_cast<double>(parentDataObjectFileSize);
	double doubleFieldSize = static_cast<double>(NETWORKCODING_FIELDSIZE);
	double doubleBlockSize = static_cast<double>(fragmentSize);
	double doubleNumBlocks = ceil( doubleFileSize * 8 / doubleFieldSize / doubleBlockSize );
	int numBlock = static_cast<int>(doubleNumBlocks);
    int blockSize = static_cast<int>(doubleBlockSize);
	HAGGLE_DBG2("total number of blocks: %d\n", numBlock);

	//sequencenumber must -1 since nc sequence number starts from 0
	BlockyPacket* block = new BlockyPacket();
    //generate coded block file
    BlockyCoderFile* encoderref = BlockyCoderFile::createEncoder((size_t)blockSize, (size_t)numBlock, originalName.c_str());
    
    if (!encoderref) {
        HAGGLE_ERR("Unable create single block encoder for file %s\n", networkCodingDataObjectUtility->getFilePath(fragDataObject).c_str());
        return NULL;
    }

    encoderref->encode(*block, sequenceNumber-1);
	if(block == NULL) {
		HAGGLE_ERR("Encoding failure\n");
		return NULL;
	}

	int coeffsSize = numBlock;
	size_t totalBytesWritten = fwrite(block->coeffs, 1, coeffsSize, pnetworkCodedBlockFile);
	totalBytesWritten += fwrite(block->data, 1,
			this->networkCodingConfiguration->getBlockSize(), pnetworkCodedBlockFile);
	this->codeTorrentUtility->freeCodedBlock(&block);
	fclose(pnetworkCodedBlockFile);

	delete encoderref;

	//encode this block as an NC data object
	DataObjectRef networkCodedDataObjectRef = DataObject::create(ncFileName,
		originalName);

	if(!networkCodedDataObjectRef) {
		HAGGLE_ERR("Unable to create data object for file %s (%s)\n", ncFileName.c_str(), fragDataObject->getFileName().c_str());
		return NULL;
	}

	//copy any necessary attributes from frag data object to NC data object
	bool addedAttributes = this->addAttributesFromFragDataObject(fragDataObject, networkCodedDataObjectRef);
	if (!addedAttributes) {
		HAGGLE_ERR("Unable to add attributes\n");
		return NULL;
	}


	delete[] buffer;

	return networkCodedDataObjectRef;
}




bool NetworkCodingEncoderService::addAttributes(DataObjectRef originalDataObject,
        DataObjectRef networkCodedDataObject) {
    //copy attributes. though eventually will use rich metadata?
    const Attributes* originalAttributes = originalDataObject->getAttributes();
    for (Attributes::const_iterator it = originalAttributes->begin();
            it != originalAttributes->end(); it++) {
        const Attribute attr = (*it).second;
        bool addAttribute = networkCodedDataObject->addAttribute(attr);
        if (!addAttribute) {
            HAGGLE_ERR("Unable to add attribute\n");
            return false;
        }
    }

    //add attribute to indicate data object is network coded block
    bool addedIsNetworkedCodedAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_NAME, "TRUE", 0);
    if (!addedIsNetworkedCodedAttribute) {
        HAGGLE_ERR("Unable to add networkcoding attribute\n");
        return false;
    }

    //add original data len attribute
    char lenBuffer[33];
    memset(lenBuffer, 0, sizeof(lenBuffer));
    int len = networkCodingDataObjectUtility->getFileLength(originalDataObject);
    if(len == 0) {
        HAGGLE_ERR("Orignal data len is zero - file already deleted\n");
        return false;
    }
    sprintf(lenBuffer, "%d", len);
    bool addedDataLenAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_LEN, lenBuffer, 0);
    if (!addedDataLenAttribute) {
        HAGGLE_ERR("Unable to add orignal data len attribute\n");
        return false;
    }

    //add dataobject id
    const char* originalId = originalDataObject->getIdStr();
    string originalStringId = originalId;
    bool addedIdAttribute = networkCodedDataObject->addAttribute(HAGGLE_ATTR_PARENT_DATAOBJECT_ID,
            originalStringId, 0);
    if (!addedIdAttribute) {
        HAGGLE_ERR("Unable to add original data object id attribute\n");
        return false;
    }

    //add dataobject name
    string originalName = networkCodingDataObjectUtility->getFileName(originalDataObject);
    HAGGLE_DBG2("Add original file name %s as attribute\n", originalName.c_str());
    bool addedNameAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_NAME, originalName, 0);
    if (!addedNameAttribute) {
        HAGGLE_ERR("Unable to add original file name attribute\n");
        return false;
    }

    //add create time
    string originalCreateTime = originalDataObject->getCreateTime().getAsString();
    HAGGLE_DBG2("Add original createtTime %s as attribute\n", originalCreateTime.c_str());
    bool addedCreatedTimeAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_CREATION_TIME, originalCreateTime, 0);
    if (!addedCreatedTimeAttribute) {
        HAGGLE_ERR("Unable to add original create time attribute\n");
        return false;
    }

    if(originalDataObject->getSignature()) { // MOS
      //add signee
      string parentSignee = originalDataObject->getSignee();
      HAGGLE_DBG2("Add original signee %s as attribute\n", parentSignee.c_str());
      bool addedSigneeAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_SIGNEE, parentSignee, 0);
      if(!addedSigneeAttribute) {
        HAGGLE_ERR("Unable to add original signee attribute\n");
        return false;
      }

      //add signature
      char *base64_signature = NULL;
      if (base64_encode_alloc(
            (char *)originalDataObject->getSignature(), originalDataObject->getSignatureLength(), &base64_signature) <= 0) {
        HAGGLE_ERR("Unable to generate base64 encoded signature\n");
        return false;
      }
      string parentSignature = base64_signature;
      HAGGLE_DBG2("Add original signature %s as attribute\n", parentSignature.c_str());
      bool addedSignatureAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_SIGNATURE, parentSignature, 0);
      if(!addedSignatureAttribute) {
        HAGGLE_ERR("Unable to add original signature as attribute\n");
        return false;
      }
      if(base64_signature) {
        free(base64_signature);
        base64_signature = NULL;
      }
    }

    return true;
}

bool NetworkCodingEncoderService::addAttributesFromFragDataObject(DataObjectRef fragDataObject,
        DataObjectRef networkCodedDataObject) {
    //copy attributes. though eventually will use rich metadata?
    const Attributes* originalAttributes = fragDataObject->getAttributes();
    for (Attributes::const_iterator it = originalAttributes->begin();
            it != originalAttributes->end(); it++) {
        const Attribute attr = (*it).second;
        bool addAttribute = networkCodedDataObject->addAttribute(attr);
        if (!addAttribute) {
            HAGGLE_ERR("Unable to add attribute\n");
            return false;
        }
    }

    //add attribute to indicate data object is network coded block
    bool addedIsNetworkedCodedAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_NAME, "TRUE", 0);
    if (!addedIsNetworkedCodedAttribute) {
        HAGGLE_ERR("Unable to add networkcoding attribute\n");
        return false;
    }

    //add original data len attribute
    char lenBuffer[33];
    memset(lenBuffer, 0, sizeof(lenBuffer));

    int len = this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(fragDataObject).fileSize;
    HAGGLE_DBG2("parent object size is %d\n", len);

    if(len == 0) {
        HAGGLE_ERR("Orignal data len is zero - file already deleted\n");
        return false;
    }
    sprintf(lenBuffer, "%d", len);
    bool addedDataLenAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_LEN, lenBuffer, 0);
    if (!addedDataLenAttribute) {
        HAGGLE_ERR("Unable to add orignal data len attribute\n");
        return false;
    }

    //add dataobject id
    string originalStringId = this->fragmentationDataObjectUtility->getOriginalDataObjectId(fragDataObject);

    bool addedIdAttribute = networkCodedDataObject->addAttribute(HAGGLE_ATTR_PARENT_DATAOBJECT_ID,
            originalStringId, 0);
    if (!addedIdAttribute) {
        HAGGLE_ERR("Unable to add original data object id attribute\n");
        return false;
    }

    //add dataobject name

    string originalName = this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(fragDataObject).fileName;
    HAGGLE_DBG2(" Add original file name %s as attribute\n", originalName.c_str());
    bool addedNameAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_NAME, originalName, 0);
    if (!addedNameAttribute) {
        HAGGLE_ERR("Unable to add original file name attribute\n");
        return false;
    }

    //add create time
    string originalCreateTime = this->fragmentationDataObjectUtility->getFragmentParentDataObjectInfo(fragDataObject).createTime;

    HAGGLE_DBG2("Add original createTime %s as attribute\n", originalCreateTime.c_str());
    bool addedCreatedTimeAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_CREATION_TIME, originalCreateTime, 0);
    if (!addedCreatedTimeAttribute) {
        HAGGLE_ERR("Unable to add original create time attribute\n");
        return false;
    }

//FIXME remove frag attributes?

    if(fragDataObject->getSignature()) { // MOS
      //add signee
      string parentSignee = fragDataObject->getSignee();
      HAGGLE_DBG2("Add original signee %s as attribute\n", parentSignee.c_str());
      bool addedSigneeAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_SIGNEE, parentSignee, 0);
      if(!addedSigneeAttribute) {
        HAGGLE_ERR("Unable to add original signee attribute\n");
        return false;
      }

      //add signature
      char *base64_signature = NULL;
      if (base64_encode_alloc(
            (char *)fragDataObject->getSignature(), fragDataObject->getSignatureLength(), &base64_signature) <= 0) {
        HAGGLE_ERR("Unable to generate base64 encoded signature\n");
        return false;
      }
      string parentSignature = base64_signature;
      HAGGLE_DBG2("Add original signature %s as attribute\n", parentSignature.c_str());
      bool addedSignatureAttribute = networkCodedDataObject->addAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_SIGNATURE, parentSignature, 0);
      if(!addedSignatureAttribute) {
        HAGGLE_ERR("Unable to add original signature as attribute\n");
        return false;
      }
      if(base64_signature) {
        free(base64_signature);
        base64_signature = NULL;
      }
    }

    return true;
}
