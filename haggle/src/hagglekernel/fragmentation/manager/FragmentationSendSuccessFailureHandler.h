/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef FRAGMENTATIONSENDSUCCESSFAILUREHANDLER_H_
#define FRAGMENTATIONSENDSUCCESSFAILUREHANDLER_H_

#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/storage/FragmentationEncoderStorage.h"
#include "Event.h"

class FragmentationSendSuccessFailureHandler {
public:
	FragmentationSendSuccessFailureHandler(
			FragmentationDataObjectUtility* _fragmentationDataObjectUtility,
			FragmentationEncoderStorage* _fragmentationEncoderStorage);
	virtual ~FragmentationSendSuccessFailureHandler();

	Event* retrieveOriginalDataObjectAndGenerateEvent(EventType type,
			DataObjectRef dataObjectRef, NodeRef node);
private:
	FragmentationDataObjectUtility* fragmentationDataObjectUtility;
	FragmentationEncoderStorage* fragmentationEncoderStorage;
};

#endif /* FRAGMENTATIONSENDSUCCESSFAILUREHANDLER_H_ */
