/* Copyright (c) 2014 SRI International and Suns-tech Incorporated and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Hasnain Lakhani (HL)
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef NETWORKCODINGDATAOBJECTUTILITY_H_
#define NETWORKCODINGDATAOBJECTUTILITY_H_

#include "DataObject.h"
#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "networkcoding/databitobject/DataObjectIdRef.h"

class NetworkCodingDataObjectUtility {
public:
    NetworkCodingDataObjectUtility();
    virtual ~NetworkCodingDataObjectUtility();

    bool isNetworkCodedDataObject(DataObjectRef dataObject);
    bool isFragmentationDataObject(DataObjectRef dataObject); // CBMEN, HL

    bool isHaggleIpc(DataObjectRef dataObject);

    bool shouldBeNetworkCodedCheckTargetNode(DataObjectRef dataObject,NodeRef targetNode);

    /**
     * should only be called when know that it is a network coded data object
     */
    const string getOriginalDataObjectId(DataObjectRef networkCodedDataObject);

    const string getFragmentationOriginalDataObjectId(DataObjectRef fragmentationDataObject); // CBMEN, HL

    const string checkAndGetOriginalDataObjectId(
            DataObjectRef networkCodedDataObject);

    DataObjectIdRef convertDataObjectIdStringToDataObjectIdType(
            string dataObjectId);

    bool copyAttributesToDataObject(DataObjectRef dataObjectOriginal,
            DataObjectRef networkCodedBlock);
        
    // CBMEN, HL
    const string& getFilePath(DataObjectRef dataObjectRef);
    const string& getFileName(DataObjectRef dataObjectRef);
    size_t getFileLength(DataObjectRef dataObjectRef);

    string convertDataObjectIdToHex(const unsigned char* dataObjectIdUnsignedChar);

    // SW: helper to retrieve size of original data object
    int getOriginalDataObjectLength(DataObjectRef networkCodedDataObject);

    // SW: helper to retrieve block size
    int getBlockSize();
        
private:
    bool shouldBeNetworkCoded(DataObjectRef dataObject,NodeRef targetNode);

    NetworkCodingConfiguration* networkCodingConfiguration;
};

#endif /* NETWORKCODINGDATAOBJECTUTILITY_H_ */
