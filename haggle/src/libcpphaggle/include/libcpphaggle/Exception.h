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
#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#if HAVE_EXCEPTION
#include <exception>
#endif

namespace haggle {

#if HAVE_EXCEPTION
/** */
class Exception : public std::exception
#else
class Exception
#endif
{
	const char *errormsg;
	const int error;
public:
	Exception(const int err = 0, const char* const msg = "Unknown Exception") : errormsg(msg), error(err) {}
	const char* getErrorMsg() {
		return errormsg;
	}
	int getError() const {
		return error;
	}
};

class MallocException : public Exception
{
public:
	MallocException(const int err = 0, const char *msg = "MallocError", ...) : Exception(err, msg) {}
};

} // namespace haggle

#endif /* _EXCEPTION_H */
