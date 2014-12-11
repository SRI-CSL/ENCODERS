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
#ifndef _FILTER_H
#define _FILTER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class Filter;
class Filters;

#include <libcpphaggle/List.h>

#include "Debug.h"
#include "Attribute.h"

using namespace haggle;

/** */
#ifdef DEBUG_LEAKS
class Filter: public LeakMonitor
#else
class Filter
#endif
{
private:
        Attributes attrs;
        long etype;
public:
        Filter(const char* filterString, const long _etype = -1);
        Filter(const Attributes *inAttrs, const long _etype = -1);
        Filter(const Attribute &attr, const long _etype = -1);
        Filter(const Filter &f);
        ~Filter();
        long getEventType() const {
                return etype;
        }
        const Attributes *getAttributes() const {
                return &attrs;
        }
        Filter *copy() const {
                return new Filter(*this);
        }
	string getFilterDescription() const;
};



// container class
/** */
class Filters : public List<Filter>
{
public:
        Filters() {};
        ~Filters() {};
};



#endif /* _FILTER_H */
