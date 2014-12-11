#include "WindowsWiFiUtils.h"

#if defined(OS_WINDOWS)
#include <libcpphaggle/Platform.h>
#include <nuiouser.h>

HANDLE WindowsWiFiUtils::hAdapter = INVALID_HANDLE_VALUE;
unsigned long WindowsWiFiUtils::adapterIndex = 0;
unsigned long WindowsWiFiUtils::adapterNameLen = 0;
wchar_t WindowsWiFiUtils::adapterName[DEVNAME_LEN] = { 0 };
TCHAR WindowsWiFiUtils::_powerManagNDISMiniport[] = TEXT("{98C5250D-C29A-4985-AE5F-AFE5367E5006}\\");
ULONG WindowsWiFiUtils::_NTEContext = 0;
WindowsWiFiUtils WindowsWiFiUtils::instance;

static bool charToWchar(const char* sSource, WCHAR *sDestination, const size_t sDestinationLen)
{
	if (sSource == NULL || sDestination == NULL)
		return false;

	const size_t iOriginalSize = strlen(sSource) + 1;

	if (sDestinationLen < iOriginalSize)
		return false;

	memset(sDestination, 0, sizeof(WCHAR) * iOriginalSize);

	mbstowcs(sDestination, sSource, iOriginalSize);

	return true;
}

static bool wcharToChar(const WCHAR* sSource, char *sDestination, const size_t sDestinationLen)
{
	if (sSource == NULL || sDestination == NULL)
		return false;

	const size_t iOriginalSize = wcslen(sSource) + 1;

	if (sDestinationLen < iOriginalSize)
		return false;

	memset(sDestination, 0, sizeof(char) * iOriginalSize);

	wcstombs(sDestination , sSource, iOriginalSize);

	return true;
}

WindowsWiFiUtils::WindowsWiFiUtils()
{
	printf("Initializing NDIS connection...\n");

	if (!init()) {
		fprintf(stderr, "NDIS Initialization failed\n");
	}
}

WindowsWiFiUtils::~WindowsWiFiUtils()
{
	if (hAdapter != INVALID_HANDLE_VALUE) {
		printf("Closing NDIS connection...\n");
		fini();
	}
}

bool WindowsWiFiUtils::getWLANStrength(unsigned long adapterIndex, const bool wifiCurrentlyUsed, PNDIS_802_11_BSSID_LIST* SSID)
{
	/**
	In order to avoid problem IOCTL_BSSID_LIST failed: 1784
	The supplied user buffer is not valid for the requested operation. ERROR_INVALID_USER_BUFFER 
	This buffer is set from 1024 to 4096 and then to 8192 as more than 25 ap can be detected
	simultaneously (4096 is not enough). Each BSSID in the list require not less than 216 bytes. 
	*/

	DWORD m_dwBytesReturned;
	UCHAR QueryBuffer[8192];
	wchar_t devname[DEVNAME_LEN];

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	/**
	If this OID_802_11_BSSID_LIST is queried 6 seconds or longer after OID_802_11_BSSID_LIST_SCAN is set, 
	the list of BSSIDs must also contain all of the BSSIDs found during the most recent network scan.
	*/

	if (!wifiCurrentlyUsed) {
		PNDISUIO_SET_OID pSetOid;
		pSetOid = (PNDISUIO_SET_OID) &QueryBuffer[0];
		pSetOid->Oid = OID_802_11_BSSID_LIST_SCAN;
		pSetOid->ptcDeviceName = devname;

		if (DeviceIoControl(hAdapter,
			IOCTL_NDISUIO_SET_OID_VALUE,
			(LPVOID) &QueryBuffer[0],
			sizeof(QueryBuffer),
			(LPVOID) &QueryBuffer[0],
			0,
			&m_dwBytesReturned,
			NULL) == 0) {
			DWORD m_dwError = GetLastError();

			fprintf(stderr, "ERROR IOCTL SET BSSID_LIST_SCAN failed: %u\n", m_dwError);

			return false;
		}

		memset(QueryBuffer,0,sizeof(QueryBuffer));
	}

	PNDISUIO_QUERY_OID pQueryOid;
	PNDIS_802_11_BSSID_LIST	pBssid_List;

	pQueryOid = (PNDISUIO_QUERY_OID) &QueryBuffer[0];

	pQueryOid->Oid = OID_802_11_BSSID_LIST;

	/**
	NOT SUPPORTED in Windows Mobile. The pDeviceName must be set instead!!
	pQueryOid->ptcDeviceName=NULL; 
	*/
	pQueryOid->ptcDeviceName = devname;
	
	if (DeviceIoControl(hAdapter,
		IOCTL_NDISUIO_QUERY_OID_VALUE,
		(LPVOID) &QueryBuffer[0],
		sizeof(QueryBuffer),
		(LPVOID) &QueryBuffer[0],
		sizeof(QueryBuffer),
		&m_dwBytesReturned, //216 bytes at least
		NULL) != 0) {

		pBssid_List = (PNDIS_802_11_BSSID_LIST)pQueryOid->Data;

		if ((pBssid_List->NumberOfItems > 0) && (pBssid_List->NumberOfItems < 100)) {
			// Make a pointer to a BSSID item
			PNDIS_WLAN_BSSID pBssid = NULL;

			if (pBssid_List->NumberOfItems > 0)
				pBssid = &(pBssid_List->Bssid[0]);

			*SSID = pBssid_List;
		}
		else {
			fprintf(stderr, "OID_802_11_BSSID_LIST ERROR. The list is empty\n");
			return false;
		}
	} else {

		fprintf(stderr, "ERROR IOCTL BSSID_LIST failed: %u\n", GetLastError());
		return false;
	}

	return true;
}

bool WindowsWiFiUtils::setAuthMode(unsigned long adapterIndex, const unsigned int mode)
{
	wchar_t devname[DEVNAME_LEN];
	struct {
		NDISUIO_SET_OID so;
		NDIS_802_11_AUTHENTICATION_MODE mode;
	} msg;

	if (mode >= Ndis802_11AuthModeMax)
		return false;

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(&msg, 0, sizeof(msg));
	msg.so.Oid = OID_802_11_AUTHENTICATION_MODE;
	msg.so.ptcDeviceName = devname;
	msg.mode = (NDIS_802_11_AUTHENTICATION_MODE)mode;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) == 0) {
		return false;
	}

	return true;
}

bool WindowsWiFiUtils::setEncryptionMode(unsigned long adapterIndex, const unsigned int mode)
{
	wchar_t devname[DEVNAME_LEN];
	struct {
		NDISUIO_SET_OID so;
		NDIS_802_11_ENCRYPTION_STATUS mode;
	} msg;

	// Check max value as found in ntddndis.h
	if (mode > 7)
		return false;

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(&msg, 0, sizeof(msg));
	msg.so.Oid = OID_802_11_ENCRYPTION_STATUS;
	msg.so.ptcDeviceName = devname;
	msg.mode = (NDIS_802_11_ENCRYPTION_STATUS)mode;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) == 0) {
		fprintf(stderr, "Could not set encryption mode=%u\n", mode);
		return false;
	}

	return true;
}

bool WindowsWiFiUtils::getCurrentSSID(unsigned long adapterIndex, ssid_t *ssid)
{
	DWORD bytes;
	wchar_t devname[DEVNAME_LEN];
	struct {
		NDISUIO_QUERY_OID qo;
		NDIS_802_11_SSID ssid;
	} msg;

	if (!ssid || !getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(*ssid, 0, sizeof(ssid_t));
	memset(&msg, 0, sizeof(msg));
	msg.qo.Oid = OID_802_11_SSID;
	msg.qo.ptcDeviceName = devname;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_QUERY_OID_VALUE, &msg, sizeof(msg), &msg, sizeof(msg),&bytes,  NULL) != 0) {
		memcpy(*ssid, msg.ssid.Ssid, msg.ssid.SsidLength);
	} else {
		fprintf(stderr, "ERROR GET SET SSID: %u\n", GetLastError());
		return false;
	}

	return true;
}

bool WindowsWiFiUtils::setWiFiSSID(unsigned long adapterIndex, const ssid_t ssid, const int *wepKey)
{
	wchar_t devname[DEVNAME_LEN];
	int auth = wepKey ? Ndis802_11AuthModeShared : Ndis802_11AuthModeOpen;
	int encrypt = wepKey ? Ndis802_11Encryption1Enabled : Ndis802_11EncryptionDisabled;


	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	if (!setAuthMode(adapterIndex, auth)) {
		fprintf(stderr, "SetAuthMode() failed : %u\n", GetLastError());
		return false;
	}

	if (!setEncryptionMode(adapterIndex, encrypt)) {
		fprintf(stderr, "setEncryptionMode() failed : %u\n", GetLastError());
		return false;
	}

	if (wepKey) {
		struct {
			NDISUIO_SET_OID so;
			NDIS_802_11_WEP wep;
			char data[256];
		} msg;

		memset(&msg, 0, sizeof(msg));

		/** 
		Why this line of code if commented?
		wep.KeyLength = _msize(wK)/sizeof(int); // key length
		*/

		msg.wep.KeyLength = 5;

		/**
		int *wepKey = wK; 
		*/

		UCHAR* wepKeyMaterial = new UCHAR[msg.wep.KeyLength];

		for (unsigned int h = 0; h < msg.wep.KeyLength; h++) {
			wepKeyMaterial[h]= (unsigned char)(wepKey[h]);
		}

		/**
		THIS IS THE RIGHT MODE TO CALCULATE THE LENGTH
		*/

		msg.wep.Length = FIELD_OFFSET(NDIS_802_11_WEP, KeyMaterial) + msg.wep.KeyLength;

		/**
		Why first assign to 0 and then OR for setting a bit...directly assign the value

		wep.KeyIndex = 0; // Was 0 1..N for the global keys 0 and for per-client key
		wep.KeyIndex = wep.KeyIndex | 0x80000000; //set bit 31 to specify that it is a transmit key
		*/
		msg.wep.KeyIndex = 0x80000000;

		memcpy(msg.wep.KeyMaterial, wepKeyMaterial, msg.wep.KeyLength);

		delete wepKeyMaterial;

		/*
		Why after ? 
		wep.KeyIndex = 0;
		wep.KeyIndex = wep.KeyIndex | 0x80000000;
		*/
		msg.wep.KeyIndex = 0x80000000;
		msg.so.Oid = OID_802_11_ADD_WEP;
		msg.so.ptcDeviceName = devname;

		if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) == 0) {
			fprintf(stderr, "ERROR IOCTL OID_802_11_ADD_WEP failed: %u\n", GetLastError());
			return false;
		}
	}
	/* Set the network SSID */
	struct {
		NDISUIO_SET_OID so;
		NDIS_802_11_SSID ssid;
	} msg;

	memset(&msg, 0, sizeof(msg));
	msg.so.Oid = OID_802_11_SSID;
	msg.so.ptcDeviceName = devname;
	msg.ssid.SsidLength = strlen(ssid);

	memcpy(msg.ssid.Ssid, ssid, msg.ssid.SsidLength);

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) == 0) {
		fprintf(stderr, "ERROR IOCTL SET SSID failed: %u\n", GetLastError());
		return false;
	}

	return true;
}

bool WindowsWiFiUtils::setBSSID(unsigned long adapterIndex, const bssid_t bssid)
{
	int auth = Ndis802_11AuthModeOpen;
	wchar_t devname[DEVNAME_LEN];
	struct {
		NDISUIO_SET_OID so;
		NDIS_802_11_MAC_ADDRESS bssid;
	} msg;

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	if (!setAuthMode( adapterIndex, auth)) {
		fprintf(stderr, "setAuthMode() failed : %u\n", GetLastError());
		return false;
	}

	if (!setEncryptionMode(adapterIndex, Ndis802_11EncryptionDisabled)) {
		fprintf(stderr, "setEncryptionMode() failed : %u", GetLastError());
		return false;
	}
	memset(&msg, 0, sizeof(msg));
	msg.so.Oid = OID_802_11_BSSID;
	msg.so.ptcDeviceName = devname;
	memcpy(&msg.bssid, bssid, sizeof(NDIS_802_11_MAC_ADDRESS));

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) == 0) {
		fprintf(stderr, "Error setting BSSID: %u\n", GetLastError());
		return false;
	}

	return true;
}

bool WindowsWiFiUtils::queryBSSID(unsigned long adapterIndex, bssid_t *bssid)
{
	wchar_t devname[DEVNAME_LEN];
	DWORD bytes;
	struct {
		NDISUIO_QUERY_OID qo;
		NDIS_802_11_MAC_ADDRESS bssid;
	} msg;

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(&msg, 0, sizeof(msg));
	msg.qo.Oid = OID_802_11_BSSID;
	msg.qo.ptcDeviceName = devname;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_QUERY_OID_VALUE, &msg, sizeof(msg), &msg, sizeof(msg), &bytes, NULL) != 0) {
		memcpy(*bssid, msg.bssid, sizeof(msg.bssid));
	} else {
		fprintf(stderr, "ERROR GET CURRENT BSSID: %u\n", GetLastError());
		return false;
	}
	return true;
}

bool WindowsWiFiUtils::disassociate(unsigned long adapterIndex)
{
	wchar_t devname[DEVNAME_LEN];
	NDISUIO_SET_OID so;

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(&so, 0, sizeof(so));
	so.Oid = OID_802_11_DISASSOCIATE;
	so.ptcDeviceName = devname;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &so, sizeof(so), NULL, 0, NULL, NULL) == 0) {
		fprintf(stderr, "IOCTL DISASSOCIATE failed: %u\n", GetLastError());
		return false;
	}
	return true;
}

bool WindowsWiFiUtils::setNetworkMode(unsigned long adapterIndex, NDIS_802_11_NETWORK_INFRASTRUCTURE networkMode)
{
	wchar_t devname[64];
	struct {
		NDISUIO_SET_OID pSetOid;
		NDIS_802_11_NETWORK_INFRASTRUCTURE mode;
	} msg;

	if (!getDeviceName(adapterIndex, devname, 64))
		return false;

	memset(&msg, 0, sizeof(msg));
	msg.pSetOid.Oid = OID_802_11_INFRASTRUCTURE_MODE;
	msg.pSetOid.ptcDeviceName = devname;
	msg.mode = networkMode;

	printf("Setting network mode %u on adapter %S\n", msg.mode, msg.pSetOid.ptcDeviceName);

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) == 0) {
		fprintf(stderr, "Error: %u setting network mode on device\n", GetLastError());
		return false;
	}
	return true;
}

bool WindowsWiFiUtils::getInterfacesInfo(const char *wifiName, unsigned char mac[WIFI_ALEN], int *ifIndex)
{
	/**
	It is possible for an adapter to have multiple
	IPv4 addresses, gateways, and secondary WINS servers
	assigned to the adapter. 

	Note that this sample code only prints out the 
	first entry for the IP address/mask, and gateway, and
	the primary and secondary WINS server for each adapter. 
	*/

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;	

	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *) MALLOC(sizeof (IP_ADAPTER_INFO));

	if (pAdapterInfo == NULL) {
		fprintf(stderr, "Error allocating memory needed to call GetAdaptersinfo\n");
		return false;
	}

	/**
	Make an initial call to GetAdaptersInfo to get
	the necessary size into the ulOutBufLen variable
	*/

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) MALLOC(ulOutBufLen);

		if (pAdapterInfo == NULL) {
			fprintf(stderr, "Error allocating memory needed to call GetAdaptersinfo\n");
			return false;
		}

	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;

		while (pAdapter != NULL) {
			if (strcmp(wifiName, pAdapter->AdapterName) == 0) {
				memcpy(mac, pAdapter->Address, pAdapter->AddressLength);
				*ifIndex = pAdapter->Index;
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}  else  {
		fprintf(stderr, "GetAdaptersInfo() failed with error: %u\n", dwRetVal);
	}

	if (pAdapterInfo)
		FREE(pAdapterInfo);

	return true;
}

bool WindowsWiFiUtils::addIPv4Address(unsigned long adapterIndex, const char *dottedInetAddress, const char *dottedMaskInetAddress)
{
	UINT dwInetAddr = inet_addr(dottedInetAddress);
	UINT dwMmaskInetAddr = inet_addr(dottedMaskInetAddress);
	ULONG NTEInstance = 0;

	if (AddIPAddress(dwInetAddr, dwMmaskInetAddr, adapterIndex, &_NTEContext, &NTEInstance) != NO_ERROR) {
		fprintf(stderr, "addIPAddress error: %u\n", GetLastError() );
		return false;
	}

	printf("addIPAddress succesfully DONE\n");
	return true;

}

void WindowsWiFiUtils::deleteIPv4Address (){

	if (_NTEContext == 0)
		return;

	if (DeleteIPAddress( _NTEContext ) == NO_ERROR) {
		printf("IPv4 address was successfully deleted.\n");
	} else {
		fprintf(stderr, "DeleteIPAddress failed with error: %d\n",GetLastError());
	} 
}


bool WindowsWiFiUtils::init()
{

	// Create a handle to the NDISUIO driver
	hAdapter = CreateFile(NDISUIO_DEVICE_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,//FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		(HANDLE)INVALID_HANDLE_VALUE);

	if (hAdapter == INVALID_HANDLE_VALUE) {
		return false;
	}

	return true;
}


void WindowsWiFiUtils::fini()
{
	if (hAdapter != INVALID_HANDLE_VALUE) {
		CloseHandle(hAdapter);
	}

	hAdapter = INVALID_HANDLE_VALUE;
}

bool WindowsWiFiUtils::getDeviceName(unsigned long adapterIndex, wchar_t *namebuf, const size_t namebuf_len)
{
	DWORD bytes;
	struct {
		NDISUIO_QUERY_BINDING q;
		char data[512];
	} msg;

	// Did we cache the name of this adapter?
	if (adapterIndex == WindowsWiFiUtils::adapterIndex) {
		if (namebuf_len <= WindowsWiFiUtils::adapterNameLen)
			return false;

		memcpy(namebuf, WindowsWiFiUtils::adapterName, WindowsWiFiUtils::adapterNameLen * sizeof(wchar_t));
		return true;
	}

	// Request the name from NDIS
	memset(&msg, 0, sizeof(msg));
	msg.q.BindingIndex = adapterIndex - 1;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_QUERY_BINDING, &msg, sizeof(msg), &msg, sizeof(msg), &bytes, NULL) != 0) {
		if (msg.q.DeviceNameLength > namebuf_len - 1) {
			fprintf(stderr, "Name buffer is too small for device name (len=%u)\n", msg.q.DeviceNameLength);
			return false;
		}
		// Cache the result for next time
		memcpy(WindowsWiFiUtils::adapterName, ((char *)&msg) + msg.q.DeviceNameOffset, msg.q.DeviceNameLength * sizeof(wchar_t));
		WindowsWiFiUtils::adapterNameLen = msg.q.DeviceNameLength;
		WindowsWiFiUtils::adapterIndex = adapterIndex;

		// Copy to return buffer
		memcpy(namebuf, ((char *)&msg) + msg.q.DeviceNameOffset, msg.q.DeviceNameLength * sizeof(wchar_t));
	} else {
		fprintf(stderr, "Could not find device name of adapter with index=%u\n", adapterIndex);
		return false;
	}

	printf("Device name of adapter with index=%u is %S\n", adapterIndex, namebuf);

	return true;
}

bool WindowsWiFiUtils::getDeviceIndex(const char* deviceName, unsigned long *ifindex)
{
	WCHAR *devname = new WCHAR[strlen(deviceName) + 1];

	if (!devname)
		return false;

	if (!charToWchar(deviceName, devname, strlen(deviceName) + 1)) {
		delete [] devname;
		return false;
	}

	delete [] devname;

	return (GetAdapterIndex(devname, ifindex) == NO_ERROR);
}

void WindowsWiFiUtils::setDevicePowerToState(CEDEVICE_POWER_STATE state, WCHAR* devName)
{
	TCHAR temp[1024];
	wcscpy(temp, _powerManagNDISMiniport);
	wcscat(temp,devName);
	SetDevicePower(temp, POWER_NAME, state);
}

CEDEVICE_POWER_STATE WindowsWiFiUtils::getDevicePowerToState(TCHAR devName[])
{

	CEDEVICE_POWER_STATE state;

	TCHAR temp[1024];
	wcscpy(temp, _powerManagNDISMiniport);
	wcscat(temp,devName);
	GetDevicePower(temp, POWER_NAME, &state);

	return state;
}

bool WindowsWiFiUtils::queryNetworkMode(unsigned long adapterIndex, unsigned int *mode)
{
	wchar_t devname[DEVNAME_LEN];
	struct {
		NDISUIO_QUERY_OID qo;
		NDIS_802_11_NETWORK_INFRASTRUCTURE mode;
	} msg;
	DWORD bytes;

	if (!mode || !getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(&msg, 0, sizeof(msg));
	msg.qo.Oid = OID_802_11_INFRASTRUCTURE_MODE;
	msg.qo.ptcDeviceName = devname;

	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_QUERY_OID_VALUE, &msg, sizeof(msg), 
		&msg, sizeof(msg), &bytes, NULL) != 0) {
			*mode = msg.mode;
	} else {
		fprintf(stderr, "Unable to query current network mode. Errorcode: %d\n", GetLastError());
		return false;
	}
	return true;
}

bool WindowsWiFiUtils::setChannel(unsigned long adapterIndex, unsigned short channel)
{
	wchar_t devname[DEVNAME_LEN];
	struct {
		NDISUIO_SET_OID so;
		NDIS_802_11_CONFIGURATION config;
	} msg;

	if (channel < 1 || channel > 13) {
		fprintf(stderr, "Invalid channel=%u\n", channel);	
		return false;
	}

	if (!getDeviceName(adapterIndex, devname, DEVNAME_LEN))
		return false;

	memset(&msg, 0, sizeof(msg));
	msg.so.Oid = OID_802_11_CONFIGURATION;
	msg.so.ptcDeviceName = devname;

	//2412000 = 1; 2417000 = 2; 2422000 = 3; 2427000 = 4; 2432000 = 5, and so forth...
	msg.config.DSConfig = 2412000 + (5000 * (channel - 1));
	
	if (DeviceIoControl(hAdapter, IOCTL_NDISUIO_SET_OID_VALUE, &msg, sizeof(msg), NULL, 0, NULL, NULL) != 0) {
			printf("Channel %u [%lu] set\n", channel, msg.config.DSConfig);
			return true;
	} else {
		fprintf(stderr, "Got an error in setting channel: %d\n", GetLastError());
	}
	return false;
}

bool WindowsWiFiUtils::SSIDCheck(const char* sSSID)
{
	//The following if limit the problem of incorrect RSSI determination to 110/2^32 = 0.00000256%

	int iSSIDLenght = strlen(sSSID);

	if (iSSIDLenght == 0) 
		return false;

	for (int i = 0; i < iSSIDLenght; i++) {
		//<32 to include the SPACE character that could be used in some SSID
		if ( sSSID[i] < 32 || sSSID[i] > 126 ) {
			return false;
		}
	}

	return true;

}

#endif /* OS_WINDOWS_MOBILE */
