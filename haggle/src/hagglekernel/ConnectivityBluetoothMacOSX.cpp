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

#include <libcpphaggle/PlatformDetect.h>


#if defined(OS_MACOSX) && defined(ENABLE_BLUETOOTH)

#define BLUETOOTH_VERSION_USE_CURRENT 1

#include "ProtocolRFCOMM.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOBluetooth/Bluetooth.h>
#include <IOBluetooth/IOBluetoothUserLib.h>

#include "ConnectivityBluetooth.h"

#include "Interface.h"

void bluetoothDiscoverySDPCompleteCallback(void *userRefCon, IOBluetoothDeviceRef foundDevRef, IOReturn status)
{
	ConnectivityBluetooth *conn = (ConnectivityBluetooth *) userRefCon;
	IOBluetoothSDPUUIDRef uuid;
	unsigned char uuid_bytes[] = HAGGLE_BLUETOOTH_SDP_UUID;
	
	uuid = IOBluetoothSDPUUIDCreateWithBytes(uuid_bytes, sizeof(uuid_bytes));

	if (uuid == NULL) {
		CM_DBG("Failed to create UUID (%ld)\n", sizeof(uuid_bytes));
	}
	
	CFStringRef nameRef = IOBluetoothDeviceGetName(foundDevRef);

	const char *name = nameRef ? CFStringGetCStringPtr(nameRef, CFStringGetSystemEncoding()) : "PeerBluetoothInterface";
	//IOBluetoothObjectRetain(foundDevRef);
	//memcpy(macaddr, btAddr, BT_ALEN);
	
	const BluetoothDeviceAddress *btAddr = IOBluetoothDeviceGetAddress(foundDevRef);
	BluetoothAddress addr((unsigned char *)btAddr);
	
	BluetoothInterface iface((unsigned char *)btAddr, name, &addr, IFFLAG_UP);
	
	if (IOBluetoothDeviceGetServiceRecordForUUID(foundDevRef, uuid) != NULL) {
		CM_DBG("%s: Found Haggle device %s\n", conn->getName(), addr.getStr());
		
		conn->report_interface(&iface, conn->rootInterface, new ConnectivityInterfacePolicyTTL(2));
	} else {
		CM_DBG("%s: Found non-Haggle device [%s]\n", conn->getName(), addr.getStr());
	}

	IOBluetoothDeviceCloseConnection(foundDevRef);
	IOBluetoothObjectRelease(foundDevRef);
	IOBluetoothObjectRelease(uuid);
}

void bluetoothDeviceFoundCallback(void *userRefCon, IOBluetoothDeviceInquiryRef inquiryRef, IOBluetoothDeviceRef deviceRef)
{
	ConnectivityBluetooth *conn = (ConnectivityBluetooth *) userRefCon;
			   
#if defined(DEBUG)
	const BluetoothDeviceAddress *btAddr = IOBluetoothDeviceGetAddress(deviceRef);
	
	HAGGLE_DBG("%s: Found Bluetooth device [%02x:%02x:%02x:%02x:%02x:%02x], performing SDP query\n", 
		   conn->getName(), 
		   btAddr->data[0] & 0xff, 
		   btAddr->data[1] & 0xff, 
		   btAddr->data[2] & 0xff, 
		   btAddr->data[3] & 0xff, 
		   btAddr->data[4] & 0xff, 
		   btAddr->data[5] & 0xff);
#endif
	
	IOBluetoothDevicePerformSDPQuery(deviceRef, bluetoothDiscoverySDPCompleteCallback, conn);
}

void bluetoothDiscoveryCompleteCallback(void *userRefCon, IOBluetoothDeviceInquiryRef inquiryRef, IOReturn error, Boolean aborted)
{
	ConnectivityBluetooth *conn = (ConnectivityBluetooth *) userRefCon;

	if (aborted) {
		HAGGLE_DBG("%s: inquiry aborted\n", conn->getName());
	} else {
		HAGGLE_DBG("%s: inquiry completed\n", conn->getName());
	}
	
	conn->isInquiring = false;
}

int ConnectivityBluetooth::doInquiry()
{
	IOReturn error;

	CM_DBG("Doing inquiry on device %s\n", rootInterface->getIdentifierStr());

	if (!inqRef) {
		CM_DBG("Failed inquiry\n");
		return -1;
	}
	
	if (isInquiring) {
		CM_DBG("%s: already inquiring\n");
		return -1;
	}
	// TODO: Check return values for Mac OS X functions
	error = IOBluetoothDeviceInquirySetCompleteCallback(inqRef, bluetoothDiscoveryCompleteCallback);
	
	error = IOBluetoothDeviceInquirySetDeviceFoundCallback(inqRef, bluetoothDeviceFoundCallback);
	
	error = IOBluetoothDeviceInquirySetInquiryLength(inqRef, 11);

	isInquiring = true;
	
	error = IOBluetoothDeviceInquiryStart(inqRef);
	
	return 0;
}

int ConnectivityBluetooth::cancelInquiry()
{	
	if (inqRef && isInquiring) {
		CM_DBG("Cancelling inquiry\n");
		
		// This should automatically stop the runloop
		IOBluetoothDeviceInquiryStop(inqRef);
		return 1;
	}

	return 0;
}

void ConnectivityBluetooth::hookStopOrCancel()
{
	cancelInquiry();
}

void ConnectivityBluetooth::hookCleanup()
{
	CM_DBG("Cleaning up %s\n", getName());

	// Shut down service:
	if (mServerHandle) {
		IOBluetoothRemoveServiceWithRecordHandle(mServerHandle);
		mServerHandle = NULL;
	}
	if (inqRef) {
		IOBluetoothDeviceInquiryDelete(inqRef);
		inqRef = NULL;
	}
}

bool ConnectivityBluetooth::run()
{
	unsigned char uuid[] = HAGGLE_BLUETOOTH_SDP_UUID;
#define MAX_PROPERTY_KEYS 10
	CFStringRef keys[MAX_PROPERTY_KEYS];
	CFStringRef values[MAX_PROPERTY_KEYS];
	long i;
	CFDictionaryRef dict;
	IOBluetoothSDPServiceRecordRef serviceRecordRef;
	UInt16 tmp;

	isInquiring = false;
	
	inqRef = IOBluetoothDeviceInquiryCreateWithCallbackRefCon(this);

	if (!inqRef) {
		CM_DBG("Failed to create inquiry reference\n");
		return false;
	}	
	
	// Set up SDP keys:
	i = 0;
	keys[i] = CFSTR("0001 - ServiceClassIDList");
	values[i] = (CFStringRef)CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);

	CFArrayAppendValue((CFMutableArrayRef) values[i], IOBluetoothSDPUUIDCreateWithBytes(uuid, sizeof(uuid)));
	i++;
	keys[i] = CFSTR("0004 - Protocol descriptor list");
	values[i] = (CFStringRef)CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
	{
		CFMutableArrayRef tmp_array;
		
		tmp_array = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
		CFArrayAppendValue((CFMutableArrayRef) values[i], tmp_array);
		tmp = htons(0x0100); // L2CAP
		CFArrayAppendValue(tmp_array, IOBluetoothSDPUUIDCreateWithBytes(&tmp, sizeof(tmp)));
	}
	{
		CFMutableArrayRef tmp_array;

		tmp_array = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
		CFArrayAppendValue((CFMutableArrayRef) values[i], tmp_array);
		tmp = htons(0x0003);	// RFCOMM
		CFArrayAppendValue(tmp_array, IOBluetoothSDPUUIDCreateWithBytes(&tmp, sizeof(tmp)));
		tmp = RFCOMM_DEFAULT_CHANNEL;	// Channel
		CFNumberRef n = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt16Type, &tmp);
		CFArrayAppendValue(tmp_array, n);
	}
	i++;
	if (i > MAX_PROPERTY_KEYS)
		CM_DBG("CODING ERROR: overran end of array in " "ConnectivityBluetoothMacOSX.cpp\n");

	// Create dictionary:
	dict = CFDictionaryCreate(kCFAllocatorDefault, (const void **) keys, (const void **) values, i, NULL, NULL);

	// Create service
	if (IOBluetoothAddServiceDict(dict, &serviceRecordRef) == kIOReturnSuccess) {
		// Get service handle:
		IOBluetoothSDPServiceRecordGetServiceRecordHandle(serviceRecordRef, &mServerHandle);
		IOBluetoothObjectRelease(serviceRecordRef);

#if 0
		// DEBUG: output dictionary as XML:
		CFDataRef data;

		data = CFPropertyListCreateXMLData(kCFAllocatorDefault, dict);
		CM_DBG("XML: %s\n", CFDataGetBytePtr(data));
		CFRelease(data);
#endif
	} else {
		CM_DBG("Unable to add service.\n");
		mServerHandle = NULL;
	}

	CFRelease(dict);
	// Need to release these:
	for (i--; i >= 0; i--)
		CFRelease(values[i]);

	while (!shouldExit()) {

		doInquiry();
		
		cancelableSleep(TIME_TO_WAIT_MSECS);

		age_interfaces(rootInterface);
	}
	return false;
}

#endif
