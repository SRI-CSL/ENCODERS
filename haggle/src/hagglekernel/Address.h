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
#ifndef _ADDRESS_H
#define _ADDRESS_H

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/List.h>
#include "Debug.h"
#include "Metadata.h"

#if defined(OS_UNIX)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#endif

#define AF_NONE 0

using namespace haggle;
class Address;

#define TRANSPORT_METADATA "Transport"
#define TRANSPORT_METADATA_TYPE_PARAM "type"
#define TRANSPORT_METADATA_PORT_PARAM "port"
#define TRANSPORT_METADATA_CHANNEL_PARAM "channel"

class Transport {
	friend class Address;	
public:
	typedef enum {
		TYPE_NONE,
		TYPE_RFCOMM,
		TYPE_UDP,
		TYPE_TCP
	} Type_t;
protected:
	Type_t type;
	static const char *type_str[];
	char *uri_prefix;
	Transport(Type_t type, const char *prefix = "");
	Transport(const Transport& t);
public:	
	virtual ~Transport();
	static const char *typeToStr(Type_t type);
	static Type_t strToType(const char *str);
	static Transport *create(Type_t type, unsigned short arg = 0);
	Type_t getType() const { return type; }
	const char *getTypeStr() const { return typeToStr(type); }
	const char *getURIPrefix() const { return uri_prefix; }
	virtual bool equal(const Transport& t) const = 0;
	virtual Transport *copy() const = 0;
	friend bool operator==(const Transport& t1, const Transport& t2);
	virtual Metadata *toMetadata() const = 0;
	static Transport *fromMetadata(const Metadata& m);
};

class TransportNone : public Transport {
public:
	TransportNone();
	TransportNone(const TransportNone& t);
	bool equal(const Transport& t) const;
	TransportNone *copy() const { return new TransportNone(*this); }
	Metadata *toMetadata() const { return NULL; }
	static TransportNone *fromMetadata(const Metadata& m) { return NULL; }
};

class TransportUDP : public Transport {
	unsigned short port;
public:
	TransportUDP(unsigned short _port);
	TransportUDP(const TransportUDP& t);
	unsigned short getPort() const { return port; }
	bool equal(const Transport& t) const;
	TransportUDP *copy() const { return new TransportUDP(*this); }
	Metadata *toMetadata() const;
	static TransportUDP *fromMetadata(const Metadata& m);
};

class TransportTCP : public Transport {
	unsigned short port;
public:
	TransportTCP(unsigned short _port);
	TransportTCP(const TransportTCP& t);
	unsigned short getPort() const { return port; }
	bool equal(const Transport& t) const;
	TransportTCP *copy() const { return new TransportTCP(*this); }
	Metadata *toMetadata() const;
	static TransportTCP *fromMetadata(const Metadata& m);
};

class TransportRFCOMM : public Transport {
	unsigned short channel;
public:
	TransportRFCOMM(unsigned short _channel);
	TransportRFCOMM(const TransportRFCOMM& t);
	unsigned short getChannel() const { return channel; }
	bool equal(const Transport& t) const;
	TransportRFCOMM *copy() const { return new TransportRFCOMM(*this); }
	Metadata *toMetadata() const;
	static TransportRFCOMM *fromMetadata(const Metadata& m);
};

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef BT_ALEN
#define BT_ALEN 6
#endif

#define ADDRESS_METADATA "Address"
#define ADDRESS_METADATA_NETWORK_PARAM "net"
#define ADDRESS_METADATA_TRANSPORT TRANSPORT_METADATA
#define ADDRESS_METADATA_TYPE_PARAM "type"

class Address
#ifdef DEBUG_LEAKS
	: LeakMonitor
#endif
{
public:
	typedef enum {
		TYPE_UNDEFINED = 0,
		TYPE_ETHERNET,
		TYPE_ETHERNET_BROADCAST,
		TYPE_BLUETOOTH,
		TYPE_IPV4,
		TYPE_IPV4_BROADCAST,
		TYPE_IPV6,
		TYPE_IPV6_BROADCAST,
		TYPE_FILEPATH,
		TYPE_UNIX
	} Type_t;
protected:
	Type_t type;
	static const char *type_str[];
	static const char *uri_prefix[];
	const unsigned char *raw;
	Transport *transport;	
	char *uri;
	char *straddr;
	Address(Type_t _type, const void *_raw, const Transport& t = TransportNone());
	Address(const Address& a);
	bool allocURI(size_t len) const;
	bool allocStr(size_t len) const;
public:
	virtual ~Address() = 0;
	static const char *typeToStr(Type_t type);
	static Type_t strToType(const char *str);
	Type_t getType() const { return type; }
	const char *getTypeStr() const { return typeToStr(type); }
	const Transport *getTransport() const { return transport; }
	const unsigned char *getRaw() const { return raw; }
	virtual size_t getLength() const = 0;
	virtual const char *getURI() const = 0;
	virtual const char *getStr() const = 0;
	bool merge(const Address& a) { return false; }
	virtual Address *copy() const = 0;
	virtual bool equal(const Address& a) const = 0;
	friend bool operator==(const Address& a1, const Address& a2);
	Metadata *toMetadata() const;
	static Address *fromMetadata(const Metadata& m);
};

class SocketAddress : public Address {
protected:
	SocketAddress(Type_t type, const void *raw, const Transport& t = TransportNone());
	SocketAddress(const SocketAddress& a);
public:
	virtual socklen_t fillInSockaddr(struct sockaddr *sa) const { return 0; };
	virtual ~SocketAddress() = 0;
	int family() const;
};

class EthernetAddress : public SocketAddress {
	unsigned char mac[ETH_ALEN];
public:
	static const Type_t class_type = TYPE_ETHERNET;
	EthernetAddress() : SocketAddress(class_type, mac) {}
	EthernetAddress(const unsigned char _mac[ETH_ALEN]);
	EthernetAddress(const EthernetAddress& a);
	~EthernetAddress();
	size_t getLength() const { return sizeof(mac); }
	const char *getURI() const;
	const char *getStr() const;
	socklen_t fillInSockaddr(struct sockaddr *sa) const;
	Address *copy() const { return new EthernetAddress(*this); }
	bool equal(const Address& a) const;
	static EthernetAddress *fromMetadata(const Metadata& m);
};

class EthernetBroadcastAddress : public EthernetAddress {
public:
	static const Type_t class_type = TYPE_ETHERNET_BROADCAST;
	EthernetBroadcastAddress() : EthernetAddress() {}
	EthernetBroadcastAddress(const unsigned char _mac[ETH_ALEN]) : EthernetAddress(_mac) { type = class_type; }
	EthernetBroadcastAddress(const EthernetBroadcastAddress& a) : EthernetAddress(a) {}
	~EthernetBroadcastAddress() {}
};

#if defined(ENABLE_BLUETOOTH)

#if defined(OS_MACOSX)
#define AF_BLUETOOTH AF_NONE
#elif defined(OS_LINUX)
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

/* Some redefinitions of Bluetooth sockaddr for portability between
 * Windows and Linux */
#define sockaddr_bt sockaddr_rc
#define bt_channel rc_channel
#define bt_family rc_family
#define bt_bdaddr rc_bdaddr

#elif defined(OS_WINDOWS)
#if !defined(WIDCOMM_BLUETOOTH)
#include <ws2bth.h>
#define sockaddr_bt _SOCKADDR_BTH
#define bt_channel port
#define bt_family addressFamily
#define bt_bdaddr btAddr
#endif
#if defined(OS_WINDOWS_MOBILE)
#if defined(WIDCOMM_BLUETOOTH)
#define AF_BLUETOOTH 0
#else
#define AF_BLUETOOTH AF_BT
#endif
#elif defined(OS_WINDOWS_DESKTOP)
#define AF_BLUETOOTH AF_BTH
#endif
#endif /* OS_LINUX */

class BluetoothAddress : public SocketAddress {
	unsigned char mac[BT_ALEN];
public:
	static const Type_t class_type = TYPE_BLUETOOTH;
	BluetoothAddress() : SocketAddress(class_type, mac) {}
	BluetoothAddress(const unsigned char _mac[BT_ALEN], const Transport& t = TransportNone());
	BluetoothAddress(const BluetoothAddress& a);
	~BluetoothAddress();
	size_t getLength() const { return sizeof(mac); }
	const char *getURI() const;
	const char *getStr() const;
	socklen_t fillInSockaddr(struct sockaddr *sa, unsigned short channel) const;
	socklen_t fillInSockaddr(struct sockaddr *sa) const;
	Address *copy() const { return new BluetoothAddress(*this); }
	bool equal(const Address& a) const;
	static BluetoothAddress *fromMetadata(const Metadata& m);
};

#endif /* ENABLE_BLUETOOTH */

class IPv4Address : public SocketAddress {
	friend class Address;
	struct in_addr ipv4;
public:
	static const Type_t class_type = TYPE_IPV4;
	IPv4Address() : SocketAddress(class_type, &ipv4) {}
	IPv4Address(struct sockaddr_in& saddr, const Transport& t = TransportNone());
	IPv4Address(struct in_addr& inaddr, const Transport& t = TransportNone());
	IPv4Address(const IPv4Address& a);
	~IPv4Address();
	size_t getLength() const { return sizeof(ipv4); }
	const char *getURI() const;
	const char *getStr() const;
	socklen_t fillInSockaddr(struct sockaddr_in *sa, unsigned short port = 0) const;
	socklen_t fillInSockaddr(struct sockaddr *sa, unsigned short port = 0) const;
	socklen_t fillInSockaddr(struct sockaddr *sa) const;
	Address *copy() const { return new IPv4Address(*this); }
	bool equal(const Address& a) const;
	static IPv4Address *fromMetadata(const Metadata& m);
};

class IPv4BroadcastAddress : public IPv4Address {
public:
	static const Type_t class_type = TYPE_IPV4_BROADCAST;
	IPv4BroadcastAddress() : IPv4Address() { type = class_type; }
	IPv4BroadcastAddress(struct sockaddr_in& saddr) : IPv4Address(saddr) { type = class_type; }
	IPv4BroadcastAddress(struct in_addr& inaddr) : IPv4Address(inaddr) { type = class_type; }
	IPv4BroadcastAddress(const IPv4BroadcastAddress& a) : IPv4Address(a) {}
	~IPv4BroadcastAddress() {}
};

#if defined(ENABLE_IPv6)
class IPv6Address : public SocketAddress {	
	struct in6_addr ipv6;
public:
	static const Type_t class_type = TYPE_IPV6;
	IPv6Address() : SocketAddress(class_type, &ipv6) {}
	IPv6Address(struct sockaddr_in6& saddr, const Transport& t = TransportNone());
	IPv6Address(struct in6_addr& inaddr, const Transport& t = TransportNone());
	IPv6Address(const IPv6Address& a);
	~IPv6Address();
	size_t getLength() const { return sizeof(ipv6); }
	const char *getURI() const;
	const char *getStr() const;
	socklen_t fillInSockaddr(struct sockaddr_in6 *sa, unsigned short port = 0) const;
	socklen_t fillInSockaddr(struct sockaddr *sa, unsigned short port = 0) const;
	socklen_t fillInSockaddr(struct sockaddr *sa) const;
	Address *copy() const { return new IPv6Address(*this); }
	bool equal(const Address& a) const;
	static IPv6Address *fromMetadata(const Metadata& m);
};

class IPv6BroadcastAddress : public IPv6Address {
public:
	static const Type_t class_type = TYPE_IPV6_BROADCAST;
	IPv6BroadcastAddress() : IPv6Address() { type = class_type; }
	IPv6BroadcastAddress(struct sockaddr_in6& saddr) : IPv6Address(saddr) { type = class_type; }
	IPv6BroadcastAddress(struct in6_addr& inaddr) : IPv6Address(inaddr) { type = class_type; }
	IPv6BroadcastAddress(const IPv6Address& a) : IPv6Address(a) {}
	~IPv6BroadcastAddress() {}
};

#endif

class FileAddress : public Address {
	const char *filepath;
public:
	static const Type_t class_type = TYPE_FILEPATH;
	FileAddress() : Address(class_type, filepath), filepath("none") {}
	FileAddress(const char *path);
	FileAddress(const FileAddress& a);
	~FileAddress();
	size_t getLength() const { return strlen(filepath); }
	const char *getURI() const;
	const char *getStr() const;
	Address *copy() const { return new FileAddress(*this); }
	bool equal(const Address& a) const;
	static FileAddress *fromMetadata(const Metadata& m);
};

#if defined(OS_UNIX)
class UnixAddress : public SocketAddress {
	const char *filepath;
public:
	static const Type_t class_type = TYPE_UNIX;
	UnixAddress() : SocketAddress(class_type, filepath), filepath("none") {}
	UnixAddress(struct sockaddr_un& saddr);
	UnixAddress(const char *path);
	UnixAddress(const UnixAddress& a);
	~UnixAddress();
	size_t getLength() const { return strlen(filepath); }
	const char *getURI() const;
	const char *getStr() const;
	socklen_t fillInSockaddr(struct sockaddr *sa) const;
	socklen_t fillInSockaddr(struct sockaddr_un *sa) const;
	Address *copy() const { return new UnixAddress(*this); }
	bool equal(const Address& a) const;
	static UnixAddress *fromMetadata(const Metadata& m);
};
#endif

class Addresses : public List<Address *>
{
public:
	Addresses() {}
	Addresses(const Addresses& adds);
	~Addresses();
	void add(Address *a) { push_back(a); }
	Addresses *copy() const;
	Address *pop();
};

#endif /* _ADDRESS_H */
