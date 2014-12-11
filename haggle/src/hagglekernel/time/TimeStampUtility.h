/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

/*
 * TimeStampUtility.h
 *
 *  Created on: Aug 30, 2012
 *      Author: jjoy
 */

#ifndef TIMESTAMPUTILITY_H_
#define TIMESTAMPUTILITY_H_

#include <libcpphaggle/String.h>

class TimeStampUtility {
public:
	TimeStampUtility();
	virtual ~TimeStampUtility();

	string generateTimeStamp();
};

#endif /* TIMESTAMPUTILITY_H_ */
