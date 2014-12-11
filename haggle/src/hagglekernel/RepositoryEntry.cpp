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
#include "RepositoryEntry.h"

RepositoryEntry::RepositoryEntry(const string _authority, const string _key, const string _value, unsigned int _id) : 
	type(VALUE_TYPE_STRING), authority(_authority), key(_key), value(NULL), len(_value.length()), id(_id)
{
	if (len) {
		value_str = (char *)malloc(len + 1);
	
		if (value_str) {
			strcpy(value_str, _value.c_str()); 
		}
	}
}

RepositoryEntry::RepositoryEntry(const string _authority, const string _key, const unsigned char* _value, size_t _len, unsigned int _id) : 
	type(VALUE_TYPE_BLOB), authority(_authority), key(_key), value(NULL), len(_len), id(_id)
{
	if (_value && len) {
		value = malloc(len);
		
		if (value) {
			memcpy(value, _value, len);
		}
	}
}

RepositoryEntry::~RepositoryEntry()
{
	if (value) 
		free(value);
}

const char *RepositoryEntry::getAuthority() const 
{
	return authority.length() ? authority.c_str() : NULL;
}

const char *RepositoryEntry::getKey() const 
{
	return key.length() ? key.c_str() : NULL;
}

const void *RepositoryEntry::getValue() const 
{
	return value;
}

const char *RepositoryEntry::getValueStr() const 
{
	return value_str;
}

const unsigned char *RepositoryEntry::getValueBlob() const
{
	return value_blob;
}

size_t RepositoryEntry::getValueLen() const 
{
	return len;
}

bool operator==(const RepositoryEntry& e1, const RepositoryEntry& e2)
{
	if (!e1.type != e2.type)
		return false;
	
	switch (e1.type) {
		case RepositoryEntry::VALUE_TYPE_STRING:
			return (strcmp(e1.authority.c_str(), e2.authority.c_str()) == 0 && 
				strcmp(e1.key.c_str(), e2.key.c_str()) == 0 && 
				strcmp(e1.value_str, e2.value_str) == 0);
		case RepositoryEntry::VALUE_TYPE_BLOB:
			return (strcmp(e1.authority.c_str(), e2.authority.c_str()) == 0 && 
			       strcmp(e1.key.c_str(), e2.key.c_str()) == 0 && 
			       e1.len == e2.len &&
			       memcmp(e1.value, e2.value, e1.len) == 0);
	}
	return false;
}
