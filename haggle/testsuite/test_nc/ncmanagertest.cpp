/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

/*
 * ncmanagertest.cpp
 *
 *  
 *      Author: josh
 */

#include <stdio.h>

#include "SQLDataStore.h"
#include "Trace.h"
#include "networkcoding/storage/NetworkCodingDecoderStorage.h"

#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "networkcoding/NetworkCodingFileUtility.h"
#include "networkcoding/service/NetworkCodingDecoderService.h"
#include "networkcoding/service/NetworkCodingEncoderService.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "networkcoding/storage/NetworkCodingEncoderStorage.h"
#include "networkcoding/CodeTorrentUtility.h"

#include "networkcoding/storage/NetworkCodingDecoderStorage.h"

#define TEST_SQL_DATABASE   "/tmp/test.db"
#define TEST_BLOCKSIZE 8

void debug() {

    char originalFile[] = "josh";

    DataObject* originalDataObject = DataObject::create(originalFile, "josh");
    bool addedAttribute = originalDataObject->addAttribute("avengers", "hulk",
            99);
    HAGGLE_DBG("added attribute=|%d|\n", addedAttribute);
    HAGGLE_DBG("create time=|%s|\n",
            originalDataObject->getCreateTime().getAsString().c_str());
    string dataObjectIdString = originalDataObject->getIdStr();

    string signee="josh";
    const unsigned char* signatureConst = reinterpret_cast<const unsigned char*>("joy");
    unsigned char* signature = const_cast<unsigned char*>(signatureConst);
    originalDataObject->setSignee(signee);
    originalDataObject->setSignature(signee,signature,3);


    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility =
            new NetworkCodingDataObjectUtility();
    DataObjectIdRef dataObjectIdref =
            networkCodingDataObjectUtility->convertDataObjectIdStringToDataObjectIdType(
                    dataObjectIdString);
    HAGGLE_DBG(
            "dataobjectidhex=|%s| original dataobjectid=|%u| converted dataobjectid=|%u|\n",
            dataObjectIdString.c_str(), originalDataObject->getId(), dataObjectIdref.c_str());

    Node* node = NULL;

    HAGGLE_DBG("sending event\n");
    Event* sendNetworkCodingEvent = new Event(EVENT_TYPE_DATAOBJECT_SEND,
            originalDataObject, node);

    NetworkCodingConfiguration* networkCodingConfiguration =
            new NetworkCodingConfiguration();
    networkCodingConfiguration->turnOnNetworkCoding();
    networkCodingConfiguration->setBlockSize(TEST_BLOCKSIZE);
    networkCodingConfiguration->setDelayDeleteNetworkCodedBlocks(10.0);
    networkCodingConfiguration->setNumberOfBlockPerDataObject(1);

    SQLDataStore* dataStore = new SQLDataStore(true, TEST_SQL_DATABASE);
    NetworkCodingDecoderStorage* decoderStorage =
            new NetworkCodingDecoderStorage(dataStore,networkCodingDataObjectUtility,networkCodingConfiguration);
    NetworkCodingFileUtility* networkCodingFileUtility =
            new NetworkCodingFileUtility();
    NetworkCodingDecoderService* decoder = new NetworkCodingDecoderService(
            decoderStorage, networkCodingFileUtility,networkCodingConfiguration,networkCodingDataObjectUtility);
    NetworkCodingEncoderStorage* networkCodingEncoderStorage =
            new NetworkCodingEncoderStorage();
    CodeTorrentUtility* codeTorrentUtility = new CodeTorrentUtility();

    NetworkCodingEncoderService* encoder = new NetworkCodingEncoderService(
            networkCodingFileUtility, networkCodingDataObjectUtility,
            networkCodingEncoderStorage, codeTorrentUtility,networkCodingConfiguration);

    Event* sendEvent = NULL;
    DataObjectRef decodedDataObject = NULL;
    do {
        DataObjectRef encodedBlock  = encoder->encodeDataObject(originalDataObject);

        const Attribute* attributeDataObjectId = encodedBlock->getAttribute(
                HAGGLE_ATTR_PARENT_DATAOBJECT_ID, "*", 1.0);
        DataObjectRef retrievedDataObject =
                networkCodingEncoderStorage->getDataObjectById(
                        attributeDataObjectId->getValue());
        if (retrievedDataObject.getObj() == NULL) {
            HAGGLE_ERR("unable to locate dataobjectid=\n");
            return;
        }
        else {
            HAGGLE_DBG("found dataobject in storage\n");
        }

        decodedDataObject = decoder->decodeDataObject(encodedBlock);
    }
    while (!decodedDataObject);

    delete dataStore;
    delete networkCodingConfiguration;
    delete encoder;
    delete decoder;

    return;
}

void testlist() {
    string originalDataObjectId = "hulk";
    string ncblockid = "ncid1";
    networkCodedBlocks_t networkCodedBlocks;
    codedblocksidlistref networkCodedBlocksList =
            networkCodedBlocks[originalDataObjectId];
    size_t beforeSize = networkCodedBlocks[originalDataObjectId]->size();
    networkCodedBlocksList->push_back(ncblockid);
    networkCodedBlocks[originalDataObjectId] = networkCodedBlocksList;
    size_t afterSize = networkCodedBlocks[originalDataObjectId]->size();
    HAGGLE_DBG("beforesize=|%d| aftersize=|%d| originalDataObjectId=|%s|\n",
            beforeSize, afterSize, originalDataObjectId.c_str());
}

int main(int argc, char *argv[]) {
    printf("network coding manager test\n");

    Trace::enableStdout();

    debug();
    //testlist();

    return 0;
}
