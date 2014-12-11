#include "WIDCOMMBluetooth.h"

#if defined(WIDCOMM_BLUETOOTH) && defined(ENABLE_BLUETOOTH)

#include <msgqueue.h>
#include "Trace.h"

WIDCOMMBluetooth *WIDCOMMBluetooth::stack = NULL;
WIDCOMMBluetooth::StackInitializer WIDCOMMBluetooth::stackInit;

WIDCOMMBluetooth::StackInitializer::StackInitializer()
{
	WIDCOMMBluetooth::init();
}
WIDCOMMBluetooth::StackInitializer::~StackInitializer()
{
	WIDCOMMBluetooth::cleanup();
}

WIDCOMMBluetooth::WIDCOMMBluetooth() : CBtIf(), hMsgQ(NULL), hStackEvent(CreateEvent(NULL, FALSE, FALSE, NULL)), 
	hInquiryEvent(CreateEvent(NULL, FALSE, FALSE, NULL)), hDiscoveryEvent(CreateEvent(NULL, FALSE, FALSE, NULL)), 
	inquiryCallback(NULL), discoveryCallback(NULL), inquiryData(NULL), discoveryData(NULL), 
	isInInquiry(false), isInDiscovery(false)
{
}

WIDCOMMBluetooth::~WIDCOMMBluetooth()
{
	CloseHandle(hStackEvent);
	CloseHandle(hInquiryEvent);
	CloseHandle(hDiscoveryEvent);
	WIDCOMMSDK_ShutDown();
}

static void linkStatusChanged(BD_ADDR remote_addr, BOOL linkUp)
{
	HAGGLE_DBG("Link status for [%02x:%02x:%02x:%02x:%02x:%02x] is %s\n", 
		remote_addr[0] & 0xff,
		remote_addr[1] & 0xff,
		remote_addr[2] & 0xff,
		remote_addr[3] & 0xff,
		remote_addr[4] & 0xff,
		remote_addr[5] & 0xff,
		linkUp ? "UP" : "DOWN");

}

int WIDCOMMBluetooth::init()
{
	BD_ADDR all_links = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	if (stack)
		return -1;

	printf("Initializing bluetooth stack\n");

	stack = new WIDCOMMBluetooth();

	if (!stack) {
		fprintf(stderr, "Bluetooth stack initialization failed\n");
		return -1;
	}
	
	if (stack->RegisterForLinkStatusChanges(linkStatusChanged, all_links) != TRUE) {
		fprintf(stderr, "Could not register for link status changes\n");
		return -1;
	}

	printf("Stack was initialized\n");

	return 0;
}

void WIDCOMMBluetooth::cleanup()
{
	if (stack) {
		printf("Deleting Bluetooth stack object\n");
		delete stack;
		printf("Bluetooth stack deleted\n");
		stack = NULL;
	}
}

HANDLE WIDCOMMBluetooth::setMsgQ(HANDLE h)
{
	if (!hMsgQ) {
		MSGQUEUEOPTIONS mqOpts = { sizeof(MSGQUEUEOPTIONS), MSGQUEUE_NOPRECOMMIT, 0, 
			sizeof(BTEVENT), FALSE };
		hMsgQ = OpenMsgQueue(GetCurrentProcess(), h, &mqOpts);
	}

	return hMsgQ;
}

bool WIDCOMMBluetooth::resetMsgQ(HANDLE h)
{
	if (hMsgQ && h == hMsgQ) {
		CloseMsgQueue(hMsgQ);
		hMsgQ = NULL;
		return true;
	}
	return false;
}

void WIDCOMMBluetooth::OnStackStatusChange(CBtIf::STACK_STATUS new_status)
{
	BTEVENT bte;

	memset(&bte, 0, sizeof(BTEVENT));

	printf("Stack status change event : status=%lu\n", new_status);

	switch (new_status) {
		case DEVST_DOWN:
			bte.dwEventId = BTE_STACK_DOWN;
			break;
		case DEVST_RELOADED:
			bte.dwEventId = BTE_STACK_UP;
			break;
		case DEVST_UNLOADED:
			bte.dwEventId = BTE_STACK_DOWN;
			break;
	}

	if (hMsgQ)
		WriteMsgQueue(hMsgQ, &bte, sizeof(BTEVENT), INFINITE, 0);
	/*
	if (new_status == DEVST_DOWN) {

		fprintf(stderr, "Cleaning up Bluetooth stack\n");

		cleanup();

		fprintf(stderr, "Waiting a couple of secs before reinitializing Bluetooth stack\n");
		Sleep(2000);

		if (init() < 0) {
			fprintf(stderr, "Could not initialize Bluetooth stack again\n");
		}

	}
	*/
}

void WIDCOMMBluetooth::OnDeviceResponded(BD_ADDR bda, DEV_CLASS devClass, BD_NAME bdName, BOOL bConnected)
{
	struct RemoteDevice rd;
	memcpy(rd.bda, bda, sizeof(BD_ADDR));
	memcpy(rd.devClass, devClass, sizeof(DEV_CLASS));
	rd.name = (char *)bdName;
	rd.bConnected = (bConnected == TRUE) ? true : false;

	/*
		Check first if the device is already in the cache. For some reason, the stack 
		sometimes calls	this handler function several times for the same device.

	*/
	for (List<struct RemoteDevice>::iterator it = inquiryCache.begin(); it != inquiryCache.end(); ++it) {
		if (memcmp((*it).bda, bda, sizeof(BD_ADDR)) == 0) {
			rd = *it;
			return;
		}
	}
	inquiryCache.push_back(rd);
	inquiryResult++;

	if (inquiryCallback)
		inquiryCallback(&rd, inquiryData);
}

void WIDCOMMBluetooth::OnInquiryComplete(BOOL success, short num_responses)
{
	isInInquiry = false;

	if (success == FALSE)
		inquiryResult = -1;

	HAGGLE_DBG("Setting inquiry complete event\n");
	SetEvent(hInquiryEvent);
}

#define INQUIRY_TIMEOUT (50000)

int WIDCOMMBluetooth::_doInquiry(widcomm_inquiry_callback_t callback, void *data, bool async)
{
	if (isInInquiry || !IsDeviceReady())
		return -1;

	isInInquiry = true;
	inquiryCallback = callback;
	inquiryData = data;
	inquiryResult = 0;

	// Make sure the event is not set
	ResetEvent(hInquiryEvent);

	inquiryCache.clear();

	if (StartInquiry() == FALSE) {
		isInInquiry = false;
		return -1;
	}

	// I we run the inquiry in asynchronous mode, the isInInquiry boolean
	// will be reset to 'false' by the OnInquiryComplete callback
	if (!async) {
		HAGGLE_DBG("Waiting for inquiry to complete\n");
		
		DWORD ret = WaitForSingleObject(hInquiryEvent, INQUIRY_TIMEOUT);

		switch (ret) {
			case WAIT_FAILED:
				inquiryResult = -1;
				// Stop the inquiry
				StopInquiry();
				break;
			case WAIT_TIMEOUT:
				HAGGLE_ERR("Inquiry TIMEOUT after %u s\n", INQUIRY_TIMEOUT/1000);
				// Try to see if we can force the stack to come out of the inquiry
				// in case it hung
				StopInquiry();
				inquiryResult = -1;
				break;
			case WAIT_OBJECT_0:
				HAGGLE_DBG("Inquiry completed with res=%d\n", inquiryResult);
				// inquiryResult set in OnDeviceResponded() or OnInquiryComplete()
				break;
		}
		// We need to unset the isInInquiry flag, otherwise we will
		// not be able to enter this function again and try another scan.
		isInInquiry = false;
	}
	return inquiryResult;
}

int WIDCOMMBluetooth::doInquiry(widcomm_inquiry_callback_t callback, void *data)
{
	return stack->_doInquiry(callback, data, false);
}

int WIDCOMMBluetooth::doInquiryAsync(widcomm_inquiry_callback_t callback, void *data)
{
	return stack->_doInquiry(callback, data, true);
}

bool WIDCOMMBluetooth::stopInquiry()
{
	if (stack->isInInquiry) {
		stack->StopInquiry();
		stack->isInInquiry = false;
		// Make sure we fire the event in case someone is waiting on it
		SetEvent(stack->hInquiryEvent);
		return true;
	}
	return false;
}

void WIDCOMMBluetooth::OnDiscoveryComplete()
{
	struct RemoteDevice rd;
	bool recordFound = false;
	BD_ADDR bdaddr;
	UINT16 num_records;
	CSdpDiscoveryRec *records;

	HAGGLE_DBG("Discovery complete, checking result...\n");

	if (GetLastDiscoveryResult(bdaddr, &num_records) != DISCOVERY_RESULT_SUCCESS)
		goto out;

	if (num_records == 0)
		goto out;

	for (List<struct RemoteDevice>::iterator it = inquiryCache.begin(); it != inquiryCache.end(); ++it) {
		if (memcmp((*it).bda, bdaddr, sizeof(BD_ADDR)) == 0) {
			rd = *it;
			recordFound = true;
		}
	}

	if (!recordFound)
		goto out;

	records = new CSdpDiscoveryRec[num_records];

	if (!records)
		goto out;

	int ret = ReadDiscoveryRecords(rd.bda, (int)num_records, records);

	if (ret && discoveryCallback)
		discoveryCallback(&rd, records, ret, discoveryData);

	delete [] records;

out:
	discoveryResult = (int)num_records;
	isInDiscovery = false;
	HAGGLE_DBG("Setting discovery complete event\n");
	SetEvent(hDiscoveryEvent);
}

#define DISCOVERY_TIMEOUT (40000)

int WIDCOMMBluetooth::_doDiscovery(const RemoteDevice *rd, GUID *guid, widcomm_discovery_callback_t callback, void *data, bool async)
{
	BD_ADDR bdaddr;

	if (!rd || isInDiscovery || IsDeviceReady() == FALSE)
		return -1;

	isInDiscovery = true;
	discoveryCallback = callback;
	discoveryData = data;
	discoveryResult = 0;

	HAGGLE_DBG("Starting Bluetooth discovery for device %s\n", rd->name.c_str());
	// Make sure the event is not set
	ResetEvent(hDiscoveryEvent);

	memcpy(bdaddr, rd->bda, sizeof(BD_ADDR));

	if (StartDiscovery(bdaddr, guid) == FALSE) {
		isInDiscovery = false;
		return -1;
	}

	// One we run discovery in asynchronous mode, the isInDiscovery boolean
	// will be reset to 'false' by the OnDiscoveryComplete callback
	if (!async) {
		HAGGLE_DBG("Waiting for discovery to complete\n");

		DWORD ret = WaitForSingleObject(hDiscoveryEvent, DISCOVERY_TIMEOUT);

		switch (ret) {
			case WAIT_FAILED:
				discoveryResult = -1;
				break;
			case WAIT_TIMEOUT:
				HAGGLE_ERR("Discovery TIMEOUT after %u s\n", DISCOVERY_TIMEOUT/1000);
				discoveryResult = -1;
				break;
			case WAIT_OBJECT_0:
				HAGGLE_DBG("Discovery completed with res=%d\n", discoveryResult);
				// discoveryResult set by OnDiscoveryComplete()
				break;
		}
		isInDiscovery = false;
	}

	return discoveryResult;
}

int WIDCOMMBluetooth::doDiscoveryAsync(const RemoteDevice *rd, GUID *guid, widcomm_discovery_callback_t callback, void *data)
{
	return stack->_doDiscovery(rd, guid, callback, data, true);
}

int WIDCOMMBluetooth::doDiscovery(const RemoteDevice *rd, GUID *guid, widcomm_discovery_callback_t callback, void *data)
{
	return stack->_doDiscovery(rd, guid, callback, data, false);
}

int WIDCOMMBluetooth::readLocalDeviceAddress(unsigned char *mac)
{
	DEV_VER_INFO dvi;

	if (stack->GetLocalDeviceVersionInfo(&dvi) == FALSE)
		return -1;

	memcpy(mac, dvi.bd_addr, sizeof(BD_ADDR));

	return 0;
}

int WIDCOMMBluetooth::readLocalDeviceName(char *name, int len)
{
	BD_NAME bdName;

	if (stack->GetLocalDeviceName(&bdName) == FALSE)
		return -1;

	strncpy(name, (char *)bdName, len);

	return strlen(name);
}

bool WIDCOMMBluetooth::_enumerateRemoteDevicesStart()
{
	if (isInInquiry)
		return false;

	inquiryCacheIterator = inquiryCache.begin();
	return true;
}
const RemoteDevice *WIDCOMMBluetooth::_getNextRemoteDevice()
{
	if (isInInquiry)
		return NULL;

	if (inquiryCacheIterator == inquiryCache.end())
		return NULL;

	RemoteDevice *rd = &(*inquiryCacheIterator);

	inquiryCacheIterator++;

	return rd;
}

bool WIDCOMMBluetooth::enumerateRemoteDevicesStart()
{
	return stack->_enumerateRemoteDevicesStart();
}

const RemoteDevice *WIDCOMMBluetooth::getNextRemoteDevice()
{
	return stack->_getNextRemoteDevice();
}

HANDLE RequestBluetoothNotifications(DWORD flags, HANDLE hMsgQ)
{ 
	return WIDCOMMBluetooth::stack->setMsgQ(hMsgQ);
}

BOOL StopBluetoothNotifications(HANDLE h)
{
	return WIDCOMMBluetooth::stack->resetMsgQ(h);
}

int BthSetMode(DWORD dwMode)
{
	return -1;
}

#endif /* WIDCOMM_BLUETOOTH && ENABLE_BLUETOOTH */
