/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONDECODINGTASK_H_
#define FRAGMENTATIONDECODINGTASK_H_

#include "fragmentation/concurrent/decoder/FragmentationDecodingTaskType.h"
#include "DataObject.h"

class FragmentationDecodingTask;

typedef Reference<FragmentationDecodingTask> FragmentationDecodingTaskRef;

class FragmentationDecodingTask {
public:

	FragmentationDecodingTask(FragmentationDecodingTaskType_t _type,
            DataObjectRef _dataObject,NodeRef _node);
    ~FragmentationDecodingTask();

    bool isCompleted() const {
        return completed;
    }

    const DataObjectRef getDataObject() const {
        return dataObject;
    }

    FragmentationDecodingTaskType_t getType() const {
        return type;
    }

    const NodeRef getNode() const {
        return node;
    }

    friend bool operator==(const FragmentationDecodingTask&a, const FragmentationDecodingTask&b);
    friend bool operator!=(const FragmentationDecodingTask&a, const FragmentationDecodingTask&b);

private:
    FragmentationDecodingTaskType_t type;
    DataObjectRef dataObject;
    NodeRef node;
    bool completed;
};

#endif /* FRAGMENTATIONDECODINGTASK_H_ */
