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

#if defined(OS_WINDOWS_DESKTOP)

#include <libcpphaggle/Watch.h>

#if defined(ENABLE_BLUETOOTH)
#include <ws2bth.h>
/*
#include <bthapi.h>
#include <bthutil.h>
#include <bt_api.h>
#include <bt_sdp.h>
*/

#include <BluetoothAPIs.h>

struct bthandle {
	SOCKET sock;
	SOCKADDR_BTH addr;
	bool isInquiring;
};

enum {
	BtModeIdle,
	BtModeDiscovery,
};
#endif

#if defined(ENABLE_ETHERNET)
#include <iphlpapi.h>
#endif 
#include <libcpphaggle/Timeval.h>

#include "ConnectivityManager.h"
#include "ConnectivityLocalWindowsXP.h"
#include "Utility.h"

ConnectivityLocalWindowsXP::ConnectivityLocalWindowsXP(ConnectivityManager *m) :
	ConnectivityLocal(m, "ConnectivityLocalWindowsXP")
{
}

ConnectivityLocalWindowsXP::~ConnectivityLocalWindowsXP()
{
}

void ConnectivityLocalWindowsXP::hookCleanup()
{
#if defined(ENABLE_BLUETOOTH)
#endif
}

#if defined(ENABLE_ETHERNET)

void ConnectivityLocalWindowsXP::findLocalEthernetInterfaces()
{
        InterfaceRefList iflist;
                
        int num = getLocalInterfaceList(iflist);
        
        while (!iflist.empty()) {
                InterfaceRef iface = iflist.pop();
                
                if (iface->isUp()) {
                        report_interface(iface, rootInterface, new ConnectivityInterfacePolicyTTL(1));
		}
        }
}
#endif

bool ConnectivityLocalWindowsXP::run()
{
	Timeval timeout;
	Watch w;

#if defined(ENABLE_BLUETOOTH)
	findLocalBluetoothInterfaces();
#endif
	while (!shouldExit()) {
		int ret;
	
		if (w.getRemainingTime(&timeout))
			goto wait;

		timeout.zero() += 3;

#if defined(ENABLE_BLUETOOTH)
		findLocalBluetoothInterfaces();
#endif
#if defined(ENABLE_ETHERNET)
                findLocalEthernetInterfaces();
#endif

wait:
		w.reset();

		ret = w.wait(&timeout);

		if (ret == Watch::FAILED) {
			HAGGLE_ERR("Watch error in %s\n", getName());
			continue;
		} else if (ret == Watch::ABANDONED) {
			break;
		}
		
		age_interfaces(NULL);
        }
	return false;
}

#if defined(ENABLE_BLUETOOTH)
static void btAddr2Mac(unsigned __int64 btAddr, char *mac)
{
	for (int i = (BT_ALEN - 1); i >= 0; i--) {
		mac[i] = (UINT8) ((unsigned __int64) 0xff & btAddr);
		btAddr = btAddr >> 8;
	}
}

// Find local bluetooth interfaces
void ConnectivityLocalWindowsXP::findLocalBluetoothInterfaces()
{
	char macaddr[BT_ALEN];
	char btName[BT_ALEN*3];

	// FIXME: why doesn't this work?!
	SOCKET s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

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

	btAddr2Mac(addr.btAddr, macaddr);

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
	
	Address addy(AddressType_BTMAC, (unsigned char *) macaddr);

	Interface iface(IFTYPE_BLUETOOTH, macaddr, &addy, btName, IFFLAG_LOCAL | IFFLAG_UP);
	
	report_interface(&iface, rootInterface, new ConnectivityInterfacePolicyTTL(1));

	closesocket(s);
}
#endif


#endif
