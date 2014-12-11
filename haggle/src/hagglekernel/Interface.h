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
#ifndef _INTERFACE_H
#define _INTERFACE_H

class Interface;

#include <time.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Thread.h>
#include <libcpphaggle/Reference.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Pair.h>
#include <haggleutils.h>

#include "Debug.h"
#include "Address.h"

using namespace haggle;

// Some interface flags
#define IFFLAG_UP                            (1<<0)
#define IFFLAG_LOCAL                         (1<<1)
#define IFFLAG_SNOOPED                       (1<<2) /* This interface was found by snooping on 
						       incoming data objects */
#define IFFLAG_STORED                        (1<<3) // This interface is in the Interface store
#define IFFLAG_ALL	                     ((1<<4)-1)

#define MAX_MEDIA_PATH_LEN 100
#define GENERIC_MAC_LEN 6
#define BT_MAC_LEN GENERIC_MAC_LEN
#define ETH_MAC_LEN GENERIC_MAC_LEN
#define WIFI_MAC_LEN ETH_LEN
#define LOCAL_PATH_MAX 108 // This is based on the UNIX_PATH_MAX in linux/un.h from the Linux headers

#define DEFAULT_INTERFACE_NAME "Unnamed Interface"

#define INTERFACE_METADATA "Interface"
#define INTERFACE_METADATA_TYPE_PARAM "type"
#define INTERFACE_METADATA_IDENTIFIER_PARAM "identifier"
#define INTERFACE_METADATA_NAME_PARAM "mac"
#define INTERFACE_METADATA_MAC_PARAM "mac"

typedef Reference<Interface> InterfaceRef;

/**
	Interface class.
	
	This is the class that keeps interface information. 
 */
#ifdef DEBUG_LEAKS
class Interface : public LeakMonitor
#else
class Interface
#endif
{
public:
	typedef enum {
		TYPE_UNDEFINED = 0,
		TYPE_APPLICATION_PORT,
		TYPE_APPLICATION_LOCAL,
		TYPE_ETHERNET,
		TYPE_WIFI,
		TYPE_BLUETOOTH,
		TYPE_MEDIA
	} Type_t;
protected:
	const Type_t type;
	// The name of this interface
	const string name;
	const unsigned char *identifier;
	size_t identifier_len;
	static const char *typestr[];

	typedef unsigned char flag_t;
	// Flag field to show certain boolean things.
	flag_t flags;

	// Identifier in string format
	string identifier_str;

	// The addresses associated with this interface.
	Addresses addresses;
	/**
		Constructors.
	*/
	Interface(Type_t type, const void *identifier = NULL, size_t identifier_len = 0, 
		  const string& name = DEFAULT_INTERFACE_NAME, 
		  const Address *addr = NULL, flag_t flags = 0);

	Interface(const Interface &iface, const void *identifier);
	
	static bool str_to_identifier(const char *id_str, void **identifier, size_t *identifier_len);
public:
	/* Template create functions. These return an interface of a specified type */
	template<typename T>
	static T *create(const void *identifier, const char *name, flag_t flags);

	template<typename T>
	static T *create(const void *identifier, const char *name, const Address& addr, flag_t flags);

	template<typename T>
	static T *create(const void *identifier, const char *name, const Addresses& addrs, flag_t flags);

	/* Create functions that return an interface down-cast to base type. */
	static Interface *create(Type_t type, const void *identifier, const char *name = DEFAULT_INTERFACE_NAME, 
				 flag_t flags = 0); 
	static Interface *create(Type_t type, const void *identifier, const char *name, 
				 const Address& addr, flag_t flags = 0); 
	static Interface *create(Type_t type, const void *identifier, const char *name, 
				 const Addresses& addrs, flag_t flags = 0); 
	
	Interface(const Interface& iface);
	/**
		Destructor.
	*/
	virtual ~Interface();

	virtual Interface *copy() const = 0;
	static Interface::Type_t strToType(const char *str);
	static const char *typeToStr(Type_t type);

	/**
		Gets the currently set name for this interface.
	*/
	const char *getName() const;

	/**
		Gets the type for this interface.
	*/
	Interface::Type_t getType() const { return type; }
	const char *getTypeStr() const { return typeToStr(type); }
	/**
		Gets a pointer to the beginning of the raw identifier which is unique for this interface.
	*/
	const unsigned char *getIdentifier() const { return identifier; }

	/** 
		Gets the length of the raw identifier which is unique for this interface.
	*/
	size_t getIdentifierLen() const;
	/**
		Gets the identifier for this interface as a pair.
	*/
	Pair<const unsigned char *, size_t> getIdentifierPair() const { return make_pair(identifier, identifier_len); }
	
	/**
		Gets the identifier as a human readable C-string.
	*/
	const char *getIdentifierStr() const { return identifier_str.c_str(); }
	static string idString(const InterfaceRef& iface); // MOS
	
	/**
		Gets the flags as a human readable C-string.
	 */
	const char *getFlagsStr() const;
	/**
		Inserts another address into this interface.
		
		The address is the property of the caller and the caller shall take 
		responsibility for the memory associated with it.
	*/
	void addAddress(const Address& addr);
	
	/**
		Inserts a set of addresses into this interface.
		
		The addresses are the property of the caller and the caller shall take 
		responsibility for the memory associated with it.
	*/
	void addAddresses(const Addresses& addrs);
	
	/**
		Returns a list of the addresses referring to this interface.
		
		The returned list is the property of the interface.
	*/
	const Addresses *getAddresses() const;
	
	/**
		Returns true iff the interface has an address that is equal to the given
		address.
	*/
	bool hasAddress(const Address& add) const;
	
	/**
		Returns the first found address of the type given by the template.
		
		May return NULL, if no such address was found.
		
		The returned address is the property of the interface.
	*/
	template<typename T>
	T *getAddress() {
		for (Addresses::iterator it = addresses.begin(); it != addresses.end(); it++) {
			if ((*it)->getType() == T::class_type) {
				return static_cast<T*>(*it);
			}
		}
		return NULL;
	}

	template<typename T> 
	const T *getAddress() const
	{
		return const_cast<Interface *>(this)->getAddress<T>();
	}
	
	/** 
		Sets a given flag.
	*/
	void setFlag(flag_t flag);

	/** 
		Resets a given flag.
	*/
	void resetFlag(flag_t flag);
	/** 
		Generic flag set check.
	*/
	bool isFlagSet(flag_t flag) const;
	/** 
		Is a flag set or not?
	*/
	bool isStored() const;

	/**
		Returns true iff this is a local interface.
	*/
	bool isLocal() const;
	/**
		Returns true iff this is an application interface.
	*/
	bool isApplication() const;
	
	/**
		Sets this interface as being up. 
		
		Interfaces are assumed to be down when they are created. (Connectivities
		should NOT call this function before inserting the interface using
		report_interface. See ConnectivityManager::report_interface for an 
		explanation.)
		
		Also sends a notification of this to haggle if the value changed.
	*/
	void up();
	/**
		Sets this interface as being down. 
		
		Also sends a notification of this to haggle if the value changed.
	*/
	void down();
	/**
		Returns true iff this interface is up.
	*/
	bool isUp() const;
	/**
		Return true iff this interface was found by snooping, i.e.,
		it was not detected by a connectivity.
	*/
	bool isSnooped() const;
	/**
		Returns true if the type and identifier match.
	*/
	static Interface *fromMetadata(const Metadata& m);
	virtual Metadata *toMetadata() const;
	virtual bool equal(Type_t type, const unsigned char *identifier) const;
	/**
		Equality operator.
	*/
	friend bool operator==(const Interface& i1, const Interface& i2);
	friend bool operator<(const Interface& i1, const Interface& i2);
};

class ApplicationInterface : public Interface {
	friend class Interface;
protected:
	ApplicationInterface(Interface::Type_t type, const void *identifier, size_t identifer_len, 
			     const string name = DEFAULT_INTERFACE_NAME, const Address *a = NULL, flag_t flags = 0);
	ApplicationInterface(const ApplicationInterface& iface, const void *identifier);
	virtual ~ApplicationInterface() = 0;
public:
	ApplicationInterface(const ApplicationInterface& iface);
};

typedef Reference<ApplicationInterface> ApplicationInterfaceRef;
typedef ReferenceList<ApplicationInterface> ApplicationInterfaceRefList;

class ApplicationPortInterface : public ApplicationInterface {
	friend class Interface;
	unsigned short port;
	void setIdentifierStr();
	ApplicationPortInterface(const void *identifier = NULL, size_t identifier_len = 0, 
				 const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
public:
	static const Type_t class_type = TYPE_APPLICATION_PORT;
	ApplicationPortInterface(const unsigned short port, const string name = DEFAULT_INTERFACE_NAME, 
				 const Address *a = NULL, flag_t flags= 0);
	ApplicationPortInterface(const ApplicationPortInterface& iface);
	~ApplicationPortInterface();
	Interface *copy() const;
};

class ApplicationLocalInterface : public ApplicationInterface {
	friend class Interface;
	char path[LOCAL_PATH_MAX];
	void setIdentifierStr();
	ApplicationLocalInterface(const void *identifier = NULL, size_t identifier_len = 0, 
				  const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
public:
	static const Type_t class_type = TYPE_APPLICATION_LOCAL;
	ApplicationLocalInterface(const string _path, const string name = DEFAULT_INTERFACE_NAME, 
				  const Address *a = NULL, flag_t flags = 0);
	ApplicationLocalInterface(const ApplicationLocalInterface& iface);
	~ApplicationLocalInterface();
	Interface *copy() const;
};

class EthernetInterface : public Interface {
	friend class Interface;
protected:
	unsigned char mac[ETH_MAC_LEN];
	void setIdentifierStr();
	EthernetInterface(const void *identifier = NULL, size_t identifier_len = 0, 
			  const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
	EthernetInterface(Interface::Type_t type, const unsigned char *_mac = NULL, 
			  const string name = DEFAULT_INTERFACE_NAME, 
			  const Address *a = NULL, flag_t flags = 0);
public:
	static const Type_t class_type = TYPE_ETHERNET;
	EthernetInterface(const unsigned char _mac[ETH_MAC_LEN], const string name = DEFAULT_INTERFACE_NAME, 
			  const Address *a = NULL, flag_t flags = 0);
	EthernetInterface(const EthernetInterface& iface);
	~EthernetInterface();
	Interface *copy() const;
	Metadata *toMetadata(bool with_mac = false) const;
	static EthernetInterface *fromMetadata(const Metadata& m);
};

typedef Reference<EthernetInterface> EthernetInterfaceRef;
typedef ReferenceList<EthernetInterface> EthernetInterfaceRefList;

class WiFiInterface : public EthernetInterface {
	friend class Interface;
	WiFiInterface(const void *identifier = NULL, size_t identifier_len = 0, 
		      const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
public:
	static const Type_t class_type = TYPE_WIFI;
	WiFiInterface(const unsigned char _mac[ETH_MAC_LEN], const string name = DEFAULT_INTERFACE_NAME, 
		      const Address *a = NULL, flag_t flags = 0);
	WiFiInterface(const WiFiInterface& iface);
	~WiFiInterface();
	Interface *copy() const;
	static WiFiInterface *fromMetadata(const Metadata& m);
};

typedef Reference<WiFiInterface> WiFiInterfaceRef;
typedef ReferenceList<WiFiInterface> WiFiInterfaceRefList;

class BluetoothInterface : public Interface {
	friend class Interface;
	unsigned char mac[BT_MAC_LEN];
	void setIdentifierStr();
	BluetoothInterface(const void *identifier = NULL, size_t identifier_len = 0, 
			   const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
public:
	static const Type_t class_type = TYPE_BLUETOOTH;
	BluetoothInterface(const unsigned char _mac[BT_MAC_LEN], const string name = DEFAULT_INTERFACE_NAME, 
			   const Address *a = NULL, flag_t flags = 0);
	BluetoothInterface(const BluetoothInterface& iface);
	~BluetoothInterface();
	Interface *copy() const;
	Metadata *toMetadata(bool with_mac = false) const;
	static BluetoothInterface *fromMetadata(const Metadata& m);
};

typedef Reference<BluetoothInterface> BluetoothInterfaceRef;
typedef ReferenceList<BluetoothInterface> BluetoothInterfaceRefList;

class MediaInterface : public Interface {
	friend class Interface;
	string path;
	void setIdentifierStr();
	MediaInterface(const void *identifier = NULL, size_t identifier_len = 0, 
		       const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
public:
	static const Type_t class_type = TYPE_MEDIA;
	MediaInterface(const string _path, const string name = DEFAULT_INTERFACE_NAME, flag_t flags = 0);
	MediaInterface(const MediaInterface& iface);
	~MediaInterface();
	Interface *copy() const;
};

typedef Reference<MediaInterface> MediaInterfaceRef;
typedef ReferenceList<MediaInterface> MediaInterfaceRefList;

typedef ReferenceList<Interface> InterfaceRefList;

template<typename T>
inline T *Interface::create(const void *identifier, const char *name, flag_t flags)
{
	size_t identifier_len = 0;
	
	if (!name)
		name = DEFAULT_INTERFACE_NAME;

	switch (T::class_type) {
	case TYPE_APPLICATION_PORT:
		identifier_len = sizeof(unsigned short);
		break;
	case TYPE_APPLICATION_LOCAL:
		identifier_len = strlen(static_cast<const char *>(identifier));
		break;
	case TYPE_ETHERNET:
		identifier_len = ETH_MAC_LEN;
		break;
	case TYPE_WIFI:
		identifier_len = ETH_MAC_LEN;
		break;
	case TYPE_BLUETOOTH:
		identifier_len = BT_MAC_LEN;
		break;
	case TYPE_MEDIA:
		identifier_len = strlen(static_cast<const char *>(identifier));
		break;
	default:
		return NULL;
	}

	return new T(identifier, identifier_len, name, flags);
}

template<typename T>
inline T *Interface::create(const void *identifier, const char *name, const Address& addr, flag_t flags)
{	
	T *iface = create<T>(identifier, name, flags);
	
	if (iface) {
		iface->addAddress(addr);
	}
	return iface;
}

template<typename T>
inline T *Interface::create(const void *identifier, const char *name, const Addresses& addrs, flag_t flags)
{
	T *iface = create<T>(identifier, name, flags);
	
	if (iface && addrs.size()) {
		iface->addAddresses(addrs);
	}
	return iface;
}

#endif /* _INTERFACE_H */
