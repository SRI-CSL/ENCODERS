/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Hasnain Lakhani (HL)
 */

#ifndef NETWORKCODINGDECODERSTORAGE_H_
#define NETWORKCODINGDECODERSTORAGE_H_


#include <stdio.h>
#include <stdlib.h>

#include "HaggleKernel.h"
#include "DataStore.h"
#include "DataObject.h"
#include "Event.h"

#include "networkcoding/codetorrentdecoder.h"
#include "networkcoding/blocky/src/blockycoderfile.h"
#include "networkcoding/NetworkCodingConstants.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "DataStore.h"
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Reference.h>

typedef Reference<BlockyCoderFile> codetorrentdecoderref;
typedef Reference<List<string> > codedblocksidlistref;

typedef Map<string, codetorrentdecoderref> decoder_t;
typedef Reference<decoder_t> decoder_ref_t;

typedef Map<string,bool> decodingCompleted_t;

typedef Map<string,codedblocksidlistref> networkCodedBlocks_t;
typedef Reference<networkCodedBlocks_t> networkCodedBlocks_ref_t;

typedef Map<string,string> networkCodedDecoderLastUsedTime_t;
typedef Reference<networkCodedDecoderLastUsedTime_t> networkCodedDecoderLastUsedTime_ref_t;

//dataobjectid,timestamp
typedef Map<string,string> reconstructedNetworkCodingDataObjectTracker_t;

class NetworkCodingDecoderStorage {
public:
    NetworkCodingDecoderStorage(HaggleKernel* haggleKernel,DataStore* _dataStore,NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility,NetworkCodingConfiguration* _networkCodingConfiguration);
    ~NetworkCodingDecoderStorage();

    codetorrentdecoderref getDecoder(string originalDataObjectId,size_t _originalDataFileSize,string _decodedFilePath);
    codetorrentdecoderref& createAndAddDecoder(const string originalDataObjectId,const size_t originalDataFileSize,const char* decodedFilePath,size_t blockSize);

    bool isDecodingCompleted(string originalDataObjectId);
    void setDecodingCompleted(string originalDataObjectId);

    codedblocksidlistref getNetworkCodedBlocks(string originalDataObjectId);
    void addCodedBlockId(string originalDataObjectId,const string networkCodedDataObjectId);

    void trackNetworkCodedBlock(const string originalDataObjectId, const string networkCodedDataObjectId);

    void disposeNetworkCodedBlocks(const string originalDataObjectId);

    void disposeDataObject(const unsigned char* dataObjectId);

    void disposeNetworkCodedBlock(DataObjectRef networkCodedBlock);

    void trackReconstructedDataObject(const string dataObjectId);
    double getResendReconstructedDelay(const string dataObjectId,double resendDelay);

    void trackDecoderLastUsedTime(string dataObjectId);
    void ageOffDecoder(double maxAgeDecoder);
    void deleteDecoderFromStorageByDataObjectId(string dataObjectId);

    bool haveDecoder(string dataObjectId); // CBMEN, HL


private:
    decoder_ref_t decoderStorage;
    decodingCompleted_t decodingCompleted;
    networkCodedBlocks_ref_t networkCodedBlocks;
    reconstructedNetworkCodingDataObjectTracker_t reconstructedNetworkCodingDataObjectTracker;
    networkCodedDecoderLastUsedTime_ref_t networkCodedDecoderLastUsedTime;

    NetworkCodingConfiguration* networkCodingConfiguration;
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;

    DataStore* dataStore;
    HaggleKernel* haggleKernel;
};

#endif /* NETWORKCODINGDECODERSTORAGE_H_ */
