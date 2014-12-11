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
#include "ResourceMonitorWindowsMobile.h"
#include <libcpphaggle/Watch.h>

ResourceMonitorWindowsMobile::ResourceMonitorWindowsMobile(ResourceManager *m) : 
	ResourceMonitor(m, "ResourceMonitorWindowsMobile")
{
}

ResourceMonitorWindowsMobile::~ResourceMonitorWindowsMobile()
{
	CloseMsgQueue(hMsgQ);
}

bool ResourceMonitorWindowsMobile::init()
{
	MSGQUEUEOPTIONS mqOpts = { sizeof(MSGQUEUEOPTIONS), MSGQUEUE_NOPRECOMMIT, 0,
		sizeof(POWER_BROADCAST_POWER_INFO), TRUE };

	hMsgQ = CreateMsgQueue(NULL, &mqOpts);

	if (!hMsgQ) {
		HAGGLE_ERR("Could not create message queue\n");
		return false;
	}

	return true;
}

long ResourceMonitorWindowsMobile::getBatteryPercent() const 
{
	SYSTEM_POWER_STATUS_EX batteryStatus;
	
	if (GetSystemPowerStatusEx(&batteryStatus, TRUE) == TRUE) {
		return static_cast<long> ( batteryStatus.BatteryLifePercent );
	}
	
	return -1;
}

unsigned int ResourceMonitorWindowsMobile::getBatteryLifeTime() const
{
	SYSTEM_POWER_STATUS_EX batteryStatus;
	
	if (GetSystemPowerStatusEx(&batteryStatus, TRUE) == TRUE) {
		return static_cast<unsigned int> ( batteryStatus.BatteryLifeTime );
	}
	
	return UINT_MAX;
}

unsigned long ResourceMonitorWindowsMobile::getAvaliablePhysicalMemory() const 
{
	MEMORYSTATUS memoryStatus;
	
	memoryStatus.dwLength = sizeof( memoryStatus);
	
	GlobalMemoryStatus (&memoryStatus);
	
	return static_cast<unsigned long> ( memoryStatus.dwAvailPhys );
}

unsigned long ResourceMonitorWindowsMobile::getAvaliableVirtualMemory() const 
{
	MEMORYSTATUS memoryStatus;
	
	memoryStatus.dwLength = sizeof( memoryStatus);
	
	GlobalMemoryStatus (&memoryStatus);
	
	return static_cast<unsigned long> ( memoryStatus.dwAvailVirtual );
}

// TODO: Here we should tell the resource manager to issue new policies 
// (through policy events) based on the current power state.
int ResourceMonitorWindowsMobile::handlePowerNotification(POWER_BROADCAST *pow)
{
	POWER_BROADCAST_POWER_INFO *powInfo = NULL;
	
	switch(pow->Message) {
	case PBT_POWERINFOCHANGE:
		HAGGLE_DBG("PBT_POWERINFOCHANGE\n");
		powInfo = (POWER_BROADCAST_POWER_INFO *)pow->SystemPowerState;

		HAGGLE_DBG("BatteryLife=%lu AC=%d Battery=%d\n", 
			powInfo->dwBatteryLifeTime, 
			powInfo->bACLineStatus,
			powInfo->bBatteryLifePercent);
		LOG_ADD("BatteryLife=%lu AC=%d Battery=%d\n", 
			powInfo->dwBatteryLifeTime, 
			powInfo->bACLineStatus,
			powInfo->bBatteryLifePercent);
		break;
	case PBT_POWERSTATUSCHANGE:
		HAGGLE_DBG("PBT_STATUSCHANGE\n");
		break;
	case PBT_RESUME:
		//HAGGLE_DBG("PBT_RESUME\n");
		break;
	case PBT_TRANSITION:
		if (pow->Flags & POWER_STATE_OFF) {
			// Not sure if this event actually means the device is shutting down...
			HAGGLE_DBG("Shutting down due to power off\n");
			getManager()->getKernel()->shutdown();
		}
		//HAGGLE_DBG("PBT_TRANSITION\n");
		break;
	default:
		HAGGLE_ERR("Unknown power broadcast type\n");
	}

	return 0;
}

#define BUFLEN (sizeof(POWER_BROADCAST) + sizeof(POWER_BROADCAST_POWER_INFO))

bool ResourceMonitorWindowsMobile::run()
{
	Watch w;

	hPowerNotif = RequestPowerNotifications(hMsgQ, POWER_NOTIFY_ALL);

	if (!hPowerNotif) {
		HAGGLE_ERR("Could not request power notifications\n");
		return false;
	}

	int resIndex = w.add(hMsgQ);

	HAGGLE_DBG("Running resource monitor\n");

	while (!shouldExit()) {
		int ret;

		w.reset();

		ret = w.wait();

		if (ret == Watch::FAILED) {
			HAGGLE_ERR("Wait on objects failed\n");
			return false;
		} else if (ret == Watch::ABANDONED) {
			// We should exit
			return false;
		}

		if (w.isSet(resIndex)) {
			char buffer[BUFLEN];
			POWER_BROADCAST *pow = (POWER_BROADCAST *)buffer;
			DWORD dwBytesRead, dwFlags = 0;

			//HAGGLE_DBG("Power notification\n");

			BOOL res = ReadMsgQueue(hMsgQ, pow, BUFLEN, &dwBytesRead, 0, &dwFlags);

			if (res == FALSE) {
				switch(GetLastError()) {

					// TODO: do something more to handle errors than just printing 
					// them.
					case ERROR_INSUFFICIENT_BUFFER:
						HAGGLE_ERR("Error: Read buffer too small... buf size=%u, bytes read=%u\n", BUFLEN, dwBytesRead);
						break;
					case ERROR_PIPE_NOT_CONNECTED:
						HAGGLE_ERR("Error: Pipe not connected...\n");
						cancel();
						break;
					case ERROR_TIMEOUT:
						HAGGLE_ERR("Error: Timeout...\n");
						break;
					case ERROR_INVALID_HANDLE:
						HAGGLE_ERR("Error: invalid handle\n");
						cancel();
					default:
						HAGGLE_ERR("Unknown ReadMsgQueue error=%d\n", GetLastError());
						cancel();
						break;

				}
			} else {
				ret = handlePowerNotification(pow);
			}
		}
	}
	return false;
}

void ResourceMonitorWindowsMobile::cleanup()
{
	if (hPowerNotif)
		StopPowerNotifications(hPowerNotif);
}
