/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef FRAGMENTATIONENCODERSERVICE_H_
#define FRAGMENTATIONENCODERSERVICE_H_

#include "DataObject.h"
#include "fragmentation/fragment/FragmentationFileUtility.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/storage/FragmentationEncoderStorage.h"
#include <libhaggle/list.h>

class FragmentationEncoderService {
public:
	FragmentationEncoderService(
			FragmentationFileUtility *_fragmentationFileUtility,
			FragmentationConfiguration* _fragmentationConfiguration,
			FragmentationEncoderStorage* _fragmentationEncoderStorage);
	virtual ~FragmentationEncoderService();

	DataObjectRef encodeDataObject(const DataObjectRef originalDataObject,
			size_t sequenceNumber, size_t fragmentSize,NodeRef nodeRef);

	bool addAttributes(DataObjectRef originalDataObject,
			DataObjectRef fragmentDataObject,
			string sequenceNumberList);

	List<DataObjectRef> getAllFragmentsForDataObject(
			DataObjectRef originalDataObject, size_t fragmentSize,NodeRef nodeRef);

	int* getIndexesOfFragmentsToCreate(size_t totalNumberOfFragments);

private:
	FragmentationFileUtility *fragmentationFileUtility;
	FragmentationDataObjectUtility *fragmentationDataObjectUtility;
	FragmentationConfiguration* fragmentationConfiguration;
	FragmentationEncoderStorage* fragmentationEncoderStorage;
};

#endif /* FRAGMENTATIONENCODERSERVICE_H_ */
