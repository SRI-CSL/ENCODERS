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

#include <libcpphaggle/Platform.h>

#if defined(OS_WINDOWS)

#include <libcpphaggle/Watch.h>

#include "ConnectivityManager.h"
#include "ConnectivityLocalWindowsMobile.h"
//#include "ConnectivityBluetooth.h"
#include "ConnectivityEthernet.h"
#include "Utility.h"

#if defined(ENABLE_BLUETOOTH)

#if defined(WIDCOMM_BLUETOOTH)
#include "WIDCOMMBluetooth.h"
#else
#include <ws2bth.h>
#endif

ConnectivityLocalWindowsMobile::ConnectivityLocalWindowsMobile(ConnectivityManager *m) :
	ConnectivityLocal(m, "ConnectivityLocalWindowsMobile")
{
}

ConnectivityLocalWindowsMobile::~ConnectivityLocalWindowsMobile()
{
}

void ConnectivityLocalWindowsMobile::handleBTEvent(BTEVENT * e)
{
	switch (e->dwEventId) {
	case BTE_CONNECTION:
		HAGGLE_DBG("Bluetooth connection event\n");
		break;
	case BTE_DISCONNECTION:
		HAGGLE_DBG("Bluetooth disconnection event\n");
		break;
	case BTE_ROLE_SWITCH:
		HAGGLE_DBG("Bluetooth role switch event\n");
		break;
	case BTE_MODE_CHANGE:
		HAGGLE_DBG("Bluetooth mode change event\n");
		break;
	case BTE_PAGE_TIMEOUT:
		HAGGLE_DBG("Bluetooth PAGE_TIMEOUT event\n");
		break;
	case BTE_KEY_NOTIFY:
		HAGGLE_DBG("Bluetooth KEY_NOTIFY event\n");
		break;
	case BTE_KEY_REVOKED:
		HAGGLE_DBG("Bluetooth KEY_REVOKED event\n");
		break;
	case BTE_LOCAL_NAME:
		HAGGLE_DBG("Bluetooth local name event.\n");
		break;
	case BTE_COD:
		HAGGLE_DBG("Bluetooth COD event\n");
		break;
	case BTE_STACK_UP:
		HAGGLE_DBG("Bluetooth stack up event\n");
		findLocalBluetoothInterfaces();
		break;
	case BTE_STACK_DOWN:
		HAGGLE_DBG("Bluetooth stack down event\n");
		// FIXME: A problem here is that the following call will also delete any other 
		// interfaces that have been discovered

		if (btIface) {
			delete_interface(btIface);
			btIface = NULL;
		}

#if defined(BLUETOOTH_STACK_FORCE_UP)
		/*
			On some Windows mobile devices, the stack seems
			to sometimes go down for no good reason. Here
			we have the option to force it back up again.
		*/
		reenableBluetoothStack = true;
#endif
		break;
		//case BTE_AVDTP_STATE:
		//      HAGGLE_DBG("Bluetooth AVDTP_STATE event\n");
		//      break;
	default:
		HAGGLE_DBG("Unknown Bluetooth Event\n");
		break;
	}
}
#if defined(WIDCOMM_BLUETOOTH)

void ConnectivityLocalWindowsMobile::findLocalBluetoothInterfaces()
{
	unsigned char macaddr[6];
	char btName[256];

	if (WIDCOMMBluetooth::readLocalDeviceAddress(macaddr) < 0)
		return;

	if (WIDCOMMBluetooth::readLocalDeviceName(btName, 256) < 0)
		return;

	BluetoothAddress addr(macaddr);

	if (btIface)
		btIface = NULL;

	BluetoothInterface btIface(macaddr, btName, &addr, IFFLAG_LOCAL | IFFLAG_UP);
	
	HAGGLE_DBG("Adding new LOCAL Bluetooth Interface: %s\n", btName);

	report_interface(&btIface, rootInterface, new ConnectivityInterfacePolicyAgeless());
}

#else

struct bthandle {
	SOCKET sock;
	SOCKADDR_BTH addr;
	bool isInquiring;
};

enum {
	BtModeIdle,
	BtModeDiscovery,
};


static void btAddr2Mac(unsigned __int64 btAddr, char *mac)
{
	for (int i = (BT_ALEN - 1); i >= 0; i--) {
		mac[i] = (UINT8) ((unsigned __int64) 0xff & btAddr);
		btAddr = btAddr >> 8;
	}
}

// Find local bluetooth interfaces
void ConnectivityLocalWindowsMobile::findLocalBluetoothInterfaces()
{
	unsigned char macaddr[BT_ALEN];
	char btName[BT_ALEN*3];

#if defined(OS_WINDOWS_MOBILE)
	SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
#elif defined(OS_WINDOWS_XP)
	// FIXME: why doesn't this work?!
	SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
#endif
	if (s == INVALID_SOCKET) {
		CM_DBG("Could not open Bluetooth socket (%s)\n", StrError(WSAGetLastError()));
		return;		//-1;
	}

	SOCKADDR_BTH addr;
	int size = sizeof(SOCKADDR_BTH);
	memset(&addr, 0, size);
	addr.addressFamily = AF_BTH;
	addr.port = 0;

	if (bind(s, (SOCKADDR *) & addr, size) != 0) {
		CM_DBG("Could not bind Bluetooth socket\n");
		closesocket(s);
		return;		//-1;
	}

	if (getsockname(s, (sockaddr *) & addr, &size) != 0) {
		CM_DBG("getsockname failed\n");
		closesocket(s);
		return;		//-1;
	}

	btAddr2Mac(addr.btAddr, (char *)macaddr);

	// Should probably find a better way to discover that something is wrong here.
	if (macaddr[0] == 0 && 
		macaddr[1] == 0 && 
		macaddr[2] == 0 && 
		macaddr[3] == 0 && 
		macaddr[4] == 0 &&
		macaddr[5] == 0)
		return;

	if (gethostname(btName, BT_ALEN*3) != 0) {
		CM_DBG("Could not get name of Bluetooth device\n");
		closesocket(s);
		return;		//-1;
	}
	
	BluetoothAddress btAddr(macaddr);

	if (btIface)
		btIface = NULL;

	BluetoothInterface btIface(macaddr, btName, &btAddr, IFFLAG_LOCAL | IFFLAG_UP);
	
	HAGGLE_DBG("Adding new LOCAL Bluetooth Interface: %s\n", btName);

	report_interface(&btIface, rootInterface, new ConnectivityInterfacePolicyAgeless());

	closesocket(s);

	return;
}
#endif /* WIDCOMM_BLUETOOTH */
#endif /* ENABLE_BLUETOOTH */

#if defined(ENABLE_ETHERNET)

#include <iphlpapi.h>

void ConnectivityLocalWindowsMobile::findLocalEthernetInterfaces()
{  
	InterfaceRefList iflist;

	int num = getLocalInterfaceList(iflist);

	if (num > 0) {
		ethIface = iflist.pop();

		if (ethIface->isUp()) {
			report_interface(ethIface, rootInterface, new ConnectivityInterfacePolicyAgeless());
		}
	}
}
#endif

void ConnectivityLocalWindowsMobile::hookCleanup()
{
#if defined(ENABLE_BLUETOOTH)
	StopBluetoothNotifications(hBTNotif);
	CloseMsgQueue(hMsgQ);
#endif

#if defined(ENABLE_ETHERNET)
	CloseHandle(notifyAddrChangeHandle);
#endif
	
#if defined(WIDCOMM_BLUETOOTH)
	//WIDCOMMBluetooth::cleanup();
#endif
}

bool ConnectivityLocalWindowsMobile::run()
{
	Watch w;

#if defined(ENABLE_BLUETOOTH)
	BTEVENT btEvent;
	DWORD dwBytesRead, dwFlags = 0;
	MSGQUEUEOPTIONS mqOpts = { sizeof(MSGQUEUEOPTIONS), MSGQUEUE_NOPRECOMMIT, 0,
		sizeof(BTEVENT), TRUE
	};

#if defined(WIDCOMM_BLUETOOTH)
	//WIDCOMMBluetooth::init();
#endif
#if defined(BLUETOOTH_STACK_FORCE_UP)
	reenableBluetoothStack = false;
#endif
	btIface = NULL;

	hMsgQ = CreateMsgQueue(NULL, &mqOpts);
	hBTNotif = RequestBluetoothNotifications(BTE_CLASS_DEVICE | BTE_CLASS_STACK, hMsgQ);

	if (!hBTNotif) {
		HAGGLE_DBG("RequestBluetoothNotifications Failed\n",);
		return false;
	}
	
	int bthIndex = w.add(hMsgQ);
	findLocalBluetoothInterfaces();
#endif

	
#if defined(ENABLE_ETHERNET)
	/*
		This function registers a notification to occur every time there is 
		a change in the device's local table that maps IP addresses to interfaces.
		It is a slightly crude way to monitor the device's network interface for
		status change, but it seems to work quite well. The event does not return
		any information about what was changed, so we need to figure that out 
		ourselves.
	*/
	ethIface = NULL;

	if (NotifyAddrChange(&notifyAddrChangeHandle, NULL) != NO_ERROR) {
		HAGGLE_ERR("Could not open NotifyRouteChange handle\n");
		return false;
	}
	int ethIndex = w.add(notifyAddrChangeHandle);

        findLocalEthernetInterfaces();
#endif

	while (!shouldExit()) {
		int ret;
	
		w.reset();

		HAGGLE_DBG("Conn Local waiting on objects\n");
		ret = w.wait();

		if (ret < 0) {
			HAGGLE_ERR("Watch error in %s\n", getName());
			continue;
		}
#if defined(ENABLE_BLUETOOTH)

		if (w.isSet(bthIndex)) {
			BOOL ret = ReadMsgQueue(hMsgQ, &btEvent, sizeof(BTEVENT),
				&dwBytesRead, 0, &dwFlags);

			if (!ret) {
				DWORD error = GetLastError();

				if (error != ERROR_TIMEOUT) {
					HAGGLE_DBG("Error - Failed to read message from queue error=%d!\n", error);
					return false;
				}
			} else {
				handleBTEvent(&btEvent);
			}
		}

#if defined(BLUETOOTH_STACK_FORCE_UP)
		// This will force the Bluetooth stack to go up again,
		// if we previously noticed it go down
		if (reenableBluetoothStack) {
			Sleep(2000);
			reenableBluetoothStack = false;
#if defined(WIDCOMM_BLUETOOTH)
			BthSetMode(0);
#else
			BthSetMode(BTH_DISCOVERABLE);
#endif
		}
#endif

#endif

#if defined(ENABLE_ETHERNET)
		if (w.isSet(ethIndex)) {
			// NOTE: This event based approach with NotifyAddrChange() does not always seem to work. 
			// Need more testing!
			HAGGLE_DBG("NotifyAddrChange!!\n");

			/*
				This is somewhat ugly. We assume that there is one Eth/WiFi network interface. 
				Whenever we notice an address change in the interface to address mapping table,
				We assume the interface went away, and we therefore have to discover it again.
			*/
			if (ethIface) {
				// Delete all remote interfaces discovered on this local interface
				delete_interface(ethIface);
				ethIface = NULL;
			}
			findLocalEthernetInterfaces();
		}
#endif
        }
	return false;
}

#endif
