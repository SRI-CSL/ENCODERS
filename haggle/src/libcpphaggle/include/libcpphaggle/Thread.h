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
#ifndef _THREAD_H
#define _THREAD_H

#include "Platform.h"

#if defined(OS_WINDOWS)
#include <windows.h>
#elif defined(OS_LINUX) || defined(OS_MACOSX)
#ifndef HAVE_PTHREADS
#define HAVE_PTHREADS 1
#endif
#include <pthread.h>
#else
#error "Unknown operating system!"
#endif

#include <stdlib.h>

#include "Map.h"
#include "Exception.h"
#include "Timeval.h"
#include "Signal.h"
#include "Mutex.h"
#include "Condition.h"
#include "String.h"

namespace haggle {

/*
#ifdef DEBUG
#define DEBUG_MUTEX
#endif
*/

/*
 Platform specific defines and typedefs.
 */

#ifdef OS_WINDOWS
typedef HANDLE thread_handle_t;
typedef DWORD thread_attr_t;
typedef DWORD thread_id_t;
#define start_ret_t DWORD WINAPI
#elif defined(HAVE_PTHREADS)

typedef pthread_t thread_handle_t;
typedef pthread_attr_t thread_attr_t;
typedef pthread_t thread_id_t;
#define start_ret_t void*
#endif

class ThreadId {
public:
	thread_id_t id;
	ThreadId(const thread_id_t _id = 0) : id(_id) {}
	ThreadId(const ThreadId& tid) : id(tid.id) {}
	void setToCurrentThread();
	bool operator() (const ThreadId& tid1, const ThreadId& tid2);
	friend bool operator== (const ThreadId& tid1, const ThreadId& tid2);
        friend bool operator<(const ThreadId& tid1, const ThreadId& tid2);
};

/*
	Threading - how to implement, start and stop a thread

	People new to Haggle: Read this whole text through BEFORE implementing a
	thread, to avoid confusion about how threads work.


	A runnable object:

	When implementing something which needs a thread, a class that inherits
	from a runnable (directly or indirectly) is needed. The runnable class
	would be declared something like:

		class MyRunnable : public Runnable {
			void run();
			void cleanup();
		};

	The Runnable class provides a mutex that any child class can use, and a
	Condition, which will be discussed later.

	The class has to implement both run() and cleanup(), even if they are
	empty.


	A thread:

	A thread is created like so:

		myThread = new Thread(myRunnable);

	where myRunnable is an object of type MyRunnable, of course.


	Starting a thread:

	Creating a thread doesn't make the thread run. To start the thread you have
	to do the following:

		myThread->start();

	After which the thread starts. The thread executes the function run() in
	the runnable given when creating the thread, and then cleanup() (same
	object), after which it terminates.


	Terminating a thread:

	A thread can terminate when its job is done, by simply exiting both the
	run() and cleanup() methods. A thread can also be told to terminate by
	another thread (or itself). Doing this requires that myRunnable->run() is
	declared such that it periodically call:

		cond->wait(mutex)

	or

		cond->timedWait(mutex, <time to wait>)

	When the return value is <What should it be??> the run() function should
	terminate.

	In order to signal a thread to terminate via the Condition, as
	explained above, you can call either:

		myThread->cancel();

	or
		myThread->stop();

	Calling cancel() tells the thread to stop, and returns immediately. The
	thread may still be executing, however, so you cannot delete the runnable
	until you are certain the thread has stopped executing, or Haggle
	will most likely crash!

	Calling stop() tells the thread to stop, and waits for it to do so before
	returning. After stop() returns, you can safely delete the runnable. If the
	operation that the runnable is doing may take some time to reach a check of
	the Condition, the main haggle thread should not be waiting on it.

*/



extern "C" start_ret_t start_thread(void *arg);

/*

	Thread running states.
*/

#define THREAD_STATE_STOPPED      0
#define THREAD_STATE_RUNNING      1
#define THREAD_STATE_CANCELLED    2
#define THREAD_STATE_CLEANUP      3
#define THREAD_STATE_JOINED       4

/* Thread cancel modes */
#define THREAD_CANCEL_TYPE_DEFERRED 0
#define THREAD_CANCEL_TYPE_ASYNCH   1

class Runnable;

/**
	Thread class.

	Used by runnables (see the Runnable class) to create an actual thread for
	execution.
*/

#define MAIN_THREAD_NAME "MainThread:0"

// Actual thread class that takes an object with the Runnable interface
class Thread 
{
	// Friends
        friend class Runnable;
        friend class Condition;
	friend class Mutex;
	friend class RecursiveMutex;
private:
        Thread();
        Thread(Runnable *r);
        ~Thread();
	// Static members
        static unsigned long totNum;
	typedef Map<ThreadId, Thread *> thread_registry_t;
	static thread_registry_t registry;
        static Mutex registryMutex;
        static bool registryAdd(Thread *thr);
        static Thread *registryRemove(const ThreadId& id);
	static Thread mainthread; // This is a thread object that represent the "main thread" in the registry
	// Non static members
        unsigned long num;
	Timeval starttime;
	char *name;

	thread_handle_t thrHandle;
        thread_attr_t attr;
        ThreadId id;
        Runnable *runObj;

        u_int8_t state;
	bool detached;
	
        Mutex mutex; 
	Signal exitSignal;

        // Start and cleanup routines
        friend start_ret_t start_thread(void *arg);
	void runloop();
	void cleanup();
	
	/**
		Test whether a cancel request has been made. If one has been made,
		then the thread will exit. Otherwise, nothing will happen. Calling
		this function in a threads run-loop will effectively create a 
		cancelation point wherever the function is called.
	 
		See the same named function in the Runnable class for additional explanation.
	 
	 */
        void testcancel();
	
	/**
		A sleep function that is guaranteed to be cancelable. It is like calling
		testcancel, although the function call will not return until the specified
		period of time (as given in milli seconds).
	 
		See the same named function in the Runnable class for additional explanation.
	 
	 */
        void cancelableSleep(unsigned long msecs);
	/*
		The following functions operate on the registry without
		locking it.
	 */
	static Thread *_selfGet();
        static Thread *_selfGetFromId(const ThreadId& id);
public:
	/*
		Static "self" functions that access the current context's thread.
	*/
	static thread_handle_t selfGetHandle();
        static ThreadId selfGetId();
	static bool selfGetNum(unsigned long *num);
	static bool selfIsRegistered();

	/**
		Return the name of the "current" thread if it is in the registry, otherwise
		return NULL;
	*/
	static const char *selfGetName();
	static Signal *selfGetExitSignal();

#ifdef DEBUG
	static void registryPrint();
#endif
        bool isSelf() const;
        thread_handle_t getHandle() const;
        unsigned long getNum() const;
	const char *getName() const;
	/**
	   This function checks whether the thread is running or not.

	   @returns a bool indicating running state.
	 */
        bool isRunning() const;
	bool isDetached() const;
	bool isJoined() const;

	/**
	   This function checks whether the thread is cancelled, which
	   means it has exited or is about to.

	   @returns a bool indicating cancelled state.
	 */
	bool isCancelled() const;
        Runnable *getRunnable();
	
	/**
		Start a thread. This will create a new thread that calls the associated
		runnable's run() function. The thread will run until the run() function
		exits, or a cancel() or stop() is called on the thread.
	 
	 */
        int start();

	/**
		Cancel a running thread. This will cause the thread to exit, whenever
		it reaches a cancelation point. These points may be platform specific.
		Windows, for example, have no build in cancelation support in threads.
		Therefore, we have tried to make the most common blocking functions
		cancelable in Windows, by using a special event (cancel_event).
	 */
        void cancel();
	/**
		Join this thread with the thread from which the call to join() is made.
		The function will block until the thread has exited.
	 */
        int join();
	
	/**
		Stop a thread. This is the same as calling first cancel() and then join().
	 */
        int stop();

	/**
	        Detach a thread. Join not needed.
	 */
	int detach();
	/**
		Returns true if two thread references refer to the same thread, otherwise false.
	 */
        static bool equal(const Thread &thr1, const Thread &thr2);
};

/**
	The runnable class is a parent class to the classes that wish to start
	executing in a thread of its own. This class contains some things that are
	good to have, or to know that something running in a different thread has,
	even if it is not neccesary to use it.
*/
class Runnable
{
        friend class Thread;
        friend start_ret_t start_thread(void *arg);
        /** 
	    The thread running the run() function, if one exists. 
	*/
        Thread *thr;
protected:
	/* The human readable name of this Runnable */
	const string name;
        /** 
		A condition variable that can be used to signal the thread to stop. 
	 */
        Condition cond;
        /** 
		A mutex. Good for making sure two threads don't use the same variable
        	at the same time. 
	 */
        Mutex mutex;
        
        /** 
		This function implements the main loop of the
		runnable's thread and has to be implemented by the
		derived class.

        	@returns a bool indicating whether the run() function
		should be called again or not. If it returns true, the
		function is called by the thread again, if it returns
		false, the thread will exit and the cleanup function
		will be called.
        */
        virtual bool run() = 0;
        /**
        	This function is for doing cleanup when the run() function exits or 
		the thread was cancelled.
        	
        	Must be overridden by child class that expects to run a thread, as it
        	will invariably be called when the thread exits.
        */
        virtual void cleanup() = 0;
	
	/**
		This is a sleep function that allows the runnable to sleep
		for a specified number of milli seconds. The function is
		guaranteed to be cancelable, i.e., if another thread calls
		cancel() on this thread, it will cause this thread to exit
		and do cleanup. Some other functions that block (i.e., platform
		specific sleep) may not be cancelable, and the thread will then
		ignore the cancel request. It is highly recommended that this
		function is used whenever the runnable wishes to sleep.

		@param msces the number of milli seconds to sleep
	 */
        void cancelableSleep(unsigned long msecs);
	/*
	  A Derived class can override hookCancel if they want to
	  do something just before the thread is cancelled.
	 */
	virtual void hookCancel();
public:
	bool shouldExit() const;
	bool start(void);
	void stop(void);
	void cancel(void);
	bool join(void);
	bool detach(void);
	bool isRunning() const;
	bool isCancelled() const; // MOS
	bool isDetached() const;
	bool isJoined() const;
	/**
		Get the name of the Runnable. May be overridden by derived classes.
	 */
	virtual const char *getName() const;
        /** Constructor */
        Runnable(const string _name = "Runnable");

        /** Destructor */
        virtual ~Runnable();
};

} // namespace haggle
 
#endif /* _THREAD_H */
