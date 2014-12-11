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
#ifndef _CONDITION_H
#define _CONDITION_H

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

#include "Timeval.h"
#include "Mutex.h"

namespace haggle {
/* Real condition variables are only available in Vista and Windows
 * Server 2008, not Windows XP or Windows Mobile */
/** Condition variable

	When running different threads, it is sometimes neccesary for a thread to
	rest and wait for new data or other event to deal with. This is where a
	condition variable comes in.

	Similarly to mutexes or sockets, a thread can wait on a condition variable,
	either indefinately (using wait), or for some specified time (using
	timedWait). Another thread can wake the thread waiting for the condition
	variable by calling the signal() function, which will immediately wake the
	waiting thread.


	Due to problems with the underlying impementations of condition variables,
	if there is more than one thread waiting on a condition variable, the
	result of calling broadcast() or signal() may be unexpected.

	For this reason, it is recommended that at most one thread waits on a
	condition variable at any time.
*/
class Condition
{
        friend class Thread;
private:
#if defined(OS_WINDOWS_VISTA_DISABLED)
        CONDITION_VARIABLE cond;
#elif defined(OS_WINDOWS)
        HANDLE cond;
#elif defined(HAS_PTHREADS)
        pthread_cond_t cond;
#endif
public:
        /**
        	This function takes a LOCKED mutex.
        	
        	It unlocks the mutex, waits for someone to call this->signal() or
        	this->broadcast(), and then locks the mutex again before returning.
        	
        	It returns true iff someone called signal() or broadcast().
        */
        bool wait(Mutex *tmutex);
        /**
        	This function takes a LOCKED mutex.
        	
        	It works similarly to wait(), but will also return if the given time
        	expires. The time should be in relative time to now. (I.e. "sleep for
        	5 seconds" is expressed as timedWait(_, {5,0}).
        	
        	This function returns true iff someone called signal() or broadcast().
        */
        bool timedWait(Mutex *tmutex, const struct timeval *time_to_wait);
        /**
        	Just like timedWait, but sleeps for an even number of seconds.
        */
        bool timedWaitSeconds(Mutex *tmutex, long seconds);
        /**
        	This function aborts all wait() or timedWait() for all threads that are
        	waiting on this Condition.
        */
        void broadcast();
        /**
        	This function aborts a wait() or timedWait() for one thread that is
        	listening for it.
        */
        void signal();

        /**
        	Constructor
        */
        Condition();
        /**
        	Destructor.
        */
        ~Condition();
};

} // namespace haggle

#endif /* CONDITION_H */
