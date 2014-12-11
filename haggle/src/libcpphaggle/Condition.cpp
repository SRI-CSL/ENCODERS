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
#include <libcpphaggle/Condition.h>

namespace haggle {

bool Condition::timedWait(Mutex *tmutex, const struct timeval *time_to_wait)
{
	bool conditionSet = false;

#if defined(OS_WINDOWS)
	DWORD ret;
	DWORD num_objects = 0;
	DWORD millisec;
	HANDLE waitobjects[1];
	
	if (!tmutex) {
		return false;
	}

	waitobjects[num_objects++] = cond;

	if (time_to_wait == NULL)
		millisec = INFINITE;
	else
		millisec = 1000 * time_to_wait->tv_sec + time_to_wait->tv_usec / 1000;

	if (millisec < 0)
		return false;

	/* Make sure the condition is not set when we start waiting */
	ResetEvent(cond);

	tmutex->unlock();

	ret = WaitForMultipleObjects(num_objects, waitobjects, FALSE, millisec);

	if (ret >= WAIT_ABANDONED_0 && ret < (WAIT_ABANDONED_0 + num_objects)) {
		//fprintf(stderr, "Condition wait was abandoned by object %d\n", (ret - WAIT_ABANDONED_0));
	} else if (ret >= WAIT_OBJECT_0 && ret < (WAIT_OBJECT_0 + num_objects)) {
		//printf("Condition wait was set for object %d\n", (ret - WAIT_OBJECT_0));
		conditionSet = true;
	} else {
		// FAILURE!
	}
	tmutex->lock();
#elif defined(HAS_PTHREADS)
	struct timeval now;
	struct timespec then;

	if (time_to_wait) {
		gettimeofday(&now, NULL);
		then.tv_nsec = (now.tv_usec + time_to_wait->tv_usec) * 1000;
		then.tv_sec = now.tv_sec + time_to_wait->tv_sec;

		if (then.tv_nsec > 1000000000) {
			then.tv_nsec -= 1000000000;
			then.tv_sec++;
		}
		if (pthread_cond_timedwait(&cond, &tmutex->mutex, &then) == 0)
			conditionSet = true;
	} else {
		if (pthread_cond_wait(&cond, &tmutex->mutex) == 0)
			conditionSet = true;
	}
#endif
	return conditionSet;

}

bool Condition::timedWaitSeconds(Mutex *tmutex, long seconds)
{
	struct timeval tv;

	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	return timedWait(tmutex, &tv);
}

bool Condition::wait(Mutex *tmutex)
{
	return timedWait(tmutex, NULL);
}

void Condition::broadcast()
{
#if defined(OS_WINDOWS_VISTA_DISABLED)
	WakeAllConditionVariable(&cond);
#elif defined(OS_WINDOWS)
	SetEvent(cond);
#elif defined(OS_UNIX)
	pthread_cond_broadcast(&cond);
#endif
}

void Condition::signal()
{
#if defined(OS_WINDOWS_VISTA_DISABLED)
	WakeConditionVariable(&cond);
#elif defined(OS_WINDOWS)
	PulseEvent(cond);
#elif defined(OS_UNIX)
	pthread_cond_signal(&cond);
#endif
}

Condition::Condition()
{
#if defined(OS_WINDOWS_VISTA_DISABLED)
	// FIXME: Error checking might be required:
	InitializeConditionVariable(&cond);
#elif defined(OS_WINDOWS)
	cond = CreateEvent(NULL, FALSE, FALSE, NULL);

#elif defined(OS_UNIX)
	pthread_cond_init(&cond, NULL);
#endif
}

Condition::~Condition()
{
#if defined(OS_WINDOWS_VISTA_DISABLED)
	DeleteConditionVariable(&cond);
#elif defined(OS_WINDOWS)
	CloseHandle(cond);
#elif defined(OS_UNIX)
	pthread_cond_destroy(&cond);
#endif
}

}; // namespace haggle
