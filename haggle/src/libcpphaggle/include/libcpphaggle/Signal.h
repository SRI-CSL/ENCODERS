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
#ifndef __SIGNAL_H_
#define __SIGNAL_H_

#include "Platform.h"

#include <haggleutils.h>

#include "Mutex.h"

namespace haggle {

class Watch;
class Watchable;
/**
	Signal implements a platform independent signal mechanism, which is
	watchable by the Watch class.

        A Watched signal will be "readable" whenever the Signal is
        marked as raised.
  */
class Signal {
	friend class Watch;
	friend class Watchable;
	friend bool operator==(const Signal& s, const Watchable& w);
	friend bool operator==(const Watchable& w, const Signal& s);
	friend bool operator!=(const Signal& s, const Watchable& w);
	friend bool operator!=(const Watchable& w, const Signal& s);
#if defined(OS_WINDOWS)
        HANDLE signal;
#elif defined(OS_UNIX)
        int signal[2];
#endif
	volatile bool raised;
	Mutex mutex;
public:
	Signal();
	~Signal();
#if defined(OS_WINDOWS)
	HANDLE getHandle();
#endif
        /**
           Check if the signal is raised (i.e., signalled).
           
           @returns true if the signal is raised and false if it is not.
         */
	bool isRaised() const;
        /**
           Raise the signal.  

           @returns true if the signal was successfully raised and
           false if it was not (e.g., it was already raised).
         */
	bool raise();
         /**
            Lower the signal.
         */
	void lower();
};

} // namespace haggle

#endif /* _SIGNAL_H_ */
