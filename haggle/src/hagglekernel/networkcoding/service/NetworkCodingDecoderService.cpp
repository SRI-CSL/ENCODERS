/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingDecoderService.h"

NetworkCodingDecoderService::NetworkCodingDecoderService(
        NetworkCodingDecoderStorage* _networkCodingDecoderStorage,
        NetworkCodingFileUtility* _networkCodingFileUtility,
        NetworkCodingConfiguration* _networkCodingConfiguration,
        NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility) {
    this->networkCodingFileUtility = _networkCodingFileUtility;
    this->networkCodingDecoderStorage = _networkCodingDecoderStorage;
    this->networkCodingConfiguration = _networkCodingConfiguration;
    this->networkCodingDataObjectUtility = _networkCodingDataObjectUtility;
}

NetworkCodingDecoderService::~NetworkCodingDecoderService() {
}

DataObjectRef NetworkCodingDecoderService::decodeDataObject(
        const DataObjectRef& dObj) {
    if (!dObj) {
        HAGGLE_DBG("Unexpected null data object\n");
        return NULL;
    }

    if (dObj->isDuplicate()) {
        HAGGLE_DBG("Data object %s is a duplicate, ignoring\n",
                dObj->getIdStr());
        return NULL;
    }

    string networkCodedDataObjectId = dObj->getIdStr();
    HAGGLE_DBG("Processing received data object %s\n",
            networkCodedDataObjectId.c_str());

    HAGGLE_DBG2("Checking if data object is networkcoded block\n");
    const Attribute* attributeIsNetworkCoding = new Attribute(
            HAGGLE_ATTR_NETWORKCODING_NAME, "TRUE");
    bool hasAttributeIsNetworkCoding = dObj->hasAttribute(
            *attributeIsNetworkCoding);
    delete attributeIsNetworkCoding;
    attributeIsNetworkCoding = NULL;
    if (!hasAttributeIsNetworkCoding) {
        HAGGLE_DBG("Data object %s does not have networkcoding attribute\n",
                networkCodedDataObjectId.c_str());
        return NULL;
    }

    HAGGLE_DBG("Data object %s has networkcoding attribute\n",
	       networkCodedDataObjectId.c_str());

    const Attribute* attributeDataObjectId = dObj->getAttribute(
            HAGGLE_ATTR_PARENT_DATAOBJECT_ID, "*", 1.0);
    if (attributeDataObjectId == NULL) {
        HAGGLE_ERR("No original data object id attribute\n");
        return NULL;
    }
    const Attribute* attributeObjectLen = dObj->getAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_LEN, "*", 1.0);
    if (attributeObjectLen == NULL) {
        HAGGLE_ERR("No original data len attribute\n");
        return NULL;
    }

    const Attribute* attributeDataObjectname = dObj->getAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_NAME, "*", 1.0);
    if (attributeDataObjectname == NULL) {
        HAGGLE_ERR("No original file name attribute\n");
        return NULL;
    }

    HAGGLE_DBG2("Received block has attributes orig id=%s len=%s\n",
            attributeDataObjectId->getValue().c_str(), attributeObjectLen->getValue().c_str());

    //HAGGLE
    int originalDataLen = atoi(attributeObjectLen->getValue().c_str());
    size_t originalDataFileSize = originalDataLen;

    if (originalDataFileSize == 0) {
        HAGGLE_ERR("Orignal data len is zero\n");
        return NULL;
    }

    const char* originalFileName = attributeDataObjectname->getValue().c_str();
    const char* networkCodedDataObjectFilePath = dObj->getFilePath().c_str();
    //decode is storage + decodedfilename
    string decodedFile = networkCodingFileUtility->createDecodedBlockFileName(
            originalFileName);
    string decodedFilePath =
            networkCodingFileUtility->createDecodedBlockFilePath(
                    decodedFile.c_str(), HAGGLE_DEFAULT_STORAGE_PATH);
    const size_t networkCodedBlockFileSize = dObj->getDataLen();

    string originalDataObjectId = attributeDataObjectId->getValue();

    HAGGLE_DBG("Received block with id=%s originalDataFileSize=%d filePath=%s decodedFile=%s id=%s ncblocklen=%d decodedFilePath=%s\n",
            dObj->getIdStr(), originalDataFileSize, networkCodedDataObjectFilePath, decodedFile.c_str(), attributeDataObjectId->getValue().c_str(), networkCodedBlockFileSize, decodedFilePath.c_str());

    this->networkCodingDecoderStorage->trackNetworkCodedBlock(
            originalDataObjectId, networkCodedDataObjectId);

    codetorrentdecoderref decoderref =
            this->networkCodingDecoderStorage->getDecoder(originalDataObjectId,originalDataFileSize,decodedFilePath);
    bool isCompleted = this->networkCodingDecoderStorage->isDecodingCompleted(
            originalDataObjectId);
    if (isCompleted) {
        HAGGLE_DBG("Ignoring unnecessary block %s of %s - already completed decoding\n",
		   networkCodedDataObjectId.c_str(), originalDataObjectId.c_str());
        //FIXME remove unnecessary network coded blocks that we receive
        //const unsigned char* networkedCodedBlockId = (const unsigned char*)networkCodedDataObjectId.c_str();
        //this->disposeDataObject(networkedCodedBlockId);
        return NULL;
    }

    HAGGLE_DBG2("Allocating memory to read block\n");
    size_t bufferLen = networkCodedBlockFileSize + 1;
    unsigned char* storagePutData = (unsigned char*) malloc(
            bufferLen * sizeof(unsigned char));
    memset(storagePutData, 0, sizeof(storagePutData));
    unsigned char * storagePutDataPointer = &storagePutData[0];

    FILE* pnetworkCodedBlockFile = fopen(networkCodedDataObjectFilePath, "rb");
    if(pnetworkCodedBlockFile) HAGGLE_DBG2("Opening file %s with file descriptor %d\n", networkCodedDataObjectFilePath, fileno(pnetworkCodedBlockFile));
    if (pnetworkCodedBlockFile == NULL) {
        HAGGLE_ERR("Unable to open block file %s\n",
                networkCodedDataObjectFilePath);
        free(storagePutData);
        storagePutData = NULL;
        storagePutDataPointer = NULL;
        return NULL;
    }
    HAGGLE_DBG2("Opened block file %s - filesize=%d\n",
            networkCodedDataObjectFilePath,networkCodedBlockFileSize);
    int count = fread(storagePutDataPointer, networkCodedBlockFileSize, 1,
            pnetworkCodedBlockFile);
    fclose(pnetworkCodedBlockFile);

    if(count != 1) {
        HAGGLE_ERR("Failed to read block file %s\n",
                networkCodedDataObjectFilePath);
        free(storagePutData);
        storagePutData = NULL;
        storagePutDataPointer = NULL;
        return NULL;
    }

    HAGGLE_DBG2("Read block from file sucessfully\n");

    int coeffsLen = decoderref->getBlocksPerGeneration();
    size_t blockSize = this->networkCodingConfiguration->getBlockSize();
    int numberOfBlocksPerDataObject = this->networkCodingConfiguration->getNumberOfBlockPerDataObject();

    for(int x=0;x<numberOfBlocksPerDataObject;x++) {
        HAGGLE_DBG2("coeffsLen=%d\n", coeffsLen);
        BlockyPacket block;
        block.generation = 0;
        block.numBlocks = (size_t)coeffsLen;
        block.blockSize = blockSize;
        uint8_t * coeffs = new uint8_t[coeffsLen];
        uint8_t *data = new uint8_t[blockSize];
        memcpy(coeffs, storagePutDataPointer, coeffsLen);
        memcpy(data, storagePutDataPointer+coeffsLen, blockSize);
        block.data = data;
        block.coeffs = coeffs;
        bool isInnovative = decoderref->store(block);
        HAGGLE_STAT("Received innovative block: %d for %s\n", isInnovative,originalDataObjectId.c_str());
	size_t numberOfBlocks = originalDataFileSize / this->networkCodingConfiguration->getBlockSize();
	HAGGLE_STAT("Number blocks: %d for %s\n",numberOfBlocks,originalDataObjectId.c_str());
        storagePutDataPointer = storagePutDataPointer + coeffsLen;
        storagePutDataPointer = storagePutDataPointer + blockSize;
    }

    if (storagePutData) {
        free(storagePutData);
        storagePutData = NULL;
        storagePutDataPointer = NULL;
    }

    if (decoderref->decode()) {
      HAGGLE_DBG("Decoding sucessfully completed for %s\n", originalDataObjectId.c_str());
        this->networkCodingDecoderStorage->setDecodingCompleted(originalDataObjectId);
        //FIXME race condition?
//        this->networkCodingDecoderStorage->disposeNetworkCodedBlocks(
//                originalDataObjectId);

        //generate received event
        string codetorrentDecoderDecodedFilePath = decoderref->getFilePath().c_str();
        HAGGLE_DBG2("codetorrentDecoderDecodedFilePath=%s decodedFilePath=%s decodedFile=%s originalFileName=%s\n",
                codetorrentDecoderDecodedFilePath.c_str(), decodedFilePath.c_str(), decodedFile.c_str(), originalFileName);
        DataObjectRef dataObjectReceived = DataObject::create(
                codetorrentDecoderDecodedFilePath, originalFileName);

	if(!dataObjectReceived) {
	  HAGGLE_ERR("Unable to create data object for file %s (%s)\n", codetorrentDecoderDecodedFilePath.c_str(), originalFileName);
	  return NULL;
	}

        dataObjectReceived->setPersistent(true);
        dataObjectReceived->setStored(true);

        bool isCopyAttributesToDataObject = this->networkCodingDataObjectUtility->copyAttributesToDataObject(
                dataObjectReceived, dObj);
        if(!isCopyAttributesToDataObject) {
            HAGGLE_ERR("Error copying attributes to reconstructed dataobject\n");
            return NULL;
        }

	dataObjectReceived->restoreEncryptedDataObject(); // MOS

        HAGGLE_STAT("Reconstructed network coded data object %s\n", dataObjectReceived->getIdStr());

        this->networkCodingDecoderStorage->trackReconstructedDataObject(dataObjectReceived->getIdStr());

        return dataObjectReceived;
    }

    //still no dataobject so return null
    return NULL;
}
