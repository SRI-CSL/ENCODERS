/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "FragmentationEncodingTask.h"

FragmentationEncodingTask::FragmentationEncodingTask(
		FragmentationEncodingTaskType_t _type, DataObjectRef _dataObject,
		NodeRefList _nodeRefList) :
		type(_type), dataObject(_dataObject), nodeRefList(_nodeRefList){

}

FragmentationEncodingTask::~FragmentationEncodingTask() {

}


bool operator==(const FragmentationEncodingTask& a,
		const FragmentationEncodingTask& b) {
	//FIXME need to check nodes?
	bool dataObjectsEqual = operator==(a.dataObject, b.dataObject);
	//bool nodesEqual = operator==(a.nodeRefList,b.nodeRefList);
	bool typesEqual = a.type == b.type;

	bool allEqual = dataObjectsEqual && typesEqual;
	return allEqual;
}

bool operator!=(const FragmentationEncodingTask&a,
		const FragmentationEncodingTask&b) {
	//FIXME need to check nodes?
	bool dataObjectsEqual = operator==(a.dataObject, b.dataObject);
	//bool nodesEqual = operator==(a.nodeRefList,b.nodeRefList);
	bool typesEqual = a.type == b.type;

	bool allEqual = dataObjectsEqual && typesEqual;
	return !allEqual;
}
