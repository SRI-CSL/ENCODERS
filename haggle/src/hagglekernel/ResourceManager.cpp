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
#include "ResourceManager.h"
#include "ResourceMonitor.h"

ResourceManager::ResourceManager(HaggleKernel *kernel) : 
	Manager("ResourceManager", kernel), resMon(NULL)
{

}

ResourceManager::~ResourceManager()
{
	if (resMon)
		delete resMon;
	if (onCheckStatusCallback)
		delete onCheckStatusCallback;
}

bool ResourceManager::init_derived()
{
#define __CLASS__ ResourceManager

	resMon = ResourceMonitor::create(this);

	if (!resMon || !resMon->init()) {
		HAGGLE_ERR("Could not initialize resource monitor\n");
                return false;
        }
	if (!resMon->start()) {
		HAGGLE_ERR("Could not start resource monitor\n");
                return false;
        }
	onCheckStatusCallback = newEventCallback(onCheckStatusEvent);

	if (!onCheckStatusCallback) {
		HAGGLE_ERR("Could not create onCheckStatusCallback");
		return false;
        }
	
	HAGGLE_DBG("Initialized resource manager\n");

	return true;
}

void ResourceManager::onStartup()
{
	onCheckStatusEvent(NULL);
}

void ResourceManager::onShutdown()
{
	// This will block until the resource monitor thread has stopped.
	HAGGLE_DBG("Stopping resource monitor\n");
	resMon->stop();
	HAGGLE_DBG("Resource monitor stopped\n");
	unregisterWithKernel();
}

void ResourceManager::onCheckStatusEvent(Event *e)
{
	unsigned long btime, physmem, virtmem;
	long bstat;
	PolicyType_t battPol = POLICY_RESOURCE_HIGH;
	PolicyType_t memPol;
	string now = Timeval::now().getAsString();
	
	bstat = resMon->getBatteryPercent();
	btime = resMon->getBatteryLifeTime();
	physmem = resMon->getAvaliablePhysicalMemory();
	virtmem = resMon->getAvaliableVirtualMemory();
	
	LOG_ADD("%s: Battery status: %ld\n", now.c_str(), bstat);
	LOG_ADD("%s: Battery time: %lu\n", now.c_str(), btime);
	LOG_ADD("%s: Physical memory: %lu\n", now.c_str(), physmem);
	LOG_ADD("%s: Virtual memory: %lu\n", now.c_str(), virtmem);
	
	HAGGLE_DBG("%s: Battery status: %ld\n", now.c_str(), bstat);
	HAGGLE_DBG("%s: Battery time: %lu\n", now.c_str(), btime);
	HAGGLE_DBG("%s: Physical memory: %lu\n", now.c_str(), physmem);
	HAGGLE_DBG("%s: Virtual memory: %lu\n", now.c_str(), virtmem);
	
	if (resMon->getPowerMode() == ResourceMonitor::POWER_MODE_BATTERY) {
		if (bstat < BATTERY_CRITICAL_LEVEL) {
			HAGGLE_DBG("Shutting down due to low power\n");
			kernel->shutdown();
			return;
		} else if (bstat < BATTERY_LOW_LEVEL) {
			battPol = POLICY_RESOURCE_LOW;
		} else if (bstat < BATTERY_MEDIUM_LEVEL)
			battPol = POLICY_RESOURCE_MEDIUM;
		else
			battPol = POLICY_RESOURCE_HIGH;
	}
	
	// FIXME: figure out good values for these:
	// TODO: Perhaps an unsigned long isn't quite enough to express memory 
	// amounts?
	if (physmem < 0x20000000L) // 512 MB
		memPol = POLICY_RESOURCE_LOW;
	else if (physmem < 0x40000000L) // 1 GB
		memPol = POLICY_RESOURCE_MEDIUM;
	else
		memPol = POLICY_RESOURCE_HIGH;
	
	// Check to see if any values changed:
	PolicyRef newPolicy = new Policy(battPol, memPol);

	if (newPolicy && newPolicy != kernel->getCurrentPolicy()) {
		// Alert the other managers:
		kernel->addEvent(new Event(EVENT_TYPE_RESOURCE_POLICY_NEW, newPolicy));
		// Keep the new value around:
		kernel->setCurrentPolicy(newPolicy);
	}
	
	// Check again in 5 minutes:
	if (!kernel->isShuttingDown())
		kernel->addEvent(new Event(onCheckStatusCallback, NULL, 5*60));
}
