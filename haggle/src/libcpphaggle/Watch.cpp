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
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Watch.h>
#include <libcpphaggle/Thread.h>
#include <string.h>

// For TRACE macro
#include <haggleutils.h>

namespace haggle {

#if defined(OS_WINDOWS)
Watchable::Watchable(SOCKET _s) : type(WATCHABLE_TYPE_SOCKET), s(_s), h(INVALID_HANDLE_VALUE) {}
Watchable::Watchable(HANDLE _h) : type(WATCHABLE_TYPE_HANDLE), s(-1), h(_h) {}
Watchable::Watchable(Signal& sig) : type(WATCHABLE_TYPE_SIGNAL), s(-1), h(sig.signal) {}
#else
Watchable::Watchable(SOCKET _s) : type(WATCHABLE_TYPE_SOCKET), s(_s) {}
Watchable::Watchable(Signal& sig) : type(WATCHABLE_TYPE_SIGNAL), s(sig.signal[0]) {}
#endif
	
Watchable::~Watchable()
{
}
	
WatchableType_t Watchable::getType() const 
{ 
	return type; 
}
	
bool operator==(const Watchable& w1, const Watchable& w2)
{
#if defined(OS_WINDOWS)
	return (w1.type == w2.type && w1.s == w2.s && w1.h == w2.h);
#else
	return (w1.type == w2.type && w1.s == w2.s);
#endif
}

bool operator==(const SOCKET s, const Watchable& w)
{
	return (w.type == WATCHABLE_TYPE_SOCKET && s == w.s);
}

bool operator==(const Watchable& w, const SOCKET s)
{
	return (w.type == WATCHABLE_TYPE_SOCKET && w.s == s);
}

bool operator==(const Signal& s, const Watchable& w)
{
#if defined(OS_WINDOWS)
	return (w.type == WATCHABLE_TYPE_SIGNAL && s.signal == w.h);
#else
	return (w.type == WATCHABLE_TYPE_SIGNAL && s.signal[0] == w.s);
#endif
}

bool operator==(const Watchable& w, const Signal& s)
{
#if defined(OS_WINDOWS)
	return (w.type == WATCHABLE_TYPE_SIGNAL && w.h == s.signal);
#else
	return (w.type == WATCHABLE_TYPE_SIGNAL && w.s == s.signal[0]);
#endif
}


bool operator!=(const Watchable& w1, const Watchable& w2)
{
#if defined(OS_WINDOWS)
	return (w1.type != w2.type || w1.s != w2.s || w1.h != w2.h);
#else
	return (w1.type != w2.type ||  w1.s != w2.s);
#endif
}

bool operator!=(const SOCKET s, const Watchable& w)
{
	return (w.type != WATCHABLE_TYPE_SOCKET || s != w.s);
}

bool operator!=(const Watchable& w, const SOCKET s)
{
	return (w.type != WATCHABLE_TYPE_SOCKET || w.s != s);
}

bool operator!=(const Signal& s, const Watchable& w)
{
#if defined(OS_WINDOWS)
	return (w.type != WATCHABLE_TYPE_SIGNAL || s.signal != w.h);
#else
	return (w.type != WATCHABLE_TYPE_SIGNAL || s.signal[0] != w.s);
#endif
}

bool operator!=(const Watchable& w, const Signal& s)
{
#if defined(OS_WINDOWS)
	return (w.type != WATCHABLE_TYPE_SIGNAL || w.h != s.signal);
#else
	return (w.type != WATCHABLE_TYPE_SIGNAL || w.s != s.signal[0]);
#endif
}

#if defined (OS_WINDOWS)
bool operator==(const HANDLE h, const Watchable& w)
{
	return (w.type == WATCHABLE_TYPE_HANDLE && h == w.h);
}
bool operator==(const Watchable& w, const HANDLE h)
{
	return (w.type == WATCHABLE_TYPE_HANDLE && w.h == h);
}

bool operator!=(const HANDLE h, const Watchable& w)
{
	return (w.type != WATCHABLE_TYPE_HANDLE || h != w.h);
}
bool operator!=(const Watchable& w, const HANDLE h)
{
	return (w.type != WATCHABLE_TYPE_HANDLE || w.h != h);
}
#endif

bool operator<(const Watchable& w1, const Watchable& w2)
{
	if (w1.type < w2.type)
		return true;
	else if (w1.type > w2.type)
		return false;

#if defined(OS_WINDOWS)
	switch (w1.type) {
		case WATCHABLE_TYPE_SOCKET:
			return (w1.s < w2.s);
		case WATCHABLE_TYPE_SIGNAL:
			return (w1.h < w2.h);
		case WATCHABLE_TYPE_HANDLE:
			return (w1.h < w2.h);
	}
#else
	switch (w1.type) {
		case WATCHABLE_TYPE_SOCKET:
			return (w1.s < w2.s);
		case WATCHABLE_TYPE_SIGNAL:
			return (w1.s < w2.s);
	}
#endif
	return false;
}

bool operator>(const Watchable& w1, const Watchable& w2)
{
	if (w1.type > w2.type)
		return true;
	else if (w1.type < w2.type)
		return false;

#if defined(OS_WINDOWS)
	switch (w1.type) {
		case WATCHABLE_TYPE_SOCKET:
			return (w1.s > w2.s);
		case WATCHABLE_TYPE_SIGNAL:
			return (w1.h > w2.h);
		case WATCHABLE_TYPE_HANDLE:
			return (w1.h > w2.h);
	}
#else
	switch (w1.type) {
		case WATCHABLE_TYPE_SOCKET:
			return (w1.s > w2.s);
		case WATCHABLE_TYPE_SIGNAL:
			return (w1.s > w2.s);
	}
#endif
	return false;
}

bool Watchable::isValid() const
{
#if defined(OS_WINDOWS)
	if (type == WATCHABLE_TYPE_HANDLE || type == WATCHABLE_TYPE_SIGNAL)
		return (h != INVALID_HANDLE_VALUE);
#endif
	return (s != INVALID_SOCKET);
}

const char *Watchable::getStr() const
{
	static char str[30];
#if defined(OS_WINDOWS)
	if (type == WATCHABLE_TYPE_HANDLE) {
		snprintf(str, 30, "HANDLE:%lu", (unsigned long)h);
	} else if (type == WATCHABLE_TYPE_SIGNAL) {
		snprintf(str, 30, "SIGNAL:%lu", (unsigned long)h);
	} else {
		snprintf(str, 30, "SOCKET:%d", s);
	}
#else
	if (type == WATCHABLE_TYPE_SIGNAL) {
		snprintf(str, 30, "SIGNAL:%d", s);
	} else {
		snprintf(str, 30, "SOCKET:%d", s);
	}
#endif
	return str;
}

SOCKET Watchable::getSocket() const
{
#if defined(OS_WINDOWS)
	return (type == WATCHABLE_TYPE_SOCKET ? s : -1);
#else
	return ((type == WATCHABLE_TYPE_SOCKET || type == WATCHABLE_TYPE_SIGNAL) ? s : -1);
#endif
}

#if defined(OS_WINDOWS)
HANDLE Watchable::getHandle() const
{
	return (type != WATCHABLE_TYPE_SOCKET ? h : INVALID_HANDLE_VALUE);
}
#endif

Watch::Watch() : numObjects(0), timeoutValid(false), s(Thread::selfGetExitSignal())
{
	int i;

	for (i = 0; i < WATCH_MAX_NUM_OBJECTS; i++) {
	        objects[i] = 0; // MOS - not needed but better for debugging
	        objectStates[i] = 0; // MOS - not needed but better for debugging
		objectIsSet[i] = 0;
#if defined (OS_WINDOWS)
		waitSockets[i].isValid = false;
#endif
	}
	// We retreive the cancel event, such that we can wait on it
	// and cancel in case someone cancels the thread.
	if (s) {
		add(*s);
	}
}

Watch::~Watch(void)
{
	clear();
}

int Watch::addsock(SOCKET sock, u_int8_t state)
{
	if (numObjects >= WATCH_MAX_NUM_OBJECTS || !(state & WATCH_STATE_ALL))
		return -1;

	objectStates[numObjects] = state;
#if defined(OS_WINDOWS)
	objects[numObjects] = WSACreateEvent();
	waitSockets[numObjects].isValid = true;
	waitSockets[numObjects].sock = sock;
#else
	objects[numObjects] = sock;
#endif
	// return the index used and then increment the count
	return numObjects++;
}

#if defined(OS_WINDOWS)
int Watch::addhandle(HANDLE h, u_int8_t state)
{
	if (numObjects >= WATCH_MAX_NUM_OBJECTS || !(state & WATCH_STATE_ALL))
		return -1;

	objects[numObjects] = h;
	objectStates[numObjects] = state;

	return numObjects++;
}
#endif

int Watch::add(Watchable wbl, u_int8_t state)
{
	if (numObjects >= WATCH_MAX_NUM_OBJECTS || !(state & WATCH_STATE_ALL))
		return -1;

	switch (wbl.type) {
#if defined(OS_WINDOWS)
		case WATCHABLE_TYPE_HANDLE:
			return addhandle(wbl.h, state);
		case WATCHABLE_TYPE_SIGNAL:
			return addhandle(wbl.h, WATCH_STATE_READ);
#else
		case WATCHABLE_TYPE_SIGNAL:
			return addsock(wbl.s, WATCH_STATE_READ);
#endif
		case WATCHABLE_TYPE_SOCKET:
			return addsock(wbl.s, state);
	}
	return -1;
}

/**
	Watch::reset() will reset all watched objects in the watch to
	unset state and also reset the remaining timeout time.

*/
void Watch::reset()
{
	int i;

	for (i = 0; i < numObjects; i++) {
		objectIsSet[i] = false;	
#if defined(OS_WINDOWS)
		if (waitSockets[i].isValid) {
			WSAEventSelect(waitSockets[i].sock, objects[i], 0);
			WSAResetEvent(objects[i]);
		}
#endif
	}
	timeoutValid = false;
}
/**
	Watch::clear() will remove all watched objects from the watch.

*/
void Watch::clear()
{
	int i = 0;

	// Don't clear the exit signal
	if (s)
		i++;

	timeoutValid = false;

	for (;i < numObjects; i++) {
		//printf("clearing wait index=%d\n", i);
#if defined(OS_WINDOWS)
		if (waitSockets[i].isValid) {
			WSACloseEvent(objects[i]);
			waitSockets[i].isValid = false;
		}
#else
		objects[i] = -1;
#endif
		objectIsSet[i] = false;
	}
	numObjects = i;
}

bool Watch::isSet(int objectIndex, u_int8_t state)
{
	if (objectIndex >= 0 && objectIndex < numObjects) {
		if (state == WATCH_STATE_NONE) 
			return (objectIsSet[objectIndex] & objectStates[objectIndex]) != 0;
		else 
			return (objectIsSet[objectIndex] & state) != 0;
	}
	return false;
}

int Watch::wait(const Timeval *timeout)
{
	int i, waitResult, numObjectsSet = 0, ret = Watch::TIMEOUT;
	
	if (numObjects == 0 && !s)
		return Watch::FAILED;

	absoluteTimeout.setNow();

	if (timeout) {
		if(*timeout <= 0)
			return Watch::TIMEOUT;
		absoluteTimeout += *timeout;
		timeoutValid = true;
	} else
		timeoutValid = false;

#if defined(OS_WINDOWS)
	DWORD millisec;
	
	if (timeout == NULL)
		millisec = INFINITE;
	else
		millisec = (DWORD)timeout->getTimeAsMilliSeconds();

	if (millisec < 0)
		return Watch::FAILED;

	for (i = 0; i < numObjects; i++) {
		int flags = 0;
		objectIsSet[i] = 0;

		if (objectStates[i] & WATCH_STATE_READ)
			flags |= (FD_READ | FD_ACCEPT | FD_CLOSE);
		else if (objectStates[i] & WATCH_STATE_WRITE)
			flags |= (FD_WRITE | FD_CLOSE);

		// Check is this object is a socket
		if (waitSockets[i].isValid) {
			//TRACE_DBG("Thread %u : object %d is a socket\n", thr ? thr->getNum() : -1, i);
			if (WSAEventSelect(waitSockets[i].sock, objects[i], flags) == SOCKET_ERROR) {
				TRACE_ERR("Could not set WSAEventSelect for object %d\n", i);
				return Watch::FAILED;
			}
		} //else {
			//TRACE_DBG("Thread %u : object %d is a handle\n", thr ? thr->getNum() : -1, i);
		//}
	}

	waitResult = WaitForMultipleObjects(numObjects, objects, FALSE, millisec);

	if (waitResult >= WAIT_ABANDONED_0 && waitResult < (int)(WAIT_ABANDONED_0 + numObjects)) {
		TRACE_ERR("Wait was abandoned for object index %d\n", (waitResult - WAIT_ABANDONED_0));
	} else if (waitResult >= WAIT_OBJECT_0 && waitResult < (int)(WAIT_OBJECT_0 + numObjects)) {
		//TRACE_DBG("Object %d is set\n", (ret - WAIT_ABANDONED_0));
		ret = Watch::SET;
		i = waitResult - WAIT_OBJECT_0;
		// Check if it was the thread cancel event that was set
		// Since we do not wait on all objects above, it should be safe to exit 
		// after the first event is found
		if (i == 0 && s) {
			TRACE_DBG("%s is cancelling itself due to cancel event\n", Thread::selfGetName());
			ret = Watch::ABANDONED;
		} 
		
		// Check sockets
		for (; i < numObjects; i++) {
			WSANETWORKEVENTS netEvents;
			DWORD res;

			/* 
			Check if an object other then a socket was set.
			If it is the first index (==0), then we know for sure that the object's state
			is set. Otherwise we do not know, and have to check with WaitForSingleObject.
			*/
			if (!waitSockets[i].isValid) {
				if (i == (waitResult - WAIT_OBJECT_0) ||  
					(WaitForSingleObject(objects[i], 0) == WAIT_OBJECT_0)) {

					/*
					  We assume that non-socket
					  objects have a binary state,
					  either set or not, which we
					  indicate by setting the
					  readable flag.
					 */
					objectIsSet[i] |= WATCH_STATE_READ;
					//TRACE_DBG("object [%d] is set\n", i);
                                        numObjectsSet++;
				}
                                
				continue;
			}
			// This call will automatically reset the Event as well
			res = WSAEnumNetworkEvents(waitSockets[i].sock, objects[i], &netEvents);

			if (res == 0) {;
				//TRACE_DBG("socket object [%d] is set\n", i);
				if (netEvents.lNetworkEvents & FD_READ) {
					//TRACE_DBG("FD_READ on object %d\n", i);
					if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
						TRACE_ERR("Read error\n");
						continue;
					}
						
					objectIsSet[i] |= WATCH_STATE_READ;
					//TRACE_DBG("Socket %d is set\n", waitSockets[i].sock);
				}
				if (netEvents.lNetworkEvents & FD_ACCEPT) {
					TRACE_DBG("FD_ACCEPT on object %d\n", i);
					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
						TRACE_ERR("Accept error\n");
						continue;
					}
						
					objectIsSet[i] |= WATCH_STATE_READ;
					//TRACE_DBG("Socket %d is set\n", waitSockets[i].sock);
				}
				if (netEvents.lNetworkEvents & FD_CONNECT) {
					TRACE_DBG("FD_CONNECT on object %d\n", i);
					if (netEvents.iErrorCode[FD_CONNECT_BIT] != 0) {
							TRACE_ERR("Connect error\n");
							continue;
						}
					objectIsSet[i] |= WATCH_STATE_READ;
					//TRACE_DBG("Socket %d is set\n", waitSockets[i].sock);
				}
				if (netEvents.lNetworkEvents & FD_CLOSE) {
					TRACE_DBG("FD_CLOSE on object %d\n", i);
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
						TRACE_ERR("Close error\n");
						continue;
					}
					objectIsSet[i] |= WATCH_STATE_READ;
					//TRACE_DBG("Socket %d is set\n", waitSockets[i].sock);
				}
				if (netEvents.lNetworkEvents & FD_WRITE) {
					TRACE_DBG("FD_WRITE on object %d\n", i);
					if (netEvents.iErrorCode[FD_WRITE_BIT] != 0) {
						TRACE_ERR("Write error\n");
						continue;
					}
					objectIsSet[i] |= WATCH_STATE_WRITE;
				}
			} else {

				switch (WSAGetLastError()) {
					case WSANOTINITIALISED:
						TRACE_ERR("WSA not initialized\n");
						break;
					case WSAENETDOWN:
						TRACE_ERR("Network subsystem is down\n");
						break;
					case WSAEINVAL:
						TRACE_ERR("Bad parameter\n");
						break;
					case WSAEINPROGRESS:
						TRACE_ERR("WSA Call in progress\n");
						break;
					case WSAENOTSOCK:
						TRACE_ERR("Handle is not a socket\n");
						break;
					case WSAEFAULT:
						TRACE_ERR("Invalid argument address\n");
						break;
					default:
						TRACE_ERR("Unknown WSAEnumNetworkEvents ERROR\n");
				}
				// Error occurred... do something to handle.
				TRACE_ERR("WSAEnumNetworkEvents ERROR\n");
				numObjectsSet = -1;
				goto out;
			}
		}
	} 
out:
	// Cleanup socket events
	for (i = 0; i < numObjects; i++) {
		if (waitSockets[i].isValid) {
			WSAEventSelect(waitSockets[i].sock, objects[i], 0);
			WSAResetEvent(objects[i]);
		}
	}

	if (waitResult == WAIT_FAILED) {
		TRACE_ERR("Wait on objects failed: %s\n", STRERROR(GetLastError()));
		return Watch::FAILED;
	}
	if (waitResult == WAIT_TIMEOUT) {
		timeoutValid = false;
		return Watch::TIMEOUT;
	}
#else
	int maxfd = -1;
	fd_set readset;
	fd_set writeset;
	fd_set errorset;
	bool validread = false, validwrite = false, validerror = false;

	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&errorset);

      	for (i = 0; i < numObjects; i++) {	        

	        if(objects[i] >= FD_SETSIZE) { // MOS - adding check to catch overflows
		  TRACE_ERR("File descriptor illegal or out of bounds for select...\n");
		  return Watch::FAILED;
	        }

		objectIsSet[i] = 0;

		if (objectStates[i] & WATCH_STATE_READ) {
			FD_SET(objects[i], &readset);
			validread = true;
		}
		if (objectStates[i] & WATCH_STATE_WRITE) {
			FD_SET(objects[i], &writeset);
			validwrite = true;
		}
		if (objectStates[i] & WATCH_STATE_EXCEPTION) {
			FD_SET(objects[i], &errorset);
			validerror = true;
		}

		if (objects[i] > maxfd)
			maxfd = objects[i];
	}
	
	if (!validread && !validwrite && !validerror) {
		TRACE_ERR("Nothing to wait on...\n");
		return Watch::FAILED;
	}

	waitResult = select(maxfd + 1, validread ? &readset : NULL, 
			    validwrite ? &writeset : NULL, 
			    validerror ? &errorset : NULL, 
			    timeout ? const_cast<struct timeval *>(timeout->getTimevalStruct()) : NULL);

	if (waitResult < 0) {
		TRACE_ERR("Wait on objects failed: %s\n", strerror(errno));
		return Watch::FAILED;
	} else if (waitResult == 0) {
		timeoutValid = false;
		return Watch::TIMEOUT;
	}

	ret = Watch::SET;

	for (i = 0; i < numObjects; i++) {

		if (FD_ISSET(objects[i], &readset)) {
			objectIsSet[i] |= WATCH_STATE_READ;
		} 
		if (FD_ISSET(objects[i], &writeset)) {
			objectIsSet[i] |= WATCH_STATE_WRITE;
		}
		if (FD_ISSET(objects[i], &errorset)) {
			objectIsSet[i] |= WATCH_STATE_EXCEPTION;
		} 

		if (objectIsSet[i] & WATCH_STATE_ALL) {
			numObjectsSet++;

			if (s && i == 0) {
				ret = Watch::ABANDONED;
			}
		}
	}
#endif
	return ret;
}


int Watch::waitTimeout(unsigned long milliseconds)
{
	unsigned long secs = milliseconds / 1000;
	Timeval timeout(secs, (milliseconds - (secs * 1000)) * 1000);

	return wait(&timeout);
}

bool Watch::getRemainingTime(Timeval *remaining)
{
	if (!remaining || !timeoutValid)
		return false;

	*remaining = absoluteTimeout - Timeval::now();
	
	if (!remaining->isValid()) {
		timeoutValid = false;
		return false;
	}

	return true;
}

}; // namespace haggle
