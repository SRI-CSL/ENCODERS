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
#ifndef _EVENTQUEUE_H
#define _EVENTQUEUE_H

/*
 Forward declarations of all data types declared in this file. This is to
 avoid circular dependencies. If/when a data type is added to this file,
 remember to add it here.
 */
class EventQueue;

#ifndef OS_WINDOWS
#include <unistd.h>
#include <fcntl.h>
#endif
#include <time.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Heap.h>
#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/Watch.h>
#include <libcpphaggle/List.h>
#include <haggleutils.h>

#include "Event.h"

typedef enum {
	EQ_ERROR = -1, EQ_TIMEOUT, EQ_EVENT, EQ_EMPTY, EQ_EVENT_SHUTDOWN
} EQEvent_t;

// CBMEN, HL, Begin
struct EventCancellationRequest {
	const EventType etype;
	const DataObjectRef dObj;
	const NodeRef node;
	const double after;

	EventCancellationRequest(
	    EventType _etype = -1,
	    DataObjectRef _dObj = NULL,
	    NodeRef _node = NULL,
	    double _after = -1) :
		etype(_etype),
		dObj(_dObj),
		node(_node),
		after(_after) {}
};
// CBMEN, HL, End

// Locking is provided by a mutex, so the queue should be thread safe
/** */
class EventQueue: public Heap {
private:
	Mutex mutex;
	Mutex shutdown_mutex;bool shutdownEvent;
	// CBMEN, HL, Begin
	Mutex cancellationMutex;
	List<EventCancellationRequest> cancellations;
	bool clearCanceledEvents() {
		// Assuming caller has already taken mutex
		Mutex::AutoLocker l(cancellationMutex);
		if (empty()) {
			return false;
		}

		HeapItem **items = getHeapArray();
		if (!items) {
			return false;
		}

		Timeval now = Timeval::now();
		for (size_t i = 0; i < size(); i++) {

			Event *e = static_cast<Event *>(items[i]);

			for (List<EventCancellationRequest>::iterator it = cancellations.begin(); it != cancellations.end(); it++) {

				EventCancellationRequest req = *it;

				if (req.etype != -1 &&
				    e->getType() != req.etype) {
					continue;
				}

				if (req.dObj &&
				    e->getDataObject() &&
				    req.dObj != e->getDataObject()) {
					continue;
				}

				if (req.node &&
				    e->getNode() &&
				    req.node != e->getNode()) {
					continue;
				}

				if (req.after > 0 &&
				    e->getTimeout() <= (now + Timeval(req.after))) {
					continue;
				}

				e->setCanceled();
				break;
			}
		}
		cancellations.clear();

		Event *e;
		while (!empty() && (e = static_cast<Event *>(front())) && e->isCanceled())
		{
			pop_front();
		}
		return true;
	}
	// CBMEN, HL, End
protected:
	Signal signal;
public:
	EventQueue() :
			Heap(), shutdownEvent(false) {
	}
	~EventQueue() {
		Event *e;

		while ((e = static_cast<Event *>(extractFirst())))
			delete e;
	}
	EQEvent_t hasNextEvent() {
		Mutex::AutoLocker l(mutex);
		return shutdownEvent ?
				EQ_EVENT_SHUTDOWN : ((clearCanceledEvents() && empty()) ? EQ_EMPTY : EQ_EVENT); // CBMEN, HL
	}
	EQEvent_t getNextEventTime(Timeval *tv) {
		Mutex::AutoLocker l(mutex);

		if (!tv)
			return EQ_ERROR;

		signal.lower();

		if (shutdownEvent) {
			tv->zero();
			return EQ_EVENT_SHUTDOWN;
		} else if (clearCanceledEvents() && !empty()) { // CBMEN, HL
			*tv = static_cast<Event *>(front())->getTimeout();
			return EQ_EVENT;
		}
		return EQ_EMPTY;
	}
	Event *getNextEvent() {
		Event *e = NULL;

		synchronized(shutdown_mutex) {
			if (shutdownEvent) {
				shutdownEvent = false;
				signal.lower();
				return new Event(EVENT_TYPE_SHUTDOWN, 0);
			}
		}

		mutex.lock();
		clearCanceledEvents(); // CBMEN, HL
		e = static_cast<Event *>(extractFirst());
		e->setScheduled(false);
		mutex.unlock();

		return e;
	}
	void enableShutdownEvent() {
		Mutex::AutoLocker l(shutdown_mutex);
		shutdownEvent = true;
		HAGGLE_DBG("Setting shutdown event\n");
		signal.raise();
	}
	void addEvent(Event *e) {
		Mutex::AutoLocker l(mutex);

		if (e && insert(e)) {
			e->setScheduled(true);
			signal.raise();
		}
	}

	// CBMEN, HL, Begin
	List<Event *> getEventList() {
		Mutex::AutoLocker l(mutex);

		List<Event *> list;
		HeapItem **items = getHeapArray();
		if (!items) {
			return list;
		}

		for (size_t i = 0; i < size(); i++) {
			Event *e = static_cast<Event *>(items[i]);
			list.push_back(new Event(*e));
		}
		return list;
	}

	// There has to be a more concise way of doing this that's still efficient - upgrade to C++11 and use lambdas?
	void cancelEvents(const EventType type) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(type, (DataObjectRef) NULL, (NodeRef) NULL, -1));
	}

	void cancelEvents(const NodeRef& node) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(-1, (DataObjectRef) NULL, (NodeRef) node, -1));
	}

	void cancelEvents(const DataObjectRef& dObj) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(-1, (DataObjectRef) dObj, (NodeRef) NULL, -1));
	}

	void cancelEvents(const EventType type, const NodeRef& node) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(type, (DataObjectRef) NULL, (NodeRef) node, -1));
	}

	void cancelEvents(const EventType type, const DataObjectRef& dObj) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(type, (DataObjectRef) dObj, (NodeRef) NULL, -1));
	}

	void cancelEvents(const double after) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(-1, (DataObjectRef) NULL, (NodeRef) NULL, after));
	}

	void cancelEvents(const EventType type, const double after) {
		Mutex::AutoLocker l(cancellationMutex);
		cancellations.push_back(EventCancellationRequest(type, (DataObjectRef) NULL, (NodeRef) NULL, after));
	}

	// CBMEN, HL, End
};

#endif /* _EVENTQUEUE_H */
