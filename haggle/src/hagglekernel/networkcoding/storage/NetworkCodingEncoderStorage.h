/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

#ifndef NETWORKENCODERSTORAGE_H_
#define NETWORKENCODERSTORAGE_H_

#include <stdio.h>
#include <stdlib.h>

#include "DataObject.h"
#include "networkcoding/codetorrentencoder.h"
#include <libcpphaggle/Reference.h>

typedef Reference<codetorrentencoder> codetorrentencoderref;

typedef Map<string, codetorrentencoderref> encoder_t;
typedef Reference<encoder_t> encoder_ref_t;

typedef Map<string,DataObjectRef> dataobjectstorage_t;
typedef Reference<dataobjectstorage_t> dataobjectstorage_ref_t;

typedef Map<string,string> networkCodedBlockSendEventTime_t;
typedef Reference<networkCodedBlockSendEventTime_t> networkCodedBlockSendEventTime_ref_t;

class NetworkCodingEncoderStorage {
public:
    NetworkCodingEncoderStorage();
    virtual ~NetworkCodingEncoderStorage();

    codetorrentencoderref getEncoder(string parentDataObjectId,string filePath,size_t blockSize);
    void addDataObject(string originalDataObjectId,const DataObjectRef originalDataObject);
    const DataObjectRef getDataObject(string originalDataObjectId);

    const DataObjectRef getDataObjectById(string id);

    void trackSendEvent(string parentDataObjectId);
    void ageOffBlocks(double maxAgeBlock);

    void deleteFromStorageByDataObjectId(string parentDataObjectId);
    void deleteTrackingInfoByDataObjectId(string parentDataObjectId); // CBMEN, HL

private:
    encoder_ref_t encoderStorage;
    dataobjectstorage_ref_t dataobjectstorage;
    networkCodedBlockSendEventTime_ref_t networkCodedBlockSendEventTime;
};

#endif /* NETWORKENCODERSTORAGE_H_ */
