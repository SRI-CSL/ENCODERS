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
#ifndef _POLICY_H
#define _POLICY_H

class Policy;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Reference.h>
#include "Debug.h"

using namespace haggle;

typedef enum {
	POLICY_RESOURCE_UNLIMITED,
	POLICY_RESOURCE_HIGH,
	POLICY_RESOURCE_MEDIUM,
	POLICY_RESOURCE_LOW
} PolicyType_t;

/** */
class Policy
#ifdef DEBUG_LEAKS
: public LeakMonitor
#endif
{
	PolicyType_t powerPolicy;
	PolicyType_t storagePolicy;
public:
	Policy(PolicyType_t _powerPolicy, PolicyType_t _storagePolicy);
	~Policy() {}
	
	PolicyType_t getPowerPolicy() { return powerPolicy; }
	PolicyType_t getStoragePolicy() { return storagePolicy; }
	
	friend bool operator==(const Policy& p1, const Policy& p2);
};

typedef Reference<Policy> PolicyRef;

#endif /* _POLICY_H */
