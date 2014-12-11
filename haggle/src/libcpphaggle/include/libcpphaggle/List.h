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
#ifndef __LIST_H
#define __LIST_H

#include <stdio.h>

#if defined(ENABLE_STL)

#include <list>

using namespace std;

#define List std::list
// Dummy namespace
namespace haggle {}

#else

namespace haggle {

/**
   This is a simple implementation of double linked list class. It tries to 
   use the same API as the STL equivalent, but is not complete.

   @author Erik Nordström
  
*/

template <typename T>
class List {
public:
	typedef unsigned long size_type;
private:
	class list_head {
	public:
		list_head *prev, *next;
		bool empty() const { return (this == next); }
		list_head() : prev(NULL), next(NULL) { prev = next = this; }
		list_head(list_head *_prev, list_head *_next) : prev(_prev), next(_next) {
			if (prev == NULL && next == NULL)
				prev = next = this;

			prev->next = this;
			next->prev = this;
		}
		virtual ~list_head() {
			next->prev = prev;
			prev->next = next;
		}
	};
        /* 
           We need this list_head-derived container class, because the
           first list_head in the List class should not contain an
           object. Hence, we need two different types of list_heads:
           one basic, which is just empty, and one that has some type
           of content. */
	template <class TT>
	class container : public list_head {
	public:
		TT obj;
		container(const TT& _obj, list_head *_next = NULL) : list_head(_next ? _next->prev : NULL, _next), obj(_obj) {}
                ~container() {}
	};

        // This size of the list.
	size_type _size;
        // This is the head of the list, and it has no content. Its
        // prev and next pointers points to the head itself when
        // empty.
	list_head head;
public:	
	class iterator {
		friend class List<T>;
		friend class List<T>::const_iterator;
		list_head *pos, *tmp;
		iterator(list_head *l) : pos(l), tmp(pos->next) {}
	public:
		iterator(const iterator& _it) : pos(_it.pos), tmp(_it.tmp) {}
		iterator() : pos(0), tmp(0) {}
		friend bool operator==(const iterator& it1, const iterator& it2) {
			return (it1.pos == it2.pos);
		}
		friend bool operator!=(const iterator& it1, const iterator& it2) {
			return (it1.pos != it2.pos);
		}
		iterator& operator++() { pos = tmp; tmp = pos->next; return *this; }
		iterator& operator--() { pos = tmp; tmp = pos->prev; return *this; }
		iterator operator++(int) { iterator it = iterator(pos); pos = tmp; tmp = pos->next; return it; }
		iterator operator--(int) { iterator it = iterator(pos); pos = tmp; tmp = pos->prev; return it; }
		T& operator*() { container<T> *c = static_cast<container<T> *>(pos); return c->obj; }
	};	
	class const_iterator {
		friend class List<T>;
		list_head *pos, *tmp;
		const_iterator(const list_head *l) : pos(const_cast<list_head *>(l)), tmp(pos->next) {}

	public:
		const_iterator(const const_iterator& _it) : pos(_it.pos), tmp(_it.tmp) {}
		const_iterator(const iterator& _it) : pos(_it.pos), tmp(_it.tmp) {}
		const_iterator() : pos(0), tmp(0) {}
		friend bool operator==(const const_iterator& it1, const const_iterator& it2) {
			return (it1.pos == it2.pos);
		}
		friend bool operator!=(const const_iterator& it1, const const_iterator& it2) {
			return (it1.pos != it2.pos);
		}
		const_iterator& operator++() { pos = tmp; tmp = pos->next; return *this; }
		const_iterator& operator--() { pos = tmp; tmp = pos->prev; return *this; }
		const_iterator operator++(int) { const_iterator it = const_iterator(pos); pos = tmp; tmp = pos->next; return it; }
		const_iterator operator--(int) { const_iterator it = const_iterator(pos); pos = tmp; tmp = pos->prev; return it; }
		const T& operator*() const { container<T> *c = static_cast<container<T> *>(pos); return c->obj; }
	};
	T& front() {
		container<T> *c = static_cast< container<T> *>(head.next);
		return c->obj;
	}
	iterator begin() { return iterator(head.next); }
	iterator end() {return iterator(&head); }
	const_iterator begin() const { return const_iterator(head.next); }
	const_iterator end() const {return const_iterator(&head); }
	size_type size() const { return _size; }
	bool empty() const { return _size == 0; }
	iterator insert(iterator it, const T& obj) {
		container<T> *c = new container<T>(obj, it.pos);
		if (c) {
			_size++;
			return iterator(c);
		}
		return end();
	}
	void push_front (const T& obj) { 
		if (new container<T>(obj, head.next))
			_size++;	      
	}
	void push_back (const T& obj) { 
		if (new container<T>(obj, &head))
			_size++;	
	}
	void pop_front() { 
		if (!empty()) { 
			_size--;
			delete head.next;
		} 
	}
	void pop_back() { 
		if (!empty()) { 
			_size--;
			delete head.prev;
		}
	}
	void remove(const T& value) {
                iterator it = begin();
                T val = value; // make a local copy of the value in
                               // case someone passes a reference to a
                               // value in the list
	
                while (it != end()) {
			if (*it == val) {
				_size--;
                                list_head *tmp = it.pos;
                                it++;
				delete tmp;
			} else {
                                it++;
                        }
		}
	}
	iterator erase(iterator it) {
                /*
                  Should we loop through the list in order to verify
                  that the iterator passed is really part of the list?

                  Doing that is obviously much slower (O(n)) that just
                  deleting the element at the position of the iterator
                  (O(1)).
                 */
                if (!empty() && it != end()) {
                        list_head *tmp = it.pos;
                        it++;
                        _size--;
                        delete tmp;
                        return it;
                }
                return end();
	}
	void clear() {
		while (!empty()) {
			pop_front();
		}
	}
	List() : _size(0), head(NULL, NULL) { head.next = &this->head; head.prev = &this->head; }
	List(const List<T>& l) : _size(l._size), head(&this->head, &this->head) {
		for (const_iterator it = l.begin(); it != l.end(); it++) {
			push_back(*it);
		}
	}
	~List() { 
		clear();
	};

	List<T>& operator=(const List<T>& l) {
		clear();
		for (const_iterator it = l.begin(); it != l.end(); it++) {
			push_back(*it);
		}
		return *this;
	}
};

} // namespace haggle

#endif /* ENABLE_STL */

#endif /* _LIST_H */

