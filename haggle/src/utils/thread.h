/* Copyright 2008 Uppsala University
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

#ifndef thread_h
#define thread_h

/*
	Minimalistic cross-platform threading API.
*/

#ifdef __cplusplus
extern "C" {
#else
#define bool int
#endif

typedef struct mutex_s *mutex_t;

// Creates a new mutex
mutex_t mutex_create(void);

// Disposes of all resources associated with a mutex.
void mutex_destroy(mutex_t m);

// Locks the mutex. Returns true iff the mutex could be locked.
// (Failiure may indicate that the mutex was destroyed by another thread.)
bool mutex_lock(mutex_t m);

// Unlocks the mutex.
void mutex_unlock(mutex_t m);

/*
	Starts a new thread which will execute "thead_func" with the paramter 
	"param" and then termintate.
	
	Returns true iff the function could be started.
*/
bool thread_start(void thread_func(int), int param);

#ifdef __cplusplus
}
#else
#undef bool
#endif

#endif
