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
#include "Policy.h"

Policy::Policy(PolicyType_t _powerPolicy, PolicyType_t _storagePolicy) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_POLICY),
#endif
	powerPolicy(_powerPolicy),
	storagePolicy(_storagePolicy)
{
}

bool operator==(const Policy& p1, const Policy& p2)
{
	return (p1.powerPolicy == p2.powerPolicy) &&
		(p1.storagePolicy == p2.storagePolicy);
}
