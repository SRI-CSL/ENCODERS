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

#ifndef _CONNECTIVITYMEDIA_H_
#define _CONNECTIVITYMEDIA_H_

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_MEDIA)
/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

#include "Connectivity.h"
#include "Interface.h"

#define HAGGLE_FOLDER_PREFIX ".haggle"

/*
 Forward declarations of all data types declared in this file. This is to
 avoid circular dependencies. If/when a data type is added to this file,
 remember to add it here.
 */
class ConnectivityMedia;

#define _IN_CONNECTIVITYMEDIA_H

#if defined(OS_MACOSX)
#include "ConnectivityMediaMacOSX.h"
//#elif defined(OS_LINUX)
//#include "ConnectivityMediaLinux.h"
//#elif defined(OS_WINDOWS)
//#include "ConnectivityMediaWindows.h"
#else
#error "Bad OS - Not supported by ConnectivityMedia.h"
#endif

#undef _IN_CONNECTIVITYMEDIA_H

#endif /* ENABLE_MEDIA */
#endif /* _CONNECTIVITYMEDIA_H_ */
