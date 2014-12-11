/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef FRAGMENTATIONDECODERSERVICE_H_
#define FRAGMENTATIONDECODERSERVICE_H_

#include "DataStore.h"

#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/storage/FragmentationDecoderStorage.h"

class FragmentationDecoderService {
public:
	FragmentationDecoderService(FragmentationDecoderStorage* _fragmentationDecoderStorage,FragmentationDataObjectUtility* _fragmentationDataObjectUtility);
	virtual ~FragmentationDecoderService();

	DataObjectRef decode(DataObjectRef fragmentDataObjectRef,size_t fragmentSize);


private:
	FragmentationDataObjectUtility* fragmentationDataObjectUtility;
	FragmentationDecoderStorage* fragmentationDecoderStorage;
};

#endif /* FRAGMENTATIONDECODERSERVICE_H_ */
