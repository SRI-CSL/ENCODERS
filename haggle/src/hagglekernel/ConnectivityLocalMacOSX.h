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
#ifndef _CONNECTIVITYLOCALMACOSX_H_
#define _CONNECTIVITYLOCALMACOSX_H_

#if defined(ENABLE_MEDIA)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFString.h>
#include <CoreServices/CoreServices.h>
#endif

#include <libcpphaggle/Platform.h>
#include "ConnectivityLocal.h"
#include "Interface.h"

/**
	Local connectivity module

	This module scans the local hardware/software to find
	bluetooth/ethernet/etc. interfaces that are accessible, and tells the
	connectivity manager about them when they are detected.
*/
class ConnectivityLocalMacOSX : public ConnectivityLocal
{
	friend class ConnectivityLocal;
private:
#if defined(ENABLE_BLUETOOTH)
        void findLocalBluetoothInterfaces();
#endif
#if defined(ENABLE_ETHERNET)
        long ethernet_interfaces_found;
#endif
#if defined(ENABLE_MEDIA)
	friend void onFSEvents(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, 
			       void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);
	
	FSEventStreamRef fsEventStream;
	CFRunLoopRef runLoop;
	void onMountVolume(char *path);
	void onUnmountVolume(char *path);
#endif

        // Called when add_interface _actually_ adds an interface
        void interface_added(const InterfaceRef &node);

        bool run();
	void hookStopOrCancel();
        void hookCleanup();
        ConnectivityLocalMacOSX(ConnectivityManager *m);
public:
        ~ConnectivityLocalMacOSX();
};

#endif
