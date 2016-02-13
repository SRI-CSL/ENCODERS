/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGDECODERSERVICE_H_
#define NETWORKCODINGDECODERSERVICE_H_

#include <stdio.h>
#include <stdlib.h>

#include "DataObject.h"
#include "Event.h"

#include "networkcoding/blocky/src/blockycoderfile.h"
#include "networkcoding/codetorrentdecoder.h"
#include "networkcoding/NetworkCodingConstants.h"
#include <libcpphaggle/Map.h>
#include "networkcoding/NetworkCodingFileUtility.h"
#include "networkcoding/storage/NetworkCodingDecoderStorage.h"
#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"

class NetworkCodingDecoderService {
public:
    NetworkCodingDecoderService(
            NetworkCodingDecoderStorage* _networkCodingDecoderStorage,
            NetworkCodingFileUtility* _networkCodingFileUtility,
            NetworkCodingConfiguration* _networkCodingConfiguration,
            NetworkCodingDataObjectUtility* _networkCodingDataObjectUtility);
    ~NetworkCodingDecoderService();

    DataObjectRef decodeDataObject(const DataObjectRef& dObj);
private:
    NetworkCodingFileUtility* networkCodingFileUtility;
    NetworkCodingDecoderStorage* networkCodingDecoderStorage;
    NetworkCodingConfiguration* networkCodingConfiguration;
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
};

#endif /* NETWORKCODINGDECODERSERVICE_H_ */
