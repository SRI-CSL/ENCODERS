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
#ifndef _CONNECTIVITYBLUETOOTH_H_
#define _CONNECTIVITYBLUETOOTH_H_

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_BLUETOOTH)
#include <haggleutils.h>

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ConnectivityBluetooth;
class ConnectivityBluetoothBase;

#include "Connectivity.h"
#include "Interface.h"

// 593C0DD8-2399-4E97-AFD8-3828A781B89D
#define HAGGLE_BLUETOOTH_SDP_UUID \
	{ \
		0x59, 0x3C, 0x0D, 0xD8,	\
		0x23, 0x99, \
		0x4E, 0x97, \
		0xAF, 0xD8, \
		0x38, 0x28, 0xA7, 0x81, 0xB8, 0x9D \
	}

// All these are in seconds:
#define DEFAULT_BASE_TIME_BETWEEN_SCANS	(120) //(80)
#define DEFAULT_RANDOM_TIME_BETWEEN_SCANS (60)

// The time to wait. (BASE_TIME_BETWEEN_SCANS +- RANDOM_TIME_AMOUNT)
#define TIME_TO_WAIT (ConnectivityBluetoothBase::baseTimeBetweenScans - \
	ConnectivityBluetoothBase::randomTimeBetweenScans +  \
		RANDOM_INT(2*ConnectivityBluetoothBase::randomTimeBetweenScans))

#define TIME_TO_WAIT_MSECS (TIME_TO_WAIT * 1000)

// Return values from classifyAddress:
#define BLUETOOTH_ADDRESS_IS_HAGGLE_NODE 0
#define BLUETOOTH_ADDRESS_IS_NOT_HAGGLE_NODE 1
#define BLUETOOTH_ADDRESS_IS_UNKNOWN 2

class ConnectivityBluetoothBase : public Connectivity {
private:
	static Mutex sdpListMutex;
	static BluetoothInterfaceRefList sdpWhiteList;
	static BluetoothInterfaceRefList sdpBlackList;
	static bool ignoreNonListedInterfaces;
protected:
	static bool readRemoteName;
public:
	static unsigned long baseTimeBetweenScans;
	static unsigned long randomTimeBetweenScans;
	/**
	FIXME: I couldn't come up with a better name for this function. It 
	should be renamed.

	This function returns one of the three return values above, depending on
	certain things:
	BLUETOOTH_ADDRESS_IS_HAGGLE_NODE:
	This is returned if the address is whitelisted.
	BLUETOOTH_ADDRESS_IS_NOT_HAGGLE_NODE:
	This is returned if the address is blacklisted, or if it is not
	whitelisted, and ignoreNonListedInterfaces is true.
	BLUETOOTH_ADDRESS_IS_UNKNOWN:
	This is returned if the address is neither whitelisted or 
	blacklisted, and ignoreNonListedInterfaces is false.
	*/
	static int classifyAddress(const BluetoothInterface &iface);
	/**
	See the other version of this function.
	*/
	static int classifyAddress(const Interface::Type_t type, const unsigned char identifier[6]);

	static void updateSDPLists(Metadata *md);
	static void clearSDPLists();

	ConnectivityBluetoothBase(ConnectivityManager *m, const InterfaceRef& iface, const string name = "Unnamed bluetooth connectivity");
	~ConnectivityBluetoothBase();
};

#define _IN_CONNECTIVITYBLUETOOTH_H

#if defined(OS_LINUX)
#include "ConnectivityBluetoothLinux.h"
#elif defined(OS_MACOSX)
#include "ConnectivityBluetoothMacOSX.h"
#elif defined(OS_WINDOWS_DESKTOP)
#include "ConnectivityBluetoothWindowsXP.h"
#elif defined(OS_WINDOWS_MOBILE)
#if defined(WIDCOMM_BLUETOOTH)
#include "ConnectivityBluetoothWindowsMobileWIDCOMM.h"
#else
#include "ConnectivityBluetoothWindowsMobile.h"
#endif
#else
#error "Bad OS - Not supported by ConnectivityBluetooth.h"
#endif

#undef _IN_CONNECTIVITYBLUETOOTH_H

#endif /* ENABLE_BLUETOOTH */

#endif
