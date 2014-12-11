/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef FORWARDINGMANAGERHELPER_H_
#define FORWARDINGMANAGERHELPER_H_



#include "Manager.h"
#include "DataObject.h"
#include "Event.h"
#include "Node.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "networkcoding/manager/NetworkCodingConfiguration.h"

class ForwardingManagerHelper {
public:
    ForwardingManagerHelper(HaggleKernel *_kernel);
    virtual ~ForwardingManagerHelper();

    void addDataObjectEventWithDelay(DataObjectRef dataObjectRef,NodeRef nodeRef);

private:
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
    NetworkCodingConfiguration* networkCodingConfiguration;
    HaggleKernel *kernel;
};

#endif /* FORWARDINGMANAGERHELPER_H_ */
