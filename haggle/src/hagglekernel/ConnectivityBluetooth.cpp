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

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_BLUETOOTH)

#include "ConnectivityBluetooth.h"

ConnectivityBluetooth::ConnectivityBluetooth(ConnectivityManager *m, const InterfaceRef& iface) :
	ConnectivityBluetoothBase(m, iface, "Bluetooth connectivity")
{
	LOG_ADD("%s: Bluetooth connectivity starting. Scan time: %ld +- %ld seconds\n",
		Timeval::now().getAsString().c_str(), baseTimeBetweenScans, randomTimeBetweenScans);
}

ConnectivityBluetooth::~ConnectivityBluetooth()
{
	LOG_ADD("%s: Bluetooth connectivity stopped.\n",
		Timeval::now().getAsString().c_str());
}

ConnectivityBluetoothBase::ConnectivityBluetoothBase(ConnectivityManager *m, 
						     const InterfaceRef& iface, 
						     const string name) :
	Connectivity(m, iface, name)
{
}

ConnectivityBluetoothBase::~ConnectivityBluetoothBase()
{

}

Mutex ConnectivityBluetoothBase::sdpListMutex;
BluetoothInterfaceRefList ConnectivityBluetoothBase::sdpWhiteList;
BluetoothInterfaceRefList ConnectivityBluetoothBase::sdpBlackList;
bool ConnectivityBluetoothBase::ignoreNonListedInterfaces = false;
unsigned long ConnectivityBluetoothBase::baseTimeBetweenScans = DEFAULT_BASE_TIME_BETWEEN_SCANS;
unsigned long ConnectivityBluetoothBase::randomTimeBetweenScans = DEFAULT_RANDOM_TIME_BETWEEN_SCANS;
bool ConnectivityBluetoothBase::readRemoteName = true;

int ConnectivityBluetoothBase::classifyAddress(const BluetoothInterface &iface)
{
	Mutex::AutoLocker l(sdpListMutex);
	
	for (BluetoothInterfaceRefList::iterator it = sdpWhiteList.begin();
		it != sdpWhiteList.end(); it++) {
		if (*it == iface)
			return BLUETOOTH_ADDRESS_IS_HAGGLE_NODE;
	}

	if (ignoreNonListedInterfaces)
		return BLUETOOTH_ADDRESS_IS_NOT_HAGGLE_NODE;
	
	for (BluetoothInterfaceRefList::iterator it = sdpBlackList.begin();
		it != sdpBlackList.end(); it++) {
		if (*it == iface)
			return BLUETOOTH_ADDRESS_IS_NOT_HAGGLE_NODE;
	}
	return BLUETOOTH_ADDRESS_IS_UNKNOWN;
}

int ConnectivityBluetoothBase::classifyAddress(const Interface::Type_t type, const unsigned char identifier[6])
{
	if (type != Interface::TYPE_BLUETOOTH)
		return -1;
	
	BluetoothInterface iface(identifier);
	
	return classifyAddress(iface);
}

void ConnectivityBluetoothBase::clearSDPLists(void)
{
	Mutex::AutoLocker l(sdpListMutex);
	sdpBlackList.clear();
	sdpWhiteList.clear();
}

void ConnectivityBluetoothBase::updateSDPLists(Metadata *md)
{
	Mutex::AutoLocker l(sdpListMutex);

	const char *param = md->getParameter("scan_base_time");

	if (param) {
		char *endptr = NULL;
		unsigned long base_time = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
			baseTimeBetweenScans = base_time;
			HAGGLE_DBG("setting Bluetooth scan base time %lu\n", baseTimeBetweenScans);
			LOG_ADD("# ConnectivityManager: setting Bluetooth scan base time %lu\n", baseTimeBetweenScans);
		}
	}

	param = md->getParameter("scan_random_time");

	if (param) {
		char *endptr = NULL;
		unsigned long random_time = strtoul(param, &endptr, 10);

		if (endptr && endptr != param) {
			randomTimeBetweenScans = random_time;
			HAGGLE_DBG("setting Bluetooth scan random time %lu\n", randomTimeBetweenScans);
			LOG_ADD("# ConnectivityManager: setting Bluetooth scan random time %lu\n", randomTimeBetweenScans);
		}
	}

	param = md->getParameter("read_remote_name");

	if (param) {
		if (strcmp(param, "true") == 0) {
			readRemoteName = true;
			HAGGLE_DBG("setting Bluetooth read_remote_name to true\n");
			LOG_ADD("# ConnectivityManager: setting Bluetooth read_remote_name to true\n");
		} else if (strcmp(param, "false") == 0) {
			readRemoteName = false;
			HAGGLE_DBG("setting Bluetooth read_remote_name to false\n");
			LOG_ADD("# ConnectivityManager: setting Bluetooth read_remote_name to false\n");
		}
	}

	/*
	 Check bluetooth module blacklisting/whitelisting data. Formatted like so:
	 
	 <Bluetooth>
	 <ClearBlacklist/>
	 <ClearWhitelist/>
	 <Blacklist>
	 <Interface type="bluetooth" mac="11:22:33:44:55:66"/>
	 </Blacklist>
	 <Whitelist>
	 <Interface type="bluetooth" mac="77:88:99:AA:BB:CC"/>
	 </Whitelist>
	 <IgnoreNonListedInterfaces>yes</IgnoreNonListedInterfaces>
	 </Bluetooth>
	 
	 Of course, all components of the bluetooth section are optional, and it 
	 is possible to insert any number of blacklisted/whitelisted interfaces.
	 
	 It is important for the interface addresses to have the bt:// prefix. If 
	 they do not, then the interfaces will not be accepted.
	 */
	
	Metadata *bl = md->getMetadata("Blacklist");
	Metadata *wl = md->getMetadata("Whitelist");
	Metadata *ignore = md->getMetadata("IgnoreNonListedInterfaces");
	
	Metadata *cbl = md->getMetadata("ClearBlacklist");
	Metadata *cwl = md->getMetadata("ClearWhitelist");
	
	if (cbl)
		sdpBlackList.clear();
	
	if (cwl)
		sdpWhiteList.clear();
	
	if (bl)  {
		BluetoothInterfaceRef iface;		
		Metadata *m = bl->getMetadata("Interface");
		
		while (m) {
			const char *type = m->getParameter("type");
			const char *name = m->getParameter("name") ? m->getParameter("name") : "noname";

			if (!type) {
				HAGGLE_ERR("No type specified for interface \'%s\'... ignoring\n", name);
			} else if (strcmp(type, "bluetooth") != 0) {
				HAGGLE_ERR("Interface type is \'%s\', and not \'bluetooth\'\n", type);
			} else {			
				iface = BluetoothInterface::fromMetadata(*m);
						
				if (iface) {
					sdpBlackList.push_back(iface);
					LOG_ADD("# ConnectivityManager: black-listing interface: type=%s identifier=%s name=%s\n", 
						type, iface->getIdentifierStr(), name);
				}
			}
			
			m = bl->getNextMetadata();
		}
	}

	if (wl) {
		BluetoothInterfaceRef iface;
		Metadata *m = wl->getMetadata("Interface");
		
		while (m) {
			const char *type = m->getParameter("type");
			const char *name = m->getParameter("name") ? m->getParameter("name") : "noname";

			if (!type) {
				HAGGLE_ERR("No type specified for interface \'%s\'... ignoring\n", name);
			} else if (strcmp(type, "bluetooth") != 0) {
				HAGGLE_ERR("Interface type is \'%s\', and not \'bluetooth\'\n", type);
			} else {
				iface = BluetoothInterface::fromMetadata(*m);
						
				if (iface) {
					sdpWhiteList.push_back(iface);
					
					LOG_ADD("# ConnectivityManager: white-listing interface: type=%s identifier=%s name=%s\n", 
						type, iface->getIdentifierStr(), name);
				}
			}
			m = wl->getNextMetadata();
		}
	}
	
	if (ignore) {
		if (ignore->getContent() == "yes")
			ignoreNonListedInterfaces = true;
		else if(ignore->getContent() == "no")
			ignoreNonListedInterfaces = false;
		else {
			HAGGLE_ERR("IgnoreNonListedInterfaces content wrong. Must be yes/no.\n");
		}
	}
}

#endif
