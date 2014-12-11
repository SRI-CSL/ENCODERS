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
#ifndef _CONNECTIVITYBLUETOOTHMACOSX_H_
#define _CONNECTIVITYBLUETOOTHMACOSX_H_

#ifndef  _IN_CONNECTIVITYBLUETOOTH_H
#error "Do not include this file directly, include ConnectivityBluetooth.h"
#endif

#include <libcpphaggle/PlatformDetect.h>

#if defined(ENABLE_BLUETOOTH)

#include <CoreFoundation/CoreFoundation.h>
#include <IOBluetooth/Bluetooth.h>
#include <IOBluetooth/IOBluetoothUserLib.h>
/**
	Bluetooth connectivity module.

	Scans a bluetooth interface to find nearby haggle nodes. Reports any found
	to the connectivity manager.
*/
class ConnectivityBluetooth : public ConnectivityBluetoothBase
{
        BluetoothSDPServiceRecordHandle	mServerHandle;
	IOBluetoothDeviceInquiryRef inqRef;
	bool isInquiring;

        bool run();
        void hookCleanup();
	void hookStopOrCancel();
	int doInquiry();
	int cancelInquiry();
	friend void bluetoothDeviceFoundCallback(void *userRefCon, 
						 IOBluetoothDeviceInquiryRef inquiryRef, 
						 IOBluetoothDeviceRef deviceRef);
        friend void bluetoothDiscoverySDPCompleteCallback(void *userRefCon,
							  IOBluetoothDeviceRef foundDevRef,
							  IOReturn status);
	friend void bluetoothDiscoveryCompleteCallback(void *userRefCon, 
						       IOBluetoothDeviceInquiryRef inquiryRef, 
						       IOReturn error, Boolean aborted);
	friend bool bluetoothDiscovery(const BluetoothInterfaceRef iface, ConnectivityBluetooth *conn);
public:
        ConnectivityBluetooth(ConnectivityManager *m, const InterfaceRef& _iface);
        ~ConnectivityBluetooth();
};

#endif /* ENABLE_BLUETOOTH */

#endif
