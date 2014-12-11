#include <libcpphaggle/Platform.h>
#if defined(OS_WINDOWS_MOBILE) && defined(UU_EXPERIMENT)
#include "WindowsWiFiUtils.h"
#include <Interface.h>
#include <Utility.h>
#include <stdio.h>
#include <connmgr.h>

#define EXPERIMENT_SSID "HaggleExperiment"
#define EXPERIMENT_CHANNEL (6)

static struct interface_config {
	const char *name;
	const unsigned char mac[6];
	const char *ip;
	const char *mask;
} interface_config[] = {
	{ "LG-1", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.1", "255.255.255.0" },
	{ "LG-2", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.2", "255.255.255.0" },
	{ "LG-3", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.3", "255.255.255.0" },
	{ "LG-4", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.4", "255.255.255.0" },
	{ "LG-5", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.5", "255.255.255.0" },
	{ "LG-6", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.6", "255.255.255.0" },
	{ "LG-7", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.7", "255.255.255.0" },
	{ "LG-8", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.8", "255.255.255.0" },
	{ "LG-9", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.9", "255.255.255.0" },
	{ "LG-10", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.10", "255.255.255.0" },
	{ "LG-11", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.11", "255.255.255.0" },
	{ "LG-12", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.12", "255.255.255.0" },
	{ "LG-13", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.13", "255.255.255.0" },
	{ "LG-14", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.14", "255.255.255.0" },
	{ "LG-15", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.15", "255.255.255.0" },
	{ "LG-16", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.16", "255.255.255.0" },
	{ "LG-17", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.17", "255.255.255.0" },
	{ "LG-18", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.18", "255.255.255.0" },
	{ "LG-19", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.19", "255.255.255.0" },
	{ "LG-20", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.20", "255.255.255.0" },
	{ "LG-21", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.21", "255.255.255.0" },
	{ "LG-22", { 0x00, 0x02, 0x78, 0x25, 0x54, 0x01 }, "192.168.4.22", "255.255.255.0" },
	{ "LG-23", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.23", "255.255.255.0" },
	{ "LG-24", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.24", "255.255.255.0" },
	{ "LG-25", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.25", "255.255.255.0" },
	{ "LG-26", { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, "192.168.0.26", "255.255.255.0" },
};

class UUExperimentSetup {
private:
	static UUExperimentSetup instance;
	UUExperimentSetup();
	~UUExperimentSetup();
};

UUExperimentSetup UUExperimentSetup::instance;

static struct interface_config *get_interface_config(const char mac[6])
{
	for (int i = 0; i < sizeof(interface_config); i++) {
		if (memcmp(interface_config[i].mac, mac, sizeof(mac)) == 0)
			return &(interface_config[i]);
	}
	return NULL;
}

UUExperimentSetup::UUExperimentSetup()
{
	InterfaceRefList iflist;
	InterfaceRef ethIface = NULL;

	// Get all local interfaces, even the ones which are not 'up'
	if (getLocalInterfaceList(iflist, false) < 0) {
		fprintf(stderr, "Could not find any local interfaces\n");
		return;
	}

	// Find the first one returned, it should be the interface we use 
	// for the experiments
	for (InterfaceRefList::iterator it = iflist.begin(); it != iflist.end(); it++) {
		ethIface = *it;

		if (ethIface->getType() == IFTYPE_ETHERNET || ethIface->getType() == IFTYPE_WIFI)
			break;
	}

	if (!ethIface) {
		fprintf(stderr, "No valid local interface to configure\n");
		return;
	}
#if 0
	HRESULT res = S_OK;
	HANDLE hConnMgrReady = ConnMgrApiReadyEvent();

	DWORD retval = WaitForSingleObject(hConnMgrReady, 5000);

	if (retval == WAIT_TIMEOUT) {
		fprintf(stderr, "Connection manager is not ready...\n");
	} else if (retval == WAIT_FAILED) {
		fprintf(stderr, "Wait for Connection manager failed!\n");
	} else {
		for (int i = 0; res == S_OK; i++) {
			CONNMGR_DESTINATION_INFO network_info;

			res = ConnMgrEnumDestinations(i, &network_info);

			if (res == S_OK) {
				printf("Network name %S\n", network_info.szDescription);
			}
		}
	}

	CloseHandle(hConnMgrReady);
#endif
	struct interface_config *ic = get_interface_config(ethIface->getIdentifier());

	if (!ic) {
		fprintf(stderr, "Could not find matching configuration for interface %s\n", 
			ethIface->getIdentifierStr());
		return;
	}

	printf("Configuring interface %s\n", ethIface->getIdentifierStr());

	unsigned long ifindex = 0;
	
	if (!WindowsWiFiUtils::getDeviceIndex(ethIface->getName(), &ifindex)) {
		fprintf(stderr, "Could not get interface index\n");
		return;
	}

	if (!WindowsWiFiUtils::setNetworkMode(ifindex, Ndis802_11IBSS)) {
		fprintf(stderr, "Could not set ad hoc mode\n");
		return;
	}

	if (!WindowsWiFiUtils::setWiFiSSID(ifindex, EXPERIMENT_SSID)) {
		fprintf(stderr, "Could not set WiFi SSID\n");
		return;
	}

	if (!WindowsWiFiUtils::setChannel(ifindex, EXPERIMENT_CHANNEL)) {
		fprintf(stderr, "Could not set WiFi channel to %u\n", EXPERIMENT_CHANNEL);
		return;
	}

	// Add the IPv4 address.
	/*
	if (!WindowsWiFiUtils::addIPv4Address(ifindex, ic->ip, ic->mask)) {
		fprintf(stderr, "Could not set IP address %s/%s\n", ic->ip, ic->mask);
		return false;;
	}
	*/
	printf("Interface %s with mac=%s was configured with ip=%s/%s and SSID=%s on channel=%u\n",
		ethIface->getName(), ethIface->getIdentifierStr(), ic->ip, ic->mask, 
		EXPERIMENT_SSID, EXPERIMENT_CHANNEL);
}

UUExperimentSetup::~UUExperimentSetup()
{

}

#endif
