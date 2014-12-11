/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

/*
 * DataObjectTypeIdentifierUtility.cpp
 *
 *  Created on: Aug 30, 2012
 *      Author: jjoy
 */

#include "DataObjectTypeIdentifierUtility.h"

// This include is for the HAGGLE_ATTR_CONTROL_NAME attribute name.
// TODO: remove dependency on libhaggle header
#include "../libhaggle/include/libhaggle/ipc.h"

DataObjectTypeIdentifierUtility::DataObjectTypeIdentifierUtility() {

}

DataObjectTypeIdentifierUtility::~DataObjectTypeIdentifierUtility() {

}

bool DataObjectTypeIdentifierUtility::isHaggleIpc(DataObjectRef dataObject) {

        if (!dataObject) {
	  HAGGLE_DBG2("Unexpected null data object\n");
	  return false;
	}

	// HAGGLE_DBG2("Checking if data object %s has Haggle IPC attribute\n", dataObject->getIdStr());

	const Attribute* attributeIsHaggleIPC = new Attribute(HAGGLE_ATTR_CONTROL_NAME, "*");
	bool hasHaggleIPCAttribute = dataObject->hasAttribute(*attributeIsHaggleIPC);
	delete attributeIsHaggleIPC;

	HAGGLE_DBG2("Data object %s has Haggle IPC attribute: %d\n",
		    dataObject->getIdStr(), hasHaggleIPCAttribute);

	return hasHaggleIPCAttribute;
}

// CBMEN, HL, Begin
bool DataObjectTypeIdentifierUtility::hasNoFileAssociated(DataObjectRef dataObjectRef) {
    if (dataObjectRef->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {
        if (dataObjectRef->getEncryptedFilePath().length() < 0) {
            HAGGLE_DBG2("Data object %s has no associated file\n", dataObjectRef->getIdStr());
            return true;
        }
        return false;
    }
	if (dataObjectRef->getFilePath().length() < 1) {
	        HAGGLE_DBG2("Data object %s has no associated file\n", dataObjectRef->getIdStr());
		//const Attributes* originalAttributes = dataObjectRef->getAttributes();
		//for (Attributes::const_iterator it = originalAttributes->begin();
		//		it != originalAttributes->end(); it++) {
		//	const Attribute attr = (*it).second;
		//	HAGGLE_ERR("attribute=|%s|\n", attr.getString().c_str());
		//}
		return true;
	}
	return false;
}
// CBMEN, HL, End

string DataObjectTypeIdentifierUtility::convertDataObjectIdToHex(const unsigned char* dataObjectIdUnsignedChar) {
    string dataObjectIdhex;
    int len = 0;
    char idStr[MAX_DATAOBJECT_ID_STR_LEN];
    // Generate a readable string of the Id
    for (int i = 0; i < DATAOBJECT_ID_LEN; i++) {
            len += sprintf(idStr + len, "%02x", dataObjectIdUnsignedChar[i] & 0xff);
    }

    dataObjectIdhex = idStr;
    // HAGGLE_DBG("converted hexid=|%s|\n",dataObjectIdhex.c_str());
    return dataObjectIdhex;
}

DataObjectIdRef DataObjectTypeIdentifierUtility::convertDataObjectIdStringToDataObjectIdType(string dataObjectId) {
    unsigned char* binaryDataObjectId = new unsigned char[DATAOBJECT_ID_LEN];
    memset(binaryDataObjectId,0,DATAOBJECT_ID_LEN);
    unsigned char* binaryDataObjectId_ptr = binaryDataObjectId;

    size_t lenstring = dataObjectId.size();
    const char* dataObjectId_ptr = dataObjectId.c_str();
    for(unsigned int i=0;i<lenstring;i+=2) {
        unsigned int number;
        sscanf(dataObjectId_ptr+i, "%02x", &number);
        binaryDataObjectId_ptr[0] = static_cast<unsigned char>(number);
        binaryDataObjectId_ptr = binaryDataObjectId_ptr+1;
    }

    this->convertDataObjectIdToHex(binaryDataObjectId);

    DataObjectIdRef haggleDataObjectId(binaryDataObjectId,DATAOBJECT_ID_LEN);
    delete[] binaryDataObjectId;

    return haggleDataObjectId;
}
