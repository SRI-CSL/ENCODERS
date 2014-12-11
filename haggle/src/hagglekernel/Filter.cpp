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
#include <string.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>

using namespace haggle;

#include "Filter.h"

Filter::Filter(const char *filterString, const long _etype) : 
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_FILTER),
#endif
	etype(_etype)
{
	string filter = filterString;
	string namevalue, name, value;
	size_t start = 0, end = 0;

        while (end != string::npos) {
                end = filter.find(" ", start);
                namevalue = filter.substr(start, end - start);
                start = end + 1;
        }
	// Also insert the last name-value pair (or the first if there was only one pair)
	namevalue = filter.substr(start);

	start = namevalue.find("=");
	value = namevalue.substr(start + 1);
	name = namevalue.substr(0, start);

	attrs.add(Attribute(name, value));
}

Filter::Filter(const Attributes *inAttrs, const long _etype) : 
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_FILTER),
#endif
	etype(_etype)
{
	if (!inAttrs) {
                return;
        }

	attrs = *inAttrs;
}

Filter::Filter(const Attribute& attr, const long _etype) : 
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_FILTER),
#endif
	etype(_etype)
{
	attrs.add(attr);
}
Filter::Filter(const Filter& f) : 
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_FILTER),
#endif
	attrs(f.attrs), etype(f.etype)
{
}

Filter::~Filter()
{
	/* new as list of objects
	   Attributes::iterator it = attrs.begin();

	   while (it != attrs.end()) {
	   Attribute *attr = *it;
	   HAGGLE_DBG("Deleting filter attr=%s:%s\n", attr->getName(), attr->getValue());
	   attrs.erase(it);
	   delete attr;
	   it = attrs.begin();
	   }
	 */
}


string Filter::getFilterDescription() const
{
	string desc = "empty filter";

	if (attrs.size())
		desc = "";

	for (Attributes::const_iterator it = attrs.begin(); it != attrs.end(); it++) {
		desc += (*it).second.getName();
		desc += ":";
		desc += (*it).second.getValue();
		desc += " ";
	}
	return desc;
}
