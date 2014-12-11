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

#include <time.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Timeval.h>

namespace haggle {

Timeval Timeval::now()
{
	struct timeval t;
	
	gettimeofday(&t, NULL);
	return Timeval(t);
}

Timeval::Timeval()
{
	t.tv_sec = 0;
	t.tv_usec = 0;
}

void Timeval::adjust(void)
{
	while (t.tv_usec < 0) {
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
	while (t.tv_usec >= 1000000) {
		t.tv_sec++;
		t.tv_usec -= 1000000;
	}
}

Timeval::Timeval(const Timeval &tv) : t(tv.t)
{
	adjust();
}

Timeval::Timeval(const struct timeval& _t) : t(_t)
{
	adjust();
}

Timeval::Timeval(const long seconds, const long microseconds)
{
	t.tv_sec = seconds;
	t.tv_usec = microseconds;
	
	adjust();
}

#define ASSTRING_FORMAT "%ld.%06ld"

Timeval::Timeval(const string str)
{
	long sec, usec;
	
	sscanf(str.c_str(), ASSTRING_FORMAT, &(sec), &(usec));
	
	t.tv_sec = sec;
	t.tv_usec = usec;
	adjust();
}
	
Timeval::Timeval(const double _t)
{
	t.tv_sec = (long)_t;
	t.tv_usec = (long)((_t - (double)t.tv_sec) * 1000000.0);
	adjust();
}

Timeval& Timeval::setNow()
{
	gettimeofday(&t, NULL);
	adjust();
	return *this;
}

Timeval& Timeval::zero() 
{ 
	t.tv_sec = 0; t.tv_usec = 0; 
	return *this; 
}
	
Timeval& Timeval::set(const struct timeval &_t) 
{ 
	t = _t; 
	return *this; 
}
	
Timeval& Timeval::set(const long seconds, const long microseconds) 
{ 
	t.tv_sec = seconds; 
	t.tv_usec = microseconds; 
	return *this; 
}
	
Timeval& Timeval::set(const double seconds) 
{ 
	t.tv_sec = (long) seconds; 
	t.tv_usec = (long)((seconds - (double)t.tv_sec) * 1000000); 
	return *this; 
}
	
bool Timeval::isValid() const 
{ 
	return (t.tv_sec >= 0 && t.tv_usec >= 0 && t.tv_usec < 1000000); 
}
	
const struct timeval *Timeval::getTimevalStruct() const 
{ 
	return &t; 
}
	
long Timeval::getSeconds() const 
{ 
	return t.tv_sec;
}	
	
long Timeval::getMicroSeconds() const 
{ 
	return t.tv_usec; 
}	
	
int64_t Timeval::getTimeAsMilliSeconds() const 
{ 
	return (((int64_t)t.tv_sec * 1000) + (int64_t)t.tv_usec / 1000); 
}

double Timeval::getTimeAsSecondsDouble() const
{
	return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;	
}

double Timeval::getTimeAsMilliSecondsDouble() const
{
	return ((double)t.tv_sec * 1000 + (double)t.tv_usec / 1000);	
}

string Timeval::getAsString() const
{
	char buf[20];

	if (t.tv_sec < 0) {
		snprintf(buf, 20, "-"ASSTRING_FORMAT, -(long)(t.tv_sec-1), 1000000 - (long)t.tv_usec);
	} else {
		snprintf(buf, 20, ASSTRING_FORMAT, (long)t.tv_sec, (long)t.tv_usec);
	}

	return string(buf);
}

bool operator<(const Timeval& t1, const Timeval& t2)
{
	return (t1.t.tv_sec < t2.t.tv_sec) ||
		((t1.t.tv_sec == t2.t.tv_sec) && (t1.t.tv_usec < t2.t.tv_usec));
}

bool operator<=(const Timeval& t1, const Timeval& t2)
{
	return (t1.t.tv_sec < t2.t.tv_sec) ||
		((t1.t.tv_sec == t2.t.tv_sec) && (t1.t.tv_usec <= t2.t.tv_usec));
}

bool operator==(const Timeval& t1, const Timeval& t2)
{
	return ((t1.t.tv_sec == t2.t.tv_sec) && (t1.t.tv_usec == t2.t.tv_usec));
}

bool operator!=(const Timeval& t1, const Timeval& t2)
{
	return !(t1 == t2);
}

bool operator>=(const Timeval& t1, const Timeval& t2) 
{
	return (t1.t.tv_sec > t2.t.tv_sec) ||
		((t1.t.tv_sec == t2.t.tv_sec) && (t1.t.tv_usec >= t2.t.tv_usec));
}

bool operator>(const Timeval& t1, const Timeval& t2)
{
	return (t1.t.tv_sec > t2.t.tv_sec) ||
		((t1.t.tv_sec == t2.t.tv_sec) && (t1.t.tv_usec > t2.t.tv_usec));
}

Timeval operator+(const Timeval& t1, const Timeval& t2)
{
	Timeval tv(t1);
	
	tv += t2;
	
	return tv;
}

Timeval operator-(const Timeval& t1, const Timeval& t2)
{	
	Timeval tv(t1);
	
	tv -= t2;
	
	return tv;
}

Timeval& Timeval::operator+=(const Timeval& tv)
{
	t.tv_sec += tv.t.tv_sec;
	t.tv_usec += tv.t.tv_usec;
	
	if (t.tv_usec < 0) {
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
	if (t.tv_usec >= 1000000) {
		t.tv_sec++;
		t.tv_usec -= 1000000;
	}
	
	return *this;
}

Timeval& Timeval::operator-=(const Timeval& tv) 
{
	t.tv_sec -= tv.t.tv_sec;
	t.tv_usec -= tv.t.tv_usec;
	
	if (t.tv_usec < 0) {
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
	if (t.tv_usec >= 1000000) {
		t.tv_sec++;
		t.tv_usec -= 1000000;
	}
	
	return *this;
}

}; // namespace haggle
