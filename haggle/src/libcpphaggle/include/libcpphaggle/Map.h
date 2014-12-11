/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 */

/* Copyright 2009 Uppsala University
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
#ifndef __MAP_H_
#define __MAP_H_

#include <stdlib.h>

#if defined(ENABLE_STL)
#include <map>

#define Map std::map
#define MultiMap std::multimap

// Dummy namespace
namespace haggle {}

#else

#include <libcpphaggle/Pair.h>

namespace haggle {

/**
	This is a minimal implementation of a class that does the same thing as 
	std::map. It is not a complete implementation of std::map, since the only 
	things implemented are the ones needed in Haggle.
*/
template <typename Key, typename Value>
class BasicMap {
public:
	typedef unsigned long size_type;
private:
	typedef Pair<Key, Value> member;

	member **the_map;
        size_type number_of_entries;
        size_type map_size;
        static const size_type npos = -1; // the largest possible position
public:
	class iterator {
		friend class BasicMap<Key, Value>;
	private:
		typedef member& reference;
		typedef member* pointer;
		
		typedef BasicMap<Key, Value> map_type;
		// Simply the index in the map array.
		size_type i;
		// The map which this is an iterator for.
		map_type *map;
		
		iterator(size_type _i, map_type *_map) : i(_i), map(_map) {}
	public:
		iterator(const iterator& _it) : i(_it.i), map(_it.map) {}
		iterator() : i(0), map(NULL) {}
		~iterator() {}
		
		/**
			Dereferencing operator for iterators.
		*/
		reference operator*()
		{
			return *(map->the_map[i]);
		}
		
		/**
			Dereferencing operator for iterators.
		*/
		pointer operator->()
		{
			return map->the_map[i];
		}
		
		iterator &operator++()
		{
			if (i < npos) {
				i++;
				if (i >= map->number_of_entries)
					i = npos;
			}
			return *this;
		}
		
		iterator operator++(int)
		{
			iterator it = iterator(i, map);
			if (i < npos) {
				i++;
				if (i >= map->number_of_entries)
					i = npos;
			}
			return it;
		}
		
		friend bool operator==(const iterator &i1, const iterator &i2)
		{
			return i1.i == i2.i && i1.map == i2.map;
		}
		
		friend bool operator!=(const iterator &i1, const iterator &i2)
		{
			return i1.i != i2.i || i1.map != i2.map;
		}
	};
	class const_iterator {
		friend class BasicMap<Key, Value>;
	private:
		typedef const member& reference;
		typedef const member* pointer;
		
		typedef const BasicMap<Key, Value> map_type;
		// Simply the index in the map array.
		size_type i;
		// The map which this is an iterator for.
		map_type *map;
		
		const_iterator(size_type _i, map_type *_map) : i(_i), map(_map) {}
	public:
		const_iterator(const const_iterator& _it) : i(_it.i), map(_it.map) {}
		const_iterator(const iterator& _it) : i(_it.i), map(_it.map) {}
		const_iterator() : i(0), map(NULL) {}
		~const_iterator() {}
		
		/**
		 Dereferencing operator for iterators.
		 */
		reference operator*()
		{
			return *(map->the_map[i]);
		}
		
		/**
		 Dereferencing operator for iterators.
		 */
		pointer operator->()
		{
			return map->the_map[i];
		}
		
		const_iterator &operator++()
		{
			if (i < npos) {
				i++;
				if (i >= map->number_of_entries)
					i = npos;
			}
			return *this;
		}
		
		const_iterator operator++(int)
		{
			const_iterator it = const_iterator(i, map);
			if (i < npos) {
				i++;
				if (i >= map->number_of_entries)
					i = npos;
			}
			return it;
		}
		
		friend bool operator==(const const_iterator &i1, const const_iterator &i2)
		{
			return i1.i == i2.i && i1.map == i2.map;
		}
		
		friend bool operator!=(const const_iterator &i1, const const_iterator &i2)
		{
			return i1.i != i2.i || i1.map != i2.map;
		}
	};
	/**
		Constructor.
	*/
	BasicMap() : the_map(NULL),
		number_of_entries(0),
                map_size(0)
	{
		
	}

        BasicMap(const BasicMap<Key, Value>& m) : the_map(new member*[m.number_of_entries]), 
                                        number_of_entries(m.number_of_entries), 
	                                // map_size(m.map_size) // MOS - hard to find bug
	                                map_size(m.number_of_entries) // MOS
        {
		for (size_type i = 0; i < number_of_entries; i++) {
			the_map[i] = new member(*m.the_map[i]);
		}
        }
	
	/**
		Destructor.
	*/
	~BasicMap()
	{
		clear();
	}
	
	/**
		Returns true iff there are no entries in the map.
	*/
	bool empty() const { return number_of_entries == 0; }
	
	/**
		Returns the number of entries in the map.
	*/
	size_t size() const { return number_of_entries; }
	
	/**
		Erases any and all entries in the map.
	*/
	void clear()
	{
                for (size_type i = 0; i < number_of_entries; i++)
                        delete the_map[i];
                
		if (the_map)
			delete [] the_map;

		number_of_entries = 0;
                map_size = 0;
		the_map = NULL;
	}
	
	/**
		Returns an iterator to the first element in the map.
	*/
	iterator begin()
	{
		if (number_of_entries > 0)
			return iterator(0, this);
		else
			return end();
	}
	const_iterator begin() const 
	{ 
		if (number_of_entries > 0)
			return const_iterator(0, this);
		else
			return end();
	}
	
	/**
		Returns an iterator to the element after the last element in the map.
	*/
	iterator end()
	{
		return iterator(npos, this);
	}
	const_iterator end() const 
	{ 
		return const_iterator(npos, this);
	}
	
private:
	/**
		Figures out wether or not the given key is in the map.
		
		Returns: <pos,true> if the key is in the map, and pos
                indicates the position. If the key is not in the map, the function
                returns <before,false> and before indicates the position the
                element should be inserted at.
			
	*/
	Pair<size_type, bool> _find(const Key& k,
                                   size_type left_min, 
                                   size_type right_max,
                                   size_type before,
                                   bool find_first = false) const
	{
		size_type pos, left_max, right_min;
		
		// Dummy check:
		if (right_max == npos || left_min > right_max) {
			return make_pair(before, false);
		}
		// Find the middle:
		pos = (right_max - left_min + 1)/2 + left_min;

		// Find the end points of the left and right hand side:
		left_max = pos-1;
		right_min = pos+1;
		
		// current key < searched for key?
		if (the_map[pos]->first < k) {
			// The key would be in the right hand side:
			return _find(k, right_min, right_max, right_min, find_first);
		}
		// searched for key < current key?
		if (k < the_map[pos]->first) {
			// The key would be in the left hand side:
			return _find(k, left_min, left_max, pos, find_first);
		}
                
                if (find_first) {
                        // We are not inserting, only finding pos for
                        // iteration. We should always return to
                        // first pos of key 'k' so we backtrack to the
                        // first position
                        while (pos > 0 && the_map[pos-1]->first == k) {
                                pos--;
                        }
                }
		// Found exact match:
		return make_pair(pos, true);
	}
	Pair<size_type, bool> _find(const Key& k, bool find_first = false) const
	{
		return _find(k, 0, number_of_entries-1, 0, find_first);
	}
	/**
	 Inserts a key-value pair into the map, and returns an
	 iterator pointing to the position where the value pair
	 was inserted.
	 */
	iterator _insert(iterator pos, const member& x)
	{
		member **new_map;
		size_type i;
		
                // If the map already has allocated space, just make room and insert.

                if (map_size > number_of_entries) {
                        
                        for (i = number_of_entries; i > pos.i; i--)
                                the_map[i] = the_map[i-1];
                        
                        the_map[pos.i] = new member(x.first, x.second);
                        number_of_entries++;
                        return pos;
                }
                new_map = new member*[number_of_entries + 1];
                
		if (new_map == NULL)
                        return end();
		
		for (i = 0; i < pos.i; i++) {
			new_map[i] = the_map[i];
		}
		
		new_map[pos.i] = new member(x.first, x.second);
		
                for (i = pos.i+1; i < number_of_entries + 1; i++) {
			new_map[i] = the_map[i-1];
		}
		
		if (the_map)
			delete [] the_map;

		the_map = new_map;
		number_of_entries++;
		map_size++;
		
		return pos;
	}
		
public:
	/**
	 Locates, if possible, any entry with the given key.
	 
	 Returns: An iterator to the entry with the given key, or end() if no 
	 such entry exists.
	 */
	iterator find(const Key& k)
	{
		Pair<size_type, bool> tmp;
		
		tmp = _find(k, true);
		
		if (tmp.second)
			return iterator(tmp.first, this);
		else
			return end();
	}
	const_iterator find(const Key& k) const
	{
		Pair<size_type, bool> tmp;
		
		tmp = _find(k, true);
		
		if (tmp.second)
			return const_iterator(tmp.first, this);
		else
			return end();
	}
	
	iterator lower_bound(const Key& k) 
	{
		return find(k);
	}
	const_iterator lower_bound(const Key& k) const
	{
		return find(k);
	}
	
	iterator upper_bound(const Key& k) 
	{
		iterator it = find(k);
		
		// iterate until we are past the last occurance
		while (it != end() && k == (*it).first) {
			it++;
		}
		return it;
	}
	const_iterator upper_bound(const Key& k) const
	{
		const_iterator it = find(k);
		
		// iterate until we are past the last occurance
		while (it != end() && k == (*it).first) {
			it++;
		}
		return it;
	}
	Pair<iterator, iterator> equal_range (const Key& k) 
	{
		iterator it_low, it_high;
		
		it_high = it_low = find(k);
		
		// now continue until we are past the last occurance
		while (it_high != end() && k == (*it_high).first) {
			it_high++;
		}
		return make_pair(it_low, it_high);
	}
	Pair<const_iterator, const_iterator> equal_range (const Key& k) const
	{
		const_iterator it_low, it_high;
		
		it_high = it_low = find(k);
		
		// now continue until we are past the last occurance
		while (it_high != end() && k == (*it_high).first) {
			it_high++;
		}
		return make_pair(it_low, it_high);
	}
	
	iterator insert_unique(iterator pos, const member& x)
	{
		// If there is already an entry at pos with the given key, 
                // return the entry and indicate that there was no insertion.
                if (pos.i < number_of_entries && the_map[pos.i]->first == x.first)
                        return pos;
		
		return _insert(pos, x);
	}
	iterator insert(iterator pos, const member& x)
	{
		return _insert(pos, x);
	}
	iterator insert(const member& x) 
	{
                Pair<size_type, bool> tmp = _find(x.first, true);
		return _insert(iterator(tmp.first, this), x);
	}

	// MOS - with this function entries are ordered on the value as well (important for node interest)

	iterator insert_sorted(const member& x) 
	{
                Pair<size_type, bool> tmp = _find(x.first, true);

		iterator it = iterator(tmp.first, this);
		
		if(tmp.second) {
		  while (it != end() && (*it).first == x.first && (*it).second < x.second) {
		    it++;
		  }
		}

		if(it == end()) return _insert(iterator(number_of_entries, this), x);
		return _insert(it, x);
	}

	Pair<iterator, bool> insert_unique(const member& x)
	{
                Pair<size_type, bool> tmp = _find(x.first);
		
		if (tmp.second)
			return make_pair(iterator(tmp.first, this), false);
		else
			return make_pair(_insert(iterator(tmp.first, this), x), true);
        }
	
        void erase(iterator pos)
        {

                if (pos.i >= number_of_entries)
                        return;
                 
                delete the_map[pos.i];

                for (size_type i = pos.i + 1; i < number_of_entries; i++) {
			the_map[i-1] = the_map[i];
		}

		number_of_entries--; // MOS - map_size can become different from num_entries - need serious use of map_size
        }
        size_type erase(const Key& k)
        {
		size_type n = 0;
		
                iterator it = find(k);

                while (it != end() && (*it).first == k) {
			iterator tmp = it;
			it++;
			erase(tmp);
			n++;
		}
                return n;
        }

	/**
		Returns a reference to the value with the given key. If there was no 
		entry for that key before this function was called, one will be added.
		In that case, the default value for the value type will be filled in 
		before the reference is returned.
	*/
	Value &operator[](const Key& k)
	{
		Pair<size_type, bool> tmp = _find(k);
		              
		if (tmp.second)
			return the_map[tmp.first]->second;
		else
			return (*insert_unique(iterator(tmp.first, this), make_pair(k, Value()))).second;
	}
        
        BasicMap<Key, Value>& operator=(const BasicMap<Key, Value>& m) {
                if (&m == this)
                        return *this;

                clear();
		map_size = m.map_size;
		number_of_entries = m.number_of_entries;
		the_map = new member*[number_of_entries];

		for (size_type i = 0; i < number_of_entries; i++) {
			the_map[i] = new member(*m.the_map[i]);
		}
		return *this;
	}
        
};

template <typename _Key, typename _Mapped>
class Map {
public:
	typedef _Key key_type;
	typedef _Mapped mapped_type;
	typedef Pair<const _Key, _Mapped> value_type;
private:
	typedef BasicMap<_Key, _Mapped> _map_type;
	
	_map_type _the_map;
public:
	typedef typename _map_type::iterator iterator;
	typedef typename _map_type::const_iterator const_iterator;
	typedef typename _map_type::size_type size_type;
	
	Map() : _the_map() {}
	Map(const Map& m) : _the_map(m._the_map) {}
	
	bool empty() const { return _the_map.empty(); }
	
	/**
	 Returns the number of entries in the map.
	 */
	size_t size() const { return _the_map.size(); }
	
	/**
	 Erases any and all entries in the map.
	 */
	void clear() { _the_map.clear(); }
	
	/**
	 Returns an iterator to the first element in the map.
	 */
	iterator begin() { return _the_map.begin(); }
	const_iterator begin() const { return _the_map.begin(); }
	iterator end() { return _the_map.end(); }
	const_iterator end() const { return _the_map.end(); }
	iterator find(const key_type& k) { return _the_map.find(k); }
	const_iterator find(const key_type& k) const { return _the_map.find(k); }
	iterator insert(iterator pos, const value_type& x) { return _the_map.insert_unique(pos, x); }
        Pair<iterator, bool> insert(const value_type& x) { return _the_map.insert_unique(x); }
	void erase(iterator pos) { _the_map.erase(pos); }
        size_type erase(const key_type& k) { return _the_map.erase(k); }
	mapped_type &operator[](const key_type& k) { return _the_map[k]; }
};	



/*
  NOTE/WARNING:

  MultiMap functionality not tested yet!!! Will probably not work at this point.

 */
template <typename _Key, typename _Mapped>
class MultiMap {
public:
	typedef _Key key_type;
	typedef _Mapped mapped_type;
	typedef Pair<const _Key, _Mapped> value_type;
private:
	typedef BasicMap<_Key, _Mapped> _map_type;
	
	_map_type _the_map;
public:
	typedef typename _map_type::iterator iterator;
	typedef typename _map_type::const_iterator const_iterator;
	typedef typename _map_type::size_type size_type;
	
	MultiMap() : _the_map() {}
	MultiMap(const MultiMap& m) : _the_map(m._the_map) {}
	
	bool empty() const { return _the_map.empty(); }

	/**
	 Returns the number of entries in the map.
	 */
	size_t size() const { return _the_map.size(); }
	
	/**
	 Erases any and all entries in the map.
	 */
	void clear() { _the_map.clear(); }
	
	/**
	 Returns an iterator to the first element in the map.
	 */
	iterator begin() { return _the_map.begin(); }
	const_iterator begin() const { return _the_map.begin(); }
	iterator end() { return _the_map.end(); }
	const_iterator end() const { return _the_map.end(); }
	iterator find(const key_type& k) { return _the_map.find(k); }
	const_iterator find(const key_type& k) const { return _the_map.find(k); }
	Pair<iterator, iterator> equal_range (const key_type& k) { return _the_map.equal_range(k); }
	Pair<const_iterator, const_iterator> equal_range (const key_type& k) const { return _the_map.equal_range(k); }	
	iterator lower_bound(const key_type& k) { return _the_map.lower_bound(k); }	
	const_iterator lower_bound(const key_type& k) const { return _the_map.lower_bound(k); }
	iterator upper_bound(const key_type& k) { return _the_map.upper_bound(k); }
	const_iterator upper_bound(const key_type& k) const { return _the_map.upper_bound(k); }
	iterator insert(iterator pos, const value_type& x) { return _the_map.insert(pos, x); }
	iterator insert(const value_type& x) { return _the_map.insert(x); } 
	void erase(iterator pos) { _the_map.erase(pos); }
	size_type erase(const key_type& k) { return _the_map.erase(k); }
};	
	
} // namespace haggle

#define Map haggle::Map
#define MultiMap haggle::MultiMap

#endif /* ENABLE_STL */

#endif /* __MAP_H_ */
