/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef APPLICATIONMANAGERHELPER_H_
#define APPLICATIONMANAGERHELPER_H_

#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "DataObject.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"

class ApplicationManagerHelper {
public:
    ApplicationManagerHelper();
    virtual ~ApplicationManagerHelper();
    bool shouldNotSendToApplication(DataObjectRef dataObjectRef);
private:
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
    FragmentationDataObjectUtility* fragmentationDataObjectUtility;
};

#endif /* APPLICATIONMANAGERHELPER_H_ */
