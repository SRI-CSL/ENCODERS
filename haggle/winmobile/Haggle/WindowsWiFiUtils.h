/* Copyright 2008 Uppsala University
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

#ifndef _WINDOWSWIFIUTILS_H_
#define _WINDOWSWIFIUTILS_H_

#include <libcpphaggle/PlatformDetect.h>

#if defined(OS_WINDOWS_MOBILE)

#include <haggleutils.h>

#include <pm.h>
#include <Winsock2.h>
#include <ntddndis.h>
#include <iphlpapi.h>

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

#define WIFI_ALEN 6
#define DEVNAME_LEN (64)

/*
 This code is based on code originally implemented by people at SUPSI, Switzerland.

*/
typedef char ssid_t[NDIS_802_11_LENGTH_SSID];
typedef unsigned char bssid_t[WIFI_ALEN];

class WindowsWiFiUtils
{
private:
	/*
	Handle to NDIS
	*/
	static HANDLE hAdapter;
	// cache latest adapter index and name for efficiency
	static unsigned long adapterIndex; 
	static wchar_t adapterName[DEVNAME_LEN];
	static unsigned long adapterNameLen;

	/**
	Purpose of this field
	*/
	static TCHAR _powerManagNDISMiniport[];

	/**
	Context when setting IP addresses
	*/
	static ULONG _NTEContext;
	

	/**
	init() - Create a handle to the NDISUIO driver.
	Note that we perform a FILE_SHARE_READ and a FILE_SHARE_WRITE
	meaning that subsequent open operations on the object succeed
	only if read/write access is requested
	*/
	static bool init();

	/**
	fini() - Closes the HANDLE to the NDISUIO driver
	*/
	static void fini();


	/**
	Constructor and Deconstructor are private to aviod creating objects
	*/
	WindowsWiFiUtils();
	~WindowsWiFiUtils();

	static WindowsWiFiUtils instance;
public:

	/**
	GetWLANStrength() - Gets a list of all APs within range of this station in a list of AP_DATA
	*/
	static bool getWLANStrength(unsigned long adapterIndex, const bool wifiCurrentlyUsed, PNDIS_802_11_BSSID_LIST* SSID);

	/**
	Support function for AP/Ad-Hoc Connection Method
	*/
	static bool setAuthMode(unsigned long adapterIndex, const unsigned int mode);
	/**
	Support function for AP/Ad-Hoc Connection Method
	*/
	static bool setEncryptionMode(unsigned long adapterIndex, const unsigned int mode);

	/**
	Get current SSID
	*/
	static bool getCurrentSSID(unsigned long adapterIndex, ssid_t *ssid);

	/**
	AP/Ad-Hoc Connection Method
	*/
	static bool setWiFiSSID(unsigned long adapterIndex, const ssid_t ssid, const int *wepKey = NULL);

	/**
	Function: SetBSSID
	Purpose:  to associate to the access point with specific MAC address.
	Return:   S_OK for success. Otherwise, a standard HRESULT error will be returned.  			 
	*/
	static bool setBSSID(unsigned long adapterIndex, const bssid_t bssid);
	static bool queryBSSID(unsigned long adapterIndex, bssid_t *bssid);

	/**
	Disassociate Wireless Device from AP
	*/
	static bool disassociate(unsigned long adapterIndex);

	/**
	Set its network mode to Ad Hoc or Infrastructure mode
	*/
	static bool setNetworkMode(unsigned long adapterIndex, NDIS_802_11_NETWORK_INFRASTRUCTURE networkMode);
	static bool queryNetworkMode(unsigned long adapterIndex, unsigned int *mode);
	static bool setChannel(unsigned long adapterIndex, unsigned short channel = 11);

	/**
	Get General Information about the Wireless Adapter
	*/
	static bool getInterfacesInfo(const char *wifiName, unsigned char mac[WIFI_ALEN], int *ifIndex);

	/**
	Add IP Address to a Wireless Adapter
	*/
	static bool addIPv4Address(unsigned long adapterIndex, const char *dottedInetAddress, const char *dottedMaskInetAddress);

	/**
	Delete IP Address to a Wireless Adapter
	*/
	static void deleteIPv4Address();

	/**
	Get the index of the Wireless  Adapter (NDISUIO)
	*/
	static bool getDeviceIndex(const char* deviceName, unsigned long *ifindex);
	
	/**
	Get the device name given an index.
	*/ 
	static bool getDeviceName(unsigned long adapterIndex, wchar_t *namebuf, const size_t namebuf_len);
	
	/**
	SetDevicePowertOState() - Sets the device power state for WiFi device
	D0 is "Full On". MSDN suggests to call SetPowerRequirement call
	but it doesn't work!!!											
	The value {98C5250D-C29A-4985-AE5F-AFE5367E5006} is the			
	"Power-manageable NDIS miniports" and is stored in the RegKey value:
	SYSTEM\\CurrentControlSet\\Control\\Power						
	*/
	static void setDevicePowerToState(CEDEVICE_POWER_STATE state, WCHAR* devName);

	/**
	getDevicePowerState() - Gets the device power state for WiFi
	*/
	static CEDEVICE_POWER_STATE getDevicePowerToState(TCHAR devName[]);

	static bool SSIDCheck(const char* sSSID);
};

#endif

#endif
