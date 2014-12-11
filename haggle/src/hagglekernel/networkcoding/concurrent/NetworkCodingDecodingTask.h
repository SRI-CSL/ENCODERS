/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGDECODINGTASK_H_
#define NETWORKCODINGDECODINGTASK_H_

#include "networkcoding/concurrent/NetworkCodingDecodingTaskType.h"
#include "DataObject.h"

class NetworkCodingDecodingTask;

typedef Reference<NetworkCodingDecodingTask> NetworkCodingDecodingTaskRef;

class NetworkCodingDecodingTask {
public:
    NetworkCodingDecodingTask(NetworkCodingDecodingTaskType_t _type,
            DataObjectRef _dataObject,NodeRef _node);
    ~NetworkCodingDecodingTask();

    bool isCompleted() const {
        return completed;
    }

    const DataObjectRef getDataObject() const {
        return dataObject;
    }

    NetworkCodingDecodingTaskType_t getType() const {
        return type;
    }

    const NodeRef getNode() const {
        return node;
    }

    friend bool operator==(const NetworkCodingDecodingTask&a, const NetworkCodingDecodingTask&b);
    friend bool operator!=(const NetworkCodingDecodingTask&a, const NetworkCodingDecodingTask&b);

private:
    NetworkCodingDecodingTaskType_t type;
    DataObjectRef dataObject;
    NodeRef node;
    bool completed;
};

#endif /* NETWORKCODINGDECODINGTASK_H_ */
