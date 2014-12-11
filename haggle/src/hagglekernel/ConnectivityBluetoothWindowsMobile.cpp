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

#if defined(OS_WINDOWS_MOBILE) && defined(ENABLE_BLUETOOTH) && !defined(WIDCOMM_BLUETOOTH)

#include "ConnectivityBluetooth.h"

#include <ws2bth.h>
#include <bthapi.h>
#include <bthutil.h>
#include <bt_api.h>
#include <bt_sdp.h>

#include "ProtocolRFCOMM.h"

#pragma comment(lib,"ws2.lib")
#pragma comment(lib,"bthguid.lib")

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

/*
	Code for detecting RFCOMM channel (2 functions) was found here:
	http://www.developer.com/ws/other/article.php/3640576
*/
int IsRfcommUuid(NodeData *pNode)
{
   if (pNode->type != SDP_TYPE_UUID)
      return FALSE;

   if (pNode->specificType      == SDP_ST_UUID16)
      return (pNode->u.uuid16   == RFCOMM_PROTOCOL_UUID16);
   else if (pNode->specificType == SDP_ST_UUID32)
      return (pNode->u.uuid32   == RFCOMM_PROTOCOL_UUID16);
   else if (pNode->specificType == SDP_ST_UUID128)
      return (0 == memcmp(&RFCOMM_PROTOCOL_UUID,&pNode->
              u.uuid128,sizeof(GUID)));

   return FALSE;
}

HRESULT FindRFCOMMChannel (unsigned char *pStream, int cStream,
			   unsigned long *nChannel)
{
	ISdpRecord **pRecordArg = NULL;
	int cRecordArg          = 0;
	ISdpStream *pIStream    = NULL;
	HRESULT hr              = 0;
	ULONG ulError           = 0;

	*nChannel = 0;

	hr = CoCreateInstance(__uuidof(SdpStream),NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(ISdpStream),(LPVOID *)
		&pIStream);

	if ( FAILED(hr) || pIStream == NULL )
		return hr;

	hr = pIStream->Validate (pStream, cStream,&ulError);

	if (SUCCEEDED(hr))
	{
		hr = pIStream->VerifySequenceOf(pStream, cStream,
			SDP_TYPE_SEQUENCE,NULL,
			(ULONG *)&cRecordArg);

		if (SUCCEEDED(hr) && cRecordArg > 0)
		{
			pRecordArg =
				(ISdpRecord **) CoTaskMemAlloc(sizeof(ISdpRecord*)
				* cRecordArg);

			if (pRecordArg != NULL)
			{
				hr =
					pIStream->RetrieveRecords(pStream, cStream,
					pRecordArg,(ULONG *)
					&cRecordArg);

				if ( FAILED(hr) )
				{
					CoTaskMemFree(pRecordArg);
					pRecordArg = NULL;
					cRecordArg = 0;
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
	}

	if (pIStream != NULL)
	{
		pIStream->Release();
		pIStream = NULL;
	}

	if ( FAILED(hr) )
		return hr;

	for (int i = 0; (*nChannel == 0) && (i < cRecordArg); i++)
	{
		ISdpRecord *pRecord = pRecordArg[i];
		// contains SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST data,
		// if available
		NodeData protocolList;

		if (ERROR_SUCCESS !=
			pRecord->GetAttribute(SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST,
			&protocolList) ||
			(protocolList.type !=
			SDP_TYPE_CONTAINER))
		{
			if (protocolList.type == SDP_TYPE_STRING)
				CoTaskMemFree(protocolList.u.str.val);
			else if (protocolList.type == SDP_TYPE_URL)
				CoTaskMemFree(protocolList.u.url.val);
			continue;
		}

		ISdpNodeContainer *pRecordContainer = protocolList.u.container;
		int cProtocols = 0;
		NodeData protocolDescriptor;

		pRecordContainer->GetNodeCount((DWORD *)&cProtocols);
		for (int j = 0; (nChannel == 0) && (j < cProtocols); j++)
		{
			pRecordContainer->GetNode(j,&protocolDescriptor);

			if (protocolDescriptor.type != SDP_TYPE_CONTAINER)
				continue;

			ISdpNodeContainer *pProtocolContainer =
				protocolDescriptor.u.container;
			int cProtocolAtoms = 0;
			pProtocolContainer->GetNodeCount((DWORD *)&cProtocolAtoms);

			for (int k = 0; (nChannel == 0) && (k < cProtocolAtoms); k++)
			{
				NodeData nodeAtom;
				pProtocolContainer->GetNode(k,&nodeAtom);

				if (IsRfcommUuid(&nodeAtom))
				{
					if (k+1 == cProtocolAtoms)
					{
						// Error: Channel ID should follow RFCOMM uuid
						break;
					}

					NodeData channelID;
					pProtocolContainer->GetNode(k+1,&channelID);

					switch(channelID.specificType)
					{
					case SDP_ST_UINT8:
						*nChannel = channelID.u.uint8;
						break;
					case SDP_ST_INT8:
						*nChannel = channelID.u.int8;
						break;
					case SDP_ST_UINT16:
						*nChannel = channelID.u.uint16;
						break;
					case SDP_ST_INT16:
						*nChannel = channelID.u.int16;
						break;
					case SDP_ST_UINT32:
						*nChannel = channelID.u.uint32;
						break;
					case SDP_ST_INT32:
						*nChannel = channelID.u.int32;
						break;
					default:
						*nChannel = 0;
					}
					break;
				}
			}
		}
		if (protocolList.type == SDP_TYPE_STRING)
			CoTaskMemFree(protocolList.u.str.val);
		else if (protocolList.type == SDP_TYPE_URL)
			CoTaskMemFree(protocolList.u.url.val);
	}

	// cleanup
	for (int i = 0; i < cRecordArg; i++)
		pRecordArg[i]->Release();
	CoTaskMemFree(pRecordArg);

	return (*nChannel != 0) ? S_OK : S_FALSE;
}

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
	rec.bthNsBlob.pRecordHandle = sdpRecordHandle;
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

static int unregisterSDPService(ULONG sdpRecordHandle)
{
	WSAQUERYSET wqs;
	BLOB b;
	ULONG sdpVersion = BTH_SDP_VERSION;
	BTHNS_SETBLOB bthNsBlob;

	memset(&bthNsBlob, 0, sizeof(bthNsBlob));
	bthNsBlob.pRecordHandle = &sdpRecordHandle;
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

// Stolen from BlueCove
static void convertUUIDBytesToGUID(char *bytes, GUID *uuid)
{
	uuid->Data1 = bytes[0] << 24 & 0xff000000 | bytes[1] << 16 & 0x00ff0000 | bytes[2] << 8 & 0x0000ff00 | bytes[3] & 0x000000ff;
	uuid->Data2 = bytes[4] << 8 & 0xff00 | bytes[5] & 0x00ff;
	uuid->Data3 = bytes[6] << 8 & 0xff00 | bytes[7] & 0x00ff;

	for (int i = 0; i < 8; i++) {
		uuid->Data4[i] = bytes[i + 8];
	}
}

/* Some code stolen from BlueCove. */
static int findHaggleService(BT_ADDR *pb)
{
	BTHNS_RESTRICTIONBLOB queryservice;
	unsigned char uuid[] = HAGGLE_BLUETOOTH_SDP_UUID;
	BLOB blob;
	WSAQUERYSET queryset;
	SOCKADDR_BTH sa;
	HANDLE hLookupSearchServices;
	int found = 0;

	memset(&queryservice, 0, sizeof(queryservice));

	queryservice.type = SDP_SERVICE_SEARCH_REQUEST;

	convertUUIDBytesToGUID((char *) uuid, &queryservice.uuids[0].u.uuid128);

	//UUID is full 128 bits
	queryservice.uuids[0].uuidType = SDP_ST_UUID128;

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
	sa.addressFamily = AF_BT;
	sa.btAddr = *pb;
	CSADDR_INFO csai;
	memset(&csai, 0, sizeof(csai));
	csai.RemoteAddr.lpSockaddr = (sockaddr *) &sa;
	csai.RemoteAddr.iSockaddrLength = sizeof(sa);
	queryset.lpcsaBuffer = &csai;

	// begin query

	if (WSALookupServiceBegin(&queryset, 0, &hLookupSearchServices)) {
		int last_error = WSAGetLastError();
		CM_DBG("WSALookupServiceBegin error [%s]", StrError(last_error));
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

	if (WSALookupServiceNext(hLookupSearchServices, LUP_RETURN_TYPE | LUP_RETURN_BLOB | LUP_RETURN_ADDR, &size, pwsaResults)) {
		int last_error = WSAGetLastError();

		switch (last_error) {
		case WSANO_DATA:
			//HAGGLE_DBG("SDP: no data\n");
			found = -1;
			break;
		case WSA_E_NO_MORE:
			//HAGGLE_DBG("End of SDP list\n");
			break;
		case WSAENOTCONN:
			found = -1;
			HAGGLE_DBG("Could not connect to SDP service\n");
			break;
		case WSASERVICE_NOT_FOUND:
			found = 0;
			HAGGLE_DBG("SDP Service not found\n");
			break;
		default:
			CM_DBG("WSALookupServiceNext error [%s]\n", StrError(last_error));
			found = -1;
		}
	} else {
		unsigned long cChannel = 0;
		found = 1;
		HAGGLE_DBG("Found Haggle service\n");
		if (ERROR_SUCCESS == FindRFCOMMChannel(
			pwsaResults->lpBlob->pBlobData,
			pwsaResults->lpBlob->cbSize,
			&cChannel)) {
			HAGGLE_DBG("RFCOMM channel is %d\n", (int)cChannel);
			found = cChannel;
		}
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

void bluetoothDiscovery(ConnectivityBluetooth *conn)
{
	//BOOL bHaveName;
	HANDLE hScan = INVALID_HANDLE_VALUE;
	WSAQUERYSET wsaq;
	BTHNS_INQUIRYBLOB queryBlob;
	queryBlob.LAP = BT_ADDR_GIAC;
	queryBlob.num_responses = 10;
	queryBlob.length = 12;
	BLOB blob;
	int count = 0;
	BthInquiryResult *p_inqRes;
	union {
		CHAR buf[4096];
		SOCKADDR_BTH __unused;
	};
	const BluetoothAddress *addr;

	blob.cbSize = sizeof(queryBlob);
	blob.pBlobData = (BYTE *) & queryBlob;

	ZeroMemory(&wsaq, sizeof(wsaq));
	wsaq.dwSize = sizeof(wsaq);
	wsaq.dwNameSpace = NS_BTH;
	wsaq.lpcsaBuffer = NULL;
	wsaq.lpBlob = &blob;
	
	addr = conn->rootInterface->getAddress<BluetoothAddress>();

	if (!addr)
		return;

	CM_DBG("Doing scan on device %s - %s\n", conn->rootInterface->getName(), addr->getStr());

	if (ERROR_SUCCESS != WSALookupServiceBegin(&wsaq, LUP_CONTAINERS, &hScan)) {
		CM_DBG("WSALookupServiceBegin failed\n");
		return;
	}
	// loop the results
	while (!conn->shouldExit()) {
		DWORD dwSize = sizeof(buf);
		LPWSAQUERYSET pwsaResults = (LPWSAQUERYSET) buf;
		ZeroMemory(pwsaResults, sizeof(WSAQUERYSET));
		pwsaResults->dwSize = sizeof(WSAQUERYSET);
		pwsaResults->dwNameSpace = NS_BTH;
		pwsaResults->lpBlob = NULL;
		unsigned char macaddr[BT_ALEN];
		string name = "bluetooth device";
		bool report_interface = false;
		InterfaceStatus_t status;

		if (WSALookupServiceNext(hScan, LUP_RETURN_NAME | LUP_RETURN_ADDR | LUP_RETURN_BLOB, 
			&dwSize, pwsaResults) != ERROR_SUCCESS) {
			CM_DBG("Found %d Haggle devices\n", count);
			break;
		}

		p_inqRes = (BthInquiryResult *) pwsaResults->lpBlob->pBlobData;

		BT_ADDR btAddr = ((SOCKADDR_BTH *) pwsaResults->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr;

		if (pwsaResults->lpszServiceInstanceName && *(pwsaResults->lpszServiceInstanceName)) {
			int namelen = wcslen(pwsaResults->lpszServiceInstanceName) + 1;
			char *tmp = new char[namelen];
			wcstombs(tmp, pwsaResults->lpszServiceInstanceName, namelen);
			name.clear();
			name = tmp;
			delete[] tmp;
		}

		btAddr2Mac(btAddr, macaddr);
		BluetoothAddress addr(macaddr);

		status = conn->is_known_interface(Interface::TYPE_BLUETOOTH, macaddr);

		if (status == INTERFACE_STATUS_HAGGLE) {
			report_interface = true;
		} else if (status == INTERFACE_STATUS_UNKNOWN) {
			//int ret = DetectRFCommChannel(&btAddr);
			int ret = findHaggleService(&btAddr);
			
			if (ret > 0) {
				report_interface = true;
				conn->report_known_interface(Interface::TYPE_BLUETOOTH, macaddr, true);
			} else if (ret == 0) {
				conn->report_known_interface(Interface::TYPE_BLUETOOTH, macaddr, false);
			}
		}
		if (report_interface) {
			CM_DBG("Found Haggle Bluetooth device [%s:%s]\n", 
				addr.getStr(), name.c_str());

			BluetoothInterface foundInterface(macaddr, name, &addr, IFFLAG_UP);

			conn->report_interface(&foundInterface, 
                                               conn->rootInterface, new ConnectivityInterfacePolicyTTL(2));
			count++;
		} else {
			CM_DBG("Bluetooth device [%s] is not a Haggle device\n", 
				addr.getStr());
		}
	}

	// cleanup
	WSALookupServiceEnd(hScan);

	return;
}

void ConnectivityBluetooth::hookCleanup()
{
	unregisterSDPService(sdpRecordHandle);
}

bool ConnectivityBluetooth::run()
{
	CoInitializeEx (0, COINIT_MULTITHREADED);

	if (registerSDPService(&sdpRecordHandle) < 0) {
		CM_DBG("WSAGetLastError=%s\n", StrError(WSAGetLastError()));
		fflush(stdout);
		CM_DBG("Could not register SDP service\n");
		return false;
	}

	cancelableSleep(5000);

	while (!shouldExit()) {
		
		bluetoothDiscovery(this);

#define BLUETOOTH_FORCE_DISCOVERABLE_MODE

#ifdef BLUETOOTH_FORCE_DISCOVERABLE_MODE
		DWORD mode;

		if (BthGetMode(&mode) == ERROR_SUCCESS) {
			/* If we are in connectable mode, but
			not discoverable, then we force the
			mode into discoverable. However, we
			honor the radio being in off mode. */
			if (mode == BTH_CONNECTABLE) {
				HAGGLE_DBG("Bluetooth is in connectable mode; forcing discoverable mode\n");
				
				if (BthSetMode(BTH_DISCOVERABLE) != ERROR_SUCCESS) {
					HAGGLE_ERR("Could not switch Bluetooth from connectable to discoverable mode\n");
				}
			}
		}
#endif
		age_interfaces(rootInterface);

		cancelableSleep(TIME_TO_WAIT * 1000);
	}

	return false;
}

#endif
