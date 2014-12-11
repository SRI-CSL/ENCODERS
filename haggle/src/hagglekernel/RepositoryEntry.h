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

#ifndef _REPOSITORY_ENTRY_H
#define _REPOSITORY_ENTRY_H

class RepositoryEntry;

#include <string.h>
#include <stdlib.h>
#include <libcpphaggle/Reference.h>

using namespace haggle;

/**
	Repository entries are triplets of strings that store a value for a 
	particular manager.
	
	The authority string should be unique for a particular manager.
	
	The key and value parts are for use by the manager.
	
	The id can be used to store several identical key-value pairs if neccesary.
	
	Matching in the data store can be done on partial repository entries. A 
	manager could for instance be interested in getting all its repository 
	entries by retrieveing with an entry where only the authority is set.
*/
class RepositoryEntry
{	
public:
	typedef enum {
		VALUE_TYPE_STRING,
		VALUE_TYPE_BLOB
	} ValueType; 
private:
	ValueType type;
	string authority;
	string key;
	union {
		char *value_str;
		unsigned char *value_blob;
		void *value;
	};
	size_t len; // Length of value
	unsigned int id;
public:	
	/**
		Constructor. The key, value and id parts are optional for matching 
		purposes.
	*/
	RepositoryEntry(const string _authority, const string _key = "", const string _value = "", unsigned int _id = 0);
	RepositoryEntry(const string _authority, const string _key, const unsigned char  *_value, size_t _len, unsigned int _id = 0);
	/**
		Destructor.
	*/
	~RepositoryEntry();
	
	/**
		Returns the type of the entry 
	 */
	ValueType getType() const { return type; }
	
	/**
		Returns the key of this repository entry.
	*/
	const char *getKey() const;

	/**
		Returns the authority of this repository entry.
	*/
	const char *getAuthority() const;
	/**
		Returns the value of this repository entry.
	*/
	const void *getValue() const;
	/**
	 Returns the value of this repository entry as a string.
	 */
	const char *getValueStr() const;
	/**
	 Returns the value of this repository entry as a blob.
	 */
	const unsigned char *getValueBlob() const;
	
	/**
	 Returns the length of the value
	 */
	size_t getValueLen() const;
	
	/**
		Returns the id of this repository entry.
	*/
	unsigned int getId() const { return id; }


	/**
		Equality operator.
	*/
        friend bool operator==(const RepositoryEntry& e1, const RepositoryEntry& e2);
};

typedef Reference<RepositoryEntry> RepositoryEntryRef;
typedef ReferenceList<RepositoryEntry> RepositoryEntryList;

#endif /* _REPOSITORY_ENTRY_H */
