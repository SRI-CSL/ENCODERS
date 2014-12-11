/* Copyright (c) 2014 SRI International and Suns-tech Incorporated and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "NetworkCodingDataObjectUtility.h"
#include "networkcoding/NetworkCodingConstants.h"
#include "fragmentation/FragmentationConstants.h"
#include "stringutils/CSVUtility.h"

// This include is for the HAGGLE_ATTR_CONTROL_NAME attribute name.
// TODO: remove dependency on libhaggle header
#include "../libhaggle/include/libhaggle/ipc.h"

NetworkCodingDataObjectUtility::NetworkCodingDataObjectUtility() {
    this->networkCodingConfiguration = new NetworkCodingConfiguration();
}

NetworkCodingDataObjectUtility::~NetworkCodingDataObjectUtility() {
    if (this->networkCodingConfiguration) {
        delete this->networkCodingConfiguration;
        this->networkCodingConfiguration = NULL;
    }
}

bool NetworkCodingDataObjectUtility::isNetworkCodedDataObject(DataObjectRef dataObject) {

    if (!dataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return false;
    }

    HAGGLE_DBG2("Checking if data object %s is a networkcoded block\n", dataObject->getIdStr());

    const Attribute* attributeIsNetworkCoding = new Attribute(HAGGLE_ATTR_NETWORKCODING_NAME,
            "TRUE");
    bool hasNetworkCodingAttribute = dataObject->hasAttribute(*attributeIsNetworkCoding);
    delete attributeIsNetworkCoding;

    HAGGLE_DBG2("Data object %s has network coding attribute: %d\n",
		dataObject->getIdStr(), hasNetworkCodingAttribute);

    return hasNetworkCodingAttribute;
}

// CBMEN, HL, Begin
bool NetworkCodingDataObjectUtility::isFragmentationDataObject(DataObjectRef dataObject) {
    if (!dataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return false;
    }

        HAGGLE_DBG2("Checking if data object %s is a fragment (but not a block)\n", dataObject->getIdStr());

    const Attribute* attributeIsFragmentation = new Attribute(
            HAGGLE_ATTR_FRAGMENTATION_NAME, "TRUE");
    bool hasFragmentationAttribute = dataObject->hasAttribute(
            *attributeIsFragmentation);
    delete attributeIsFragmentation;

    HAGGLE_DBG2("Data object %s has fragmentation attribute: %d\n",
            dataObject->getIdStr(), hasFragmentationAttribute);

    if(!hasFragmentationAttribute) return false;

    // MOS
    const Attribute* attributeIsNetworkCoding = new Attribute(HAGGLE_ATTR_NETWORKCODING_NAME, "TRUE");
    bool hasNetworkCodingAttribute = dataObject->hasAttribute(*attributeIsNetworkCoding);
    delete attributeIsNetworkCoding;

    HAGGLE_DBG2("Data object %s has network coding attribute: %d\n",
            dataObject->getIdStr(), hasNetworkCodingAttribute);

    if(hasNetworkCodingAttribute) return false;

    return true;
}

const string NetworkCodingDataObjectUtility::getFragmentationOriginalDataObjectId(DataObjectRef fragmentationDataObject) {
    if (!fragmentationDataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return "";
    }

    const Attribute* attributeDataObjectId = fragmentationDataObject->getAttribute(
            HAGGLE_ATTR_FRAGMENTATION_PARENT_DATAOBJECT_ID, "*", 1.0);
    if (NULL == attributeDataObjectId) {
        HAGGLE_DBG2("No parent data object id attribute found\n");
        return (char*) NULL;
    }
    const string& originalDataObjectId = attributeDataObjectId->getValue();
    HAGGLE_DBG2("Found parent data object id %s\n", originalDataObjectId.c_str());

    if (originalDataObjectId.c_str() == NULL || originalDataObjectId.empty()
            || originalDataObjectId.length() < 1) {
        HAGGLE_DBG2("Invalid parent data object for block %s\n",
                fragmentationDataObject->getIdStr());
        return (char*) NULL;
    }

    return originalDataObjectId;
}
// CBMEN, HL, End

bool NetworkCodingDataObjectUtility::isHaggleIpc(DataObjectRef dataObject) {

    if (!dataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return false;
    }

    HAGGLE_DBG2("Checking if data object %s has Haggle IPC attribute\n", dataObject->getIdStr());

    const Attribute* attributeIsHaggleIPC = new Attribute(HAGGLE_ATTR_CONTROL_NAME, "*");
    bool hasHaggleIPCAttribute = dataObject->hasAttribute(*attributeIsHaggleIPC);
    delete attributeIsHaggleIPC;

    HAGGLE_DBG2("Data object %s has Haggle IPC attribute: %d\n",
		dataObject->getIdStr(), hasHaggleIPCAttribute);

    return hasHaggleIPCAttribute;
}

bool NetworkCodingDataObjectUtility::shouldBeNetworkCodedCheckTargetNode(DataObjectRef dataObject,NodeRef targetNode) {
	bool dataObjectShouldBeNetworkCoded = this->shouldBeNetworkCoded(dataObject,targetNode);
	//short circuit and return early
	if(!dataObjectShouldBeNetworkCoded) {
		return dataObjectShouldBeNetworkCoded;
	}

	bool isExists = false;

	if( this->networkCodingConfiguration->isEnabledForAllTargets()) {
		HAGGLE_DBG2("Network coding turned on for al targets\n");
		isExists = true;
	}
	else {
		std::vector<std::string> whitelistTargetNodes = this->networkCodingConfiguration->getWhitelistTargetNodeNames();
		std::string targetNodeName = targetNode->getName().c_str();
		isExists = itemInVector(targetNodeName,whitelistTargetNodes);
		HAGGLE_DBG2("IsTurning on network coding for node %s is %d\n",targetNodeName.c_str(),isExists);
	}

	return isExists;

}

bool NetworkCodingDataObjectUtility::shouldBeNetworkCoded(DataObjectRef dataObject,NodeRef targetNode) {
    if (!dataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return false;
    }

    HAGGLE_DBG2("Checking if data object %s should be networkcoded - isControlMessage=%d isNodeDescription=%d isThisNodeDescription=%d\n",
            dataObject->getIdStr(), dataObject->isControlMessage(), dataObject->isNodeDescription(), dataObject->isThisNodeDescription());
    if (!this->networkCodingConfiguration->isNetworkCodingEnabled(dataObject,targetNode)) {
        HAGGLE_DBG2("Network coding not enabled, so not coding data object\n");
        return false;
    }

    if (dataObject->isControlMessage() || dataObject->isNodeDescription()
            || dataObject->isThisNodeDescription()) {
        HAGGLE_DBG2("Data object %s should not be networkcoded\n", dataObject->getIdStr());
        return false;
    }

    bool isNetworkCodedObject = this->isNetworkCodedDataObject(dataObject);
    if (isNetworkCodedObject) {
        HAGGLE_DBG2("Data object %s already network coded\n", dataObject->getIdStr());
        return false;
    }

    else if (isHaggleIpc(dataObject)) {
        HAGGLE_DBG2("Haggle IPC data object %s should not be networkcoded\n", dataObject->getIdStr());
        return false;
    }
    // CBMEN, HL - remove check so that data objects with only ciphertext can also get network coded
    // } else if (dataObject->getFilePath().length() < 1) {
        // HAGGLE_DBG2("No network coding - file path length is < 1 %d\n", dataObject->getFilePath().length());
        //const Attributes* originalAttributes = dataObject->getAttributes();
        //for (Attributes::const_iterator it = originalAttributes->begin();
        //        it != originalAttributes->end(); it++) {
        //    const Attribute attr = (*it).second;
        //    HAGGLE_DBG2("attribute=%s\n", attr.getString().c_str());
	//}
        // return false;
    // }

    //source only coding
    bool isSourceCoding = networkCodingConfiguration->isSourceOnlyCodingEnabled();
    if(isSourceCoding) {
	//check if we are the source
        bool isWeAreTheSource = (dataObject->getSignee() == networkCodingConfiguration->getNodeIdStr());
        if(!isWeAreTheSource) {
		HAGGLE_DBG2("we are not the source for id=%s source %s != we %s\n",
			dataObject->getIdStr(),dataObject->getSignee().c_str(),networkCodingConfiguration->getNodeIdStr().c_str() );
		return false;
        }
    }

    //break down if statements as we make an io call to get the file size
    // FILE *filePointer = fopen(dataObject->getFilePath().c_str(), "rb");
    // fseek(filePointer, 0, SEEK_END);
    // long fileSize = ftell(filePointer);
    // fclose(filePointer);
    long fileSize = getFileLength(dataObject); // MOS - this is faster - also avoids possible error on fopen

    if (fileSize < networkCodingConfiguration->getMinimumFileSize()) {
        HAGGLE_DBG2("No network coding - filesize=%d smaller than minimumFileSize=%d\n",
                fileSize, networkCodingConfiguration->getMinimumFileSize());
        return false;
    } else {
        HAGGLE_DBG2("Data object %s should be networkcoded\n", dataObject->getIdStr());
        return true;
    }

}

const string NetworkCodingDataObjectUtility::getOriginalDataObjectId(
        DataObjectRef networkCodedDataObject) {
    if (!networkCodedDataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return "";
    }

    const Attribute* attributeDataObjectId = networkCodedDataObject->getAttribute(
            HAGGLE_ATTR_PARENT_DATAOBJECT_ID, "*", 1.0);
    if (NULL == attributeDataObjectId) {
        HAGGLE_DBG2("No parent data object id attribute found\n");
        return (char*) NULL;
    }
    const string& originalDataObjectId = attributeDataObjectId->getValue();
    HAGGLE_DBG2("Found parent data object id %s\n", originalDataObjectId.c_str());

    if (originalDataObjectId.c_str() == NULL || originalDataObjectId.empty()
            || originalDataObjectId.length() < 1) {
        HAGGLE_DBG2("Invalid parent data object for block %s\n",
                networkCodedDataObject->getIdStr());
        return (char*) NULL;
    }

    return originalDataObjectId;
}

int NetworkCodingDataObjectUtility::getOriginalDataObjectLength(
        DataObjectRef networkCodedDataObject) {
    if (!networkCodedDataObject) {
        HAGGLE_DBG2("Unexpected null data object\n");
        return 0;
    }

    const Attribute* attributeDataObjectLength = networkCodedDataObject->getAttribute(
            HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_LEN, "*", 1.0);
    if (NULL == attributeDataObjectLength) {
        HAGGLE_DBG2("No parent data object length attribute found\n");
        return 0;
    }
    const string& originalDataObjectLength = attributeDataObjectLength->getValue();
    HAGGLE_DBG2("Found parent data object length %s\n", originalDataObjectLength.c_str());

    if (originalDataObjectLength.c_str() == NULL || originalDataObjectLength.empty()
            || originalDataObjectLength.length() < 1) {
        HAGGLE_DBG2("Invalid parent data object length for block %s\n",
                networkCodedDataObject->getIdStr());
        return 0;
    }

    return atoi(originalDataObjectLength.c_str());
}

const string NetworkCodingDataObjectUtility::checkAndGetOriginalDataObjectId(
        DataObjectRef networkCodedDataObject) {
    if (this->isNetworkCodedDataObject(networkCodedDataObject)) {
        const string& originalDataObjectId = this->getOriginalDataObjectId(networkCodedDataObject);
        if (originalDataObjectId.c_str() == NULL || originalDataObjectId.empty()
                || originalDataObjectId.length() < 1) {
	    HAGGLE_DBG2("returning null\n");
            return (char*) NULL;
        }
        return originalDataObjectId;
    } else {
        HAGGLE_DBG2("returning null\n");
        return (char*) NULL;
    }
}

DataObjectIdRef NetworkCodingDataObjectUtility::convertDataObjectIdStringToDataObjectIdType(
        string dataObjectId) {
    unsigned char* binaryDataObjectId = new unsigned char[DATAOBJECT_ID_LEN];
    memset(binaryDataObjectId, 0, DATAOBJECT_ID_LEN);
    unsigned char* binaryDataObjectId_ptr = binaryDataObjectId;

    size_t lenstring = dataObjectId.size();
    const char* dataObjectId_ptr = dataObjectId.c_str();
    for (unsigned int i = 0; i < lenstring; i += 2) {
        unsigned int number;
        sscanf(dataObjectId_ptr + i, "%02x", &number);
        binaryDataObjectId_ptr[0] = static_cast<unsigned char>(number);
        binaryDataObjectId_ptr = binaryDataObjectId_ptr + 1;
    }

    this->convertDataObjectIdToHex(binaryDataObjectId);

    DataObjectIdRef haggleDataObjectId(binaryDataObjectId, DATAOBJECT_ID_LEN);
    delete[] binaryDataObjectId;

    return haggleDataObjectId;
}

string NetworkCodingDataObjectUtility::convertDataObjectIdToHex(
        const unsigned char* dataObjectIdUnsignedChar) {
    string dataObjectIdhex;
    int len = 0;
    char idStr[MAX_DATAOBJECT_ID_STR_LEN];
    // Generate a readable string of the Id
    for (int i = 0; i < DATAOBJECT_ID_LEN; i++) {
        len += sprintf(idStr + len, "%02x", dataObjectIdUnsignedChar[i] & 0xff);
    }

    dataObjectIdhex = idStr;
    // HAGGLE_DBG2("converted hexid=%s\n", dataObjectIdhex.c_str());
    return dataObjectIdhex;
}

bool NetworkCodingDataObjectUtility::copyAttributesToDataObject(DataObjectRef dataObjectOriginal,
        DataObjectRef networkCodedBlock) {
    HAGGLE_DBG2("Copying attributes from block to parent data object\n");

    string parentSignature = "";
    string parentSignee = "";

    const Attributes * attributes = networkCodedBlock->getAttributes();
    for (Attributes::const_iterator it = attributes->begin(); it != attributes->end(); it++) {
        const Attribute attribute = (*it).second;
        string name = attribute.getName();
        string value = attribute.getValue();
        unsigned long weight = attribute.getWeight();

        HAGGLE_DBG2("Attribute name %s\n", name.c_str());
        if (name == HAGGLE_ATTR_NETWORKCODING_PARENT_CREATION_TIME) {
            HAGGLE_DBG2( "Previous create time was %s - setting create time to %s\n",
                    dataObjectOriginal->getCreateTime().getAsString().c_str(), value.c_str());
            dataObjectOriginal->setCreateTime(value);
            dataObjectOriginal->setReceiveTime(Timeval::now()); // MOS
            HAGGLE_DBG2("Parent create time is now %s\n",
                    dataObjectOriginal->getCreateTime().getAsString().c_str());
        } else if (name == HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_SIGNATURE) {
            HAGGLE_DBG2("Parent signature is %s\n", value.c_str());
            parentSignature = value;
        } else if (name == HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_SIGNEE) {
            HAGGLE_DBG2("Parent Signee is %s\n", value.c_str());
            parentSignee = value;
        } else if (name != HAGGLE_ATTR_NETWORKCODING_NAME
                && name != HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_NAME
                && name != HAGGLE_ATTR_PARENT_DATAOBJECT_ID
                && name != HAGGLE_ATTR_NETWORKCODING_PARENT_ORIG_LEN) {
            HAGGLE_DBG2("Adding name=%s value=%s weight=%d\n",
                    name.c_str(), value.c_str(), weight);
            bool isAddAttribute = dataObjectOriginal->addAttribute(name, value, weight);
            if (!isAddAttribute) {
                HAGGLE_ERR("Unable to add attribute=%s\n", name.c_str());
                return false;
            }
        }

    }

    //setting signee and signature
    if (parentSignee.empty() || parentSignature.empty()) {
        HAGGLE_DBG2("Parent has no signature/signee - parentSignee=%s parentSignature=%s\n",
                parentSignee.c_str(), parentSignature.c_str());
        return true; // MOS - allow fragments without signature to go through (verified later)
    }

    dataObjectOriginal->setSignee(parentSignee);

    unsigned char* parentSignatureUnsignedChar = NULL;
    size_t signature_len = 0;
    base64_decode_context ctx;

    base64_decode_ctx_init(&ctx);

    if (!base64_decode_alloc(&ctx, parentSignature.c_str(), parentSignature.length(),
            (char **) &parentSignatureUnsignedChar, &signature_len)) {
        HAGGLE_ERR("Could not create signature from attribute\n");
        return false;
    }

    dataObjectOriginal->setSignature(parentSignee, parentSignatureUnsignedChar,
            signature_len);
    return true;
}


const string& NetworkCodingDataObjectUtility::getFilePath(DataObjectRef dataObjectRef) {
    if (dataObjectRef->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {
        return dataObjectRef->getEncryptedFilePath();
    }
    return dataObjectRef->getFilePath();
}

const string& NetworkCodingDataObjectUtility::getFileName(DataObjectRef dataObjectRef) {
    return dataObjectRef->getFileName();
}

size_t NetworkCodingDataObjectUtility::getFileLength(DataObjectRef dataObjectRef) {
    if (dataObjectRef->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {
        return dataObjectRef->getEncryptedFileLength();
    }
    return dataObjectRef->getDataLen();
}

int 
NetworkCodingDataObjectUtility::getBlockSize()
{
    return networkCodingConfiguration->getBlockSize();
}
