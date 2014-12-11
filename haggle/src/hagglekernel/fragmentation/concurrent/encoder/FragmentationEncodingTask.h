/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONENCODINGTASK_H_
#define FRAGMENTATIONENCODINGTASK_H_

#include "DataObject.h"
#include "fragmentation/concurrent/encoder/FragmentationEncodingTaskType.h"

class FragmentationEncodingTask;

typedef Reference<FragmentationEncodingTask> FragmentationEncodingTaskRef;


class FragmentationEncodingTask {
public:
	FragmentationEncodingTask(FragmentationEncodingTaskType_t _type,
            DataObjectRef _dataObject,NodeRefList _nodeRefList);
    virtual ~FragmentationEncodingTask();

    const DataObjectRef getDataObject() const {
        return dataObject;
    }

    const FragmentationEncodingTaskType_t getType() const {
        return type;
    }

    const NodeRefList getNodeRefList() const {
        return nodeRefList;
    }

    friend bool operator==(const FragmentationEncodingTask&a, const FragmentationEncodingTask&b);
    friend bool operator!=(const FragmentationEncodingTask&a, const FragmentationEncodingTask&b);

private:
	FragmentationEncodingTaskType_t type;
    DataObjectRef dataObject;
    NodeRefList nodeRefList;
};

#endif /* FRAGMENTATIONENCODINGTASK_H_ */
