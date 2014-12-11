/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGENCODERTASK_H_
#define NETWORKCODINGENCODERTASK_H_

#include "DataObject.h"
#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTaskType.h"

class NetworkCodingEncoderTask;

typedef Reference<NetworkCodingEncoderTask> NetworkCodingEncoderTaskRef;

class NetworkCodingEncoderTask {
public:
    NetworkCodingEncoderTask(NetworkCodingEncoderTaskType_t _type,
            DataObjectRef _dataObject,NodeRefList _nodeRefList);
    ~NetworkCodingEncoderTask();

    const DataObjectRef getDataObject() const {
        return dataObject;
    }

    const NetworkCodingEncoderTaskType_t getType() const {
        return type;
    }

    const NodeRefList getNodeRefList() const {
        return nodeRefList;
    }

    friend bool operator==(const NetworkCodingEncoderTask&a, const NetworkCodingEncoderTask&b);
    friend bool operator!=(const NetworkCodingEncoderTask&a, const NetworkCodingEncoderTask&b);

private:
    NetworkCodingEncoderTaskType_t type;
    DataObjectRef dataObject;
    NodeRefList nodeRefList;
};

#endif /* NETWORKCODINGENCODERTASK_H_ */
