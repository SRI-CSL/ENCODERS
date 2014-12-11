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

#include "ConnectivityInterfacePolicy.h"

#include <libcpphaggle/Platform.h>

ConnectivityInterfacePolicyTTL::ConnectivityInterfacePolicyTTL(long TimeToLive)
{
	base_ttl = TimeToLive;
	update();
}

ConnectivityInterfacePolicyTTL::~ConnectivityInterfacePolicyTTL()
{
}

void ConnectivityInterfacePolicyTTL::update()
{
	current_ttl = base_ttl;
}

void ConnectivityInterfacePolicyTTL::age()
{
	if (current_ttl > 0)
		current_ttl--;
}

bool ConnectivityInterfacePolicyTTL::isDead() const
{
	return current_ttl == 0;
}

Timeval ConnectivityInterfacePolicyTTL::lifetime() const
{
	return Timeval(-1);
}

const char *ConnectivityInterfacePolicyTTL::ageStr() const
{
	sprintf(const_cast<ConnectivityInterfacePolicyTTL *>(this)->agestr, "ttl=%lu", current_ttl);
	
	return agestr;
}

ConnectivityInterfacePolicyAgeless::ConnectivityInterfacePolicyAgeless()
{
}

ConnectivityInterfacePolicyAgeless::~ConnectivityInterfacePolicyAgeless()
{
}

void ConnectivityInterfacePolicyAgeless::update()
{
}

void ConnectivityInterfacePolicyAgeless::age()
{
}

bool ConnectivityInterfacePolicyAgeless::isDead() const
{
	return false;
}

Timeval ConnectivityInterfacePolicyAgeless::lifetime() const
{
	return Timeval(-1);
}

const char *ConnectivityInterfacePolicyAgeless::ageStr() const
{
	return "infinite";
}


ConnectivityInterfacePolicyTime::ConnectivityInterfacePolicyTime(const Timeval &ttl)
{
	expiry = ttl;
}

ConnectivityInterfacePolicyTime::~ConnectivityInterfacePolicyTime()
{
}

void ConnectivityInterfacePolicyTime::update()
{
}

void ConnectivityInterfacePolicyTime::age()
{
}

bool ConnectivityInterfacePolicyTime::isDead() const
{
	return (expiry < Timeval::now());
}

Timeval ConnectivityInterfacePolicyTime::lifetime() const
{
	return expiry;
}

const char *ConnectivityInterfacePolicyTime::ageStr() const
{
	Timeval age = expiry - Timeval::now();

	memset(const_cast<ConnectivityInterfacePolicyTime *>(this)->agestr, '\0', sizeof(agestr));
	strncpy(const_cast<ConnectivityInterfacePolicyTime *>(this)->agestr, age.getAsString().c_str(), sizeof(agestr) - 1);

	return agestr;
}

