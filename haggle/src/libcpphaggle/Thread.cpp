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
#include <string.h>
#include <stdio.h>

#include <libcpphaggle/Exception.h>
#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Watch.h>

#if defined(OS_UNIX)
#include <errno.h>
#include <sys/time.h>
#elif defined(OS_WINDOWS)
#define snprintf _snprintf
#endif

// For TRACE macro
#include <haggleutils.h>
#include <prng.h>

namespace haggle {

// Static variables
unsigned long Thread::totNum = 0;
Thread::thread_registry_t Thread::registry;
Thread Thread::mainthread;
Mutex Thread::registryMutex("Thread registry mutex");

int cancel_other(Thread *thr);


#if HAVE_EXCEPTION	
static void unexpected_exception_handler()
{
	if (!Thread::selfIsRegistered()) {
		TRACE_ERR("Uncaught exception in unknown thread\n");
		return;
	}
	
	TRACE_ERR("Uncaught exception in thread %s!\n", Thread::selfGetName());
	TRACE_ERR("Please do exception handling...\n");
#if defined(DEBUG) && 1
	// This is an intentional crash, to figure out where the exception happened:
	char *asdf = NULL;
	asdf[0] = 'A';
#endif
}
#endif /* HAVE_EXCEPTION */

void ThreadId::setToCurrentThread()
{
#if defined(OS_WINDOWS)
	id = GetCurrentThreadId();
#elif defined(HAVE_PTHREADS)
	id = pthread_self();
#endif
}

bool operator==(const ThreadId& tid1, const ThreadId& tid2)
{
#if defined(OS_WINDOWS) 
        return (tid1.id == tid2.id);
#elif defined(HAVE_PTHREADS)
        return pthread_equal(tid1.id, tid2.id) != 0;
#endif
}

bool operator<(const ThreadId& tid1, const ThreadId& tid2)
{
#if defined(OS_WINDOWS) || defined(OS_LINUX)
	return (tid1.id < tid2.id);
#elif defined(OS_MACOSX)
        
	if (tid1.id == NULL && tid2.id == NULL)
		return false;
	if (tid1.id == NULL)
		return true;
	if (tid2.id == NULL)
		return false;
	// This is kind of ugly
        return (uintptr_t)tid1.id < (uintptr_t)tid2.id;
#endif	
}

bool ThreadId::operator()(const ThreadId& tid1, const ThreadId& tid2)
{
#if defined(OS_WINDOWS) || defined(OS_LINUX)
	return (tid1.id < tid2.id);
#elif defined(OS_MACOSX)
	if (tid1.id == NULL && tid2.id == NULL)
		return false;
	if (tid1.id == NULL)
		return true;
	if (tid2.id == NULL)
		return false;
	// This is kind of ugly
	return (uintptr_t)tid1.id < (uintptr_t)tid2.id;
#endif	
}

bool Thread::registryAdd(Thread *thr)
{
	Mutex::AutoLocker l(registryMutex);
	
	if (!thr)
		return false;

	if (!registry.insert(make_pair(thr->id, thr)).second) {
		TRACE_DBG("Thread %s already in registry\n", thr->getName());
		return false;
	}
        return true;
}

Thread *Thread::registryRemove(const ThreadId& id)
{
	Mutex::AutoLocker l(registryMutex);
	Thread *thr = NULL;

	thread_registry_t::iterator it = registry.find(id);

	if (it != registry.end()) {
		thr = (*it).second;
		registry.erase(it);
	}
	
	return thr;
}

#ifdef DEBUG

void Thread::registryPrint()
{
	Mutex::AutoLocker l(registryMutex);
	
	printf("==== Thread registry ====\n");
	
	for (thread_registry_t::iterator it = registry.begin(); it != registry.end(); it++) {
		Thread *thr = (*it).second;
		printf("Thread [%lu] - %s sysid=%lu Runtime: %s\n", thr->getNum(), thr->getName(),(unsigned long)thr->id.id, (Timeval::now() - thr->starttime).getAsString().c_str());
	}
}
#endif


start_ret_t start_thread(void *arg)
{
#ifdef OS_WINDOWS
	prng_init();
#endif
	Runnable *runObj = static_cast < Runnable * >(arg);
	Thread *thr = runObj->thr;
#if HAVE_EXCEPTION	
        std::set_terminate(unexpected_exception_handler);
#endif
	thr->runloop();

	return 0;
}

void Thread::runloop()
{
	id.setToCurrentThread();

	TRACE_DBG("Starting thread %s\n", name);
	
	if (!Thread::registryAdd(this)) {
		TRACE_ERR("Could not add thread %s to registry\n", getName());
		return;
	}

	while (runObj->run()) {
		if (exitSignal.isRaised())
			break;
	}

	exitSignal.lower();

	cleanup();

	Thread::registryRemove(id);
}

void Thread::cleanup()
{
	mutex.lock();
	state = THREAD_STATE_CLEANUP;
	mutex.unlock();
	// Call the cleanup function that the run object has registered
	runObj->cleanup();
	mutex.lock();
	state = THREAD_STATE_STOPPED;
	mutex.unlock();

	TRACE_DBG("Thread %s cleanup done, stopped running...\n", getName());
}

	
Thread *Thread::_selfGet()
{ 
	return _selfGetFromId(selfGetId());
}

/*
   "_selfGetFromId" is an internal function which is not protected by
   a mutex, which means that protection has to be provided by the
   calling function.
 */
Thread *Thread::_selfGetFromId(const ThreadId& id)
{	
	thread_registry_t::iterator it;
	
	it = registry.find(id);

	return it == registry.end() ? NULL : (*it).second;
}

bool Thread::selfGetNum(unsigned long *num)
{
	Mutex::AutoLocker l(registryMutex);
	
	if (!num)
		return false;
	
	Thread *thr = _selfGet();
	
	if (thr) {
		*num = thr->getNum();
		return true;
	}
	
	return false;
}

const char *Thread::selfGetName()
{
	Mutex::AutoLocker l(registryMutex);

	Thread *thr = _selfGet();
	
	if (thr)
		return thr->getName();	
	
	return NULL;
}

bool Thread::selfIsRegistered()
{
	Mutex::AutoLocker l(registryMutex);
	
	if (_selfGet())
		return true;
	
	return false;
}

Signal *Thread::selfGetExitSignal()
{
	Mutex::AutoLocker l(registryMutex);
	
	Thread *thr = _selfGet();
	
	if (thr) 
		return &thr->exitSignal;
	
	return NULL;
}

// Constructor for static Thread object (main thread) in thread class
Thread::Thread() : 
	num(totNum++), starttime(Timeval::now()), name(NULL), runObj(NULL), 
	state(THREAD_STATE_STOPPED), detached(false)
{
	id.setToCurrentThread();
	size_t len = strlen(MAIN_THREAD_NAME) + 1;
	name = (char *)malloc(len);

	if (name) {
		memset(name, '\0', len);
		strcpy(name, MAIN_THREAD_NAME);
	} else {
		name = NULL;
		len = 0;
	}

	registry.insert(make_pair(id, this));
}

Thread::Thread(Runnable *r) : 
	num(totNum++), starttime(Timeval::now()), name(NULL), runObj(r), 
	state(THREAD_STATE_STOPPED), detached(false)
{	
	if (!runObj)
		return;

	if (runObj->isRunning()) {
		// Runobject is already associated with a thread and is running
		runObj = NULL;
		return;
	} 

	runObj->thr = this;

#if defined(HAVE_PTHREADS)
	// Defaults to joinable
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif

	// Allocate name string, add room for thread number and null-char
	size_t len = strlen(runObj->getName()) + 20 + 1;

	if (len > 0) {
		name = (char *)malloc(len);
		
		if (name) {
			memset(name, '\0', len);
			snprintf(name, len, "%s:%lu", runObj->getName(), num);
		} else {
			name = NULL;
			len = 0;
		}
	}
}

Thread::~Thread()
{
	if (strcmp(name, MAIN_THREAD_NAME) == 0) {
		/* 
		 For some reason, some Android versions do not like that we remove the
		 main thread object from the registry here. The application hangs at that
		 point. It is, however, probably safe to not remove the object here,
		 since reaching this point means that Haggle is exiting and the registry
		 will be destroyed anyway.
		 */
// MOS - this causes pthread lock failure (errno = 25)
// #if !defined(OS_ANDROID)
//		registryRemove(id);
// #endif
	} else {
		/*
		If the thread is running, we must stop it (cancel, then
		join) before we delete the thread object.
		*/
		stop();

#ifdef OS_WINDOWS
		if (thrHandle)
			CloseHandle(thrHandle);
#else
		pthread_attr_destroy(&attr);
#endif
		if (runObj)
			runObj->thr = NULL;
	}
	if (name)
		free(name);
}

const char *Thread::getName() const
{
	return name;
}

thread_handle_t Thread::selfGetHandle()
{
#if defined(OS_WINDOWS)
	return GetCurrentThread();
#else
	return pthread_self();
#endif
}

ThreadId Thread::selfGetId()
{
#if defined(OS_WINDOWS)
	return ThreadId(GetCurrentThreadId());
#else
	// FIXME: make sure this casting is safe and returns something useful
	return ThreadId(pthread_self());
#endif
}

bool Thread::isSelf() const
{
	return (selfGetId() == id);
}
	
thread_handle_t Thread::getHandle() const 
{
	return thrHandle;
}

unsigned long Thread::getNum() const 
{
	return num;
}
	
bool Thread::equal(const Thread &thr1, const Thread &thr2)
{
	return (thr1.id == thr2.id);
}

bool Thread::isRunning() const
{
	Mutex::AutoLocker l(const_cast<Thread *>(this)->mutex);
	return (state == THREAD_STATE_RUNNING);
}
	
bool Thread::isDetached() const 
{
	return detached;
}

bool Thread::isJoined() const 
{
	Mutex::AutoLocker l(const_cast<Thread *>(this)->mutex);
	return (state != THREAD_STATE_JOINED);
}

bool Thread::isCancelled() const
{
	return exitSignal.isRaised();
}

Runnable *Thread::getRunnable() 
{
	return runObj;
}
	
int Thread::start()
{
	Mutex::AutoLocker l(mutex);
	int ret = 0;

	if (state != THREAD_STATE_STOPPED) {
		return -1;
	}

	if (!runObj)
		return -1;

	state = THREAD_STATE_RUNNING;

#ifdef OS_WINDOWS
	thrHandle = CreateThread(NULL, 0, start_thread, static_cast < void *>(runObj), 0, &this->id.id);

	if (thrHandle == NULL) {
		state = THREAD_STATE_STOPPED;
		return -1;
	}
#else
	ret = pthread_create(&thrHandle, &attr, start_thread, static_cast < void *>(runObj));

	if (ret != 0) {
		state = THREAD_STATE_STOPPED;
		return -1;
	}
	
#endif
	TRACE_DBG("Thread %d started for Runnable \"%s\"\n", num, runObj->getName());

	return 0;
}

void Thread::cancel()
{
	Mutex::AutoLocker l(mutex);

	if (state != THREAD_STATE_RUNNING) {
		TRACE_DBG("Trying to cancel thread %d which is not running (Runnable \"%s\")\n", 
		   num, runObj->getName());
		return;
	}
	
	state = THREAD_STATE_CANCELLED;

	if (isSelf()) {
		TRACE_DBG("Thread %d is cancelling self\n", num);
	}

	exitSignal.raise();	

        // Call the runnable's cancel hook
        runObj->hookCancel();
}


/*
	Return values for join function.
*/

enum {
	THREAD_JOIN_NOT_JOINABLE = -4,
	THREAD_JOIN_DEADLOCK = -2,
	THREAD_JOIN_FAILED = -1,
	THREAD_JOIN_OK = 0,
	THREAD_JOIN_NO_THREAD = 1,
};

int Thread::join()
{
	int ret = THREAD_JOIN_FAILED;
#ifdef OS_WINDOWS
	DWORD result;

	result =  WaitForSingleObject(thrHandle, INFINITE);
	
	if (result == WAIT_OBJECT_0) {
		TRACE_DBG("Thread %d joined...\n", num);		
		ret = THREAD_JOIN_OK;
		state = THREAD_STATE_JOINED;
	} else {
		switch (GetLastError()) {
		case ERROR_INVALID_THREAD_ID:
			TRACE_DBG("No thread to join with given ID\n");
			ret = THREAD_JOIN_NO_THREAD;
			break;
		case ERROR_THREAD_1_INACTIVE:
			TRACE_ERR("Thread is inactive");
			ret = THREAD_JOIN_NO_THREAD;
			break;		
		default:
			TRACE_ERR("Unhandled thread join error!\n");
			ret = THREAD_JOIN_FAILED;
			break;
		}
		TRACE_ERR("Thread %d failed to join : %s\n", num, STRERROR(ERRNO));
	}
#elif defined(HAVE_PTHREADS)
        ret = pthread_join(thrHandle, NULL);

	switch (ret) {
	case 0:
		TRACE_DBG("Thread %d joined...\n", num);
		ret = THREAD_JOIN_OK;
		state = THREAD_STATE_JOINED;
		break;
	case ESRCH:
		TRACE_DBG("No thread to join with given ID\n");
		ret = THREAD_JOIN_NO_THREAD;
		break;
	case EINVAL:
		TRACE_ERR("Thread is not joinable\n");
		ret = THREAD_JOIN_NOT_JOINABLE;
		break;
	case EDEADLK:
		TRACE_ERR("Thread deadlocked or join called by the thread itself!\n");
		ret = THREAD_JOIN_DEADLOCK;
		break;
	default:
		TRACE_ERR("Unhandled thread join error : %s\n", strerror(errno));
		ret = THREAD_JOIN_FAILED;
		break;
	}
	
#endif /* OS_WINDOWS */
	return ret;
}

int Thread::detach()
{
	int res = 0;

	if (!isRunning())
		return -1;

	detached = true;
	
#if defined(HAS_PTHREADS)
	res = pthread_detach(thrHandle);

	if (res != 0) {

		switch (res) {
		case EINVAL:
			fprintf(stderr, "pthread_detach: not a joinable thread\n");
			break;
		case ESRCH:
			fprintf(stderr, "pthread_detach: no thread for given ID\n");
			break;
		default:
			fprintf(stderr, "pthread_detach: unknown failure\n");
		}
	}
#endif
	return res;
}
		

int Thread::stop()
{
	int ret = 0;

	if (isRunning()) {

		cancel();
		
		// Try to wait for the thread to finish if it is in a joinable
		// state
		ret = join();
	}

	return ret;
}

void Thread::cancelableSleep(unsigned long msecs)
{	
	Watch w;
	
	mutex.lock();

	if (state != THREAD_STATE_RUNNING) {
		mutex.unlock();
		return;
	}
	mutex.unlock();

	w.waitTimeout(msecs);
}

void Runnable::hookCancel() 
{
}
	
const char *Runnable::getName() const
{ 
	return name.c_str(); 
}

void Runnable::cancelableSleep(unsigned long msecs) 
{
	if (thr) 
		thr->cancelableSleep(msecs);
}

bool Runnable::shouldExit() const 
{ 
	return (thr ? thr->isCancelled() : true); 
}	

bool Runnable::start(void)
{
	if (!thr) {
		new Thread(this);
		return thr->start() == 0;
	}
	return false;
}
	
void Runnable::stop(void) 
{ 
	if (thr) { 
		thr->stop(); 
	} 
}
void Runnable::cancel(void) 
{ 
	if (thr) { 
		thr->cancel(); 
	} 
}
	
bool Runnable::join(void) 
{ 
	if (!thr)
		return false;

	return thr->join() == THREAD_JOIN_OK; 
}

bool Runnable::detach(void) 
{ 
	if (!thr) 
		return false;

	return thr->detach() == 0; 
}
	
bool Runnable::isRunning() const
{ 
	if (thr) 
		return thr->isRunning(); 
	return false; 
}

bool Runnable::isCancelled() const
{ 
	if (thr) 
		return thr->isCancelled(); 
	return false; 
}
		
bool Runnable::isDetached() const
{
	if (thr)
		return thr->isDetached();
	return false;
}

bool Runnable::isJoined() const
{
	if (thr)
		return thr->isJoined();
	return false;
}
	
Runnable::Runnable(const string _name) : 
	thr(NULL), name(_name), mutex() 
{
	//TRACE_DBG("Creating runnable \'%s\'\n", name.c_str());	
}

Runnable::~Runnable()
{
	if (!thr)
		return;
	
	if (thr->isRunning()) {
		TRACE_DBG("Runnable %s : cancelling its running thread with id=%d\n", 
			  getName(), thr->getNum());
		thr->cancel();	
		
		if (!thr->isDetached() && 
		    !thr->isSelf()) {
			int ret = thr->join();
			
			if (ret != THREAD_JOIN_OK) {
				if (ret == THREAD_JOIN_NO_THREAD) {
					// This should be fine...
					TRACE_DBG("Thread %d is no longer executing...\n", 
						  thr->getNum());
				} else {
					// Some error that indicates something
					// bad... throw an exception to point
					// this out.  This should never
					// happen... if it does, we need to
					// fix the problem or handle it
					// properly.
					TRACE_ERR("Thread join failed for runnable %s!, error=%d\n", 
						  name.c_str(), ret);
				}
			}
		}
	}
	
	TRACE_DBG("Deleting runnable \'%s\' whose thread has id=%d\n", 
		  getName(), thr->getNum());
	
	delete thr;
	thr = NULL;
}

}; // namespace haggle
