/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 */

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
#include "Trace.h"
#include "Address.h"
#include "XMLMetadata.h"

static String shortToStr(unsigned short n)
{
	static char buf[10];

	snprintf(buf, 10, "%u", n);

	return buf;
}

const char *Transport::type_str[] = 
{
	"none",
	"rfcomm",
	"udp",
	"tcp",
	NULL
};

const char *Transport::typeToStr(Type_t type)
{
	return type_str[type];
}

Transport::Type_t Transport::strToType(const char *str)
{
	int i = 0;

	while (type_str[i]) {
		if (strcmp(str, type_str[i]) == 0) 
			return (Type_t)i;
		i++;
	}
	return TYPE_NONE;
}

Transport::Transport(Type_t _type, const char *prefix) : type(_type), uri_prefix(NULL) 
{
	uri_prefix = new char[strlen(prefix) + 1];

	if (uri_prefix)
		strcpy(uri_prefix, prefix);
}

Transport::Transport(const Transport& t) : type(t.type), uri_prefix(NULL)
{
	uri_prefix = new char[strlen(t.uri_prefix) + 1];
	
	if (uri_prefix)
		strcpy(uri_prefix, t.uri_prefix);
}

Transport::~Transport()
{
	if (uri_prefix)
		delete [] uri_prefix;
}

Transport* Transport::create(Type_t type, unsigned short arg)
{
	Transport *t = NULL;

	switch (type) {
	case TYPE_NONE:
		t = new TransportNone();
		break;
	case TYPE_UDP:
		t = new TransportUDP(arg);
		break;
	case TYPE_TCP:
		t = new TransportTCP(arg);
		break;
	case TYPE_RFCOMM:
		t = new TransportRFCOMM(arg);
	}
	return t;
}

TransportNone::TransportNone() : Transport(TYPE_NONE, "none://")
{
}

TransportNone::TransportNone(const TransportNone& t) : Transport(t)
{
}

bool TransportNone::equal(const Transport& t) const
{
	return getType() == t.getType();
}

TransportUDP::TransportUDP(unsigned short _port) : Transport(TYPE_UDP, "udp://"), port(_port)
{
}

TransportUDP::TransportUDP(const TransportUDP& t) : Transport(t), port(t.port)
{
}

Metadata *TransportUDP::toMetadata() const
{
	Metadata *m;

	m = new XMLMetadata(TRANSPORT_METADATA);

	if (m) {
		m->setParameter(TRANSPORT_METADATA_TYPE_PARAM, getTypeStr());
		m->setParameter(TRANSPORT_METADATA_PORT_PARAM, shortToStr(port).c_str());
	}

	return m;
}

TransportUDP *TransportUDP::fromMetadata(const Metadata& m)
{
	TransportUDP *t = NULL;
	const char *param;

	if (!m.isName(TRANSPORT_METADATA))
		return NULL;

	param = m.getParameter(TRANSPORT_METADATA_TYPE_PARAM);

	if (param && strcmp(param, typeToStr(TYPE_UDP)) == 0) {
		param = m.getParameter(TRANSPORT_METADATA_PORT_PARAM);

		if (param && strlen(param) > 0) {
			char *endptr = NULL;
			unsigned short port = (unsigned short)strtoul(param, &endptr, 10);
			// if (endptr == '\0') {
			// SW: MOS: - nasty bug - occasional failue to initialize UDP properly
			if ((NULL != endptr) && (endptr[0] == '\0')) {
				t = new TransportUDP(port);
			}
		}
	}		       
	return t;	
}

bool TransportUDP::equal(const Transport& t) const
{
	return getType() == t.getType() && port == static_cast<const TransportUDP&>(t).port;
}

TransportTCP::TransportTCP(unsigned short _port) : Transport(TYPE_TCP, "tcp://"), port(_port) 
{
}

TransportTCP::TransportTCP(const TransportTCP& t) : Transport(t), port(t.port)
{
}

Metadata *TransportTCP::toMetadata() const
{
	Metadata *m;

	m = new XMLMetadata(TRANSPORT_METADATA);

	if (m) {
		m->setParameter(TRANSPORT_METADATA_TYPE_PARAM, getTypeStr());
		m->setParameter(TRANSPORT_METADATA_PORT_PARAM, shortToStr(port).c_str());
	}

	return m;
}

TransportTCP *TransportTCP::fromMetadata(const Metadata& m)
{
	TransportTCP *t = NULL;
	const char *param;

	if (!m.isName(TRANSPORT_METADATA))
		return NULL;

	param = m.getParameter(TRANSPORT_METADATA_TYPE_PARAM);

	if (param && strcmp(param, typeToStr(TYPE_TCP)) == 0) {
		param = m.getParameter(TRANSPORT_METADATA_PORT_PARAM);

		if (param && strlen(param) > 0) {
			char *endptr = NULL;
			unsigned short port = (unsigned short)strtoul(param, &endptr, 10);
			// if (endptr == '\0') {
			// SW: MOS: - nasty bug - occasional failue to initialize TCP properly
			if ((NULL != endptr) && (endptr[0] == '\0')) {
				t = new TransportTCP(port);
			}
		}
	}		       
	return t;	
}

bool TransportTCP::equal(const Transport& t) const
{
	return getType() == t.getType() && port == static_cast<const TransportTCP&>(t).port;
}

TransportRFCOMM::TransportRFCOMM(unsigned short _channel) : Transport(TYPE_RFCOMM, "rfcomm://"), channel(_channel) 
{
}

TransportRFCOMM::TransportRFCOMM(const TransportRFCOMM& t) : Transport(t), channel(t.channel)
{
}

bool TransportRFCOMM::equal(const Transport& t) const
{
	return getType() == t.getType() && channel == static_cast<const TransportRFCOMM&>(t).channel;
}

Metadata *TransportRFCOMM::toMetadata() const
{
	Metadata *m;

	m = new XMLMetadata(TRANSPORT_METADATA);

	if (m) {
		m->setParameter(TRANSPORT_METADATA_TYPE_PARAM, getTypeStr());
		m->setParameter(TRANSPORT_METADATA_CHANNEL_PARAM, shortToStr(channel).c_str());
	}

	return m;
}

TransportRFCOMM *TransportRFCOMM::fromMetadata(const Metadata& m)
{
	TransportRFCOMM *t = NULL;
	const char *param;

	if (!m.isName(TRANSPORT_METADATA))
		return NULL;

	param = m.getParameter(TRANSPORT_METADATA_TYPE_PARAM);

	if (param && strcmp(param, typeToStr(TYPE_RFCOMM)) == 0) {
		param = m.getParameter(TRANSPORT_METADATA_CHANNEL_PARAM);

		if (param && strlen(param) > 0) {
			char *endptr = NULL;
			unsigned short channel = (unsigned short)strtoul(param, &endptr, 10);
			//if (endptr == '\0') {
			// SW: MOS: - nasty bug - occasional failue to initialize RFCOMM properly
			if ((NULL != endptr) && (endptr[0] == '\0')) {
				t = new TransportRFCOMM(channel);
			}
		}
	}		       
	return t;	
}

bool operator==(const Transport& t1, const Transport& t2)
{
	return t1.equal(t2);
}

Transport *Transport::fromMetadata(const Metadata& m)
{
	Transport *t = NULL;
	const char *param;

	if (!m.isName(TRANSPORT_METADATA))
		return NULL;
	
	param = m.getParameter(TRANSPORT_METADATA_TYPE_PARAM);

	if (!param)
		return NULL;

	switch (strToType(param)) {
	case TYPE_UDP:
		t = TransportUDP::fromMetadata(m);
		break;
	case TYPE_TCP:
		t = TransportTCP::fromMetadata(m);
		break;
	case TYPE_RFCOMM:
		t = TransportRFCOMM::fromMetadata(m);
		break;
	default:
		break;
	}
	return t;
}

const char *Address::type_str[] = {
	"undefined",
	"ethernet",
	"ethernet_broadcast",
	"bluetooth",
	"ipv4",
	"ipv4_broadcast",
	"ipv6",
	"ipv6_broadcast",
	"filepath",
	"unix",
	NULL
};

const char *Address::uri_prefix[] = {
	"undef://",
	"eth://",
	"eth://",
	"bt://",
	"inet://",
	"inet://",
	"inet6://",
	"inet6://",
	"file://",
	"unix://",
	NULL
};

const char *Address::typeToStr(Type_t type)
{
	return type_str[type];
}

Address::Type_t Address::strToType(const char *str)
{
	int i = 0;

	while (type_str[i]) {
		if (strcmp(str, type_str[i]) == 0)
			return (Type_t)i;
		i++;
	}
	return TYPE_UNDEFINED;
}

Address::Address(Type_t _type, const void *_raw, const Transport& t) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_ADDRESS),
#endif
	type(_type), raw(static_cast<const unsigned char *>(_raw)), transport(t.copy()), uri(NULL), straddr(NULL)
{
}

Address::Address(const Address& a) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_ADDRESS),
#endif
	type(a.type), raw(NULL), transport(NULL), uri(NULL), straddr(NULL)
{
	if (a.uri && strlen(a.uri)) {
		if (allocURI(strlen(a.uri) + 1))
			strcpy(uri, a.uri);
	}

	if (a.straddr && strlen(a.straddr)) {
		if (allocStr(strlen(a.straddr) + 1))
			strcpy(straddr, a.straddr);
	}

	if (a.transport)
		transport = a.transport->copy();
}

bool Address::allocURI(size_t len) const
{
	if (!uri || sizeof(uri) < len) {
		if (uri)
			delete [] uri;
		
		const_cast<Address *>(this)->uri = new char[len];
	}
	return uri != NULL;
}

bool Address::allocStr(size_t len) const
{
	if (!straddr || sizeof(straddr) < len) {
		if (straddr)
			delete [] straddr;
		
		const_cast<Address *>(this)->straddr = new char[len];
	}
	return straddr != NULL;
}

Address::~Address() 
{
	if (transport)
		delete transport;

	if (uri)
		delete [] uri;

	if (straddr)
		delete [] straddr;
}

Metadata *Address::toMetadata() const
{
	Metadata *m;
	const Transport *t;

	m = new XMLMetadata(ADDRESS_METADATA);
	
	if (!m)
		return NULL;
	
	const char *str = getStr();
	if(!str) return NULL;

	m->setParameter(ADDRESS_METADATA_NETWORK_PARAM, str);
	m->setParameter(ADDRESS_METADATA_TYPE_PARAM, type_str[type]);
	
	t = getTransport();

	if (t) {
		m->addMetadata(t->toMetadata());
	}

	return m;
}

Address *Address::fromMetadata(const Metadata& m)
{
	Address *a = NULL;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;

	const char *param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (!param)
		return NULL;
	
	switch (strToType(param)) {
#if defined(ENABLE_ETHERNET)
	case TYPE_ETHERNET:
	case TYPE_ETHERNET_BROADCAST:
		a = EthernetAddress::fromMetadata(m);
		break;
#endif
#if defined(ENABLE_BLUETOOTH)
	case TYPE_BLUETOOTH:
		a = BluetoothAddress::fromMetadata(m);
		break;
#endif
	case TYPE_IPV4:
	case TYPE_IPV4_BROADCAST:
		a = IPv4Address::fromMetadata(m);
		break;
#if defined(ENABLE_IPv6)
	case TYPE_IPV6:
	case TYPE_IPV6_BROADCAST:
		a = IPv6Address::fromMetadata(m);
		break;
#endif
#if defined(OS_UNIX)
	case TYPE_UNIX:
		a = UnixAddress::fromMetadata(m);
		break;
#endif
	case TYPE_FILEPATH:
		a = FileAddress::fromMetadata(m);
		break;
	default:
		break;
	}
	
	return a;
}

bool operator==(const Address &a1, const Address &a2)
{
	return a1.equal(a2);
}

SocketAddress::SocketAddress(Type_t type, const void *raw, const Transport& t) : Address(type, raw, t)
{
}

SocketAddress::SocketAddress(const SocketAddress& a) : Address(a)
{
}

SocketAddress::~SocketAddress()
{
}

int SocketAddress::family() const
{
	int f = AF_NONE;
	
	switch (getType()) {
	case Address::TYPE_IPV4:
		f = AF_INET;
		break;
#if defined(ENABLE_IPV6)
	case Address::TYPE_IPV6:
		f = AF_INET;
		break;
#endif
#if defined(ENABLE_BLUETOOTH)
	case Address::TYPE_BLUETOOTH:
		f = AF_BLUETOOTH;
		break;
#endif
#if defined(OS_UNIX)
	case Address::TYPE_UNIX:
		f = AF_UNIX;
		break;
#endif
	default:
		break;
	}
	return f;
}

EthernetAddress::EthernetAddress(const unsigned char _mac[ETH_ALEN]) : SocketAddress(class_type, mac)
{
	memcpy(mac, _mac, ETH_ALEN);
}

EthernetAddress::EthernetAddress(const EthernetAddress& a) : SocketAddress(a)
{
	memcpy(mac, a.mac, ETH_ALEN);
	raw = mac;
}

EthernetAddress::~EthernetAddress()
{
}

static bool str_to_mac(unsigned char *mac, const char *str)
{
	unsigned int i, j = 0;

	for (i = 0; i < strlen(str); i++) {
		if (i == 0 || str[i-1] == ':') {
			char *endptr = NULL;
			mac[j++] = (unsigned char) strtol(&(str[i]), &endptr, 16);
			if (endptr == &str[i])
				break;
		}
	}

	return j == 6 ? true : false;
}

EthernetAddress *EthernetAddress::fromMetadata(const Metadata& m)
{
	EthernetAddress *a = NULL;
	const char *param;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;
	
	param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (param && strToType(param) == class_type) {
		param = m.getParameter(ADDRESS_METADATA_NETWORK_PARAM);

		if (param) {
			unsigned char mac[ETH_MAC_LEN];
			
			if (str_to_mac(mac, param)) {
				a = new EthernetAddress(mac);
			}
		}
	}

	return a;
}

const char *EthernetAddress::getURI() const
{
	if (!uri && allocURI(strlen(uri_prefix[type]) + strlen(getStr()) + 1)) {
		sprintf(uri, "%s%s", uri_prefix[type], getStr());
	}

	return uri;
}

const char *EthernetAddress::getStr() const
{
	if (!straddr && allocStr(18)) {
		sprintf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	
	return straddr;
}

socklen_t EthernetAddress::fillInSockaddr(struct sockaddr *sa) const
{
	if (!sa)
		return 0;
	
	memset(sa, 0, sizeof(*sa));

#if defined(OS_MACOSX)
	sa->sa_family = AF_LINK;
#elif defined(OS_LINUX)
	// AF_LINK does not exist in Linux
	//sa->sa_family = AF_LINK;
#endif
	memcpy(sa->sa_data, mac, ETH_ALEN);

	return sizeof(sa->sa_family) + ETH_ALEN;
}

bool EthernetAddress::equal(const Address& a) const
{
	return (type == a.getType() && memcmp(mac, static_cast<const EthernetAddress&>(a).mac, ETH_ALEN) == 0);
}

#if defined(ENABLE_BLUETOOTH)
BluetoothAddress::BluetoothAddress(const unsigned char _mac[BT_ALEN], const Transport& t) : 
	SocketAddress(class_type, mac, t)
{
	memcpy(mac, _mac, BT_ALEN);
}

BluetoothAddress::BluetoothAddress(const BluetoothAddress& a) : SocketAddress(a)
{
	memcpy(mac, a.mac, BT_ALEN);
	raw = mac;
}

BluetoothAddress::~BluetoothAddress()
{
}

const char *BluetoothAddress::getURI() const
{
	if (!uri && allocURI(strlen(uri_prefix[type]) + strlen(getStr()) + 1)) {
		sprintf(uri, "%s%s", uri_prefix[type], getStr());
	}
	
	return uri;
}

const char *BluetoothAddress::getStr() const
{
	if (!straddr && allocStr(18)) {
		sprintf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	
	return straddr;
}

socklen_t BluetoothAddress::fillInSockaddr(struct sockaddr *sa, unsigned short channel) const
{
	if (!sa)
		return 0;

	memset(sa, 0, sizeof(*sa));

#if defined(OS_LINUX) || (defined(OS_WINDOWS) && !defined(WIDCOMM_BLUETOOTH))
	struct sockaddr_bt *sa_bt = (struct sockaddr_bt *)sa;
	sa_bt->bt_family = AF_BLUETOOTH;

	if (channel != 0)
		sa_bt->bt_channel = channel;
	else if (getTransport()) {
		switch (getTransport()->getType()) {
		case Transport::TYPE_RFCOMM:
			sa_bt->bt_channel = htons(reinterpret_cast<const TransportRFCOMM *>(getTransport())->getChannel());
			break;
		default:
			break;
		}
	} 
	memcpy(&sa_bt->bt_bdaddr, mac, BT_ALEN);	
	return sizeof(struct sockaddr_bt);
#else
	return 0;
#endif

}

socklen_t BluetoothAddress::fillInSockaddr(struct sockaddr *sa) const
{
	return fillInSockaddr(sa, 0);
}

bool BluetoothAddress::equal(const Address& a) const
{
	return (type == a.getType() && memcmp(mac, a.getRaw(), BT_ALEN) == 0 && 
		((transport && a.getTransport() && *transport == *a.getTransport()) || 
		 (!transport && !a.getTransport())));
}

BluetoothAddress *BluetoothAddress::fromMetadata(const Metadata& m)
{
	BluetoothAddress *a = NULL;
	const char *param;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;
	
	param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (param && strToType(param) == class_type) {
		param = m.getParameter(ADDRESS_METADATA_NETWORK_PARAM);

		if (param) {
			unsigned char mac[BT_MAC_LEN];
			
			if (str_to_mac(mac, param)) {
				const Metadata *tm = m.getMetadata(ADDRESS_METADATA_TRANSPORT);
				Transport *t = NULL;

				if (tm) {
					t = Transport::fromMetadata(*tm);
				}
				
				a = t ? new BluetoothAddress(mac, *t) : new BluetoothAddress(mac);
			}
		}
	}

	return a;
}

#endif /* ENABLE_BLUETOOTH */

IPv4Address::IPv4Address(struct sockaddr_in& saddr, const Transport& t) : SocketAddress(class_type, &ipv4, t)
{
	memcpy(&ipv4, &saddr.sin_addr, sizeof(ipv4));
}

IPv4Address::IPv4Address(struct in_addr& inaddr, const Transport& t) : SocketAddress(class_type, &ipv4, t)
{
	memcpy(&ipv4, &inaddr, sizeof(ipv4));
}

IPv4Address::IPv4Address(const IPv4Address& a) : SocketAddress(a)
{
	memcpy(&ipv4, &a.ipv4, sizeof(ipv4));
	raw = (unsigned char *)&ipv4;
}

IPv4Address::~IPv4Address()
{
}

const char *IPv4Address::getURI() const
{
	if (!uri && allocURI(strlen(uri_prefix[type]) + strlen(getStr()) + 1)) {
	        const char *str = getStr();
		if(!str) { delete [] uri; const_cast<IPv4Address *>(this)->uri = NULL; return NULL; }
		sprintf(uri, "%s%s", uri_prefix[type], str);		
	}
	return uri;
}

const char *IPv4Address::getStr() const
{
	if (!straddr && allocStr(16)) {
		if (!inet_ntop(AF_INET, raw, straddr, 16)) {
			delete [] straddr;
			const_cast<IPv4Address *>(this)->straddr = NULL;
		}
	}
	
	return straddr;
}

socklen_t IPv4Address::fillInSockaddr(struct sockaddr_in *inaddr, unsigned short port) const
{
	if (!inaddr || !getTransport())
		return 0;

	memset(inaddr, 0, sizeof(*inaddr));
	
	inaddr->sin_family = AF_INET;
	
	if (port == 0) {
		switch (getTransport()->getType()) {
		case Transport::TYPE_UDP:
			inaddr->sin_port = htons(reinterpret_cast<const TransportUDP *>(getTransport())->getPort());
			break;
		case Transport::TYPE_TCP:
			inaddr->sin_port = htons(reinterpret_cast<const TransportTCP *>(getTransport())->getPort());
			break;
		default:
			inaddr->sin_port = htons(port);
			break;
		}
	} else {
		inaddr->sin_port = htons(port);
	}

	memcpy(&inaddr->sin_addr.s_addr, &ipv4, sizeof(ipv4));
	
	return sizeof(struct sockaddr_in);
}

socklen_t IPv4Address::fillInSockaddr(struct sockaddr *sa, unsigned short port) const
{ 
	return fillInSockaddr(reinterpret_cast<struct sockaddr_in *>(sa), port); 
}

socklen_t IPv4Address::fillInSockaddr(struct sockaddr *sa) const
{
	return fillInSockaddr(reinterpret_cast<struct sockaddr_in *>(sa), 0);
}
										    
bool IPv4Address::equal(const Address& a) const
{
	return (type == a.getType() && memcmp(&ipv4, a.getRaw(), sizeof(ipv4)) == 0 && 
		((transport && a.getTransport() && *transport == *a.getTransport()) || 
		 (!transport && !a.getTransport())));
}

IPv4Address *IPv4Address::fromMetadata(const Metadata& m)
{
	IPv4Address *a = NULL;
	const char *param;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;
	
	param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (param && (strToType(param) == class_type || strToType(param) == TYPE_IPV4_BROADCAST)) {
		const char *nparam = m.getParameter(ADDRESS_METADATA_NETWORK_PARAM);

		if (nparam) {
			struct in_addr ip;
			
			if (inet_pton(AF_INET, nparam, &ip) == 1) {
				const Metadata *tm = m.getMetadata(ADDRESS_METADATA_TRANSPORT);
				Transport *t = NULL;
				
				if (tm) {
					t = Transport::fromMetadata(*tm);
				}
				
				if (strToType(param) == class_type)
					a = t ? new IPv4Address(ip, *t) : new IPv4Address(ip);
				else
					a = new IPv4BroadcastAddress(ip);
			}
		}
	}

	return a;
}

#if defined(ENABLE_IPv6)
IPv6Address::IPv6Address(struct sockaddr_in6& saddr, const Transport& t) : SocketAddress(class_type, &ipv6, t)
{
	memcpy(&ipv6, &saddr.sin6_addr, sizeof(ipv6));
	
	if (t.getType() != Transport::TYPE_UDP &&
	    t.getType() != Transport::TYPE_TCP &&
	    t.getType() != Transport::TYPE_NONE) {
		HAGGLE_ERR("Bad IPv6 transport type %s\n", t.getURIPrefix());
	}
}

IPv6Address::IPv6Address(struct in6_addr& inaddr, const Transport& t) : SocketAddress(class_type, &ipv6, t)
{
	memcpy(&ipv6, &inaddr.s6_addr, sizeof(ipv6));
}

IPv6Address::IPv6Address(const IPv6Address& a) : SocketAddress(a)
{
	memcpy(&ipv6, &a.ipv6, sizeof(ipv6));
	raw = (unsigned char *)&ipv6;
}

IPv6Address::~IPv6Address()
{
}

const char *IPv6Address::getURI() const
{
	if (!uri && allocURI(strlen(uri_prefix[type]) + strlen(getStr()) + 1)) {
	        const char *str = getStr();
		if(!str) { delete [] uri; const_cast<IPv6Address *>(this)->uri = NULL; return NULL; }
		sprintf(uri, "%s%s", uri_prefix[type], str);
	}

	return uri;
}

const char *IPv6Address::getStr() const
{
	if (!straddr && allocStr(48)) {
		if (!inet_ntop(AF_INET6, raw, straddr, 48)) {
			delete [] straddr;
			const_cast<IPv6Address *>(this)->straddr = NULL;
		}
	}
	
	return straddr;
}

socklen_t IPv6Address::fillInSockaddr(struct sockaddr_in6 *in6addr, unsigned short port) const
{
	if (!in6addr || !getTransport())
		return 0;

	memset(in6addr, 0, sizeof(*in6addr));

	in6addr->sin6_family = AF_INET6;
	
	if (port == 0) {
		switch (getTransport()->getType()) {
		case Transport::TYPE_UDP:
			in6addr->sin6_port = htons(reinterpret_cast<const TransportUDP *>(getTransport())->getPort());
			break;
		case Transport::TYPE_TCP:
			in6addr->sin6_port = htons(reinterpret_cast<const TransportTCP *>(getTransport())->getPort());
			break;
		default:
			in6addr->sin6_port = htons(port);
			break;
		}
	} else {
		in6addr->sin6_port = htons(port);
	}

	memcpy(&in6addr->sin6_addr, &ipv6, sizeof(ipv6));

	return sizeof(struct sockaddr_in6);
}

socklen_t IPv6Address::fillInSockaddr(struct sockaddr *sa, unsigned short port) const
{ 
	return fillInSockaddr(reinterpret_cast<struct sockaddr_in6 *>(sa), port); 
}

socklen_t IPv6Address::fillInSockaddr(struct sockaddr *sa) const
{
	return fillInSockaddr(reinterpret_cast<struct sockaddr_in6 *>(sa), 0);
}

bool IPv6Address::equal(const Address& a) const
{
	return (type == a.getType() && memcmp(&ipv6, a.getRaw(), sizeof(ipv6)) == 0 && 
		((transport && a.getTransport() && *transport == *a.getTransport()) || 
		 (!transport && !a.getTransport())));
}

IPv6Address *IPv6Address::fromMetadata(const Metadata& m)
{
	IPv6Address *a = NULL;
	const char *param;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;
	
	param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (param && (strToType(param) == class_type || strToType(param) == TYPE_IPV6_BROADCAST)) {
		const char *nparam = m.getParameter(ADDRESS_METADATA_NETWORK_PARAM);

		if (nparam) {
			struct in6_addr ip;
			
			if (inet_pton(AF_INET6, nparam, &ip) == 1) {
				const Metadata *tm = m.getMetadata(ADDRESS_METADATA_TRANSPORT);
				Transport *t = NULL;
				
				if (tm) {
					t = Transport::fromMetadata(*tm);
				}
				
				if (strToType(param) == class_type)
					a = t ? new IPv6Address(ip, *t) : new IPv6Address(ip);
				else
					a = new IPv6BroadcastAddress(ip);
			}
		}
	}

	return a;
}

#endif

FileAddress::FileAddress(const char *path) : Address(class_type, filepath)
{
	if (path && strlen(path) > 0) {
		filepath = new char[strlen(path) + 1];

		if (filepath) {
			strcpy(const_cast<char *>(filepath), path);
		}
	}
}

FileAddress::FileAddress(const FileAddress& a) : Address(a)
{
	if (a.filepath && strlen(a.filepath) > 0) {
		filepath = new char[strlen(a.filepath) + 1];

		if (filepath) {
			strcpy(const_cast<char *>(filepath), a.filepath);
		}
	}
	raw = reinterpret_cast<const unsigned char *>(filepath);
}

FileAddress::~FileAddress()
{
	if (filepath)
		delete [] filepath;
}

const char *FileAddress::getURI() const
{
	if (!uri && allocURI(strlen(uri_prefix[type]) + strlen(getStr()) + 1)) {
		sprintf(uri, "%s%s", uri_prefix[type], getStr());
	}

	return uri;
}

const char *FileAddress::getStr() const
{
	return filepath;
}

bool FileAddress::equal(const Address& a) const
{
	return (type == a.getType() && 
		((filepath && a.getRaw() && strcmp(filepath, (char*)a.getRaw()) == 0) || 
		 (!filepath && !a.getRaw())));
}

FileAddress *FileAddress::fromMetadata(const Metadata& m)
{
	FileAddress *a = NULL;
	const char *param;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;
	
	param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (param && strToType(param) == class_type) {
		// TODO
	}

	return a;
}

#if defined(OS_UNIX)
UnixAddress::UnixAddress(struct sockaddr_un& saddr) : SocketAddress(class_type, filepath)
{
	if (strlen(saddr.sun_path) > 0) {
		filepath = new char[strlen(saddr.sun_path) + 1];

		if (filepath) {
			strcpy(const_cast<char *>(filepath), saddr.sun_path);
		}
	}
}

UnixAddress::UnixAddress(const char *path) : SocketAddress(class_type, filepath)
{
	if (path && strlen(path) > 0) {
		filepath = new char[strlen(path) + 1];
		
		if (filepath) {
			strcpy(const_cast<char *>(filepath), path);
		}
	}
}

UnixAddress::UnixAddress(const UnixAddress& a) : SocketAddress(a)
{
	if (a.filepath && strlen(a.filepath) > 0) {
		filepath = new char[strlen(a.filepath) + 1];

		if (filepath) {
			strcpy(const_cast<char *>(filepath), a.filepath);
		}
	}
	raw = reinterpret_cast<const unsigned char *>(filepath);
}

UnixAddress::~UnixAddress()
{
	if (filepath)
		delete [] filepath;
}

const char *UnixAddress::getURI() const
{
	if (!uri && allocURI(strlen(uri_prefix[type]) + strlen(getStr()) + 1)) {
		sprintf(uri, "%s%s", uri_prefix[type], getStr());
	}

	return uri;
}

const char *UnixAddress::getStr() const
{
	return filepath;
}

socklen_t UnixAddress::fillInSockaddr(struct sockaddr *sa) const
{
	return fillInSockaddr((struct sockaddr_un *)sa);
}

socklen_t UnixAddress::fillInSockaddr(struct sockaddr_un *unaddr) const
{
	if (!unaddr)
		return 0;

	memset(unaddr, 0, sizeof(*unaddr));

	if (!filepath || strlen(filepath) == 0)
		return 0;

	unaddr->sun_family = AF_UNIX;
	strcpy(unaddr->sun_path, filepath);
	
	return sizeof(struct sockaddr_un);
}

bool UnixAddress::equal(const Address& a) const
{
	return (type == a.getType() && 
		((filepath && a.getRaw() && strcmp(filepath, (char*)a.getRaw()) == 0) || 
		 (!filepath && !a.getRaw())));
}

UnixAddress *UnixAddress::fromMetadata(const Metadata& m)
{
	UnixAddress *a = NULL;
	const char *param;

	if (!m.isName(ADDRESS_METADATA))
		return NULL;
	
	param = m.getParameter(ADDRESS_METADATA_TYPE_PARAM);

	if (param && strToType(param) == class_type) {
		// TODO
	}

	return a;
}
#endif // OS_UNIX

// Container list
Addresses::Addresses(const Addresses& adds) : List<Address *>()
{
	for (Addresses::const_iterator it = adds.begin();
		it != adds.end(); it++) {
		push_back((*it)->copy());
	}
}

Addresses::~Addresses()
{
	while (!empty()) {
		delete pop();
	}
}

Addresses *Addresses::copy() const
{
	return new Addresses(*this);
}

Address *Addresses::pop()
{
	if (empty())
		return NULL;
	else {
		Address *n = front();
		pop_front();
		return n;
	}
}
