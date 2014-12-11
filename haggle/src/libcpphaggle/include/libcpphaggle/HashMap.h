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
#ifndef _HASHMAP_H
#define _HASHMAP_H

#include <limits.h>

#if defined(ENABLE_STL)
#include <map>
using namespace std;

#define HashMap multimap

// Dummy namespace
namespace haggle {}

#else
#ifndef _WIN32
#include <stdint.h>
#endif

#include "Pair.h"
#include "List.h"
#include "String.h"

namespace haggle {
/**
   This is a simple implementation of a HashMap. It can be used to
   replace STL's map and multimap, but it does not implement all features
   of those containers. It also does not comply exactly with their APIs.
  
   However, in most simple cases, this HashMap can transparently replace
   map and multimap.
  
   Note, that this hash map does not guarantee ordering of objects, and
   it works by default as a multimap.

   The implementation is quite simple, and could probably be improved a
   lot.
  
   @author Erik Nordström
*/
  
/* FYI: This is the "One-at-a-Time" algorithm by Bob Jenkins */
/* from requirements by Colin Plumb. */
/* (http://burtleburtle.net/bob/hash/doobs.html) */
/* adapted from Perl sources ( hv.h ) */
static inline unsigned long hash_cstr(char* k) {
	unsigned long hash = 0;
		
	while (*k != '\0') {
		hash += *k++;
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
		
	return hash + (hash << 15);
}
	
/*--- HashPJW ---------------------------------------------------
 *  An adaptation of Peter Weinberger's (PJW) generic hashing
 *  algorithm based on Allen Holub's version. Accepts a pointer
 *  to a datum to be hashed and returns an unsigned integer.
 *     Modified by sandro to include end
 *     Taken from http://www.ddj.com/articles/1996/9604/9604b/9604b.htm?topic=algorithms
 *-------------------------------------------------------------*/
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))	

static inline unsigned int generic_hash(const char *start, const char *end) {
	unsigned int hash_value, i;
	if (end) {
		for (hash_value = 0; start < end; ++start ) {
			hash_value = ( hash_value << ONE_EIGHTH ) + *start;
			if (( i = hash_value & HIGH_BITS ) != 0 )
				hash_value =
					( hash_value ^ ( i >> THREE_QUARTERS )) &
					~HIGH_BITS;
		}
	} else {
		for (hash_value = 0; *start; ++start )	{
			hash_value = ( hash_value << ONE_EIGHTH ) + *start;
			if (( i = hash_value & HIGH_BITS ) != 0 )
				hash_value =
					( hash_value ^ ( i >> THREE_QUARTERS )) &
					~HIGH_BITS;
		}
		/* and the extra null value, so we match if working by length */
		hash_value = ( hash_value << ONE_EIGHTH ) + *start;
		if (( i = hash_value & HIGH_BITS ) != 0 ) {
			hash_value =
				( hash_value ^ ( i >> THREE_QUARTERS )) &
				~HIGH_BITS;
		}
	}
	return hash_value;
}
	
/*
  Define a set of hash functions based on type.
  For now, only distinguish between strings, pointers, and integer types.
*/
template<typename T> 
struct Hash {
	static unsigned int hash(const T& k) { return generic_hash((const char *)&k, (const char *)&k + sizeof(T)); }
};
template<typename T> 
struct Hash<T*> {
	static unsigned int hash(const T* k) { 
		uintptr_t p = reinterpret_cast<uintptr_t>(k);
		return generic_hash((const char *)&p, (const char *)&p + sizeof(uintptr_t)); 
	}
};
template<> 
struct Hash<string> { 
	static unsigned int hash(const string& k) { return generic_hash((const char *)k.c_str(), (const char *)k.c_str() + k.length()); }
};
	

/* from SGI STL */
const unsigned long ms_primes[] =
{
	7ul,          13ul,         29ul,
	53ul,         97ul,         193ul,       389ul,       769ul,
	1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
	49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
	1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
	50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
	1610612741ul, 3221225473ul, 4294967291ul, 0
};
 

/**
   HashMap: a class that implements a multimap container using a hash table.
	   
*/
template<typename KeyType, typename ValueType>
class HashMap {
public:
	typedef unsigned long size_type;
private:
	typedef Pair<KeyType, ValueType > PairType;
	typedef List<PairType > ListType;
	size_type _size;
	unsigned long table_size;
	ListType *table;
	
	/**
	   Get an index into the hash table given a key.
	   @param k the key.
	   @returns the index into the hash table
	*/
	size_type getIndex(const KeyType& k) const {
		return Hash<KeyType>::hash(k) % table_size;
	}
	/**
	   Get the next prime number that is larger than the
	   given integer.
	   @param n the integer to compare against.
	   @returns the first prime that follows n
	*/
	unsigned long getPrime(unsigned long n) const {
		const unsigned long* ptr = &ms_primes[0];
		for (; *ptr != 0; ++ptr) {
			if (n < *ptr)
				return *ptr;
		}
		return 0;
	}
	/**
	   Grow the hash table if necessary.
	*/
	void grow() {
		size_type new_size = getPrime(table_size + 1);

		if (!new_size || new_size <= table_size)
			return;
			
		HashMap<KeyType, ValueType> largerHashMap(new_size);

		for (iterator it = begin(); it != end(); ++it) {
			largerHashMap.insert(*it);
		}
			
		*this = largerHashMap;
	}
	/**
	   Create a new hash map with the given size. The actual size 
	   of the hash table may be equal or greater than the size.
	   @param tsize the minumum size of the hash table.
	*/
	HashMap(const size_type tsize) : _size(0), table_size(getPrime(tsize)), table(new ListType[table_size]) {}
public:
	class iterator {
		friend class HashMap<KeyType, ValueType>;
		friend class HashMap<KeyType, ValueType>::const_iterator;
		HashMap<KeyType, ValueType> *m;
		size_type table_index;
		typename ListType::iterator it;
		iterator(HashMap<KeyType, ValueType> *_m, const size_type _table_index, typename ListType::iterator _it) : m(_m), table_index(_table_index), it(_it) {}
		inline void find_next_filled_bucket() {
			if (table_index < m->table_size) {
				// Advance iterator position
				it++;
				
				// Check if the iterator points to a
				// filled bucket, otherwise advance to
				// the the next filled one
				if (it == m->table[table_index].end()) {
					while (++table_index < m->table_size) {
						// Go to next bucket
						it = m->table[table_index].begin();
						// Check if the bucket has contents
						if (it != m->table[table_index].end())
							break;
					}
				}
			} 
		}
		iterator& begin() { 
			if (it == m->table[0].end()) 
				find_next_filled_bucket();
			return *this;
		}
	public:
		iterator(const iterator& _it) : m(_it.m), table_index(_it.table_index), it(_it.it) {}
		iterator() : m(0), table_index(0), it() {}
		friend bool operator==(const iterator& it1, const iterator& it2) {
			return (it1.it == it2.it);
		}
		friend bool operator!=(const iterator& it1, const iterator& it2) {
			return !(it1 == it2);
		}
		iterator& operator++() { find_next_filled_bucket(); return *this; }
		iterator operator++(int) { iterator cit = *this; find_next_filled_bucket(); return cit; }
		PairType& operator*() { return *it; }
	};	
	class const_iterator {
		friend class HashMap<KeyType, ValueType>;
		HashMap<KeyType, ValueType> *m;
		size_type table_index;
		typename ListType::const_iterator it;
		const_iterator(const HashMap<KeyType, ValueType> *_m, const size_type _table_index, typename ListType::const_iterator _it) : m(const_cast<HashMap<KeyType, ValueType> *>(_m)), table_index(_table_index), it(_it) {}
		
		inline void find_next_filled_bucket() {
			if (table_index < m->table_size) {
				// Advance list iterator position
				it++;
				
				// Check if the iterator points to a
				// filled bucket, otherwise advance to
				// the the next filled one
				if (it == m->table[table_index].end()) {
					while (++table_index < m->table_size) {
						// Go to next bucket
						it = m->table[table_index].begin();
						// Check if the bucket has contents
						if (it != m->table[table_index].end())
							break;
					}
				}
			} 
		}
		const_iterator& begin() { 
			if (it == m->table[0].end()) 
				find_next_filled_bucket();
			return *this;
		}
	public:
		const_iterator(const const_iterator& _it) : m(_it.m), table_index(_it.table_index), it(_it.it) {}
		const_iterator(const iterator& _it) : m(_it.m), table_index(_it.table_index), it(_it.it) {}
		const_iterator() : m(0), table_index(0), it() {}
		friend bool operator==(const const_iterator& it1, const const_iterator& it2) {
			return (it1.it == it2.it);
		}
		friend bool operator!=(const const_iterator& it1, const const_iterator& it2) {
			return !(it1 == it2);
		}
		const_iterator& operator++() { find_next_filled_bucket(); return *this; }
		const_iterator operator++(int) { const_iterator cit = *this; find_next_filled_bucket(); return cit; }
		const PairType& operator*() const { return *it; }
	};
	iterator begin() { return iterator(this, 0, table[0].begin()).begin(); }
	iterator end() {return iterator(this, table_size-1, table[table_size-1].end()); }
	const_iterator begin() const { return const_iterator(this, 0, table[0].begin()).begin(); }
	const_iterator end() const {return const_iterator(this, table_size-1, table[table_size-1].end()); }
	size_type size() const { return _size; }
	bool empty() const { return _size == 0; }
	virtual iterator insert(const PairType& p) {
		typename ListType::iterator it, it_insert;
		const KeyType& k = p.first;

		if (_size + 1 > table_size) {
			grow();
		}

		size_type index = getIndex(k);

                // Handle conflicts.
                it = table[index].begin();
                it_insert = it;

                while (it != table[index].end()) {
                        // Find any occurence of the same key
                        if (k == (*it).first) {
                                // find the last occurence of this same key
                                while (it != table[index].end() && k == (*it).first) {
                                        it_insert = ++it;
                                }
                                break;
			}
                        it_insert = ++it;
                }
		_size++;

		return iterator(this, index, table[index].insert(it_insert, p));
	}
	iterator find(const KeyType& k) {		
		typename ListType::iterator it;
		size_type index = getIndex(k);

		for (it = table[index].begin(); it != table[index].end(); ++it) {
			if (k == (*it).first) {
				return iterator(this, index, it);
			}
		}
		return end();
	}
	const_iterator find(const KeyType& k) const {
		typename ListType::const_iterator it;

		size_type index = getIndex(k);
			
		for (it = table[index].begin(); it != table[index].end(); ++it) {
			if (k == (*it).first) {
				return const_iterator(this, index, it);
				
			}
		}
		return end();
	}
	iterator lower_bound(const KeyType& k) {
		return find(k);
	}	
	const_iterator lower_bound(const KeyType& k) const {
		return find(k);
	}
	iterator upper_bound(const KeyType& k) {
		iterator it = find(k);
				
		// iterate until we are past the last occurance
		while (it != end() && k == (*it).first ) {
			it++;
		}
		return it;
	}	
	const_iterator upper_bound(const KeyType& k) const {
		const_iterator it = find(k);
			
		// iterate until we are past the last occurance
		while (it != end() && k == (*it).first) {
			it++;
		}
		return it;
	}
	Pair<iterator,iterator> equal_range (const KeyType& k) {
		iterator it_low, it_high;
			
		it_high = it_low = find(k);
			
		// now continue until we are past the last occurance
		while (it_high != end() && k == (*it_high).first) {
			it_high++;
		}
		return make_pair(it_low, it_high);
	}
	Pair<const_iterator,const_iterator> equal_range(const KeyType& k) const {
		const_iterator it_low, it_high;
			
		it_high = it_low = find(k);
					
		// now continue until we are past the last occurance
		while (it_high != end() && k == (*it_high).first) {
			it_high++;
		}
		return make_pair(it_low, it_high);
	}
		
	void erase(iterator& pos) {
		table[pos.table_index].erase(pos.it);
		_size--;
	}

	size_type erase(const KeyType& k) {
                KeyType key = k; 
                ListType *l = &table[getIndex(key)];
                typename ListType::iterator it = l->begin();
                size_type n = 0;
                
                while (it != l->end()) {
                        if ((*it).first == key) {
                                it = l->erase(it);
                                _size--;
                                n++;
                        } else {
                                it++;
                        }
                }

		return n;
	}
	void clear() {
		for (size_type i = 0; i < table_size; i++) {
			table[i].clear();
		}
		_size = 0;
	}
	HashMap(const HashMap<KeyType, ValueType>& m) : _size(m._size), table_size(m.table_size), table(new ListType[table_size]) {
		for (size_type i = 0; i < table_size; i++) {
			table[i] = m.table[i];
		}
	}
	HashMap() : _size(0), table_size(ms_primes[0]), table(new ListType[table_size]) {}
	virtual ~HashMap() { delete [] table; }

	HashMap<KeyType, ValueType>& operator=(const HashMap<KeyType, ValueType>& m) {
		delete [] table;
		table = NULL;
		_size = m._size;
		table_size = m.table_size;
		table = new ListType[table_size];

		for (size_type i = 0; i < table_size; i++) {
			table[i] = m.table[i];
		}
		return *this;
	}
	friend bool operator==(const HashMap<KeyType, ValueType>& m1, const HashMap<KeyType, ValueType>& m2) {
		return (&m1 == &m2);
	}
};

} // namespace haggle

#endif /* ENABLE_STL */

#endif /* _HASHMAP_H */

