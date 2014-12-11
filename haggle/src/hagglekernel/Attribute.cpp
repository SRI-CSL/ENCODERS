/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 */

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
#include <stdlib.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <haggleutils.h>

#include "DataObject.h"
#include "Node.h"
#include "Interface.h"
#include "Attribute.h"

Attribute::Attribute(const string _name, const string _value, unsigned long _weight) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_ATTRIBUTE),
#endif
	name(_name), value(_value), weight(_weight)
{
}

// Copy-constructor
Attribute::Attribute(const Attribute& attr) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_ATTRIBUTE),
#endif
	name(attr.getName()), value(attr.getValue()), weight(attr.getWeight())
{
}

Attribute::~Attribute()
{
}

string Attribute::getWeightAsString() const
{
	char weightstr[11];
	
	snprintf(weightstr, 11, "%lu", weight);
	
	return weightstr; 
}


string Attribute::getString() const
{ 
	char weightstr[11];
	
	snprintf(weightstr, 11, "%lu", weight);

	return string(name + "=" + value + ":" + weightstr); 
}

bool operator==(const Attribute& a, const Attribute& b)
{
	if (a.getName() != b.getName())
		return false;

	if (a.getValue() == "*" || b.getValue() == "*") {
		return true;
	} else {
		return (a.getValue() == b.getValue());
	}
}

bool operator<(const Attribute& a, const Attribute& b)
{
	if (a.getName() != b.getName())
		return (a.getName() < b.getName());

	// MOS - canonical ordering is important for ids
	// hence we replace to following (it is ok to keep wildcards 
	// for matching (==), although not nice.

	// if (a.getValue() == "*") {
	// 	return false;
	// } else {
	//         return (a.getValue() < b.getValue());
	// }

	if(a.getValue() != b.getValue())
	  return a.getValue() < b.getValue();
	else return a.getWeight() < b.getWeight();	
}

Attributes *Attributes::copy() const {
        return new Attributes(*this);
}

Attributes::iterator Attributes::find(const Attribute& a) {
        Pair<iterator, iterator> p = equal_range(a.getName());
		
        for (; p.first != p.second; ++p.first) {
                if (a == (*p.first).second)
                        return p.first;
        }
        return end();
}
	
Attributes::const_iterator Attributes::find(const Attribute& a) const {
        Pair<const_iterator, const_iterator> p = equal_range(a.getName());
		
        for (; p.first != p.second; ++p.first) {
                if (a == (*p.first).second)
                        return p.first;
        }
        return end();
}

Attributes::size_type Attributes::erase(const Attribute& a) { 
        Pair<iterator, iterator> p = equal_range(a.getName());
		
        for (; p.first != p.second; ++p.first) {
                if (a == (*p.first).second) {
                        BasicMap<string, Attribute>::erase(p.first);
                        return 1;
                }
        }
        return 0;
}

bool Attributes::add(const Attribute& a) 
{ 
         return insert_sorted(make_pair(a.getName(), a)) != end(); // MOS - using sorted version now
}
