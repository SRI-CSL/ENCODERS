/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TIMEVAL_H
#define _TIMEVAL_H

#include <haggleutils.h>
#include "Platform.h"
#include "String.h"

namespace haggle {
/**
	Conveniance class to work with struct timeval.
	Timeval's behavior for negative times might be viewed as somewhat strange:
	the tv_usec variable is always positive, which means that -0.5 seconds is 
	represented by {-1,500000}, i.e. -1 second + 500000 microseconds.
*/
class Timeval {
	struct timeval t;
private:
	void adjust(void);
public:
	static Timeval now();
	Timeval();
	Timeval(const Timeval &tv);
	Timeval(const struct timeval& _t);
	Timeval(const long seconds, const long microseconds);
	Timeval(const double _t);
	Timeval(const string str);
	/*
	  Set Timeval to current time.
	 */
	Timeval& setNow();
	/*
	  Set Timeval to zero.
	 */
	Timeval& zero();
	/*
	  Set Timeval given a C-struct timeval.
	 */
	Timeval& set(const struct timeval &_t);
	/*
	  Set Timeval given a seconds and microseconds.
	 */
	Timeval& set(const long seconds, const long microseconds);
	/*
	  Set Timeval given a seconds and microseconds double.
	 */
	Timeval& set(const double seconds);
	/*
	  Returns true if t.tv_sec is positive or zero, and tv_usec within its valid limits (0-99999).
	 */
	bool isValid() const;
	/*
	  Returns a pointer to the C-struct representation of the Timeval.
	 */
	const struct timeval *getTimevalStruct() const;
	/* 
	   Returns the "seconds" part of the Timeval.
	 */
	long getSeconds() const;	
	/* 
	   Returns the "micro seconds" part of the Timeval. (Note that this it not the time in micro seconds).
	 */
	long getMicroSeconds() const;	
	/* 
	   Returns the total time in milli seconds as a 64-bit integer.
	 */
	int64_t getTimeAsMilliSeconds() const;
	/*
	   Returns the total time in seconds as a double.
	 */
	double getTimeAsSecondsDouble() const;

	/* 
	   Returns the total time in milli seconds as a double.
	 */
	double getTimeAsMilliSecondsDouble() const;

	/*
	  Get Timeval as a string we can print.
	 */
	string getAsString() const;

	// Operators
	friend bool operator<(const Timeval& t1, const Timeval& t2);
	friend bool operator<=(const Timeval& t1, const Timeval& t2);
	friend bool operator==(const Timeval& t1, const Timeval& t2);
	friend bool operator!=(const Timeval& t1, const Timeval& t2);
	friend bool operator>=(const Timeval& t1, const Timeval& t2);
	friend bool operator>(const Timeval& t1, const Timeval& t2);

	friend Timeval operator+(const Timeval& t1, const Timeval& t2);
	friend Timeval operator-(const Timeval& t1, const Timeval& t2);

	Timeval& operator+=(const Timeval& tv);
	Timeval& operator-=(const Timeval& tv);
};

}; // namespace haggle

#endif
