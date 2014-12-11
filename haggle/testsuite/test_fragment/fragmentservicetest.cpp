/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Yu-Ting Yu (yty)
 */

#include "fragmentation/service/FragmentationEncoderService.h"
#include "fragmentation/service/FragmentationDecoderService.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/storage/FragmentationEncoderStorage.h"
#include "Node.h"
#include "fragmentation/FragmentationConstants.h"


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

#include "LossRateSlidingWindowElement.h"
#include "LossRateSlidingWindowEstimator.h"

#define TEST_SQL_DATABASE   "/tmp/test.db"
#define TEST_BLOCKSIZE 8


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <iostream>

DataObjectTypeIdentifierUtility* _dataObjectTypeIdentifierUtility;
FragmentationDecoderStorage* _fragmentationDecoderStorage;
FragmentationEncoderStorage* fragmentationEncoderStorage;
FragmentationDataObjectUtility* _fragmentationDataObjectUtility;
FragmentationDecoderService* fragmentationDecoderService;
FragmentationConfiguration* fragmentationConfiguration;
FragmentationFileUtility* fragmentationFileUtility;
FragmentationEncoderService* fragmentationEncoderService;
DataObject* originalDataObject = NULL;

#include <vector>
#include "stringutils/CSVUtility.h"

void test_split() {
    printf("*******performing string split\n");
    string stringToSplit = "";
    std::vector<std::string> x = split("one,two,,three", ',');

    for(std::vector<std::string>::iterator iter = x.begin();iter!=x.end();iter++) {
        std::string val = (*iter);
        printf("val=|%s|\n",val.c_str());
    }

    bool exists = itemInVector("one",x);
    bool notexists = itemInVector("josh",x);
    printf("exists=|%d| notexists=|%d|\n",exists,notexists);

    printf("*******end string split\n");
}

void init() {
	_dataObjectTypeIdentifierUtility = new DataObjectTypeIdentifierUtility();
	_fragmentationDecoderStorage = new FragmentationDecoderStorage(NULL,
			_dataObjectTypeIdentifierUtility);
	fragmentationEncoderStorage = new FragmentationEncoderStorage();
	_fragmentationDataObjectUtility = new FragmentationDataObjectUtility();

	fragmentationDecoderService = new FragmentationDecoderService(
			_fragmentationDecoderStorage, _fragmentationDataObjectUtility);

	fragmentationConfiguration = new FragmentationConfiguration();
	fragmentationConfiguration->setNumberFragmentsPerDataObject(1);
	fragmentationConfiguration->setResendReconstructedDelay(100.0);
	fragmentationFileUtility = new FragmentationFileUtility();
	fragmentationEncoderService = new FragmentationEncoderService(
			fragmentationFileUtility, fragmentationConfiguration,
			fragmentationEncoderStorage);

	string filePath = "josh";
	string fileName = "josh";
	originalDataObject = DataObject::create(filePath, fileName);
	originalDataObject->setStored(true);

}

void fragment() {
	init();

	size_t fragmentSize = 990000; //64000;

	DataObjectRef originalDataObjectRef = originalDataObject;
	NodeRef nodeRef = Node::create(Node::TYPE_APPLICATION, "test",
			Timeval::now());
	List<DataObjectRef> dataObjectRefList =
			fragmentationEncoderService->getAllFragmentsForDataObject(
					originalDataObjectRef, fragmentSize, nodeRef);

	DataObjectRef reconstructedDataObjectRef = NULL;

	int count = 0;
	for (List<DataObjectRef>::iterator iter = dataObjectRefList.begin();
			iter != dataObjectRefList.end(); iter++) {
		DataObjectRef fragmentDataObject = *iter;
		DataObjectRef decode1 = fragmentationDecoderService->decode(
				fragmentDataObject, fragmentSize);
		if (decode1) {
			HAGGLE_DBG("reconstructed decode1 count=|%d|\n", count);
			reconstructedDataObjectRef = decode1;
			break;
		}
		DataObjectRef decode2 = fragmentationDecoderService->decode(
				fragmentDataObject, fragmentSize);
		if (decode2) {
			HAGGLE_DBG("reconstructed decode2\n");
			reconstructedDataObjectRef = decode2;
		}
		count++;
	}

	HAGGLE_DBG("reconstructed filepath=|%s| id=|%s| parentid=|%s|\n",
			reconstructedDataObjectRef->getFilePath().c_str(), reconstructedDataObjectRef->getIdStr(), originalDataObjectRef->getIdStr());
}


void encodeDecode() {
	init();

	DataObjectRef originalDataObjectRef = originalDataObject;
	size_t fragmentationSize = 3;

	size_t dataLen = originalDataObject->getDataLen();
	size_t totalNumberOfFragments =
		_fragmentationDataObjectUtility->calculateTotalNumberOfFragments(
																	dataLen, fragmentationSize);
	srand(time(NULL));

	DataObjectRef reconstructedDataObjectRef = NULL;
	NodeRef nodeRef = Node::create(Node::TYPE_APPLICATION, "test",
		Timeval::now());
	int count = 0;

	while (!reconstructedDataObjectRef) {
		List<DataObjectRef> fragmentList =
		fragmentationEncoderService->getAllFragmentsForDataObject(
			originalDataObjectRef, fragmentationSize, nodeRef);
		for (List<DataObjectRef>::iterator it = fragmentList.begin();it != fragmentList.end() && !reconstructedDataObjectRef; it++) {
			DataObjectRef fragmentFile = (*it);
			reconstructedDataObjectRef = fragmentationDecoderService->decode(fragmentFile, fragmentationSize);
			count++;
		}
	}
	HAGGLE_DBG("reconstructed count=|%d|\n", count);
	HAGGLE_DBG("reconstructed filepath=|%s| id=|%s| parentid=|%s|\n",
	reconstructedDataObjectRef->getFilePath().c_str(), reconstructedDataObjectRef->getIdStr(), originalDataObjectRef->getIdStr());

	double resendReconstructedDelay =
		_fragmentationDecoderStorage->getResendReconstructedDelay(reconstructedDataObjectRef->getIdStr(),fragmentationConfiguration->getResendReconstructedDelay());
	HAGGLE_DBG("resendReconstructedDelay=|%f|\n", resendReconstructedDelay);
}


void debugNc() {

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

		std::cout<<"akl: one decoding block"<<std::endl;
    }
    while (!decodedDataObject);

    delete dataStore;
    delete networkCodingConfiguration;
    delete encoder;
    delete decoder;

    return;
}


void encodeDecodeMix() {
	init();

	DataObjectRef originalDataObjectRef = originalDataObject;
	size_t fragmentationSize = 3;

	size_t dataLen = originalDataObject->getDataLen();
	size_t totalNumberOfFragments =
			_fragmentationDataObjectUtility->calculateTotalNumberOfFragments(
					dataLen, fragmentationSize);



	//NC
	char originalFile[] = "josh";

	NetworkCodingDataObjectUtility* networkCodingDataObjectUtility =
            new NetworkCodingDataObjectUtility();
    
    string dataObjectIdString = originalDataObject->getIdStr();
    
    DataObjectIdRef dataObjectIdref =
            networkCodingDataObjectUtility->convertDataObjectIdStringToDataObjectIdType(
                    dataObjectIdString);

    
    
    NetworkCodingConfiguration* networkCodingConfiguration =
            new NetworkCodingConfiguration();
    networkCodingConfiguration->turnOnNetworkCoding();
    networkCodingConfiguration->setBlockSize(fragmentationSize);
    networkCodingConfiguration->setDelayDeleteNetworkCodedBlocks(10.0);
    networkCodingConfiguration->setNumberOfBlockPerDataObject(1);


	dataLen = originalDataObject->getDataLen();
	std::cout<<"akl: dataLen: "<<dataLen<<std::endl;
	std::cout<<"akl: fragSize: "<<fragmentationSize<<std::endl;
	std::cout<<"akl: numFrag: "<<totalNumberOfFragments<<std::endl;
    
    SQLDataStore* dataStore = new SQLDataStore(true, TEST_SQL_DATABASE);
    NetworkCodingDecoderStorage* decoderStorage =
            new NetworkCodingDecoderStorage(dataStore,networkCodingDataObjectUtility,networkCodingConfiguration);
    NetworkCodingFileUtility* networkCodingFileUtility =
            new NetworkCodingFileUtility();
    NetworkCodingDecoderService* networkCodingDecoderService = new NetworkCodingDecoderService(
            decoderStorage, networkCodingFileUtility,networkCodingConfiguration,networkCodingDataObjectUtility);
    NetworkCodingEncoderStorage* networkCodingEncoderStorage =
            new NetworkCodingEncoderStorage();
    CodeTorrentUtility* codeTorrentUtility = new CodeTorrentUtility();

    NetworkCodingEncoderService* networkCodingEncoderService = new NetworkCodingEncoderService(
            networkCodingFileUtility, networkCodingDataObjectUtility,
            networkCodingEncoderStorage, codeTorrentUtility,networkCodingConfiguration);

	std::cout<<"akl: dataLen: "<<dataLen<<std::endl;
	std::cout<<"akl: fragSize: "<<fragmentationSize<<std::endl;
	std::cout<<"akl: numFrag: "<<totalNumberOfFragments<<std::endl;

	srand(time(NULL));

	DataObjectRef reconstructedDataObjectRef = NULL;
	NodeRef nodeRef = Node::create(Node::TYPE_APPLICATION, "test",
			Timeval::now());
	int count = 0;


	//while (!reconstructedDataObjectRef) {
	while (count < 10) {
		List<DataObjectRef> fragmentList =
				fragmentationEncoderService->getAllFragmentsForDataObject(
						originalDataObjectRef, fragmentationSize, nodeRef);
		for (List<DataObjectRef>::iterator it = fragmentList.begin();
				it != fragmentList.end() && !reconstructedDataObjectRef; it++) {
			DataObjectRef fragmentFile = (*it);

			std::cout<<"akl: encode frag"<<std::endl;
			DataObjectRef ncblock = networkCodingEncoderService->encodeFragment(fragmentFile, fragmentationSize);

        
	        	networkCodingDecoderService->decodeDataObject(ncblock);
			std::cout<<"akl: push nc object"<<std::endl;
			
			count++;
		}
	}


	int nccount = 0;
    DataObjectRef decodedDataObject = NULL;
    do {
    	nccount++;
        DataObjectRef encodedBlock  = networkCodingEncoderService->encodeDataObject(originalDataObject);        

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

        decodedDataObject = networkCodingDecoderService->decodeDataObject(encodedBlock);

        HAGGLE_DBG("akl: decode NC block #%d\n", nccount);
    }
    while (!decodedDataObject);

    delete dataStore;
    delete networkCodingConfiguration;
    delete networkCodingEncoderService;
    delete networkCodingDecoderService;

	if(decodedDataObject){

		HAGGLE_DBG("akl: reconstructed filepath=|%s| id=|%s| parentid=|%s|\n",
			decodedDataObject->getFilePath().c_str(), decodedDataObject->getIdStr(), originalDataObjectRef->getIdStr());

	}
}

void testIndex() {
	size_t totalNumberOfFragments = 5;

	int* listIndex = fragmentationEncoderService->getIndexesOfFragmentsToCreate(
			totalNumberOfFragments);
	for (int x = 0; x < 5; x++) {
		HAGGLE_DBG("index|%d|=|%d|\n", x, listIndex[x]);
	}

}

void testLossRate(){
	HAGGLE_DBG("TESTLOSSRATE\n");


	LossRateSlidingWindowElement e;
	e.printWindow();

	bool trueflag = 1;
	bool falseflag = 0;

	for(int i=2;i<50;i++){
		HAGGLE_DBG("i=%d\n", i);

		if(i%2!=0)
			e.advance(trueflag);
		else
			e.advance(falseflag);
		e.printWindow();
		HAGGLE_DBG("loss rate estimate: %f\n", e.getLossRate());
	}
}

void testLossRateEstimate(){
	LossRateSlidingWindowEstimator e;
	Map<string, bool> flagmap;

	flagmap["test1"] = true;
	flagmap["test2"] = true;

	for(Map<string, bool>::iterator it = flagmap.begin();it!=flagmap.end();it++){
		HAGGLE_DBG("%s, %d\n",it->first.c_str(), it->second);
	}



	e.printAllSlidingWindow();	

	for(Map<string, bool>::iterator it = flagmap.begin();it!=flagmap.end();it++){
		HAGGLE_DBG("%s, %d\n",it->first.c_str(), it->second);
	}


	int i=1;

	e.advance(flagmap);

	e.printAllSlidingWindow();

	for(Map<string, bool>::iterator it = flagmap.begin();it!=flagmap.end();it++){
		HAGGLE_DBG("%s, %d\n",it->first.c_str(), it->second);
	}

	i++;

	e.advance(flagmap);
	e.printAllSlidingWindow();

	flagmap["test3"] = true;
	
	for(Map<string, bool>::iterator it = flagmap.begin();it!=flagmap.end();it++){
		HAGGLE_DBG("%s, %d\n",it->first.c_str(), it->second);
	}

	i++;
	
	e.advance(flagmap);
	e.printAllSlidingWindow();

	flagmap.erase("test1");
	for(Map<string, bool>::iterator it = flagmap.begin();it!=flagmap.end();it++){
		HAGGLE_DBG("%s, %d\n",it->first.c_str(), it->second);
	}

	i++;

	e.advance(flagmap);
	e.printAllSlidingWindow();

	for(;i<=50;i++){
		if(i > 20)
			flagmap.erase("test2");
		HAGGLE_DBG("i=%d\n", i);
		e.advance(flagmap);
		e.printAllSlidingWindow();
		
		HAGGLE_DBG("test1, loss rate %f\n", e.queryLossRate("test1"));
		HAGGLE_DBG("test2, loss rate %f\n", e.queryLossRate("test2"));
		HAGGLE_DBG("test3, loss rate %f\n", e.queryLossRate("test3"));


	}

}

int main(int argc, char *argv[]) {
	Trace::enableStdout();
        Trace::trace.setTraceType(TRACE_TYPE_DEBUG2);

//	test_split();

//  fragment();

//	_fragmentationDataObjectUtility->allocateFile("allocatetest",17);

//	encodeDecode();

//        debugNc();
		
//	encodeDecodeMix();
//	testLossRate();
//
	testLossRateEstimate();

	return 0;
}
