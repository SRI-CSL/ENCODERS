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

#include <libcpphaggle/PlatformDetect.h>

#if defined(OS_MACOSX) && defined(ENABLE_BLUETOOTH)

#include <IOBluetooth/IOBluetoothTypes.h>

// Objective C API
#import <IOBluetooth/objc/IOBluetoothDevice.h>
#import <IOBluetooth/objc/IOBluetoothRFCOMMChannel.h>

#define _IN_PROTOCOLRFCOMMMACOSX
#include "ProtocolRFCOMMMacOSX.h"
#undef _IN_PROTOCOLRFCOMMMACOSX
#import <IOBluetooth/objc/IOBluetoothRFCOMMChannel.h>

#include "Protocol.h"


/*
 
 *******************************************************************************
 RFCOMMClientInterface ObjC implementation
 *******************************************************************************
 
 */
@implementation RFCOMMClientInterface

-(id)initClientWithChannelID:(ProtocolRFCOMMClient *)_p channelID:(UInt8)channel
{
	self = [super init];
	if (self) {
		p = _p;
		channelID = channel;
	}
	return self;
}

-(id)initClientWithRFCOMMChannelRef:(ProtocolRFCOMMClient *)_p rfcommChannelRef:(IOBluetoothRFCOMMChannelRef)rfcommChannelRef
{
	self = [super init];
	if (self) {
		p = _p;
		mRFCOMMChannel = [IOBluetoothRFCOMMChannel withRFCOMMChannelRef: rfcommChannelRef];
		[mRFCOMMChannel retain];
		[mRFCOMMChannel setDelegate: self];
		channelID = [mRFCOMMChannel getChannelID];
		
		HAGGLE_DBG("Created new RFCOMM client on channel %d\n", channelID);
	}
	return self;
}
-(void)dealloc
{
	
	HAGGLE_DBG("deallocating RFCOMM client\n");
	if (mRFCOMMChannel != nil)
		[self closeConnection];

	[super dealloc];
}

// Returns the name of the local bluetooth device
- (NSString *)localDeviceName
{
    BluetoothDeviceName localDeviceName;

    if (IOBluetoothLocalDeviceReadName(localDeviceName, NULL, NULL, NULL) == kIOReturnSuccess) {
        return [NSString stringWithUTF8String:(const char*)localDeviceName];
    }

    return nil;
}
-(BOOL)isConnected
{
	if (mRFCOMMChannel == nil)
		return FALSE;
	
	return TRUE;
}

// Connection Method:
// returns TRUE if the connection was successful:
- (BOOL)connectToPeer:(BluetoothDeviceAddress *)peerAddr
{
	BOOL returnValue = FALSE;
	IOBluetoothDevice *peerDevice;
	IOReturn status;
	NSAutoreleasePool *pool = NULL;
	
	if (mRFCOMMChannel != nil)
		return false;
	
	pool = [[NSAutoreleasePool alloc] init];
	
	if (pool == nil)
		return FALSE;

	peerDevice = [[IOBluetoothDevice withAddress:peerAddr] autorelease];

	if (peerDevice == nil)
		goto exit;
		
	// Before we can open the RFCOMM channel, we need to open a connection to the device.
	// The openRFCOMMChannel... API probably should do this for us, but for now we have to
	// do it manually.
	// This -openConnection call is synchronous, but there is also an asynchronous version -openConnection:
	status = [peerDevice openConnection];
	
	if (status != kIOReturnSuccess) {
		HAGGLE_DBG("Error: 0x%lx opening connection to device.\n", status );
		goto exit;
	}
	
	// Open the RFCOMM channel on the new device connection and set ourselves as delegate
	status = [peerDevice openRFCOMMChannelSync:&mRFCOMMChannel 
			     withChannelID:channelID delegate:self];
	
	if ((status == kIOReturnSuccess) && (mRFCOMMChannel != nil)) {
		// Retain the channel
		[mRFCOMMChannel retain];
		
		// And the return value is TRUE !!
		returnValue = TRUE;
	}
	else {
		if (mRFCOMMChannel) {
			[mRFCOMMChannel release];
			mRFCOMMChannel = nil;
		}
		HAGGLE_DBG("Error: 0x%lx - unable to open RFCOMM channel.\n", status );
	}
	
exit:
	[pool release];

	return returnValue;
}
// Disconnection:
// closes the channel:
- (void)closeConnection
{
	if (mRFCOMMChannel != nil) {
		IOBluetoothDevice *device = [mRFCOMMChannel getDevice];
		
		// This will close the RFCOMM channel and start an
		// inactivity timer to close the baseband connection
		// if no other channels (L2CAP or RFCOMM) are open.
		
		[mRFCOMMChannel closeChannel];
                
		// Release the channel object since we are done with
		// it and it isn't useful anymore.
		[mRFCOMMChannel release];
		mRFCOMMChannel = nil;
        
		// This signals to the system that we are done with
		// the baseband connection to the device.  If no other
		// channels are open, it will immediately close the
		// baseband connection.
		[device closeConnection];
		
		p->closeIPCWriteSocket();
	} else {
		HAGGLE_DBG("%s connection already closed?\n", p->getName());
	}
}

// Send Data method
// returns TRUE if all the data was sent:
- (BOOL)sendData:(const void *)buffer length:(size_t)length bytes:(size_t*)bytes
{
        char *idx = (char *)buffer;

	if (mRFCOMMChannel != nil) {
		ssize_t numBytesRemaining;
		IOReturn result;
		BluetoothRFCOMMMTU rfcommChannelMTU;
		
		numBytesRemaining = length;
		result = kIOReturnSuccess;
		
		// Get the RFCOMM Channel's MTU.  Each write can only
		// contain up to the MTU size number of bytes.
		rfcommChannelMTU = [mRFCOMMChannel getMTU];
		
		// Loop through the data until we have no more to send.
		while ((result == kIOReturnSuccess) && (numBytesRemaining > 0)) {
			// finds how many bytes I can send:
			*bytes = ((numBytesRemaining > rfcommChannelMTU) ? rfcommChannelMTU :  numBytesRemaining);
			
			// This method won't return until the buffer
			// has been passed to the Bluetooth hardware
			// to be sent to the remote device.
			// Alternatively, the asynchronous version of
			// this method could be used which would queue
			// up the buffer and return immediately.
			result = [mRFCOMMChannel writeSync:idx length:*bytes];
			
			// Updates the position in the buffer:
			numBytesRemaining -= *bytes;
			idx += *bytes;
		}
		
		// We are successful only if all the data was sent:
		if ((numBytesRemaining == 0) && (result == kIOReturnSuccess)) {
			return TRUE;
		}
	}
	
	return FALSE;
}

// Returns the name of the device we are connected to
// returns nil if not connection:
- (NSString *)remoteDeviceName
{
	NSString *deviceName = nil;
	
	if ( mRFCOMMChannel != nil ) {
		// Gets the device:
		IOBluetoothDevice *device = [mRFCOMMChannel getDevice];
		
		// .. and its name:
		deviceName = [device getName];
	}
	
	return deviceName;
}

// Implementation of delegate calls (see IOBluetoothRFCOMMChannel.h) Only the basic ones:
- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)rfcommChannel data:(void *)dataPointer length:(size_t)dataLength;
{
	ssize_t sent = send(p->getIPCWriteSocket(), dataPointer, dataLength, 0);
	
	if (sent <= 0 || (size_t)sent != dataLength) {
		HAGGLE_DBG("Could not send data to other process, ret=%d\n", sent);
	}
	HAGGLE_DBG("Wrote %d bytes data to other IPC socket\n", sent);
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)rfcommChannel;
{
	[mRFCOMMChannel release];
	mRFCOMMChannel = nil;
       
	// This will signal  that the peer closed the channel.
	p->closeIPCWriteSocket();
	
	HAGGLE_DBG("Protocol %s - peer closed RFCOMM channel\n", p->getName());
}

@end

/*
 
 *******************************************************************************
 RFCOMMServerInterface ObjC implementation
 *******************************************************************************
 
 */
@implementation RFCOMMServerInterface

-(id)initServerWithChannelID:(ProtocolRFCOMMServer *)_p channelID:(UInt8)channel
{
	self = [super init];
	if (self) {
		p = _p;
		channelID = channel;
	}
	
	return self;
}

-(void)dealloc
{
	if (mIncomingChannelNotification != nil)
		[self stop];
	
	[super dealloc];
}

- (BOOL)isListening
{
	if ( mIncomingChannelNotification != nil ) {
		return TRUE;
	}
	return FALSE;
}

// Starts listening on the assigned incoming channel
- (void)listen
{
	// Unregisters the notification:
	if ( mIncomingChannelNotification == nil ) {
		mIncomingChannelNotification = [IOBluetoothRFCOMMChannel registerForChannelOpenNotifications:self selector:@selector(rfcommChannelOpened:channel:) withChannelID:channelID direction:kIOBluetoothUserNotificationChannelDirectionIncoming];
		
		if (mIncomingChannelNotification == nil) {
			HAGGLE_ERR("%s Could not start listening on channel %d\n", p->getName(), channelID);
			return;
		}
		p->setMode(PROT_MODE_LISTENING);
		
		HAGGLE_DBG("%s Listening on connections on channel %d\n", p->getName(), channelID);
	}
}
// Stops listening on the assigned incoming channel
- (void)stop
{
	// Unregisters the notification:
	if ( mIncomingChannelNotification != nil ) {
		[mIncomingChannelNotification unregister];
		mIncomingChannelNotification = nil;
		p->setMode(PROT_MODE_IDLE);
	}
}

// New RFCOMM channel show up:
// A new RFCOMM channel shows up we have to make sure it is the one we are waiting for:
- (void) rfcommChannelOpened:(IOBluetoothUserNotification *)inNotification channel:(IOBluetoothRFCOMMChannel *)newChannel
{
	// Make sure the channel is an incoming channel on the right channel ID.
	// This isn't strictly necessary since we only registered a notification for this case,
	// but it can't hurt to double-check.
	if ( ( newChannel != nil ) && [newChannel isIncoming] && ( [newChannel getChannelID] == channelID ) ) {
		
		HAGGLE_DBG("Incoming client connection on channel %d\n", channelID);
		
		p->handleRFCOMMChannelOpen([newChannel getRFCOMMChannelRef]);
	}
}


@end

/*
 
 *********************************************************************
 C+ implementation
 *********************************************************************
 
 */

//list<BluetoothDeviceAddress> ProtocolRFCOMM::peers;

ProtocolRFCOMM::ProtocolRFCOMM(const InterfaceRef&  _iface, const InterfaceRef& _peer_iface, 
			       const unsigned short _channel, const short _flags, ProtocolManager * m) : 
	ProtocolSocket(Protocol::TYPE_RFCOMM, "RFCOMM", _iface, _peer_iface, _flags, m), channel(_channel)
{
}

ProtocolRFCOMM::~ProtocolRFCOMM()
{
}


void ProtocolRFCOMMServer::handleRFCOMMChannelOpen(IOBluetoothRFCOMMChannelRef mRFCOMMChannelRef)
{
	const BluetoothDeviceAddress *macAddr;
	IOBluetoothDeviceRef devRef = IOBluetoothRFCOMMChannelGetDevice(mRFCOMMChannelRef);

	if (!devRef) {
		HAGGLE_DBG("Could not get device reference from channel\n");
		return;
	}

	macAddr = IOBluetoothDeviceGetAddress(devRef);
	
	//Interface peer(IFACE_TYPE_BT, mac_addr->data, NULL, 
	HAGGLE_DBG("%s Connection attempt from %02X:%02X:%02X:%02X:%02X:%02X\n", 
		   getName(),
		   macAddr->data[0] & 0xff, 
		   macAddr->data[1] & 0xff, 
		   macAddr->data[2] & 0xff, 
		   macAddr->data[3] & 0xff, 
		   macAddr->data[4] & 0xff, 
		   macAddr->data[5] & 0xff);

	ProtocolRFCOMMClient *p = new ProtocolRFCOMMReceiver((const char *)macAddr->data,
							   mRFCOMMChannelRef,
							   (unsigned short)channel,
							   this->getLocalInterface(), 
							   getManager());
	
	p->setFlag(PROT_FLAG_CONNECTED);

	p->registerWithManager();

	HAGGLE_DBG("Accepted client, starting client thread\n");
	
	p->startTxRx();
}

ProtocolRFCOMMServer::ProtocolRFCOMMServer(const InterfaceRef &_iface, ProtocolManager *m) : 
	ProtocolRFCOMM(_iface, NULL, RFCOMM_DEFAULT_CHANNEL, PROT_FLAG_SERVER, m)
{
	mRFCOMMServerInterface = [[RFCOMMServerInterface alloc] initServerWithChannelID: this channelID: channel];
#if HAVE_EXCEPTION
	if (mRFCOMMServerInterface == nil)
		throw ProtocolException(-1, "Could not create objC RFCOMM server");
#endif
	/* 
	 Since the Mac OS X Bluetooth API is not sockets based, we run our server in a thread that 
	 uses a runloop with callback to listen to incoming connections.
	*/
	start();

	HAGGLE_DBG("%s started\n", getName());
}


/*
	This callback will be called regularly to check whether the server should
	cancel. It is necesary since the CFRunLoops does not respond to pthread
	cancel requests, and calling CFRunLoopStop() from another thread does not
	seem to work (it just blocks).
 */
void testCancelCallBack (CFRunLoopTimerRef timer, void *info)
{
	ProtocolRFCOMMServer *p = static_cast<ProtocolRFCOMMServer *>(info);
	
	if (p->shouldExit()) {
		CFRunLoopStop(CFRunLoopGetCurrent());
	}
}

bool ProtocolRFCOMMServer::run()
{
	CFRunLoopTimerContext cfctx;
	
	memset(&cfctx, 0, sizeof(CFRunLoopTimerContext));
	cfctx.version = 0;
	cfctx.info = this;
	
	CFRunLoopTimerRef cfRLTimerRef = CFRunLoopTimerCreate(kCFAllocatorDefault, 
					    CFAbsoluteTimeGetCurrent() + 2, 2, 0, 0, 
					    testCancelCallBack, &cfctx);
	
	/* 
	 This is ugly, but apparently this is how Apple designed it.
	 It seems impossible to cancel a runloop from another thread,
	 therefore we have a timer the fires regularly and its 
	 callback checks whether the Protocol thread should 
	 cancel or not.
	 */
	
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), cfRLTimerRef, kCFRunLoopCommonModes);
	
	CFRelease(cfRLTimerRef);
	
	// Start listening
	[mRFCOMMServerInterface listen];
	
	CFRunLoopRun();
		
	HAGGLE_DBG("%s runloop finished!\n", getName());
	
	return false;
}

void ProtocolRFCOMMServer::hookCleanup()
{	
	HAGGLE_DBG("cleanup done...\n");
}

void ProtocolRFCOMMServer::hookShutdown()
{
	HAGGLE_DBG("%s shutdown called - stopping runloop\n", getName());
	HAGGLE_DBG("Deregistering channel open notifications\n");
	
	if (mRFCOMMServerInterface != nil) {
		[mRFCOMMServerInterface stop];
		HAGGLE_DBG("Deregistered channel open notifications\n");
	}
	
	HAGGLE_DBG("%s shutdown hook done\n", getName());
}

ProtocolRFCOMMServer::~ProtocolRFCOMMServer()
{
	if (mRFCOMMServerInterface != nil) {
		[mRFCOMMServerInterface release];
	}
}

ProtocolRFCOMMClient::ProtocolRFCOMMClient(const InterfaceRef& iface, 
					   const InterfaceRef& peerIface, 
					   const unsigned short channel, 
					   ProtocolManager *m) :
	ProtocolRFCOMM(iface, peerIface, channel, PROT_FLAG_CLIENT, m)
{
#if HAVE_EXCEPTION
	if (!peerIface)
		throw ProtocolException(-1, "No peer Interface");
#endif
	mRFCOMMClientInterface = [[RFCOMMClientInterface alloc] initClientWithChannelID: this channelID: channel];

#if HAVE_EXCEPTION
	if (!mRFCOMMClientInterface)
		throw ProtocolException(-1, "Could not create objC RFCOMM client");
#endif

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
		[mRFCOMMClientInterface release];
#if HAVE_EXCEPTION
		throw ProtocolException(-1, "Could not create socket pair");		
#else
		fprintf(stderr, "Could not create socket pair");
		return;
#endif
	}
	
	// Set the socket in the ProtocolSocket parent class to our read socket
	// so that we can read incoming data with the inherited functions
	setSocket(sockets[0]);
}

ProtocolRFCOMMClient::ProtocolRFCOMMClient(const char *devaddr, 
					   IOBluetoothRFCOMMChannelRef rfcommChannelRef,
					   const unsigned short channel,
					   const InterfaceRef& _localIface,
					   ProtocolManager *m) :
	ProtocolRFCOMM(_localIface, NULL, channel, PROT_FLAG_CLIENT, m)
{
	char devname[30] = "bluetooth peer";
	
	BluetoothAddress addr((unsigned char *) devaddr);
	
	IOBluetoothDeviceRef dev = IOBluetoothRFCOMMChannelGetDevice(rfcommChannelRef);
	
	if (dev) {
		CFStringGetCString(IOBluetoothDeviceGetName(dev), devname, 30, kCFStringEncodingUTF8);
	}	
	
	peerIface = Interface::create<BluetoothInterface>(devaddr, devname, addr, IFFLAG_UP);
	
	mRFCOMMClientInterface = [[RFCOMMClientInterface alloc] initClientWithRFCOMMChannelRef: this 
									rfcommChannelRef: rfcommChannelRef];

#if HAVE_EXCEPTION
	if (mRFCOMMClientInterface == nil)
		throw ProtocolException(-1, "Could not create objC RFCOMM client");
#endif
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
		[mRFCOMMClientInterface release];
#if HAVE_EXCEPTION
		throw ProtocolException(-1, "Could not create socket pair");		
#else
		fprintf(stderr, "Could not create socket pair");
		return;
#endif
	}
	
	// Set the socket in the ProtocolSocket parent class to our read socket
	// so that we can read incoming data with the inherited functions
	setSocket(sockets[0]);
}

ProtocolRFCOMMClient::~ProtocolRFCOMMClient()
{
	HAGGLE_DBG("%s destructor\n", getName());
	
	if (mRFCOMMClientInterface == nil)
		[mRFCOMMClientInterface release];
	
	// Close the write end
	if (sockets[1] != -1)
		CLOSE_SOCKET(sockets[1]);
	
	// Close the read end
	if (sockets[0] != -1) {
		CLOSE_SOCKET(sockets[0]);
	}
}

void ProtocolRFCOMMClient::closeConnection()
{
	[mRFCOMMClientInterface closeConnection];
	unSetFlag(PROT_FLAG_CONNECTED);
}


ProtocolEvent ProtocolRFCOMMClient::connectToPeer()
{
	BluetoothDeviceAddress devAddr;
	
	BluetoothAddress *addr = peerIface->getAddress<BluetoothAddress>();
	
	if(!addr) {
		HAGGLE_DBG("Peer interface does not have a bluetooth mac address.\n");
		return PROT_EVENT_ERROR;
	}

	memcpy(&devAddr, addr->getRaw(), addr->getLength());

	HAGGLE_DBG("%s Trying to connect over RFCOMM to [%s] channel=%u\n", 
		   getName(), peerIface->getName(), channel);
	
	if ([mRFCOMMClientInterface connectToPeer:&devAddr] == FALSE) {
		HAGGLE_DBG("%s Connection failed to [%s] channel=%u\n", 
			   getName(), peerIface->getName(), channel);
		return PROT_EVENT_ERROR;
	}
	
	HAGGLE_DBG("%s Connected to [%s] channel=%u\n", 
		   getName(), peerIface->getName(), channel);
	
	setFlag(PROT_FLAG_CONNECTED);
		
	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolRFCOMMClient::sendData(const void *buf, size_t len, const int flags, size_t *bytes)
{
	if (![mRFCOMMClientInterface sendData:buf length:len bytes:bytes])
		return PROT_EVENT_ERROR;

	*bytes = len;

	return PROT_EVENT_SUCCESS;
}

#endif
