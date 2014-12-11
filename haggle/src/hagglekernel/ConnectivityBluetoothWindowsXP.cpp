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

#if defined(OS_WINDOWS_DESKTOP) && defined(ENABLE_BLUETOOTH)

#include "ConnectivityBluetooth.h"

#include <ws2bth.h>
#include "ProtocolRFCOMM.h"
#include <BluetoothAPIs.h>

struct sdp_record {
	char data1;
	char len;
	char data2[6];
	char uuid[16];
	char data[16];
	char channel;
} sdp_rec = {
	0x35, 0x00, {
	0x09, 0x00, 0x01, 0x35, 0x11, 0x1c}, {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {
	0x09, 0x00, 0x04, 0x35, 0x0c, 0x35, 0x03, 0x19, 0x01, 0x00, 0x35, 0x05, 0x19, 0x00, 0x03, 0x08,}, {
	0}
};

//      Makes this bluetooth "service" visible to anyone else searching
static int registerSDPService(ULONG * sdpRecordHandle)
{
	WSAQUERYSET wqs;
	unsigned char uuid[] = HAGGLE_BLUETOOTH_SDP_UUID;
	BLOB b;
	ULONG sdpVersion = BTH_SDP_VERSION;
	struct sdp_record *sdpRec;
	struct {
		BTHNS_SETBLOB bthNsBlob;
		char data[sizeof(struct sdp_record) - 1];
	} rec;

	*sdpRecordHandle = 0;

	memset(&rec, 0, sizeof(rec));
	rec.bthNsBlob.pRecordHandle = (HANDLE *) sdpRecordHandle;
	rec.bthNsBlob.ulRecordLength = sizeof(sdp_rec);
	rec.bthNsBlob.pSdpVersion = &sdpVersion;

	sdpRec = (struct sdp_record *) &rec.bthNsBlob.pRecord;

	/* Intitialize the record with the default values */
	memcpy(sdpRec, &sdp_rec, sizeof(sdp_rec));

	/* Add our UUID and channel */
	sdpRec->len = sizeof(struct sdp_record) - 2;
	memcpy(sdpRec->uuid, uuid, sizeof(uuid));
	sdpRec->channel = RFCOMM_DEFAULT_CHANNEL;

	b.cbSize = sizeof(rec);
	b.pBlobData = (PBYTE) & rec;

	memset(&wqs, 0, sizeof(WSAQUERYSET));
	wqs.dwSize = sizeof(WSAQUERYSET);
	wqs.dwNameSpace = NS_BTH;
	wqs.lpBlob = &b;

	int ret = WSASetService(&wqs, RNRSERVICE_REGISTER, 0);

	if (ret == SOCKET_ERROR)
		return -1;

	return 0;
}

//      Removes the bluetooth "service" registered above

static int deRegisterSDPService(ULONG sdpRecordHandle)
{
	WSAQUERYSET wqs;
	BLOB b;
	ULONG sdpVersion = BTH_SDP_VERSION;
	BTHNS_SETBLOB bthNsBlob;

	memset(&bthNsBlob, 0, sizeof(bthNsBlob));
	bthNsBlob.pRecordHandle = (HANDLE *) & sdpRecordHandle;
	bthNsBlob.ulRecordLength = sizeof(sdp_rec);
	bthNsBlob.pSdpVersion = &sdpVersion;

	b.cbSize = sizeof(bthNsBlob);
	b.pBlobData = (PBYTE) & bthNsBlob;

	memset(&wqs, 0, sizeof(WSAQUERYSET));
	wqs.dwSize = sizeof(WSAQUERYSET);
	wqs.dwNameSpace = NS_BTH;
	wqs.lpBlob = &b;

	int ret = WSASetService(&wqs, RNRSERVICE_DELETE, 0);

	if (ret == SOCKET_ERROR)
		return -1;

	return 0;
}

// Scan for services like the ones registered/deregistered above:
static void convertUUIDBytesToGUID(char *bytes, GUID * uuid)
{
	uuid->Data1 = bytes[0] << 24 & 0xff000000 | bytes[1] << 16 & 0x00ff0000 | bytes[2] << 8 & 0x0000ff00 | bytes[3] & 0x000000ff;
	uuid->Data2 = bytes[4] << 8 & 0xff00 | bytes[5] & 0x00ff;
	uuid->Data3 = bytes[6] << 8 & 0xff00 | bytes[7] & 0x00ff;

	for (int i = 0; i < 8; i++) {
		uuid->Data4[i] = bytes[i + 8];
	}
}

/* Some code stolen from BlueCove. */
static int findHaggleService(BTH_ADDR * pb)
{
	BTHNS_RESTRICTIONBLOB queryservice;
	unsigned char uuid[] = HAGGLE_BLUETOOTH_SDP_UUID;
	GUID guid;
	BLOB blob;
	WSAQUERYSET queryset;
	SOCKADDR_BTH sa;
	HANDLE hLookupSearchServices;
	int found = 0;

	memset(&queryservice, 0, sizeof(queryservice));

	queryservice.type = SDP_SERVICE_SEARCH_REQUEST;

	convertUUIDBytesToGUID((char *) uuid, &guid);

	//UUID is full 128 bits
	queryservice.uuids[0].uuidType = SDP_ST_UUID128;

	memcpy(&queryservice.uuids[0].u.uuid128, &guid, sizeof(guid));
	// build BLOB pointing to service query

	blob.cbSize = sizeof(queryservice);
	blob.pBlobData = (BYTE *) & queryservice;

	// build query
	memset(&queryset, 0, sizeof(WSAQUERYSET));

	queryset.dwSize = sizeof(WSAQUERYSET);
	queryset.dwNameSpace = NS_BTH;
	queryset.lpBlob = &blob;


	// Build address
	memset(&sa, 0, sizeof(sa));
	sa.addressFamily = AF_BTH;
	sa.btAddr = *pb;;
	CSADDR_INFO csai;
	memset(&csai, 0, sizeof(csai));
	csai.RemoteAddr.lpSockaddr = (sockaddr *) & sa;
	csai.RemoteAddr.iSockaddrLength = sizeof(sa);
	queryset.lpcsaBuffer = &csai;

	// begin query

	if (WSALookupServiceBegin(&queryset, 0, &hLookupSearchServices)) {
		CM_DBG("WSALookupServiceBegin error %s", StrError(WSAGetLastError()));
		return -1;
	}
	// fetch results
	int bufSize = 0x2000;
	void *buf = malloc(bufSize);

	if (buf == NULL) {
		WSALookupServiceEnd(hLookupSearchServices);
		return NULL;
	}
	memset(buf, 0, bufSize);

	LPWSAQUERYSET pwsaResults = (LPWSAQUERYSET) buf;
	pwsaResults->dwSize = sizeof(WSAQUERYSET);
	pwsaResults->dwNameSpace = NS_BTH;
	pwsaResults->lpBlob = NULL;

	DWORD size = bufSize;

	if (WSALookupServiceNext(hLookupSearchServices, 0, &size, pwsaResults)) {
		int last_error = WSAGetLastError();

		switch (last_error) {
		case WSANO_DATA:
			found = 0;
			break;
		case WSA_E_NO_MORE:
			found = 0;
			CM_DBG("WSA_E_NO_MORE: No matching service found\n");
			break;
		default:
			CM_DBG("WSALookupServiceNext error [%s]\n", StrError(last_error));
			found = -1;
		}
	} else {
		found++;
	}
	WSALookupServiceEnd(hLookupSearchServices);
	free(buf);

	return found;
}

static void btAddr2Mac(unsigned __int64 btAddr, unsigned char *mac)
{
	for (int i = (BT_ALEN - 1); i >= 0; i--) {
		mac[i] = (UINT8) ((unsigned __int64) 0xff & btAddr);
		btAddr = btAddr >> 8;
	}
}

void bluetoothDiscovery(InterfaceRef& iface, ConnectivityBluetooth *conn)
{
	//BOOL bHaveName;
	HANDLE hScan = INVALID_HANDLE_VALUE;
	WSAQUERYSET wsaq;
	BTHNS_INQUIRYBLOB queryBlob;
	queryBlob.LAP = BTH_ADDR_GIAC;
	queryBlob.length = 12;
	BLOB blob;
	int count = 0;
	BTH_DEVICE_INFO *p_inqRes;
	union {
		CHAR buf[4096];
		SOCKADDR_BTH __unused;
	};

	blob.cbSize = sizeof(queryBlob);
	blob.pBlobData = (BYTE *) & queryBlob;

	ZeroMemory(&wsaq, sizeof(wsaq));
	wsaq.dwSize = sizeof(wsaq);
	wsaq.dwNameSpace = NS_BTH;
	wsaq.lpcsaBuffer = NULL;
	wsaq.lpBlob = &blob;

	if (!iface || iface->getType() != IFTYPE_BLUETOOTH)
		return;

	CM_DBG("Doing scan on device %s - %s\n", iface->getName(), iface->getIdentifierStr());

	if (ERROR_SUCCESS != WSALookupServiceBegin(&wsaq, LUP_CONTAINERS, &hScan)) {
		CM_DBG("WSALookupServiceBegin failed\n");
		return;
	}
	// loop the results
	while (true) {
		DWORD dwSize = sizeof(buf);
		LPWSAQUERYSET pwsaResults = (LPWSAQUERYSET) buf;
		ZeroMemory(pwsaResults, sizeof(WSAQUERYSET));
		pwsaResults->dwSize = sizeof(WSAQUERYSET);
		pwsaResults->dwNameSpace = NS_BTH;
		pwsaResults->lpBlob = NULL;
		unsigned char macaddr[BT_ALEN];

		if (WSALookupServiceNext(hScan, LUP_RETURN_NAME | LUP_RETURN_ADDR | LUP_RETURN_BLOB, &dwSize, pwsaResults) != ERROR_SUCCESS) {
			CM_DBG("Found %d Haggle devices\n", count);
			break;
		}

		p_inqRes = (BTH_DEVICE_INFO *) pwsaResults->lpBlob->pBlobData;
		/*tempList[count].btMajor = p_inqRes->cod & 0x001f00;
		   tempList[count].btMinor = p_inqRes->cod & 0x0000fc;

		   bHaveName = pwsaResults->lpszServiceInstanceName && *(pwsaResults->lpszServiceInstanceName);
		   sprintf(tempList[count].btName,
		   "%S",
		   bHaveName ? pwsaResults->lpszServiceInstanceName : L""); */
		BTH_ADDR btAddr = ((SOCKADDR_BTH *) pwsaResults->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr;

		btAddr2Mac(btAddr, macaddr);
		Address addr(AddressType_BTMAC, macaddr);
		
		if (findHaggleService(&btAddr) > 0) {
			CM_DBG("Found Bluetooth device [%s]\n", addr.getAddrStr());

			Interface iface(IFTYPE_BLUETOOTH, macaddr, &addr, "Bluetooth");

			conn->report_interface(&iface, conn->rootInterface, new ConnectivityInterfacePolicyTTL(2));

			count++;
		} else {
			CM_DBG(
				"Bluetooth device [%s] not a Haggle device\n", 
				addr.getAddrStr());
		}
	}

	// cleanup
	WSALookupServiceEnd(hScan);

	return;
}

void ConnectivityBluetooth::hookCleanup()
{
	deRegisterSDPService(sdpRecordHandle);
}

bool ConnectivityBluetooth::run()
{
	if (registerSDPService(&sdpRecordHandle) < 0) {
		CM_DBG("WSAGetLastError=%s\n", StrError(WSAGetLastError()));
		fflush(stdout);
		CM_DBG("Could not register SDP service\n");
		return false;
	}
	while (!shouldExit()) {
		bluetoothDiscovery(rootInterface, this);

		cancelableSleep(TIME_TO_WAIT_MSECS);

		age_interfaces(rootInterface);
	}
	return false;
}

#endif
