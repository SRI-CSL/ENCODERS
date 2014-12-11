/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Yu-Ting Yu (yty)
 */

#ifndef NETWORKCODINGENCODERSERVICE_H_
#define NETWORKCODINGENCODERSERVICE_H_

#include <stdio.h>
#include <stdlib.h>

#include "DataObject.h"
#include "Event.h"

#include "networkcoding/NetworkCodingFileUtility.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "networkcoding/storage/NetworkCodingEncoderStorage.h"
#include "networkcoding/CodeTorrentUtility.h"
#include "networkcoding/manager/NetworkCodingConfiguration.h"

#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "networkcoding/singleblockencoder.h"

class NetworkCodingEncoderService {
public:
    NetworkCodingEncoderService(
            NetworkCodingFileUtility* _networkCodingFileUtility,
            NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility,
            NetworkCodingEncoderStorage* _networkCodingEncoderStorage,
            CodeTorrentUtility* _codeTorrentUtility,
            NetworkCodingConfiguration* _networkCodingConfiguration);
    ~NetworkCodingEncoderService();

    DataObjectRef encodeDataObject(const DataObjectRef originalDataObject);

    DataObjectRef encodeFragment(const DataObjectRef fragDataObject, const size_t fragmentSize);
	
private:
    bool addAttributes(DataObjectRef originalDataObject,DataObjectRef networkCodedDataObject);
    bool addAttributesFromFragDataObject(DataObjectRef fragDataObject,DataObjectRef networkCodedDataObject);
    bool writeCodedBlocks(FILE* pnetworkCodedBlockFile, codetorrentencoderref& encoderref, int numberOfBlocks);

    NetworkCodingFileUtility* networkCodingFileUtility;
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;

    NetworkCodingEncoderStorage* networkCodingEncoderStorage;
    CodeTorrentUtility* codeTorrentUtility;
    NetworkCodingConfiguration* networkCodingConfiguration;
    FragmentationDataObjectUtility* fragmentationDataObjectUtility;
};

#endif /* NETWORKCODINGENCODERSERVICE_H_ */
