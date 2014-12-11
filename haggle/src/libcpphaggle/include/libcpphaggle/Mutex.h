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
#ifndef __MUTEX_H_
#define __MUTEX_H_

#include "Platform.h"

#if defined(OS_WINDOWS)
#include <windows.h>
#elif defined(OS_LINUX) || defined(OS_MACOSX)
#define HAS_PTHREADS
#include <pthread.h>
#else
#error "Unknown operating system!"
#endif

#include <stdlib.h>
#include "String.h"

namespace haggle {
/** 
    Mutex class.
*/
class Mutex
{
        friend class Thread;
        friend class Condition;
private:
#ifdef OS_WINDOWS
        bool _recursive;
        HANDLE mutex;
#else
        pthread_mutex_t mutex;
#endif
public:
        /**
	   Locks the mutex.

	   Returns true iff the lock was aquired.
        */
        virtual bool lock();
        /**
	   Tries to lock the mutex. Returns nonzero if it fails.

	   Returns true iff the lock was aquired.
        */
        virtual bool trylock();
        /**
	   Unlocks the mutex. Returns true.
        */
        virtual bool unlock();
        /**
	   Constructor
        */
        Mutex(bool recursive = false);
        /**
	   Destructor
        */
        virtual ~Mutex();

	/**
	   The AutoLocker class can be used to automatically lock and
	   unlock a mutex within the context of a function.
	 */
	class AutoLocker {
	private:
		Mutex *m;
	public:
		inline AutoLocker(Mutex& _m) : m(&_m) { m->lock(); }
		inline AutoLocker(Mutex *_m) : m(_m) { m->lock(); }
		inline ~AutoLocker() { m->unlock(); }
	};
};

/**
	A recursive mutex behaves like a mutex, except for the fact that it allows
	a single thread to lock the mutex more than once.
*/
class RecursiveMutex : public Mutex
{
public:
	/**
		Constructor
	*/
	RecursiveMutex() : Mutex(true) {}
	/**
		Destructor
	*/
	~RecursiveMutex() {}
};

#define synchronized(mutex) \
        bool done = false; \
        for (Mutex::AutoLocker l(mutex); !done; done = true)
        
} // namespace haggle


#endif /* __MUTEX_H_ */
