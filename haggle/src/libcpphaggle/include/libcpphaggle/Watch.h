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
#ifndef _WATCH_H
#define _WATCH_H

#include <haggleutils.h>

#include "Platform.h"
#include "Timeval.h"
#include "Signal.h"

namespace haggle {
/*
	This is a platform independent wrapper class for waiting on objects, e.g., 
	file descriptors, sockets, generic events, etc. It is cancelable on all 
	platforms when called from a running thread, i.e., on Windows 
	when someone calls thread->cancel(), the wait function will notice through 
	the current thread's exit signal.
*/
typedef enum {
	WATCHABLE_TYPE_SOCKET,
	WATCHABLE_TYPE_SIGNAL
#if defined(OS_WINDOWS)
	WATCHABLE_TYPE_HANDLE,
#endif
} WatchableType_t;

/*
	A Wrapper for things to watch.
*/

typedef int SOCKET;

class Watchable {
	friend class Watch;
	WatchableType_t type;
	SOCKET s;
#if defined(OS_WINDOWS)
	HANDLE h;
public:
	Watchable(HANDLE _h);
	HANDLE getHandle() const;
#else
public:
#endif
	Watchable(SOCKET _s);
	Watchable(Signal& sig);
	virtual ~Watchable();
	WatchableType_t getType() const;
	const char *getStr() const;
	bool isValid() const;
	SOCKET getSocket() const;
	
	friend bool operator==(const Watchable& w1, const Watchable& w2);
	friend bool operator==(const SOCKET s, const Watchable& w);
	friend bool operator==(const Watchable& w1, const SOCKET s);
	friend bool operator==(const Signal& s, const Watchable& w);
	friend bool operator==(const Watchable& w, const Signal& s);

	friend bool operator!=(const Watchable& w1, const Watchable& w2);
	friend bool operator!=(const SOCKET s, const Watchable& w);
	friend bool operator!=(const Watchable& w1, const SOCKET s);
	friend bool operator!=(const Signal& s, const Watchable& w);
	friend bool operator!=(const Watchable& w, const Signal& s);
#if defined (OS_WINDOWS)
	friend bool operator==(const HANDLE h, const Watchable& w);
	friend bool operator==(const Watchable& w1, const HANDLE h);
	friend bool operator!=(const HANDLE h, const Watchable& w);
	friend bool operator!=(const Watchable& w1, const HANDLE h);
#endif

	friend bool operator<(const Watchable& w1, const Watchable& w2);
	friend bool operator>(const Watchable& w1, const Watchable& w2);
};

// This number cannot be too low. The kernel can use quite a lot of sockets and
// other watchable objects in its event loop
#define WATCH_MAX_NUM_OBJECTS 20

#define WATCH_OBJECT_INDEX_MIN 0
#define WATCH_OBJECT_INDEX_MAX (WATCH_MAX_NUM_OBJECTS-1)

/*
  Watch flags that determine which events to watch for
  each watchable item.
*/
#define WATCH_STATE_NONE       0x0
#define WATCH_STATE_READ       0x1
#define	WATCH_STATE_WRITE      0x2
#define	WATCH_STATE_EXCEPTION  0x4
#define	WATCH_STATE_ALL        (WATCH_STATE_READ | WATCH_STATE_WRITE | WATCH_STATE_EXCEPTION)
#define WATCH_STATE_DEFAULT    (WATCH_STATE_READ)

/** */
class Watch
{
private:
#if defined(OS_WINDOWS)
	struct {
		SOCKET sock;
		bool isValid;
	} waitSockets[WATCH_MAX_NUM_OBJECTS];
	HANDLE objects[WATCH_MAX_NUM_OBJECTS]; // The object handles to watch#if defined(OS_WINDOWS)
	int addhandle(HANDLE h, u_int8_t state);
#else
	int objects[WATCH_MAX_NUM_OBJECTS]; // The object file descriptors to watch
#endif
	u_int8_t objectStates[WATCH_MAX_NUM_OBJECTS]; // The states to watch for
	u_int8_t objectIsSet[WATCH_MAX_NUM_OBJECTS]; // The states currently set
	int numObjects;
	Timeval absoluteTimeout; // Time when waiting was started
	bool timeoutValid;  // True when absolute timeout is set
	Signal *s;
	int addsock(SOCKET sock, u_int8_t state);
public:
	enum {
		FAILED = -1,
		TIMEOUT = 0,
		SET = 1,
		ABANDONED = 2
	};
	/*
		Add objects to watch set.
		Returns -1 on error or an index between 0 and WATCH_OBJECT_INDEX_MAX.
		When a wait returns, the index can be used to see if the object was set.
	*/
	int add(Watchable wbl, u_int8_t state = WATCH_STATE_DEFAULT);
	/**
		Wait on the objects added to the Watch. If timeToWait is NULL, then
		the timeout will be infinate.

		To know exactly which objects that are set, call
		isSet() using the index of a particular object that
		was returned by add()/addsocket().

		@returns Watch::FAILED if there was an error,
		Watch::TIMEOUT if there was a timeout, Watch::SET if
		at least one watched object is set, or
		Watch::ABANDONED if the Watch was abandoned (this
		usually happens when a thread should exit).
	*/
	int wait(const Timeval *timeout = NULL);
	/**
           Convenience function to wait a specified number of milliseconds.
           
           @param milliseconds the number of milliseconds to wait
           @returns see add()
        */
	int waitTimeout(unsigned long milliseconds);
	
	/**
		After a timeout, this can be used to retreive the remaining time for the timeout.

		@returns true on success, false on error (e.g., this
		function is called when the remaining time has already
		passed).
	*/
	bool getRemainingTime(Timeval *remaining);

	/* 
		Remove all objects from watch set
	*/
	void clear();
	/* 
		Reset status of objects, i.e., they will remain in the wait set,
		but they will no longer be "set"/signaled.
	*/
	void reset();
	/*
		Check if ANY of the states are set according to the
		function argument "state". If WATCH_STATE_NONE is
		given, the state used is the one given when the object
		was first added to the watch.
	*/
	bool isSet(int objectIndex, u_int8_t state = WATCH_STATE_NONE);
	bool isReadable(int objectIndex) { return isSet(objectIndex, WATCH_STATE_READ); }
	bool isWriteable(int objectIndex) { return isSet(objectIndex, WATCH_STATE_WRITE); }
	bool isReadOrWriteable(int objectIndex) { return isSet(objectIndex, WATCH_STATE_READ | WATCH_STATE_WRITE); }
      
	Watch(void);
	~Watch(void);
};

} // namespace haggle

#endif /* _WATCH */
