/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Hasnain Lakhani (HL)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONDATAOBJECTUTILITY_H_
#define FRAGMENTATIONDATAOBJECTUTILITY_H_

#include "dataobject/DataObjectTypeIdentifierUtility.h"
#include "DataObject.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"

typedef struct _FragmentationPositionInfo {
	size_t startPosition;
	size_t actualFragmentSize;
} FragmentationPositionInfo;

typedef struct _FragmentParentDataObjectInfo {
	string dataObjectId;
	size_t fileSize;
	string fileName;
	string createTime;
	//size_t sequenceNumber;
	List<string> sequenceNumberList;
	size_t totalNumberOfFragments;
} FragmentParentDataObjectInfo;

class FragmentationDataObjectUtility {
public:
	FragmentationDataObjectUtility();
	virtual ~FragmentationDataObjectUtility();

	bool isFragmentationDataObject(DataObjectRef dataObject);

	FragmentationPositionInfo calculateFragmentPositionInfoEncoder(size_t sequenceNumber,
			size_t fragmentSize, size_t fileSize);

	FragmentationPositionInfo calculateFragmentationPositionInfo(size_t sequenceNumber,
			size_t fragmentSize, size_t fileSize);

	bool storeFragmentToDataObject(string originalDataObjectFilePath,
			string fragmentDataObjectFilePath, size_t parentDataObjectStartPosition, size_t fragmentSize, size_t fragmentStartPosition);

	FragmentParentDataObjectInfo getFragmentParentDataObjectInfo(
			DataObjectRef fragmentDataObjectRef);

	bool allocateFile(string filePath, size_t fileSize);

	size_t calculateTotalNumberOfFragments(size_t dataLen, size_t fragmentSize);

	bool shouldBeFragmented(DataObjectRef dataObject,NodeRef targetNode);

	const string getOriginalDataObjectId(DataObjectRef fragmentationDataObject);
	const string checkAndGetOriginalDataObjectId(DataObjectRef fragmentationDataObject);

	DataObjectIdRef convertDataObjectIdStringToDataObjectIdType(string dataObjectId);
	string convertDataObjectIdToHex(const unsigned char* dataObjectIdUnsignedChar);

    bool shouldBeFragmentedCheckTargetNode(DataObjectRef dataObject,NodeRef targetNode);

	bool copyAttributesToDataObject(DataObjectRef dataObjectOriginal,
			DataObjectRef fragmentationBlock);
	
	// CBMEN, HL
    const string& getFilePath(DataObjectRef dataObjectRef);
    const string& getFileName(DataObjectRef dataObjectRef);
    size_t getFileLength(DataObjectRef dataObjectRef);
private:
	FragmentationConfiguration* fragmentationConfiguration;
	DataObjectTypeIdentifierUtility* dataObjectTypeIdentifierUtility;
	NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
}
;

#endif /* FRAGMENTATIONDATAOBJECTUTILITY_H_ */
