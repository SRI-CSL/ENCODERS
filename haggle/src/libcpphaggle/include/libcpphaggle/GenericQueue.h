/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Sam Wood (SW)
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
#ifndef _GENERICQUEUE_H
#define _GENERICQUEUE_H

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Mutex.h>
#include <libcpphaggle/Signal.h>
#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/Watch.h>

namespace haggle {
/**
	Queue event types returned by, e.g., retrieve()
*/
typedef enum {
	QUEUE_WATCH_ERROR = - 2,  // A non-queue error occurred in watched items
	QUEUE_ERROR = -1,
	QUEUE_TIMEOUT,
	QUEUE_EMPTY,
	QUEUE_WATCH_READ, // A watched item is readable
	QUEUE_WATCH_WRITE, // A watched item is writeable
	QUEUE_WATCH_ABANDONED, // A watched item is writeable
	QUEUE_ELEMENT
} QueueEvent_t;

/**
	A thread-safe queue. It's main purpose is to transfer data between modules
	and worker threads (manager modules).
	
	This queue is not quite thread safe. This is not yet a problem in Haggle,
	because the problem only arises when there is more than one thread getting
	things through the queue, and so far this doesn't happen in Haggle.
	
	The problem is that if there is more than one thread getting data from the 
	queue, only one element in the queue, and both threads try to retrieve at 
	the same time, both may see the signal as being up, even though there is 
	only one element in the queue.
	
	This would cause one of the threads to retrieve the object, and the other 
	to get NULL. This may also corrupt the list and break the entire Queue.
	
	The ONLY reason why this class is not called "Queue" is because there are
	multiple references to the other "Queue" class in haggle, and I wanted to
	make a minimal change/addition. This way the old "Queue" class works just
	the same as before, but this queue class can also be used.
*/
template<class T>
class GenericQueue {
protected:
	const string name;
	Mutex mutex;
	Signal signal;
	List<T> lst;
	// Set to true iff no more inserts is allowed.
	bool isClosed;
public:
	/**
		Closes the queue so that no further elements can be inserted.
		
		FIXME: this really should wake anyone waiting for the queue to alert 
		them to the fact that no more data is coming.
	*/
	void close(void) { isClosed = true; };
	
	/**
		Data insertion function: The inserted element becomes the queue's
		property. The data inside the inserted element also becomes the queue's
		property.
		
		Returns: true iff the given element was inserted.
	*/
	bool insert(T qe, bool unique = false)
	{
		Mutex::AutoLocker l(mutex);
		
		if (isClosed)
			return false;
		
		if (unique) {
			for (typename List<T>::iterator it = lst.begin(); it != lst.end(); it++) {
				if (qe == *it)
					return false;
			}
		}
		lst.push_back(qe);
		
		// Signal that there is something in the list:
		if (!signal.isRaised())
			signal.raise();
		
		return true;
	}
	
	/**
		Data retreival function. The retrieved element is the receiver's
		property. The data inside the retrieved element is also the receiver's
		property.
		
		The funtion does NOT block. 
		
		Returns: [QUEUE_ERROR, QUEUE_ELEMENT, QUEUE_EMPTY].
	*/
	QueueEvent_t retrieveTry(T *qe)
	{
		Timeval timeout;
		timeout.zero();
		return retrieve(qe, &timeout);
	}
	
	/**
		Data retreival function. The retrieved element is the receiver's
		property. The data inside the retrieved element is also the receiver's
		property.
		
		Returns data if some is immediately available or available after less
		than the given Timeval. A NULL Timeval waits until a queue element is
		available.

		Returns: [QUEUE_ERROR, QUEUE_ELEMENT, QUEUE_EMPTY, QUEUE_TIMEOUT, 
		          QUEUE_WATCH_ERROR].
	*/
	QueueEvent_t retrieve(T *qe, const Timeval *timeout = NULL)
	{
		Watch w;
		int res;

		if (!qe)
			return QUEUE_ERROR;

		*qe = NULL; // MOS 

		if (!signal.isRaised()) {
			// Check if the queue is empty
			synchronized(mutex) {
				if (lst.empty() && timeout && 
					timeout->getSeconds() == 0 && 
					timeout->getMicroSeconds() == 0) {
					return QUEUE_EMPTY;
				}
			}
			//mutex.unlock(); // SW: misplaced unlock() (no corresponding lock())

			w.add(signal);

			// Wait for something to be in the queue
			res = w.wait(timeout);

			if (res == Watch::TIMEOUT)
				return QUEUE_TIMEOUT;
			else if (res == Watch::FAILED)
				return QUEUE_WATCH_ERROR;
			else if (res == Watch::ABANDONED)
				return QUEUE_WATCH_ABANDONED;
		}
		// Lock the mutex while getting the first element from the list
		synchronized(mutex) {
		        if(lst.empty()) return QUEUE_ERROR; // MOS
			
			*qe = lst.front();
			lst.pop_front();

			if (lst.empty())
				signal.lower();
		}

		return QUEUE_ELEMENT;
	}
	
	/**
		Data retreival function. The retrieved element is the receiver's
		property. The data inside the retrieved element is also the receiver's
		property.
		
		Returns data if some was immediately available or available after less
		than the given timeout, otherwise returns NULL. When NULL is returned,
		timed_out indicates whether the socket is readable or if the wait timed
		out.
		
		Returns: [QUEUE_ERROR, QUEUE_ELEMENT, QUEUE_EMPTY, QUEUE_TIMEOUT, 
		          QUEUE_WATCH_ERROR, QUEUE_WATCH_READ, QUEUE_WATCH_WRITE].
	*/
	QueueEvent_t retrieve(T *qe, const Watchable wbl, const Timeval *timeout = NULL, bool writeevent = false)
	{
		Watch w;
		int res, sigindex, wblindex;
		
		if (!qe)
			return QUEUE_ERROR;

		*qe = NULL; // MOS 
		
		if (!signal.isRaised()) {
			
			// Check if the queue is empty
			synchronized(mutex) {
				if (lst.empty() && timeout && 
					timeout->getSeconds() == 0 && 
					timeout->getMicroSeconds() == 0) {
					return QUEUE_EMPTY;
				}
			}
			
			sigindex = w.add(signal);
			wblindex = w.add(wbl, writeevent ? WATCH_STATE_WRITE : WATCH_STATE_READ);
			
			res = w.wait(timeout);
			
			if (res == Watch::FAILED) {
				TRACE_ERR("retrieve on queue failed : %s\n", STRERROR(ERRNO));
				return QUEUE_WATCH_ERROR;
			} else if (res == Watch::TIMEOUT)
				return QUEUE_TIMEOUT;
			else if (res == Watch::ABANDONED)
				return QUEUE_WATCH_ABANDONED;
			
			if (w.isReadable(wblindex)) {
				return QUEUE_WATCH_READ;
			}
			if (w.isWriteable(wblindex)) {
				return QUEUE_WATCH_WRITE;
			}
			if (!w.isSet(sigindex) && !signal.isRaised()) {
				return QUEUE_ERROR;
			}
		}
		
		// Lock the mutex while getting the first element from the list
		synchronized(mutex) {
		        if(lst.empty()) return QUEUE_ERROR; // MOS

			*qe = lst.front();
			lst.pop_front();
			
			if (lst.empty())	
				signal.lower();
		}
		
		return QUEUE_ELEMENT;
	}
	
	/**
	   Get the number of elements in the queue;
	*/
	unsigned long size() const { return lst.size(); }
	
	/**
	   Checks whether the queue is empty or not.
	*/
	bool empty() const { return lst.empty(); }
	
	/**
		Constructor
	*/
	GenericQueue(const string _name = "Unnamed Queue") : 
		name(_name), mutex(), isClosed(false)
	{}
	
	/**
		Destructor
		
		When a queue is destroyed, it automatically destroys its contents by
		itself.
	*/
	~GenericQueue()
	{
		// Autolock the mutex
		Mutex::AutoLocker l(mutex);
		
		// Close the queue, just in case:
		close();
		
		// Delete all elements in the list
		while (!lst.empty()) {
		        lst.pop_front();
		}
	}
};

} // namespace haggle

#endif
