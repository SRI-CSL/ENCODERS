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
#include <libcpphaggle/Reference.h>

namespace haggle {

HashMap<void *, RefCounter *> RefCounter::objects;
Mutex RefCounter::objectsMutex;
unsigned long RefCounter::totNum = 0;

RefCounter::RefCounter(void *_obj) : 
	countMutex(),
	refcount(1),
	objectMutex(),
	obj(_obj), 
	identifier(totNum++)
{
	objects.insert(RefcountPair(obj, this));
}

RefCounter *RefCounter::create(void *_obj)
{
	Mutex::AutoLocker l(objectsMutex);
	RefCounter *refCount;
	// NULL is ok, but we don't need a RefCounter to it.
	if (!_obj)
		return NULL;

	RefcountMap::iterator it = objects.find(_obj);
	
	if (it != objects.end()) {
		refCount = (RefCounter *) (*it).second;
		if (refCount->inc_count() == 0) {
  		        assert(false); // MOS
			return NULL;
		}
		return refCount;
	}
	return new RefCounter(_obj);
}
/**
    Destructor.
*/
RefCounter::~RefCounter()
{
}
	
}; // namespace haggle


