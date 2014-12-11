/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#include "NetworkCodingDecodingTask.h"

NetworkCodingDecodingTask::NetworkCodingDecodingTask(NetworkCodingDecodingTaskType_t _type,
        DataObjectRef _dataObject,NodeRef _node) :
        type(_type), dataObject(_dataObject), node(_node), completed(false) {

}

NetworkCodingDecodingTask::~NetworkCodingDecodingTask() {

}

bool operator==(const NetworkCodingDecodingTask& a, const NetworkCodingDecodingTask& b) {
    bool dataObjectEqual = operator==(a.dataObject,b.dataObject);
    bool typesEqual = a.type == b.type;

    bool allEqual = dataObjectEqual && typesEqual;

    return allEqual;
}

bool operator!=(const NetworkCodingDecodingTask& a, const NetworkCodingDecodingTask& b) {
    bool dataObjectEqual = operator==(a.dataObject,b.dataObject);
    bool typesEqual = a.type == b.type;

    bool allEqual = dataObjectEqual && typesEqual;

    return !allEqual;
}
