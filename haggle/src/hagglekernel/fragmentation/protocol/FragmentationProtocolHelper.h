/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONPROTOCOLHELPER_H_
#define FRAGMENTATIONPROTOCOLHELPER_H_

#include "fragmentation/utility/FragmentationDataObjectUtility.h"

class FragmentationProtocolHelper {
public:
	FragmentationProtocolHelper();
	virtual ~FragmentationProtocolHelper();

	Event* onSendDataObject(DataObjectRef& originalDataObjectRef, NodeRef& node, EventType send_data_object_actual_event);

private:
	FragmentationDataObjectUtility* fragmentationDataObjectUtility;
};

#endif /* FRAGMENTATIONPROTOCOLHELPER_H_ */
