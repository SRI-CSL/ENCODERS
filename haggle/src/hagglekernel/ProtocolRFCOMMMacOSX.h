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
#ifndef _PROTOCOLRFCOMMMACOSX_H
#define _PROTOCOLRFCOMMMACOSX_H

#include <libcpphaggle/Platform.h>

#if defined(OS_MACOSX)

#include "ProtocolSocket.h"
// Get RFCOMM channel number
#include "ProtocolRFCOMM.h"
#include <libcpphaggle/Timeval.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOBluetooth/Bluetooth.h>
#include <IOBluetooth/IOBluetoothUserLib.h>

class ProtocolRFCOMMClient;
class ProtocolRFCOMMServer;

#ifdef _IN_PROTOCOLRFCOMMMACOSX
/*
  Obective C++ definitions. Only visible internally to ProtocolRFCOMM
  class on Mac OS X.
*/

@interface RFCOMMClientInterface : NSObject
{
	IOBluetoothRFCOMMChannel *mRFCOMMChannel;
	BluetoothRFCOMMChannelID channelID;
	ProtocolRFCOMMClient *p;
}

// Init function for object construction
-(id)initClientWithChannelID:(ProtocolRFCOMMClient *)_p channelID:(UInt8)channel;

-(id)initClientWithRFCOMMChannelRef:(ProtocolRFCOMMClient *)_p rfcommChannelRef:(IOBluetoothRFCOMMChannelRef)rfcommChannelRef;

// Returns the name of the local bluetooth device
- (NSString *)localDeviceName;

- (BOOL)isConnected;

// Connection Method:
// returns TRUE if the connection was successful:
- (BOOL)connectToPeer:(BluetoothDeviceAddress *)peerAddr;

// Disconnection:
// closes the channel:
- (void)closeConnection;

// Returns the name of the device we are connected to
// returns nil if not connection:
- (NSString *)remoteDeviceName;

// Send Data method
// returns TRUE if all the data was sent:
- (BOOL)sendData:(const void *)buffer length:(const size_t)length bytes:(size_t*)bytes;

// Implementation of delegate calls
- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel *)rfcommChannel data:(void *)dataPointer length:(size_t)dataLength;
- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel *)rfcommChannel;

@end

@interface RFCOMMServerInterface : NSObject
{
	IOBluetoothUserNotification *mIncomingChannelNotification;
	BluetoothRFCOMMChannelID channelID;
	ProtocolRFCOMMServer *p;
}

// Init function for object construction
-(id)initServerWithChannelID:(ProtocolRFCOMMServer *)_p channelID:(UInt8)channel;


// Stops listening to incoming connections
- (void)listen;

- (BOOL)isListening;
// Stops listening to incoming connections
- (void)stop;

- (void)rfcommChannelOpened:(IOBluetoothUserNotification *)inNotification channel:(IOBluetoothRFCOMMChannel *)newChannel;

@end

#else

typedef void RFCOMMClientInterface;
typedef void RFCOMMServerInterface;

#endif /* _IN_PROTOCOLRFCOMMMACOSX */

/** */
class ProtocolRFCOMM : public ProtocolSocket
 {
 protected:
	 unsigned short channel;
 public:
	 ProtocolRFCOMM(const InterfaceRef& _localIface,
			const InterfaceRef& _peerIface,
			const unsigned short _channel = RFCOMM_DEFAULT_CHANNEL,
			const short _flags = PROT_FLAG_CLIENT,
			ProtocolManager *m = NULL);
	virtual ~ProtocolRFCOMM() = 0;
};

/** */
class ProtocolRFCOMMServer : public ProtocolRFCOMM
{
	friend void testCancelCallBack (CFRunLoopTimerRef timer, void *info);
	RFCOMMServerInterface *mRFCOMMServerInterface;
	
	bool run();
	void hookCleanup();
        void hookShutdown();
public:
	void handleRFCOMMChannelOpen(IOBluetoothRFCOMMChannelRef mRFCOMMChannelRef);

	bool clientIsRunning(char mac[6]);
        ProtocolRFCOMMServer(const InterfaceRef& _iface, ProtocolManager *m);
        ~ProtocolRFCOMMServer();
};

#define RFCOMM_BUFFER_LEN 2048

/** */
class ProtocolRFCOMMClient : public ProtocolRFCOMM
{
	friend class ProtocolRFCOMMServer;
	RFCOMMClientInterface *mRFCOMMClientInterface;
	/* 
		IPC sockets for incoming data.
	 
		Since the Bluetooth API in Mac OS X is callback based (and not socket based), there
		is no socket available for the event loop in the Protocol base class to monitor
		for incoming data. In order to emulate a socket API, we use a socket pair
		on which the incoming data callback of the Bluetooth API writes all incoming data.
		The main event loop in the Protocol class can then monitor the read end of 
		the socket pair and read the data from it as usual. With this trick,
		we do not have to implement our own event loop in this class, and we can instead
		reuse the one in the base class.
	 */
	int sockets[2];
		
	ProtocolEvent sendData(const void *buf, size_t len, const int flags, size_t *bytes);

public:	
        ProtocolRFCOMMClient(const char *devAddr,
			     IOBluetoothRFCOMMChannelRef rfcommChannel,
                             const unsigned short channel,
                             const InterfaceRef& _localIface,
                             ProtocolManager *m = NULL);
	int getIPCWriteSocket() { return sockets[1]; }
	void closeIPCWriteSocket() { if (sockets[1]) { close(sockets[1]); sockets[1] = -1; } }
	int getIPCReadSocket() { return sockets[0]; }
	friend void clientEventListener(IOBluetoothRFCOMMChannelRef rfcommChannel, 
					ProtocolRFCOMMClient *p, 
					IOBluetoothRFCOMMChannelEvent *event);
	void closeConnection();
	ProtocolEvent connectToPeer();
   
        ProtocolRFCOMMClient(const InterfaceRef& _localIface,
                             const InterfaceRef& _peerIface,
                             const unsigned short channel = RFCOMM_DEFAULT_CHANNEL,
                             ProtocolManager *m = NULL);

        ~ProtocolRFCOMMClient();
};

/** */
class ProtocolRFCOMMSender : public ProtocolRFCOMMClient
{
	public:
        ProtocolRFCOMMSender(
			const InterfaceRef& _localIface, 
			const InterfaceRef& _peerIface, 
			const unsigned short channel = RFCOMM_DEFAULT_CHANNEL, 
			ProtocolManager *m = NULL) :
			ProtocolRFCOMMClient(_localIface, _peerIface, channel, m) {}
		virtual bool isSender() { return true; }
};

/** */
class ProtocolRFCOMMReceiver : public ProtocolRFCOMMClient
{
	public:
        ProtocolRFCOMMReceiver(
			const char *devAddr,
			IOBluetoothRFCOMMChannelRef rfcommChannel,
			const unsigned short channel,
			const InterfaceRef& _localIface,
			ProtocolManager *m = NULL) :
			ProtocolRFCOMMClient(devAddr, rfcommChannel, channel, _localIface, m) {}
		virtual bool isReceiver() { return true; }
};

#endif /* OS_MACOSX */

#endif /* _PROTOCOLRFCOMMMACOSX_H */
