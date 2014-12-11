/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

/*
 * DataObjectTypeIdentifierUtility.h
 *
 *  Created on: Aug 30, 2012
 *      Author: jjoy
 */

#ifndef DATAOBJECTTYPEIDENTIFIERUTILITY_H_
#define DATAOBJECTTYPEIDENTIFIERUTILITY_H_

#include "DataObject.h"

class DataObjectTypeIdentifierUtility {
public:
	DataObjectTypeIdentifierUtility();
	virtual ~DataObjectTypeIdentifierUtility();

	bool isHaggleIpc(DataObjectRef dataObject);

	bool hasNoFileAssociated(DataObjectRef dataObjectRef);

	string convertDataObjectIdToHex(const unsigned char* dataObjectIdUnsignedChar);

    DataObjectIdRef convertDataObjectIdStringToDataObjectIdType(
            string dataObjectId);
};

#endif /* DATAOBJECTTYPEIDENTIFIERUTILITY_H_ */
