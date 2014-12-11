/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
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
#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

class Attribute;
class Attributes;

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

#include "Debug.h"

#define ATTR_WILDCARD "*"			// Wildcard attribute for queries. 
#define ATTR_WEIGHT_NO_MATCH -1		// Attribute weight to exclude a query result if that attribute matches.

using namespace haggle;

/**
	Attribute class.
*/
#ifdef DEBUG_LEAKS
class Attribute : public LeakMonitor
#else
class Attribute
#endif
{
private:
        string name;
        string value;
        unsigned long weight;
public:
	/**
		Constructor that takes two strings of type "ABC" and "DEF" and 
		creates an attribute of type "ABC=DEF" from it.
	*/
        Attribute(const string _name = "", const string _value = "", unsigned long _weight = 1);
        /// Copy-constructor
        Attribute(const Attribute& attr);
	/**
		Destructor.
	*/
        ~Attribute();
	/**
		Copying function, returns a copy of the attribute.
	*/
        Attribute *copy() const {
                return new Attribute(*this);
        }
		
	/**
		Returns the name part of the attribute.
		In an attribute of type "ABC=DEF:1", "ABC" is the name part.
	*/
        const string& getName() const {
                return name;
        }
		
	/**
		Returns the value part of the attribute.
		In an attribute of type "ABC=DEF:1", "DEF" is the value part.
	*/
        const string& getValue() const {
                return value;
        }
	/**
		Returns the weight part of the attribute.
		In an attribute of type "ABC=DEF:1", "1" is the weight part.
	*/
        unsigned long getWeight() const {
                return weight;
        }
	
	string getWeightAsString() const;
		
	/**
		Sets the name part of the attribute.
		In an attribute of type "ABC=DEF:1", "ABC" is the name part.
	*/
        void setName(const char *_name) {
                name = _name;
        }
	/**
		Sets the value part of the attribute.
		In an attribute of type "ABC=DEF:1", "DEF" is the value part.
	*/
        void setValue(const char *_value) {
                value = _value;
        }
	/**
		Sets the weight part of the attribute.
		In an attribute of type "ABC=DEF:1", "1" is the weight part.
	*/
        void setWeight(unsigned long _weight) {
                weight = _weight;
        }
	/**
		Returns a string describing the attribute. It is given in the same
		format as is read by the constructor that only takes one string.
	*/
	string getString() const;
	/**
		Equality operator
	*/
        friend bool operator==(const Attribute &a, const Attribute &b);
	/**
		Less-than operator
	*/
        friend bool operator<(const Attribute &a, const Attribute &b);
};

// container class
/** */
// class Attributes : public HashMap<string, Attribute> 
class Attributes : public BasicMap<string, Attribute> // MOS - we don't use HashMap anymore because we need a canonical order
{
public:
        Attributes() {};
        ~Attributes() {};
        Attributes *copy() const;
	iterator find(const Attribute& a);
	const_iterator find(const Attribute& a) const;
	size_type erase(const Attribute& a);
	bool add(const Attribute& a);
};


#endif /* _ATTRIBUTE_H */
