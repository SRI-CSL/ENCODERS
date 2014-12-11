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
#ifndef _INTERFACESTORE_H
#define _INTERFACESTORE_H

#include <libcpphaggle/List.h>
#include <libcpphaggle/Mutex.h>

using namespace haggle;

#include "Node.h"
#include "ConnectivityInterfacePolicy.h"

class InterfaceRecord;

/** */
class InterfaceStore : protected List<InterfaceRecord *>
{
	//friend class ConnectivityManager;
	Mutex mutex;
	size_type remove_children(const InterfaceRef &parent, InterfaceRefList *ifl = NULL);
public:
	InterfaceStore();
	InterfaceStore(const InterfaceStore &ifs);
	~InterfaceStore();

	// A retrieve criteria
	class Criteria {
	public:
		virtual bool operator() (const InterfaceRecord& ir) const
		{
			return true;
		}
		virtual ~Criteria() {}
	};
	// Locking
	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }
	bool trylock() { return mutex.trylock(); }
	/**
		Check if an Interface is in the store.
		Returns: true if it is in the store, otherwise false.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	bool stored(const Interface &iface);
	/**
		Check if an Interface is in the store.
		Returns: true if it is in the store, otherwise false.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	bool stored(const InterfaceRef &iface);
	/**
		Check if an Interface is in the store.
		Returns: true if it is in the store, otherwise false.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	bool stored(Interface::Type_t type, const unsigned char *identifier);
	/**
		Add or update an interface to the store given previously unreferenced interfaces.
		Returns: If successful (i.e., the interface was added or updated), a valid interface
		reference to the interface in the store, otherwise InterfaceRef::null on error.
		The returned reference might reference a different copy of the interface given as input argument.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef addupdate(Interface *iface, const Interface *parent, ConnectivityInterfacePolicy *policy, bool *was_added = NULL);
	/**
		Add or update an interface to the store given previously unreferenced interfaces.
		Returns: If successful (i.e., the interface was added or updated), a valid interface
		reference to the interface in the store, otherwise InterfaceRef::null on error.
		The returned reference might reference a different copy of the interface given as input argument.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef addupdate(Interface *iface, const InterfaceRef &parent, ConnectivityInterfacePolicy *policy, bool *was_added = NULL);
	/**
		Add or update an interface to the store given interface references. The optional argument "was_added"
		can be passed if the caller wants to know whether the Interface was added to the data store
		or just updated, indicated by true and false, respectively.
		Returns: If successful (i.e., the interface was added or updated), a valid interface
		reference to the interface in the store, otherwise InterfaceRef::null on error.
		The returned reference might reference a different copy of the interface given as input argument.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef addupdate(InterfaceRef& iface, const InterfaceRef& parent, ConnectivityInterfacePolicy *policy, bool *was_added = NULL);
	/**
		Retrieve a reference to an interface from the store given another reference.
		The reference passed as in argument might reference a copy of an interface
		already located in the store.
		Returns: A valid interface reference if the interface was found in the store,
		otherwise InterfaceRef::null.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef retrieve(const InterfaceRef& iface);
	/**
		Retrieve a reference to an interface from the store.
		Returns: A valid interface reference if the interface was found in the store,
		otherwise InterfaceRef::null.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef retrieve(const Interface &iface);
	/**
		Retrieve a reference to an interface from the store, given an address.
		Returns: A valid interface reference if the interface was found in the store,
		otherwise InterfaceRef::null.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef retrieve(const Address &add);
	/**
		Retrieve a reference to an interface from the store, given type and identifier.
		Returns: A valid interface reference if the interface was found in the store,
		otherwise InterfaceRef::null.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef retrieve(Interface::Type_t type, const unsigned char *identifier);
	/**
		Retrieve all interfaces that match the given
		criteria. The interfaces that match the criteria will
		be appended to the list passed as argument.

                @param c the criteria to match.
                @param ifl the interface reference list to append matching interfaces to.
                @returns The number of interfaces that matched the criteria.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	size_type retrieve(const Criteria& c, InterfaceRefList& ifl);
	/**
		Retrieve a reference to the parent of the given interface.

		Returns: A valid interface reference if the parent was found in the store,
		otherwise InterfaceRef::null.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	InterfaceRef retrieveParent(const InterfaceRef& iface);
	/**
		Remove an interface from the store matching a given an interface name.
		Any children interfaces are also removed.

                @param name the name to match.
                @param ifl an optional pointer to an interface reference list onto which
                removed interfaces will be appended. The removed interface list might
                include the children of the matching interfaces.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	size_type remove(const string name, InterfaceRefList *ifl = NULL);
	/**
		Remove an interface from the store, given an interface pointer.
                Any children interfaces are also removed.

		@param iface the interface to match.
                @param ifl an optional pointer to an interface reference list onto which
                removed interfaces will be appended. The removed interface list might
                include the children of the matching interfaces.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	size_type remove(const Interface *iface, InterfaceRefList *ifl = NULL);
	/**
           	Remove an interface from the store, given an interface reference.
                Any children interfaces are also removed.

		@param iface the interface to match.
                @param ifl an optional pointer to an interface reference list onto which
                removed interfaces will be appended. The removed interface list might
                include the children of the matching interfaces.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
        size_type remove(const InterfaceRef& iface, InterfaceRefList *ifl = NULL);
	/**
		Remove an interface from the store, given its type and identifier.
                Any children interfaces are also removed.

		@param iface the interface to match.
                @param ifl an optional pointer to an interface reference list onto which
                removed interfaces will be appended. The removed interface list might
                include the children of the matching interfaces.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	size_type remove(Interface::Type_t type, const unsigned char *identifier, InterfaceRefList *ifl = NULL);
	/**
		Age an interface associated with a specific parent interface. If the
                interface dies due to age, it is removed along with any children.

                @param parent the parent interface to age.
                @param ifl an optional pointer to an interface reference list onto which
                dead interfaces will be appended. The dead interface list might
                include the children of the matching interfaces.
		@param lifetime an optional parameter that when passed will contain the
		lifetime of the next child interface to "die" after the function returns.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
        size_type age(const Interface *parent = NULL, InterfaceRefList *ifl = NULL, Timeval *lifetime = NULL);
	/**
		Age an interface associated with a specific parent interface. If the
                interface dies due to age, it is removed along with any children

                @param type the type of the parent interface to age.
                @param identifier the identifier of the parent interface to age.
                @param ifl an optional pointer to an interface reference list onto which
                dead interfaces will be appended. The dead interface list might
                include the children of the matching interfaces.
		@param lifetime an optional parameter that when passed will contain the
		lifetime of the next child interface to "die" after the function returns.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	size_type age(Interface::Type_t type, const unsigned char *identifier, InterfaceRefList *ifl = NULL, Timeval *lifetime = NULL);
	/**
		Age an interface associated with a specific parent interface reference. If the
                interface dies due to age, it is removed along with any children

		@param parent a reference to the parent interface to age.x
                @param ifl an optional pointer to an interface reference list onto which
                dead interfaces will be appended. The dead interface list might
                include the children of the matching interfaces.
		@param lifetime an optional parameter that when passed will contain the
		lifetime of the next child interface to "die" after the function returns.
                @returns The number of interfaces that were removed from the store.

		DEADLOCK WARNING: the calling thread may not hold the lock on an
		interface reference while calling this function.
	*/
	size_type age(const InterfaceRef &parent = NULL, InterfaceRefList *ifl = NULL, Timeval *lifetime = NULL);

	void print();
	void getInterfacesAsMetadata(Metadata *m); // CBMEN, HL
};


/** */
class InterfaceRecord
#ifdef DEBUG_LEAKS
 : public LeakMonitor
#endif
{
	friend class InterfaceStore::Criteria;
public:
	// The interface
	InterfaceRef iface;
	// The parent interface, i.e., the interface on which the one above was discovered
	InterfaceRef parent;
	/*
	 Ageing policy. The interface is considered alive as long as this
	 does not say it is dead.
	*/
	ConnectivityInterfacePolicy *cip;

	InterfaceRecord(const InterfaceRef &_iface, const InterfaceRef &_parent, ConnectivityInterfacePolicy *_cip) :
#ifdef DEBUG_LEAKS
		LeakMonitor(LEAK_TYPE_INTERFACE_RECORD),
#endif
		iface(_iface), parent(_parent), cip(_cip)
	{}
	~InterfaceRecord() { delete cip; }
};

#endif /* _INTERFACESTORE_H */
