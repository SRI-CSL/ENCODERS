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

#if defined(OS_MACOSX)

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_BLUETOOTH)
#include <CoreFoundation/CoreFoundation.h>
#include <IOBluetooth/Bluetooth.h>
#include <IOBluetooth/IOBluetoothUserLib.h>
#endif

#if defined(ENABLE_ETHERNET)
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/types.h>
#if !defined(OS_MACOSX_IPHONE)
#include <net/route.h>
#include <sys/kern_event.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#endif
#endif

#if defined(ENABLE_MEDIA)
#define HAGGLE_VOLUME_PATH "/Volumes"
#endif

#include "ConnectivityLocalMacOSX.h"
#include "ConnectivityBluetooth.h"
#include "ConnectivityEthernet.h"
#include "ConnectivityManager.h"
#include "Interface.h"
#include "Utility.h"

#define ENABLE_KERNEL_SOCKET_CODE	0


static const char *blacklist_device_names[] = { "vmnet", "fw", "lo", "gif", "stf", NULL };

static bool isBlacklistDeviceName(const char *devname)
{
	int i = 0;
	while (blacklist_device_names[i]) {
		if (strncmp(blacklist_device_names[i], devname, strlen(blacklist_device_names[i])) == 0) {
			CM_DBG("Interface %s is blacklisted according to %s\n", devname, blacklist_device_names[i]);
			return true;
		}
		i++;
	}
	
	return false;
}

ConnectivityLocalMacOSX::ConnectivityLocalMacOSX(ConnectivityManager *m) :
	ConnectivityLocal(m, "ConnectivityLocalMacOSX")
{
}

ConnectivityLocalMacOSX::~ConnectivityLocalMacOSX()
{
}

#if defined(ENABLE_BLUETOOTH)

void ConnectivityLocalMacOSX::findLocalBluetoothInterfaces()
{
	IOReturn ret;
	unsigned char macaddr[BT_MAC_LEN];
	const char *name = "LocalBluetoothDevice";
	ret = IOBluetoothLocalDeviceAvailable();

	// if we can't find any bluetooth device, return silently:
	if (!ret)
		return;

	BluetoothHCIPowerState powerState;
	IOBluetoothLocalDeviceGetPowerState(&powerState);

	printf("read power state\n");

	// return if the bluetooth interface is not switched on
	if (powerState != kBluetoothHCIPowerStateON) {
		return;
	}
	
	/*
	 READ This!!!!!! Information about IOBluetooth limitations...
	 ============================================================
	 For some reason, calling any mac Bluetooth API functions
	 here that involves the local device will have the effect that we
	 cannot start an RFCOMM server in another thread. It seems
	 that these functions internally claim the device for this
	 thread, such that another thread cannot call similar
	 functions with the desired effect... Really, really stupid!
	 However, as the Bluetooth API for Mac seems to assume only
	 one local device, and the mac address of it is not necessary
	 here, it is sufficient to just fake something for the time
	 being. Therefore the MAC address of the local device can 
	 be hard coded above. This of course only works for a specific
	 machine, but is the only way to get Bluetooth to work on
	 Mac OS X until we figure out a better solution.
	 
	 The code below discovers the local interface available on
	 the machine, but has the effect described above. Therefore,
	 the code is disabled with a macro.
	 */
	/*
	   Get the device name:
	 */
	
  	BluetoothDeviceName localDevName;

 	ret = IOBluetoothLocalDeviceReadName(localDevName, NULL, NULL, NULL);

	printf("read the device name\n");
 	// if we don't get a good return value here, the bluetooth device is off.
 	if (ret != kIOReturnSuccess)
 		return;
	
	name = (char *)localDevName;
	/*
	   Get the device adress:
	 */
	
	BluetoothDeviceAddress btAddr;

 	ret = IOBluetoothLocalDeviceReadAddress(&btAddr, NULL, NULL, NULL);

 	if (ret != kIOReturnSuccess) {
 		CM_DBG("Could not get local Bluetooth device address\n");
 		return;
 	}

 	memcpy(macaddr, &btAddr, BT_MAC_LEN);

	BluetoothAddress addr(macaddr);
	/*
	   Put it in the found devices table:
	 */
	BluetoothInterface iface(macaddr, name, &addr, IFFLAG_UP | IFFLAG_LOCAL);
	
 	report_interface(&iface, rootInterface, new ConnectivityInterfacePolicyTTL(1));	

	// TODO: Check return values
}
#endif

#if defined(ENABLE_MEDIA)

void ConnectivityLocalMacOSX::onMountVolume(char *path)
{
	
	// TODO: Implement add media interface
	//Interface *iface = new Interface(IFACE_TYPE_MEDIA,
//					 macaddr,
//					 NULL,
//					 0,
//					 ifr->ifr_name,
//					 IFACE_FLAG_LOCAL);
//	
//	ethernet_interfaces_found++;
//	
//	if (add_interface(iface->copy(), NULL, new ConnectivityInterfacePolicyTTL(1)))
//		interface_added(iface);
//	else
//		delete iface;
}

void ConnectivityLocalMacOSX::onUnmountVolume(char *path)
{
	CM_DBG("%s\n", path);
	
	Interface *interface = NULL; //find_interface_by_name(path);
	if (interface) {
		delete_interface(interface);
	} else {
		CM_DBG("interface not found\n");
	}
}


void onFSEvents(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, 
		       void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
	char **paths = (char **) eventPaths;
	ConnectivityLocalMacOSX *conn = (ConnectivityLocalMacOSX *) clientCallBackInfo;
	unsigned int i;
	for (i = 0; i < numEvents; i++) {
		if (eventFlags[i] & kFSEventStreamEventFlagMount) {
			conn->onMountVolume(paths[i]);
		}
		if (eventFlags[i] & kFSEventStreamEventFlagUnmount) {
			conn->onUnmountVolume(paths[i]);
		}
	}
}

#endif /* ENABLE_MEDIA */

bool ConnectivityLocalMacOSX::run()
{
#if defined(ENABLE_ETHERNET)
	long interfaces_found_last_time;
#endif
#if ENABLE_MAC_OS_X_KERNEL_SOCKET_CODE
	int kEventSock;
#endif
	
#if defined(ENABLE_MEDIA)
	//CFStringRef cfPath;
//	FSEventStreamContext context;
//	
//	context.version = 0;
//	context.info = (void *)this;
//	context.retain = NULL;
//	context.release = NULL;
//	context.copyDescription = NULL;
//	
//	cfPath = CFStringCreateWithCString(kCFAllocatorDefault, HAGGLE_VOLUME_PATH, kCFStringEncodingUTF8);
//	
//	CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **) &cfPath, 1, NULL);
//	
//	fsEventStream = FSEventStreamCreate(NULL, &onFSEvents, &context, pathsToWatch, kFSEventStreamEventIdSinceNow,	/* Or a previous event ID */
//					    1.0,	/* Latency in seconds */
//					    kFSEventStreamCreateFlagNone);
//	
//	CFRelease(cfPath);
//	CFRelease(pathsToWatch);
//	runLoop = CFRunLoopGetCurrent();
//	CFRetain(runLoop);
//	FSEventStreamScheduleWithRunLoop(fsEventStream, runLoop, kCFRunLoopDefaultMode);
	//FSEventStreamStart(fsEventStream);
	//CFRunLoopRun();
	
#endif

#if ENABLE_KERNEL_SOCKET_CODE
	/*
	   This is apparently how you register a kernel event socket...

	   Not sure what it's good for, but...
	 */
	int ret = 0;

	kEventSock = -1;

	if (getuid() != 0) {
		CM_DBG("Not opening Event socket since not root user\n");
		goto kernel_socket_failed;
	}
	kEventSock = socket(PF_SYSTEM, SOCK_RAW, SYSPROTO_EVENT);

	if (kEventSock < 0) {
		CM_DBG("Could not open kernel event socket");
		goto kernel_socket_failed;
	}

	struct kev_request req;

	req.vendor_code = KEV_VENDOR_APPLE;
	req.kev_class = KEV_NETWORK_CLASS;
	req.kev_subclass = KEV_ANY_SUBCLASS;

	ret = ioctl(kEventSock, SIOCSKEVFILT, &req);

	if (ret < 0) {
		CM_DBG("Could not register kernel event class");
		close(kEventSock);
		kEventSock = -1;
		goto kernel_socket_failed;
	}
	//haggle->registerSocket(kEventSock, this);
      kernel_socket_failed:
#endif
	while (!shouldExit()) {

#if defined(ENABLE_BLUETOOTH)
		findLocalBluetoothInterfaces();
#endif
#if defined(ENABLE_ETHERNET)
                InterfaceRefList iflist;
		interfaces_found_last_time = ethernet_interfaces_found;
		ethernet_interfaces_found = 0;
		
                int num = getLocalInterfaceList(iflist, true);
                
		//CM_DBG("Found %d local interfaces\n", num);

                if (num) {
                        while (!iflist.empty()) {
                                InterfaceRef iface = iflist.pop();
                                
                                if (isBlacklistDeviceName(iface->getName())) {
					CM_DBG("Interface %s is Blacklisted\n", iface->getName());
					continue;
				}
                                
                                if (iface->isUp()) {
                                        report_interface(iface, rootInterface, new ConnectivityInterfacePolicyTTL(1));
                                        ethernet_interfaces_found++;
                                } else {
					CM_DBG("Local interface is marked DOWN\n");
				}
                        }
                }
#endif

#if ENABLE_KERNEL_SOCKET_CODE
#error should be hanging on socket for <insert good time here> seconds.

		/*
		   This is how you receive kernel events from a socket...

		   does this happen when new interfaces go up/down?
		   This code is from the previous connection manager
		 */
		struct {
			struct kern_event_msg kevm;
			char buf[4000];
		} kmsg;
		int ret;

		ret = recv(kEventSock, &kmsg, sizeof(kmsg), 0);

		if (ret < 0) {
			CM_DBG("Error reading socket\n");
			return -1;
		}
		CM_DBG("Received kernel event, tot_size=%u\n", kmsg.kevm.total_size);

		// TODO: Find out how to interpret the events
#endif
		cancelableSleep(15000);
		age_interfaces(rootInterface);
	}
	return false;
}

void ConnectivityLocalMacOSX::hookStopOrCancel()
{
#if defined(ENABLE_MEDIA)
	//FSEventStreamStop(fsEventStream);
	//CFRunLoopStop(runLoop);	
	
	// This removes the stream from the runloop
	//FSEventStreamInvalidate(fsEventStream);
#endif
}


void ConnectivityLocalMacOSX::hookCleanup()
{
#if defined(ENABLE_MEDIA)
	//CFRelease(fsEventStream);
//	CFRelease(runLoop);
#endif
#if ENABLE_KERNEL_SOCKET_CODE
	close(kEventSock);
#endif
}


#endif
