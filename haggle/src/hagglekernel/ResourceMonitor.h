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
#ifndef _RESOURCEMONITOR_H_
#define _RESOURCEMONITOR_H_

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

class ResourceMonitor;

#include <libcpphaggle/Platform.h>
#include "ResourceManager.h"

class ResourceMonitor : public ManagerModule<ResourceManager>
{
protected:
	ResourceMonitor(ResourceManager *m, const string name);
public:
	typedef enum PowerMode {
		POWER_MODE_AC = 0,
		POWER_MODE_USB,
		POWER_MODE_BATTERY,
		POWER_MODE_UNKNOWN,
	} PowerMode_t;	
	static const char *power_mode_str[];

	virtual ~ResourceMonitor() = 0;
	static ResourceMonitor *create(ResourceManager *m);

	virtual PowerMode_t getPowerMode() const = 0;
	/**
	   Returns: battery charge left in percent.
	*/
	virtual long getBatteryPercent() const = 0;
	/**
	   Returns: battery time left in seconds.
	*/
	virtual unsigned int getBatteryLifeTime() const = 0;
	
	/**
	   Returns: number of bytes of physical memory left
	*/
	virtual unsigned long getAvaliablePhysicalMemory() const = 0;
	/**
	   Returns: number of bytes of virtual memory left
	*/
	virtual unsigned long getAvaliableVirtualMemory() const = 0;
};

#endif
