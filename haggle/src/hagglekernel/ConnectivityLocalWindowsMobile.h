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
#ifndef _CONNECTIVITYLOCALWINDOWSMOBILE_H_
#define _CONNECTIVITYLOCALWINDOWSMOBILE_H_

#include <libcpphaggle/Platform.h>
#include "ConnectivityLocal.h"
#include "Interface.h"

#if defined(ENABLE_BLUETOOTH)
#if defined(WIDCOMM_BLUETOOTH)
#include "WIDCOMMBluetooth.h"
#else
#include <bthapi.h>
#include <bthutil.h>
#include <bt_api.h>
#include <bt_sdp.h>
#endif
#endif /* ENABLE_BLUETOOTH */

/*
	If BLUETOOTH_STACK_FORCE_UP is defined, we will always
	try to reenable the stack when we notice it go down.
*/
//#define BLUETOOTH_STACK_FORCE_UP

/**
	Local connectivity module

	This module scans the local hardware/software to find
	bluetooth/ethernet/etc. interfaces that are accessible, and tells the
	connectivity manager about them when they are detected.
	*/
class ConnectivityLocalWindowsMobile : public ConnectivityLocal
{
	friend class ConnectivityLocal;
private:
#if defined(ENABLE_BLUETOOTH)
	// This is kind of ugly as we more or less assume there is only one local Bluetooth interface
	InterfaceRef btIface; 
#if defined (BLUETOOTH_STACK_FORCE_UP)
	bool reenableBluetoothStack;
#endif
	HANDLE hMsgQ;
	HANDLE hBTNotif;
	void handleBTEvent(BTEVENT *e);
	void findLocalBluetoothInterfaces();
#endif
#if defined(ENABLE_ETHERNET)
	// This is kind of ugly as we more or less assume there is only one local Ethernet/WiFI interface
	InterfaceRef ethIface;
	HANDLE notifyAddrChangeHandle;
        void findLocalEthernetInterfaces();
#endif
        bool run();
        void hookCleanup();
        ConnectivityLocalWindowsMobile(ConnectivityManager *m);
public:
        ~ConnectivityLocalWindowsMobile();
};

#endif
