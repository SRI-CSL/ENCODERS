#ifndef WIDCOMM_BLUETOOTH_H
#define WIDCOMM_BLUETOOTH_H

/*
	WIDCOMMBluetooth wraps the WIDCOMMM Bluetooth API in something that is more useful
	for the modular Haggle, which splits local device discovery, remote device 
	discovery and RFCOMM operations in different managers and modules.

	The reason we need this wrapper is because the WIDCOMM API allows only one global 
	reference to the Bluetooth stack in any application (in the form of an instance of
	a class that inherits CBtIf). All operations on the stack needs this reference
	(even just reading the local device), and we therefore cannot access the stack
	in several places in the code, unless we pass the reference around. 
	
	As there is no mechanism in Haggle to pass a reference between managers and modules, 
	we instead wrap the API in this semi C-style API that we can include and access from 
	anywhere in the code.

	The need for this wrapper really illustrates how inflexible the WIDCOMM API is, 
	as it assumes one device and a monolithic application. (Yes, this is what most Windows 
	mobile applications look like.)

	Note: this wrapper makes no attempt to be thread safe in any way.

	Author: Erik Nordström
*/
#include <libcpphaggle/Platform.h>

#if defined(WIDCOMM_BLUETOOTH) && defined(ENABLE_BLUETOOTH)

#include <windows.h>
#include <BtSdkCE.h> // Includes everything neeeded from the WIDCOMM SDK
#include <libcpphaggle/List.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Mutex.h>

using namespace haggle;

struct RemoteDevice {
	BD_ADDR bda;
	DEV_CLASS devClass;
	string name;
	bool bConnected;
};

// Callback functions for inquiry and discovery in the asynchronous API
typedef void (*widcomm_inquiry_callback_t) (struct RemoteDevice *, void *);
typedef void (*widcomm_discovery_callback_t) (struct RemoteDevice *, CSdpDiscoveryRec *, int, void *);
/*
	For Bluetooth stack notifications, we mimic the MS API. 
*/

/* MS Bluetooth event classes */
enum {
	BTE_CLASS_CONNECTIONS = 1,
	BTE_CLASS_PAIRING = 2,
	BTE_CLASS_DEVICE = 4,
	BTE_CLASS_STACK = 8,
	BTE_CLASS_AVDTP = 16,
};

/* MS Bluetooth events for stack class */
enum {
	BTE_CONNECTION = 100,
	BTE_DISCONNECTION = 101,
	BTE_ROLE_SWITCH = 102,
	BTE_MODE_CHANGE = 103,
	BTE_PAGE_TIMEOUT = 104,
	BTE_KEY_NOTIFY = 200,
	BTE_KEY_REVOKED = 201,
	BTE_LOCAL_NAME = 300,
	BTE_COD = 301,
	BTE_STACK_UP = 400,
	BTE_STACK_DOWN = 401,
};

typedef struct _BTEVENT {
	DWORD dwEventId;
	DWORD dwReserved;
	BYTE baEventData[64];
} BTEVENT, *PBTEVENT;

enum {
	FOO = 1,
	BAR = 2,
};

// Stolen from BlueCove
static inline void convertUUIDBytesToGUID(char *bytes, GUID *uuid)
{
	uuid->Data1 = bytes[0] << 24 & 0xff000000 | bytes[1] << 16 & 0x00ff0000 | bytes[2] << 8 & 0x0000ff00 | bytes[3] & 0x000000ff;
	uuid->Data2 = bytes[4] << 8 & 0xff00 | bytes[5] & 0x00ff;
	uuid->Data3 = bytes[6] << 8 & 0xff00 | bytes[7] & 0x00ff;

	for (int i = 0; i < 8; i++) {
		uuid->Data4[i] = bytes[i + 8];
	}
}

HANDLE RequestBluetoothNotifications(DWORD flags, HANDLE hMsgQ);
BOOL StopBluetoothNotifications(HANDLE h);
int BthSetMode(DWORD dwMode);

class WIDCOMMBluetooth : public CBtIf
{
	friend HANDLE RequestBluetoothNotifications(DWORD flags, HANDLE hMsgQ);
	friend BOOL StopBluetoothNotifications(HANDLE h);
	friend int BthSetMode(DWORD dwMode);
	/* 
	   A singleton for the one instance of this class.
	   The API only allows one instance per application... 
	*/
	static WIDCOMMBluetooth *stack;
	class StackInitializer {
		friend class WIDCOMMBluetooth;
		StackInitializer();
		~StackInitializer();
	};
	static StackInitializer stackInit;
	List<struct RemoteDevice> inquiryCache; // Cache of discovered devices
	List<struct RemoteDevice>::iterator inquiryCacheIterator; // Iterator to enumerate remote devices
	HANDLE hMsgQ;
	HANDLE hStackEvent;
	HANDLE hInquiryEvent;  // Used to synchronize inquiry
	HANDLE hDiscoveryEvent; // Used to synchronize discovery (reading service records)
	widcomm_inquiry_callback_t inquiryCallback;  // Inquiry allback for the asynchronous API
	widcomm_discovery_callback_t discoveryCallback; // Service record callback for the asynchronous API
	void *inquiryData;
	void *discoveryData;
	bool isInInquiry;
	bool isInDiscovery;
	int inquiryResult;
	int discoveryResult;

	int _doInquiry(widcomm_inquiry_callback_t callback = NULL, void *data = NULL, bool async = false);
	int _doDiscovery(const RemoteDevice *rd, GUID *guid, widcomm_discovery_callback_t callback = NULL, void *data = NULL, bool async = false);

	bool _enumerateRemoteDevicesStart();
	const RemoteDevice *_getNextRemoteDevice();

	// The functions implemented from CBtIf
	void OnInquiryComplete(BOOL success, short num_responses);
	void OnDeviceResponded(BD_ADDR bda, DEV_CLASS devClass, BD_NAME bdName, BOOL bConnected);
	void OnDiscoveryComplete();
	void OnStackStatusChange(CBtIf::STACK_STATUS new_status);
	
	HANDLE setMsgQ(HANDLE h);
	bool resetMsgQ(HANDLE h);

	WIDCOMMBluetooth(void);
	~WIDCOMMBluetooth(void);
public:
	static int readLocalDeviceAddress(unsigned char *mac);
	static int readLocalDeviceName(char *name, int len);
	static bool enumerateRemoteDevicesStart();
	static const RemoteDevice *getNextRemoteDevice();

	static bool stopInquiry();
	// A blocking inquiry
	static int doInquiry(widcomm_inquiry_callback_t callback = NULL, void *data = NULL);
	// A non-blocking inquiry
	static int doInquiryAsync(widcomm_inquiry_callback_t callback, void *data);
	// A blocking discovery
	static int doDiscovery(const RemoteDevice *rd, GUID *guid, widcomm_discovery_callback_t callback = NULL, void *data = NULL);
	// A non-blocking discovery
	static int doDiscoveryAsync(const RemoteDevice *rd, GUID *guid, widcomm_discovery_callback_t callback, void *data);
	
	static int init();
	static void cleanup();
};

#endif /* WIDCOMM_BLUETOOTH && ENABLE_BLUETOOTH */

#endif /* WIDCOMM_BLUETOOTH_H */
