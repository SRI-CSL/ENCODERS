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

#include "Connectivity.h"
#include "ConnectivityManager.h"

/*
	This function makes sure an interface is in the table.
*/
InterfaceStatus_t Connectivity::report_interface(InterfaceRef& found, const InterfaceRef& found_by,
								      ConnectivityInterfacePolicy *policy)
{
	return getManager()->report_interface(found, found_by, policy);
}

InterfaceStatus_t Connectivity::report_interface(Interface *found, const InterfaceRef &found_by, ConnectivityInterfacePolicy *policy)
{
	return getManager()->report_interface(found, found_by, policy);
}

void Connectivity::age_interfaces(const InterfaceRef &whose, Timeval *lifetime)
{
	getManager()->age_interfaces(whose, lifetime);
}

InterfaceStatus_t Connectivity::have_interface(Interface::Type_t type, const unsigned char *identifier)
{
	return getManager()->have_interface(type, identifier);
}

InterfaceStatus_t Connectivity::report_known_interface(const Interface& iface, bool isHaggle)
{
	return getManager()->report_known_interface(iface, isHaggle);
}

InterfaceStatus_t Connectivity::report_known_interface(const InterfaceRef& iface, bool isHaggle)
{
	return getManager()->report_known_interface(iface, isHaggle);
}

InterfaceStatus_t Connectivity::report_known_interface(Interface::Type_t type, const unsigned char *identifier, bool isHaggle)
{
	return getManager()->report_known_interface(type, identifier, isHaggle);
}

InterfaceStatus_t Connectivity::is_known_interface(Interface::Type_t type, const unsigned char *identifier)
{
	return getManager()->is_known_interface(type, identifier);
}

InterfaceStatus_t Connectivity::is_known_interface(const Interface *iface)
{
	return getManager()->is_known_interface(iface);
}

void Connectivity::delete_interface(InterfaceRef iface)
{
	getManager()->delete_interface(iface);
}

void Connectivity::delete_interface(Interface *iface)
{
	getManager()->delete_interface(iface);
}

void Connectivity::delete_interface(const string name)
{
	getManager()->delete_interface(name);
}

void Connectivity::delete_interface(Interface::Type_t type, const unsigned char *identifier)
{
	getManager()->delete_interface(type, identifier);
}


bool Connectivity::handleInterfaceUp(const InterfaceRef &iface)
{
	return false;
}

void Connectivity::handleInterfaceDown(const InterfaceRef &iface)
{
	if (iface == rootInterface)
		cancelDiscovery();
}

void Connectivity::cleanup()
{
	// Call the connectivity specific hook
	hookCleanup();
	
	getKernel()->addEvent(new Event((static_cast < ConnectivityManager * >(getManager()))->deleteConnectivityEType, this));
}

Connectivity::Connectivity(ConnectivityManager * _m, const InterfaceRef& _iface, const string _name) :
	ManagerModule<ConnectivityManager>(_m, _name), rootInterface(_iface)
{
}

Connectivity::~Connectivity()
{
}

bool Connectivity::startDiscovery(void)
{
	if (!isRunning()) {
		if (!hookStart())
			return false;
		
		return start();
	}
	return false;
}

void Connectivity::cancelDiscovery(void)
{
	hookStopOrCancel();

	cancel();
}
