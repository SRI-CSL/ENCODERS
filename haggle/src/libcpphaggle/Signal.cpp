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
#include <libcpphaggle/Signal.h>
#include <libcpphaggle/Exception.h>

#ifdef OS_UNIX
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#endif

namespace haggle {

Signal::Signal() : raised(false), mutex("SignalMutex")
{
#ifdef OS_WINDOWS
	signal = CreateEvent(NULL, TRUE, FALSE, NULL);

#elif defined(OS_UNIX)
	if (pipe(signal) == -1) {
                fprintf(stderr, "Could not open signal pipe\n");
        }
#endif
}

Signal::~Signal()
{
#ifdef OS_WINDOWS
	CloseHandle(signal);
#elif defined(OS_UNIX)
	close(signal[0]);
	close(signal[1]);
#endif
}

	
#if defined(OS_WINDOWS)
HANDLE Signal::getHandle() 
{ 
	return signal; 
}
#endif

bool Signal::isRaised() const 
{ 
	return raised; 
}

bool Signal::raise()
{
	Mutex::AutoLocker l(mutex);
	bool was_raised;

	if (raised) {
		return false;
	}
	raised = true;

#ifdef OS_WINDOWS
	SetEvent(signal);
#elif defined(OS_UNIX)
	char c = 's';

	if (write(signal[1], &c, 1) != 1)
		raised = false;
#endif
	was_raised = raised;

	return was_raised;
}


void Signal::lower()
{
	Mutex::AutoLocker l(mutex);

	if (!raised) {
		return;
	}
	raised = false;
#ifdef OS_WINDOWS
	ResetEvent(signal);
#elif defined(OS_UNIX)
	// Read one character
	char c;
	
	if (read(signal[0], &c, 1) != 1)
		raised = true;
#endif
}

}; // namespace haggle
