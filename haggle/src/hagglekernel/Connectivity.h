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
#ifndef _CONNECTIVITY_H_
#define _CONNECTIVITY_H_

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class Connectivity;
class ConnectivityEventData;

#include <libcpphaggle/Timeval.h>

#include "ManagerModule.h"
#include "ConnectivityManager.h"
#include "Interface.h"

using namespace haggle;

/** Connectivity class - a class from which specific connectivities inherit
*/
class Connectivity : public ManagerModule<ConnectivityManager>
{
protected:
	// The interface associated with this connectivity
	const InterfaceRef rootInterface;
	/**
		For simplicity, the specific connectivity managers should NOT override
		this function, only run().
	*/
	void cleanup();
	
	/**
		Utility function to age all interfaces cound by this connectivity.
	*/
	void age_interfaces(const InterfaceRef &whose, Timeval *lifetime = NULL);
	/**
		Utility function to call the same-named function in the Connectivity
		Manager.
	*/
	InterfaceStatus_t report_interface(InterfaceRef& found, const InterfaceRef& found_by, ConnectivityInterfacePolicy *policy);
	/**
		Utility function to call the same-named function in the Connectivity
		Manager.
	*/
	InterfaceStatus_t report_interface(Interface *found, const InterfaceRef& found_by, ConnectivityInterfacePolicy *policy);
	
	/**
		Utility function to check if an interface already exists.
	*/
	InterfaceStatus_t have_interface(Interface::Type_t type, const unsigned char *identifier);
	/**
		Utility function to call the same-named function in the Connectivity
		Manager.
	*/
	InterfaceStatus_t report_known_interface(const Interface& iface, bool isHaggle = false);
	/**
		Utility function to call the same-named function in the Connectivity
		Manager.
	*/
	InterfaceStatus_t report_known_interface(const InterfaceRef& iface, bool isHaggle = false);
	/**
		Utility function to call the same-named function in the Connectivity
		Manager.
	*/
	InterfaceStatus_t report_known_interface(Interface::Type_t type, const unsigned char *identifier, bool isHaggle = false);
	/**
		Utility functions to check if an interface is already known from before.
	*/
	InterfaceStatus_t is_known_interface(Interface::Type_t type, const unsigned char *identifier);
	InterfaceStatus_t is_known_interface(const Interface *iface);

	/**
		Utility function to delete an interface by reference.
	*/
	void delete_interface(Interface *iface);
	
	/**
		Utility function to delete an interface by reference.
	*/
	void delete_interface(InterfaceRef iface);

	/**
		Utility function to delete an interface by its name.
	*/
	void delete_interface(const string name);
	/**
		Utility function to delete an interface by type and identifier.
	*/
	void delete_interface(Interface::Type_t type, const unsigned char *identifier);

	/**
	 Function that is automatically called when cleanup
	 is called. A connectivity can use the hook to do
	 cleanup  stuff without overriding the entire cleanup
	 function.
	 */   
	virtual void hookCleanup() {}

	/**
	   Function that is automatically called when startDiscovery
	   is called. A connectivity can use the hook to do
	   initialization stuff.
	*/   
	virtual bool hookStart() { return true; }
	
	/**
	   Function that is automatically called when stopDiscovery or
	   cancelDiscovery are called. A connectivity can use the hook
	   to do cancel discovery services without overriding
	   stopDiscovery or cancelDiscovery.
	*/
	virtual void hookStopOrCancel() {}
public:
	/**
		Tells the connectivity to handle the fact that the given interface has
		appeared.
		
		Returns true iff the connectivity takes possession of the interface, and
		no new connectivity should be started.
	*/
	virtual bool handleInterfaceUp(const InterfaceRef &iface);
	/**
		Tells the connectivity to handle the fact that the given interface has
		disappeared.
	*/
	virtual void handleInterfaceDown(const InterfaceRef &iface);
	/*
		Starting and stopping discovery: is done by the ConnectionManager.
	*/
	/**
		Returns true iff it successfully started discovery.
		
		This function can be overridden if the connectivity does not wish to run
		in it's own thread. The default implementation uses a thread.
	*/
	virtual bool startDiscovery();
	/**
		Tells the connectivity to finish. The connectivity will not 
		neccesarily have finished by the time cancelDiscovery() returns.
		
		This function can be overridden if the connectivity does not wish to run
		in it's own thread. The default implementation uses a thread.
	*/
	virtual void cancelDiscovery();
	/**
		Tells the connectivity that the resource policies have changed.
	*/
	virtual void setPolicy(PolicyRef newPolicy) {}
	/**
		Constructor:
	*/
	Connectivity(ConnectivityManager *m, const InterfaceRef& iface, const string name = "Unnamed connectivity");
	/**
		Destructor:
	*/
	~Connectivity();
};

#endif
