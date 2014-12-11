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
#ifndef _CONNECTIVITYINTERFACEPOLICY_H
#define _CONNECTIVITYINTERFACEPOLICY_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ConnectivityInterfacePolicy;
class ConnectivityInterfacePolicyTTL;
class ConnectivityInterfacePolicyAgeless;
class ConnectivityInterfacePolicyTime;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Timeval.h>

using namespace haggle;

/*
	In order to avoid creating a lot of files for subclasses, new TTLPolicies 
	should be put in this file.
*/

/**
	This class is the prototype for all TTL policies.
	
	TTL policies are meant to be stack allocated.
*/
class ConnectivityInterfacePolicy {
protected:
	char agestr[20];
public:
	virtual ~ConnectivityInterfacePolicy() {};
	/** 
		Called whenever the associated interface is (re)added.
	*/
	virtual void update() = 0;
	/** 
		Called whenever the connectivity that added the associated interface 
		calls age_all_interfaces().
	*/
	virtual void age() = 0;
	/**
		Called to check if this interface is dead. If it returns true, the 
		interface is marked as down and removed from the connectivity manager's 
		list of active interfaces.
	*/
	virtual bool isDead() const = 0;
	
	/**
		Get the lifetime left for this interface in absolute time.
	 */
	virtual Timeval lifetime() const = 0;
	/**
		Give a string description of the current age.
	 */
	virtual const char *ageStr() const = 0;
};

/**
	This policy makes use of aging.
*/
class ConnectivityInterfacePolicyTTL : public ConnectivityInterfacePolicy {
	long current_ttl;
	long base_ttl;
public:
	ConnectivityInterfacePolicyTTL(long TimeToLive);
	~ConnectivityInterfacePolicyTTL();
	void update();
	void age();
	bool isDead() const;
	Timeval lifetime() const;
	const char *ageStr() const;
};/**
	This policy does not make use of aging. It will always say the interface
	is alive.
*/
class ConnectivityInterfacePolicyAgeless : public ConnectivityInterfacePolicy {
public:
	ConnectivityInterfacePolicyAgeless();
	~ConnectivityInterfacePolicyAgeless();
	void update();
	void age();
	bool isDead() const;
	Timeval lifetime() const;
	const char *ageStr() const;
};
/**
	This policy does not make use of aging. It will always say the interface
	is alive.
*/
class ConnectivityInterfacePolicyTime : public ConnectivityInterfacePolicy {
	Timeval expiry;
public:
	/**
		The ttl parameter signifies the actual time to live. Not "when", but 
		"in how long".
	*/
	ConnectivityInterfacePolicyTime(const Timeval &ttl);
	~ConnectivityInterfacePolicyTime();
	void update();
	void age();
	bool isDead() const;
	Timeval lifetime() const;
	const char *ageStr() const;
};

#endif
