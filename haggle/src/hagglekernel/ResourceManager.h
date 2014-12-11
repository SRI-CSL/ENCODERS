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
#ifndef _RESOURCEMANAGER_H
#define _RESOURCEMANAGER_H

class ResourceManager;

#include <libcpphaggle/Platform.h>
#include "Manager.h"
#include "Policy.h"
#include "ResourceMonitor.h"


#define BATTERY_CRITICAL_LEVEL 8
#define BATTERY_LOW_LEVEL 33
#define BATTERY_MEDIUM_LEVEL 66
#define BATTERY_HIGH_LEVEL // everything above medium

/** 
    ResourceManager: 

Monitors resources (e.g., Battery power, disk and memory space) and
generates policy events depending on current resource status.
*/

class ResourceManager : public Manager
{
	friend class ResourceMonitor;
public:
	ResourceManager(HaggleKernel *kernel);
	~ResourceManager();
private:	
	ResourceMonitor *resMon;
	EventCallback<EventHandler> *onCheckStatusCallback;
	void onShutdown();
	void onStartup();
	void onCheckStatusEvent(Event *e);
	bool init_derived();
};


#endif /* _RESOURCEMANAGER_H */
