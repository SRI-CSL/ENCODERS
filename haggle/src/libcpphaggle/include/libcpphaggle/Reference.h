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
#ifndef __REFERENCE_H_
#define __REFERENCE_H_

#include "Platform.h"
#include "Pair.h"
#include "List.h"
#include "Mutex.h"
#include "HashMap.h"
#include "String.h"

#include <assert.h>

// For TRACE macro
#include <haggleutils.h>

#define DEBUG_REFERENCE

#if defined(DEBUG) && defined(DEBUG_REFERENCE)
#define REFERENCE_DBG(f, ...) TRACE_DBG(f, ## __VA_ARGS__)
#else
#define REFERENCE_DBG(f, ...)
#endif

namespace haggle {
	
/**
 This class is for keeping a reference count to the object being 
 referenced. It is allocated on the heap, so that there is only one 
 reference count and one mutex to protect that reference count.
 */
class RefCounter {
	typedef Pair<void *, RefCounter *> RefcountPair;
	typedef HashMap<void *, RefCounter *> RefcountMap;
	/**
	 This is a store of objects that have already been refcounted. It is 
	 used to ensure that objects have only one reference counter.
	 */
	static RefcountMap objects;
	
	/**
	 This is a mutex to protect the store above from being manipulated by 
	 more than one thread at a time.
	 */
	static Mutex objectsMutex;
	/**
	 The total number of such object having been refcounted.
	 Used for debugging purposes.
	*/
	static unsigned long totNum;
	/**
	 The mutex protecting the reference count
	 */
	Mutex countMutex;
	/**
	 The reference count. This will start at 1, and when it reaches 0, 
	 the object will be deleted.
	 */
	unsigned long refcount;
	
	/**
	 Constructor.
	 */
	RefCounter(void *_obj);
public:
	
	/**
	 The mutex protecting the object
	 */
	RecursiveMutex objectMutex;
	
	/**
	 The object being refcounted.
	 */
	void *obj;
	
	/**
	 An identifying integer for the object being pointed to.
	 Used for debugging purposes.
	 */
	const unsigned long identifier;
	
	static RefCounter *create(void *_obj);
	/**
	 Destructor.
	 */
	~RefCounter();
	
	template<typename T>
	T *object() { return static_cast<T *>(obj); }
	/**
	 This function increases the reference count atomically.
	 */
	unsigned long inc_count()
	{
		Mutex::AutoLocker l(countMutex);
		if (refcount == 0) {
  		        assert(false); // MOS
			return 0;
		}
		return ++refcount;
	}
	
	/**
	 This function decreases the reference count atomically.
	 */
	template<typename T>
	unsigned long dec_count()
	{
		unsigned long ret;
		countMutex.lock();

		if (refcount == 0) {
			countMutex.unlock();
			return 0;
		}

		ret = --refcount;

		countMutex.unlock();

		if (ret == 0) {	
			objectsMutex.lock();
			objects.erase(obj);
			T * tmp_obj = static_cast<T *>(obj);
			obj = NULL; // MOS
			objectsMutex.unlock();			
			delete this;
			delete tmp_obj;
		} 
		return ret;
	}
	
	/**
	 This function returns the current reference count:
	 */
	unsigned long count()
	{
		return refcount;
	}
};
	
/**
   The Reference class is used for refcounting any type of object. This is done
   without having to make any changes at all to the class being refcounted.
	
   The main use for these references is for passing objects in events. Anyone 
   receiving that event can therefore keep a copy of the reference to that 
   object without having to worry about the object being deleted. An object is
   deleted automatically when there are no longer any references to that 
   object.
	
   The aim here is to make the references as invisible as possible. The ideal 
   is to be able to treat a reference as a pointer to the object. Unfortunately
   this is not always possible. These references can't really point to object
   allocated on the stack, since it tries to assume control of destruction of 
   the object, so some care must be taken to make sure that doesn't happen.
*/
template<typename T>
class Reference {		
	/**
           This class is used to make sure only one thread at a time is accessing
           the refcounted object. It is used in conjunction with the 
           Reference::operator-> function. See that function for more 
           information.
	*/
	class LockProxy {
		/**
                   The object whose member function has been called, and so needs 
                   protecting.
		*/
		T *obj;
		/**
                   The mutex in the object's RefCounter instance.
		*/
		RecursiveMutex *m;
            public:
		/**
                   Constructor. This will lock the object's access mutex, which will
                   be released when this object is destroyed. Since this object is used
                   as it is, that means at the end of the sequence point where the 
                   object's member function was called.
		*/
		LockProxy(T *_obj, RecursiveMutex *_m) : obj(_obj), m(_m) { if (m) m->lock(); }
		/**
                   Destructor. Releases the hold on the object's access mutex.
		*/
		~LockProxy() { if (m) m->unlock(); }
		/**
                   This, like Reference::operator->, allows the Reference to be 
                   treated like a pointer to the object it is a reference to.
		*/
		T *operator->() { return obj; }
		const T *operator->() const { return obj; }
	};
	
	/**
           The data for this reference
	*/
public:
	RefCounter *refCount;
private:
    public:

	/**
           Locking function that can be used to lock access to the object, in case
           prolonged access to part of the referenced object is needed (such as
           going through a list inside the object provided by a member function).
		
           Analogous with ThreadMutex::lock.
	*/
	void lock() const
	{
		if (refCount) {
			Reference<T> *thisObj = const_cast< Reference<T> *>(this);
			thisObj->refCount->objectMutex.lock();
		}
	}
	
	/**
           Unlocking function.
		
           Analogous with ThreadMutex::unlock.
	*/
	void unlock() const
	{
		if (refCount) {
			Reference<T> *thisObj = const_cast< Reference<T> *>(this);
			thisObj->refCount->objectMutex.unlock();
		}
	}
	
	/**
           Attempted locking function. Returns true iff the lock was obtained.
		
           Analogous with ThreadMutex::trylock.
	*/
	bool trylock() const
	{
		if (refCount) {
			Reference<T> *thisObj = const_cast< Reference<T> *>(this);
			return thisObj->refCount->objectMutex.trylock();
		}
		return false;
	}
	
	/**
           Returns the current reference count of the object referenced.
	*/
	unsigned long refcount() const { return refCount ? refCount->count() : 0; }
	
	/**
           Returns the identifying integer associated with this object.
	*/
	unsigned long getId() const { return refCount ? refCount->identifier : 0; }
	
	/**
           Constructor
	*/
	Reference(const T *obj = NULL) : refCount(RefCounter::create(const_cast<T *>(obj)))
	{
		if (!refCount) {
			// ERROR!
		}
	}
	
	/**
           Copy constructor. 
	*/
	
	Reference(const Reference<T> &eo) : refCount(NULL)
	{
		if (!eo.refCount)
			return;
		
		eo.refCount->inc_count();
		refCount = eo.refCount;
	}
	
	template<typename TT>
	Reference(const Reference<TT> &eo) : refCount(NULL)
	{
		if (!eo.refCount)
			return;
		// This is a compiler check to see if static casting is allowed.
		// Such casting is only allowed between objects related through
		// inheritence.
		const T *obj = static_cast<const T *>(eo.getObj());
		
		if (!obj)
			return;
				
		eo.refCount->inc_count();
		refCount = eo.refCount;
	}
	
	/**
           Destructor.
	*/
	virtual ~Reference(void)
	{
		if (!refCount)
			return;

		refCount->dec_count<T>();
	}

	/**
	   This function accesses the referenced object directly. USE WITH CARE,
	   for example, by locking the reference while accessing the object.
	*/
	const T *getObj() const { return refCount ? refCount->object<T>() : NULL; }
	/**
	   This function accesses the referenced object directly. USE WITH CARE,
	   for example, by locking the reference while accessing the object.
	*/
	T *getObj() { return refCount ? refCount->object<T>() : NULL; }
	/**
           This function will return a heap-allocated copy of this reference.
	*/
	Reference<T> *copy() const { return new Reference<T>(*this); }

	/**
           This function provides access to any and all member functions of the 
           refcounted object. By treating the refcount as a pointer to the 
           object, this makes function access completely transparent.
		
           The lock proxy ensures that the object is locked while calling 
           functions.
	*/
	LockProxy operator->()
	{
		if (refCount)
			return LockProxy(const_cast<T *>(refCount->object<T>()), &refCount->objectMutex);
		else
			return LockProxy(NULL, NULL);
	}
	
	const LockProxy operator->() const
	{
		if (refCount)
			return LockProxy(refCount->object<T>(), &refCount->objectMutex);
		else
			return LockProxy(NULL, NULL);
	}
	/**
           Assignment operator. This allows references to be assigned to one 
           another, which makes the reference behave even more like a pointer.
	*/
	Reference<T>& operator=(const Reference<T>& eo)
	{
		// Handle self-assignment:
		if (this == &eo)
			// Do nothing:
			return *this;
		
		// First increase the reference count of the object we'll be pointing to
		// when this is done. 
		if (eo.refCount)
			eo.refCount->inc_count();
		// Then decrease the reference count of the object we were pointing at 
		// before this operation.
		if (refCount)
			refCount->dec_count<T>();
		// Do the actual assignment:
		refCount = eo.refCount;
		
		return *this;
	}
	
	/**
           This function provides direct access to the referenced object. In case 
           this is absolutely necessary.
		
           It was commented out to find all instances of its use. No instance was 
           found where it was absolutely needed, therefore it has not been 
           uncommented. If it is to be uncommented, make sure it's use is 
           absolutely required.
	*/
	//T *operator*() const { return refCount->obj; }

	template<typename TT>
	friend bool operator<(const Reference<T>& eo1, const Reference<TT>& eo2)
	{
		if (eo1.refCount == NULL || eo2.refCount == NULL)
			return false;
		return *eo1.getObj() < *eo2.getObj();
	}

	template<typename TT>
	friend bool operator<(const Reference<T>& eo1, const TT *eo2)
	{
		if (eo1.refCount == NULL && eo2 == NULL)
			return false;
		if (eo1.refCount == NULL || eo2 == NULL)
			return false;
		return eo1.getObj() < *eo2;
	}

	template<typename TT>
	friend bool operator<(const Reference<T>& eo1, const TT &eo2)
	{
		if (eo1.refCount == NULL)
			return false;
		if (eo1.refCount == NULL || &eo2 == NULL)
			return false;
		return *eo1.getObj() < eo2;
	}
	
	/**
           Equality operator. Returns true iff the two references point to the 
           same object, or what the references point to are equal.
	*/
	template<typename TT>
	friend bool operator==(const Reference<T>& eo1, const Reference<TT>& eo2)
	{
		if (eo1.refCount == eo2.refCount)
			return true;
		if (eo1.refCount == NULL || eo2.refCount == NULL)
			return false;
		return *eo1.getObj() == *eo2.getObj();
	}
	
	/**
           Equality operator. Returns true iff the reference points to the object,
           or what the reference points to equals the object.
	*/
	template<typename TT>
	friend bool operator==(const Reference<T>& eo1, const TT *eo2)
	{
		if (eo1.refCount == NULL && eo2 == NULL)
			return true;
		if (eo1.refCount == NULL || eo2 == NULL)
			return false;
		return eo1.getObj() == *eo2;
	}
	template<typename TT>
	friend bool operator==(const Reference<T>& eo1, const TT &eo2)
	{
		if (eo1.refCount == NULL && &eo2 == NULL)
			return true;
		if (eo1.refCount == NULL || &eo2 == NULL)
			return false;
		return *eo1.getObj() == eo2;
	}
	
	/**
           Inequality operator. Returns false iff the two references point to the 
           same object.
	*/
	template<typename TT>
	friend bool operator!=(const Reference<T>& eo1, const Reference<TT>& eo2) {
		return !(eo1 == eo2); 
	}
	/**
           Inequality operator. Returns false iff the reference points to the 
           object, or what the reference points to equals the object.
	*/
	template<typename TT>
	friend bool operator!=(const Reference<T>& eo1, const TT *eo2) {
		return !(eo1 == eo2); 
	}
	
	/**
           Boolean conversion operator. This returns true iff the reference is set
           to point to an object. This helps the reference mimic a pointer to the
           object.
	*/
	operator bool() const
	{
		return (this->refCount != NULL);
	}
	//static void *operator new(size_t size) { throw Exception(0, "Heap allocation not allowed"); }
};

// A generic container class for references
template<class T >
class ReferenceList : public List<Reference<T> >
{
    public:
        ReferenceList() {}
        ReferenceList(const Reference<T>& item) : List<Reference<T> >()
	{
		push_back(item);
	}
        ReferenceList(const ReferenceList<T> & eoList) : List<Reference<T> >()
	{
		typename List<Reference<T> >::const_iterator it;
		
                for (it = eoList.begin(); it != eoList.end(); it++) {
                        this->push_back(*it);
                }
        }
        ~ReferenceList() {}

        ReferenceList<T> *copy() const
	{
                return new ReferenceList(*this);
        }
	int add(const Reference<T> &eo) { this->push_back(eo); return this->size(); }
        Reference<T> pop() 
	{
                if (this->empty())
			return NULL;

		Reference<T> eo = this->front();
		this->pop_front();

		return eo;
	}
};

} // namespace haggle

#endif //_REFERENCE_H_ 
