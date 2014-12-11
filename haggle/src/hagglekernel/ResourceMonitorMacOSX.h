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
#ifndef _RESOURCEMONITORMACOSX_H
#define _RESOURCEMONITORMACOSX_H

#include <libcpphaggle/Platform.h>
#include "ResourceMonitor.h"

/** */
class ResourceMonitorMacOSX : public ResourceMonitor
{
	friend class ResourceMonitor;
	void cleanup();
	bool run();
	ResourceMonitorMacOSX(ResourceManager *m);
public:
	~ResourceMonitorMacOSX();

	PowerMode_t getPowerMode() const { return POWER_MODE_AC; }
	/**
	   Returns: battery charge left in percent.
	*/
	long getBatteryPercent() const;
	/**
	   Returns: battery time left in seconds.
	*/
	unsigned int getBatteryLifeTime() const;
	
	/**
	   Returns: number of bytes of physical memory left
	*/
	unsigned long getAvaliablePhysicalMemory() const;
	/**
	   Returns: number of bytes of virtual memory left
	*/
	unsigned long getAvaliableVirtualMemory() const;
};


#endif /* _RESOURCEMONITORMACOSX_H */
