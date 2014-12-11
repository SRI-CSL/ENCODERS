/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */


#ifndef FRAGMENTATIONFORWARDINGMANAGERHELPER_H_
#define FRAGMENTATIONFORWARDINGMANAGERHELPER_H_

#include "Manager.h"
#include "DataObject.h"
#include "Event.h"
#include "Node.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"

class FragmentationForwardingManagerHelper {
public:
	FragmentationForwardingManagerHelper(HaggleKernel *_kernel);
	virtual ~FragmentationForwardingManagerHelper();

	void addDataObjectEventWithDelay(DataObjectRef dataObjectRef,NodeRef nodeRef);
private:
	FragmentationDataObjectUtility* fragmentationDataObjectUtility;
	FragmentationConfiguration* fragmentationConfiguration;
	HaggleKernel *kernel;
};

#endif /* FRAGMENTATIONFORWARDINGMANAGERHELPER_H_ */
