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
#include <libcpphaggle/Mutex.h>
#include <libcpphaggle/Thread.h>

#include <assert.h>

namespace haggle {

bool Mutex::lock()
{
	bool ret;
#if defined(OS_WINDOWS)
	ret = WaitForSingleObject(mutex, INFINITE) == WAIT_OBJECT_0;
#else
	ret = pthread_mutex_lock(&mutex) == 0;
	assert(ret); // MOS
#endif
	return ret;
}

bool Mutex::trylock()
{
#if defined(OS_WINDOWS)
	return WaitForSingleObject(mutex, 0) == WAIT_OBJECT_0;
#else
	return pthread_mutex_trylock(&mutex) == 0;
#endif
}

bool Mutex::unlock()
{
#if defined(OS_WINDOWS)
	if (_recursive)
		ReleaseMutex(mutex);
	else
		ReleaseSemaphore(mutex, 1, NULL);
#else
	int ret = pthread_mutex_unlock(&mutex) == 0;
	assert(ret); // MOS
#endif
	return true;
}

Mutex::Mutex(bool recursive)
#if defined(OS_WINDOWS)
	: _recursive(recursive)
#endif
{
#if defined(OS_WINDOWS)
	if (recursive)
		mutex = CreateMutex(NULL, NULL, NULL);
	else
		mutex = CreateSemaphore(NULL, 1, 1, NULL);
	
	if (mutex == NULL)
		throw Exception(0, "Unable to create mutex\n");
#else
	if (recursive) {
		pthread_mutexattr_t attr;

		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&mutex, &attr);
		pthread_mutexattr_destroy(&attr);
	} else {
		pthread_mutex_init(&mutex, NULL);
	}
#endif
}

Mutex::~Mutex()
{
#if defined(OS_WINDOWS)
	CloseHandle(mutex);
#else
	pthread_mutex_destroy(&mutex);
#endif
}
	
}; // namespace haggle
