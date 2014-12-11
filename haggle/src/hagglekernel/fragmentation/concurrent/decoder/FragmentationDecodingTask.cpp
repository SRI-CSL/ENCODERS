/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationDecodingTask.h"

FragmentationDecodingTask::FragmentationDecodingTask(
		FragmentationDecodingTaskType_t _type, DataObjectRef _dataObject,
		NodeRef _node) :
		type(_type), dataObject(_dataObject), node(_node), completed(false){

}

FragmentationDecodingTask::~FragmentationDecodingTask() {

}

bool operator==(const FragmentationDecodingTask& a, const FragmentationDecodingTask& b) {
    bool dataObjectEqual = operator==(a.dataObject,b.dataObject);
    bool typesEqual = a.type == b.type;

    bool allEqual = dataObjectEqual && typesEqual;

    return allEqual;
}

bool operator!=(const FragmentationDecodingTask& a, const FragmentationDecodingTask& b) {
    bool dataObjectEqual = operator==(a.dataObject,b.dataObject);
    bool typesEqual = a.type == b.type;

    bool allEqual = dataObjectEqual && typesEqual;

    return !allEqual;
}
