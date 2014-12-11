/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
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
#include <string.h>

#include <libcpphaggle/Platform.h>
#include <base64.h>
#include "XMLMetadata.h"
#include "Trace.h"
#include "Interface.h"

using namespace haggle;

const char *Interface::typestr[] = {
	"undefined",
	"application[port]",
	"application[local]",
	"ethernet",
	"wifi",
	"bluetooth",
	"media",
	NULL,
};

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

Interface::Interface(Interface::Type_t _type, const void *_identifier, size_t _identifier_len,
		     const string& _name, const Address *addr, const flag_t _flags) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_INTERFACE),
#endif
	type(_type), name(_name), identifier(static_cast<const unsigned char *>(_identifier)), 
	identifier_len(_identifier_len), flags(_flags & (IFFLAG_ALL ^ IFFLAG_STORED)), 
	identifier_str("Undefined")
{
	if (addr)
		addAddress(*addr);
}

Interface::Interface(const Interface &iface) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_INTERFACE),
#endif
	type(iface.type), name(iface.name), identifier(NULL), 
	identifier_len(iface.identifier_len), flags(iface.flags & (IFFLAG_ALL ^ IFFLAG_STORED)),  
	identifier_str(iface.identifier_str), addresses(iface.addresses)
{
}

Interface::Interface(const Interface &iface, const void *_identifier) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_INTERFACE),
#endif
	type(iface.type), name(iface.name), identifier(static_cast<const unsigned char *>(_identifier)), 
	identifier_len(iface.identifier_len), flags(iface.flags & (IFFLAG_ALL ^ IFFLAG_STORED)),  
	identifier_str(iface.identifier_str), addresses(iface.addresses)
{
}

Interface::~Interface(void)
{
}

Interface *Interface::create(Type_t type, const void *identifier, const char *name, flag_t flags)
{
	if (!name)
		name = DEFAULT_INTERFACE_NAME;
	
	switch (type) {
	case TYPE_APPLICATION_PORT:
		return new ApplicationPortInterface(*static_cast<const unsigned short *>(identifier), name, NULL, flags);
	case TYPE_APPLICATION_LOCAL:
		return new ApplicationLocalInterface(static_cast<const char *>(identifier), name, NULL, flags);
	case TYPE_ETHERNET:
		return new EthernetInterface(static_cast<const unsigned char *>(identifier), name, NULL, flags);
	case TYPE_WIFI:
		return new WiFiInterface(static_cast<const unsigned char *>(identifier), name, NULL, flags);
	case TYPE_BLUETOOTH:
		return new BluetoothInterface(static_cast<const unsigned char *>(identifier), name, NULL, flags);
	case TYPE_MEDIA:
		return new MediaInterface(static_cast<const char *>(identifier), name, flags);
	default:
		HAGGLE_ERR("Unknown type=%u\n", type);
		return NULL;
		
	}
	return NULL;
}

Interface *Interface::create(Type_t type, const void *identifier, const char *name, const Address& addr, flag_t flags)
{	
	Interface *iface = create(type, identifier, name, flags);

	if (iface) {
		iface->addAddress(addr);
	}
	return iface;
}

Interface *Interface::create(Type_t type, const void *identifier, const char *name, const Addresses& addrs, flag_t flags)
{
	Interface *iface = create(type, identifier, name, flags);
	
	if (iface && addrs.size()) {
		iface->addAddresses(addrs);
	}
	return iface;
}

const char *Interface::typeToStr(Interface::Type_t type)
{
	return typestr[type];
}

Interface::Type_t Interface::strToType(const char *str)
{
	int i = 0;

	while (typestr[i]) {
		if (strcmp(typestr[i], str) == 0) {
			return (Interface::Type_t)i;
		}
		i++;
	}
	return Interface::TYPE_UNDEFINED;
}

bool Interface::str_to_identifier(const char *id_str, void **identifier, size_t *identifier_len)
{
	struct base64_decode_context b64_ctx;
	
	base64_decode_ctx_init(&b64_ctx);
	
	if (!base64_decode_alloc(&b64_ctx, id_str, strlen(id_str), (char**)identifier, identifier_len))
		return false;
	
	return true;
}

size_t Interface::getIdentifierLen() const
{
	return identifier_len;
}

string Interface::idString(const InterfaceRef& iface)
{
  string id(iface->getIdentifierStr());
  return id;
}

const char *Interface::getFlagsStr() const
{
	static string str;
	
	str.clear();
	
	if (flags & IFFLAG_UP)
		str += "UP";
	else
		str += "DOWN";
	
	if (flags & IFFLAG_LOCAL)
		str += "|LOCAL";
	
	if (flags & IFFLAG_SNOOPED)
		str += "|SNOOPED";
	
	return str.c_str();
}

void Interface::addAddress(const Address& addr)
{
	for (Addresses::iterator it = addresses.begin(); it != addresses.end(); it++) {
		// Are these the same?
		if (addr == *(*it)) {
			// Merge:
			(*it)->merge(addr);
			return;
		}
	}
	// Address not found in list, insert it.
	addresses.push_front(addr.copy());
}

void Interface::addAddresses(const Addresses& addrs)
{
	for (Addresses::const_iterator it = addrs.begin(); it != addrs.end(); it++) {
		addAddress(**it);
	}
}

bool Interface::hasAddress(const Address& add) const
{
	for (Addresses::const_iterator it = addresses.begin(); it != addresses.end(); it++) {
		// Are these the same?
		if (add == *(*it)) {
			return true;
		}
	}
	return false;
}

const char *Interface::getName() const
{
	return name.c_str();
}

const Addresses *Interface::getAddresses() const
{
	return &addresses;
}

void Interface::setFlag(flag_t flag)
{
	flags |= flag;
}

void Interface::resetFlag(flag_t flag)
{
	flags &= (flag ^ 0xff);
}

bool Interface::isLocal() const
{
	return (flags & IFFLAG_LOCAL) != 0;
}

bool Interface::isFlagSet(flag_t flag) const
{
	return (flags & flag) != 0;
}

bool Interface::isStored() const
{
	return (flags & IFFLAG_STORED) != 0;
}

bool Interface::isApplication() const
{
	return (type == TYPE_APPLICATION_PORT || type == TYPE_APPLICATION_LOCAL);
}

void Interface::up()
{
	flags |= IFFLAG_UP;
}

void Interface::down()
{
	flags ^= IFFLAG_UP;
}

bool Interface::isUp() const
{
	return (flags & IFFLAG_UP) != 0;
}

bool Interface::isSnooped() const
{
	return (flags & IFFLAG_SNOOPED) != 0;
}

Interface *Interface::fromMetadata(const Metadata& m)
{
	Addresses addrs;
	Interface *iface = NULL;
	void *identifier = NULL;
	
	if (!m.isName(INTERFACE_METADATA)) {
		HAGGLE_ERR("Metadata=%s is not interface metadata\n", 
			   m.getName().c_str());
		return NULL;
	}
	
	const char *param = m.getParameter(INTERFACE_METADATA_TYPE_PARAM);
	
	if (!param) {
		HAGGLE_ERR("No interface type parameter\n");
		return NULL;
	}
	Type_t type = strToType(param);
	
	if (type == TYPE_UNDEFINED) {
		HAGGLE_ERR("type is undefined\n");
		return NULL;
	}
		
	param = m.getParameter(INTERFACE_METADATA_IDENTIFIER_PARAM);
		
	if (param) {
		size_t identifier_len;
		
		if (!str_to_identifier(param, &identifier, &identifier_len)) {
			HAGGLE_ERR("could not parse identifier parameter\n");
			return NULL;
		}
	} else if ((param = m.getParameter(INTERFACE_METADATA_MAC_PARAM))) {
		identifier = malloc(GENERIC_MAC_LEN);
		
		if (!identifier || !str_to_mac((unsigned char *)identifier, param)) {
			HAGGLE_ERR("could not parse mac parameter\n");
			return NULL;
		}
	}

	const Metadata *am = m.getMetadata(ADDRESS_METADATA);
			
	while (am) {
		Address *addr = Address::fromMetadata(*am);
		
		if (addr)
			addrs.push_back(addr);
		
		am = m.getNextMetadata();
	}

	param = m.getParameter(INTERFACE_METADATA_NAME_PARAM);
	iface = create(type, identifier, param ? param : DEFAULT_INTERFACE_NAME, addrs, 0);
	free(identifier);
	
	return iface;
}

Metadata *Interface::toMetadata() const
{
	char *b64str = NULL;
	
	Metadata *m = new XMLMetadata(INTERFACE_METADATA); 
	
	if (!m)
		return NULL;
	
	m->setParameter(INTERFACE_METADATA_TYPE_PARAM, getTypeStr());
	
	base64_encode_alloc((const char *)getIdentifier(), getIdentifierLen(), &b64str);
	
	if (!b64str) {
		delete m;
		return NULL;
	}
	
	m->setParameter(INTERFACE_METADATA_IDENTIFIER_PARAM, b64str);
	
	free(b64str);
	
	const Addresses *adds = getAddresses();
	
	for (Addresses::const_iterator it = adds->begin(); it != adds->end(); it++) {
		m->addMetadata((*it)->toMetadata());
	}
	
	return m;
}

bool Interface::equal(const Interface::Type_t type, const unsigned char *identifier) const
{
	if (type != this->type)
		return false;
	
	return memcmp(this->identifier, identifier, identifier_len) == 0;
}

bool operator==(const Interface& i1, const Interface& i2)
{
	if (i1.type != i2.type || i1.identifier_len != i2.identifier_len)
		return false;

	return memcmp(i1.identifier, i2.identifier, i1.identifier_len) == 0;
}

bool operator<(const Interface& i1, const Interface& i2)
{
	if (i1.type < i2.type)
		return true;
	else if (i1.type > i2.type)
		return false;
	else if (i1.identifier_len < i2.identifier_len)
		return true;

	return memcmp(i1.identifier, i2.identifier, i1.identifier_len) < 0;
}

ApplicationInterface::ApplicationInterface(Interface::Type_t type, const void *identifier, size_t identifier_len, 
					   const string name, const Address *a, flag_t flags) : 
	Interface(type, identifier, identifier_len, name, a, flags)
{
}

ApplicationInterface::ApplicationInterface(const ApplicationInterface& iface, const void *identifier) : 
	Interface(iface, identifier)
{
}

ApplicationInterface::~ApplicationInterface()
{
}

ApplicationPortInterface::ApplicationPortInterface(const void *identifier, size_t identifier_len, const string name, flag_t flags) :
	ApplicationInterface(Interface::TYPE_APPLICATION_PORT, identifier, identifier_len, name, NULL, flags), port(0)
{
	if (identifier && identifier_len == sizeof(port)) {
		memcpy(&port, identifier, sizeof(port));
		setIdentifierStr();
	}
}

ApplicationPortInterface::ApplicationPortInterface(const unsigned short _port, const string name, const Address *a, flag_t flags) : 
	ApplicationInterface(Interface::TYPE_APPLICATION_PORT, &port, sizeof(port), name, a, flags), port(_port)
{
	setIdentifierStr();
}

ApplicationPortInterface::ApplicationPortInterface(const ApplicationPortInterface& iface) : 
	ApplicationInterface(iface, &port), port(iface.port)
{
}

ApplicationPortInterface::~ApplicationPortInterface()
{
}

void ApplicationPortInterface::setIdentifierStr()
{
	char port_str[20];
	snprintf(port_str, 20, "%u", port);
	identifier_str = string("Application on port ") + port_str;
}

Interface *ApplicationPortInterface::copy() const
{
	return new ApplicationPortInterface(*this);
}

ApplicationLocalInterface::ApplicationLocalInterface(const void *identifier, size_t identifier_len, const string name, flag_t flags) :
	ApplicationInterface(Interface::TYPE_APPLICATION_LOCAL, path, identifier_len, name, NULL, flags)
{
	if (identifier && identifier_len > 0 && identifier_len < LOCAL_PATH_MAX) {
		strcpy(path, static_cast<const char *>(identifier));
		setIdentifierStr();
	}
}

ApplicationLocalInterface::ApplicationLocalInterface(const string _path, const string name, const Address *a, flag_t flags) :
	ApplicationInterface(Interface::TYPE_APPLICATION_LOCAL, path, _path.length(), name, a, flags)
{
	if (_path.length() < LOCAL_PATH_MAX) {
		strcpy(path, _path.c_str());
		setIdentifierStr();
	}
}

ApplicationLocalInterface::~ApplicationLocalInterface()
{
}

ApplicationLocalInterface::ApplicationLocalInterface(const ApplicationLocalInterface& iface) :
	ApplicationInterface(iface, path)
{
	strcpy(path, iface.path);
}

void ApplicationLocalInterface::setIdentifierStr()
{
	identifier_str = string("Application on local unix socket: ") + path;
}

Interface *ApplicationLocalInterface::copy() const
{
	return new ApplicationLocalInterface(*this);
}

EthernetInterface::EthernetInterface(const void *_identifier, size_t identifier_len, 
				     const string name, flag_t flags) :
	Interface(Interface::TYPE_ETHERNET, mac, ETH_MAC_LEN, name, NULL, flags)
{	
	bzero(mac, ETH_MAC_LEN);
	if (_identifier && identifier_len == ETH_MAC_LEN) {
		memcpy(mac, (unsigned char *)_identifier, ETH_MAC_LEN);
		setIdentifierStr();
	}
}

EthernetInterface::EthernetInterface(Interface::Type_t type, const unsigned char *_mac, 
				     const string name, const Address *a, flag_t flags) :
	Interface(type, mac, ETH_MAC_LEN, name, a, flags)
{
	if (_mac) {
		memcpy(mac, _mac, ETH_MAC_LEN);
		setIdentifierStr();
	}
}

EthernetInterface::EthernetInterface(const unsigned char _mac[ETH_MAC_LEN], const string name, const Address *a, flag_t flags) : 
	Interface(Interface::TYPE_ETHERNET, mac, ETH_MAC_LEN, name, a, flags)
{
	if (_mac) {
		memcpy(mac, _mac, ETH_MAC_LEN);
		setIdentifierStr();
	}
}

EthernetInterface::EthernetInterface(const EthernetInterface& iface) :
	Interface(iface, mac)
{
	if (iface.mac) {
		memcpy(mac, iface.mac, ETH_MAC_LEN);
	}
}

EthernetInterface::~EthernetInterface()
{
}

void EthernetInterface::setIdentifierStr()
{
	char buf[18];
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		identifier[0], identifier[1],
		identifier[2], identifier[3],
		identifier[4], identifier[5]);
    // SW: allocating sting on heap
	identifier_str = string(buf);
}

Interface *EthernetInterface::copy() const
{
	return new EthernetInterface(*this);
}

Metadata *EthernetInterface::toMetadata(bool with_mac) const
{
	Metadata *m = this->Interface::toMetadata();
	
	if (m && with_mac) {
		m->setParameter(INTERFACE_METADATA_MAC_PARAM, getIdentifierStr());
	}
	
	return m;
}

EthernetInterface *EthernetInterface::fromMetadata(const Metadata& m)
{
	Interface *iface = Interface::fromMetadata(m);
	
	if (iface && iface->getType() != TYPE_ETHERNET) {
		delete iface;
		return NULL;
	}
	
	return static_cast<EthernetInterface *>(iface);
}

WiFiInterface::WiFiInterface(const void *identifier, size_t identifier_len, const string name, flag_t flags) :
	EthernetInterface(Interface::TYPE_WIFI, static_cast<const unsigned char *>(identifier), name, NULL, flags)
{
}

WiFiInterface::WiFiInterface(const unsigned char _mac[ETH_MAC_LEN], const string name, 
			     const Address *a, flag_t flags) :
	EthernetInterface(Interface::TYPE_WIFI, _mac, name, a, flags)
{
}

WiFiInterface::WiFiInterface(const WiFiInterface& iface) :
	EthernetInterface(iface)
{
}

WiFiInterface::~WiFiInterface()
{
}

Interface *WiFiInterface::copy() const
{
	return new WiFiInterface(*this);
}

WiFiInterface *WiFiInterface::fromMetadata(const Metadata& m)
{
	Interface *iface = Interface::fromMetadata(m);
	
	if (iface && iface->getType() != TYPE_WIFI) {
		delete iface;
		return NULL;
	}
	
	return static_cast<WiFiInterface *>(iface);
}

BluetoothInterface::BluetoothInterface(const void *identifier, size_t identifier_len, 
				       const string name, flag_t flags) :
	Interface(Interface::TYPE_BLUETOOTH, mac, BT_MAC_LEN, name, NULL, flags)
{
	if (identifier && identifier_len == BT_MAC_LEN) {
		memcpy(mac, identifier, BT_MAC_LEN);
		setIdentifierStr();
	}
}

BluetoothInterface::BluetoothInterface(const unsigned char _mac[BT_MAC_LEN], const string name, const Address *a, flag_t flags) :
	Interface(Interface::TYPE_BLUETOOTH, mac, BT_MAC_LEN, name, a, flags)
{
	if (_mac) {
		memcpy(mac, _mac, BT_MAC_LEN);
		setIdentifierStr();
	}
}

BluetoothInterface::BluetoothInterface(const BluetoothInterface& iface) :
	Interface(iface, mac)
{
	if (iface.mac) {
		memcpy(mac, iface.mac, BT_MAC_LEN);
	}
}

BluetoothInterface::~BluetoothInterface()
{
}

void BluetoothInterface::setIdentifierStr()
{
	char buf[30];
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		identifier[0], identifier[1],
		identifier[2], identifier[3],
		identifier[4], identifier[5]);
	identifier_str = buf;
}

Interface *BluetoothInterface::copy() const
{
	return new BluetoothInterface(*this);
}

Metadata *BluetoothInterface::toMetadata(bool with_mac) const
{
	Metadata *m = this->Interface::toMetadata();
	
	if (m && with_mac) {
		m->setParameter(INTERFACE_METADATA_MAC_PARAM, getIdentifierStr());
	}
	
	return m;
}

BluetoothInterface *BluetoothInterface::fromMetadata(const Metadata& m)
{
	Interface *iface = Interface::fromMetadata(m);
	
	if (iface && iface->getType() != TYPE_BLUETOOTH) {
		delete iface;
		return NULL;
	}
	
	return static_cast<BluetoothInterface *>(iface);
}

MediaInterface::MediaInterface(const void *identifier, size_t identifier_len, 
			       const string name, flag_t flags) :
	Interface(Interface::TYPE_MEDIA, path.c_str(), identifier_len, name, NULL, flags)
{
	if (identifier && identifier_len > 0) {
		path = static_cast<const char *>(identifier);
		setIdentifierStr();
	}
}

MediaInterface::MediaInterface(const string _path, const string name, flag_t flags) :
	Interface(Interface::TYPE_MEDIA, path.c_str(), _path.length(), name, NULL, flags), 
	path(_path)
{
	setIdentifierStr();
}

MediaInterface::MediaInterface(const MediaInterface& iface) :
	Interface(iface, path.c_str()), path(iface.path)
{
}

MediaInterface::~MediaInterface()
{
}

void MediaInterface::setIdentifierStr()
{
	identifier_str = path;
}

Interface *MediaInterface::copy() const
{
	return new MediaInterface(*this);
}
