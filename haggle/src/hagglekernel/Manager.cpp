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

#include "Manager.h"
#include "Event.h"
#include "HaggleKernel.h"
#include "Attribute.h"

#define MANAGER_CONFIG_ATTR "ManagerConfiguration"
#define FILTER_CONFIG MANAGER_CONFIG_ATTR "=" ATTR_WILDCARD

Manager::Manager(const char *_name, HaggleKernel * _kernel) :
		EventHandler(),
#if defined(ENABLE_METADATAPARSER)
                MetadataParser(string(_name).substr(0, string(_name).find("Manager"))), 
#endif
		name(_name), state(MANAGER_STATE_STOPPED), registered(false), 
		readyForStartup(false), readyForShutdown(false), startupComplete(false), 
		kernel(_kernel)
{
}

Manager::~Manager()
{
	// Remove the configuration filter
	unregisterEventTypeForFilter(configEType);
}

bool Manager::init()
{
#define __CLASS__ Manager
	int ret;
	ret = setEventHandler(EVENT_TYPE_PREPARE_STARTUP, _onPrepareStartup);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_STARTUP, _onStartup);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_PREPARE_SHUTDOWN, _onPrepareShutdown);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	ret = setEventHandler(EVENT_TYPE_SHUTDOWN, _onShutdown);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}

	// CBMEN, HL - added event for dynamic configuration
	ret = setEventHandler(EVENT_TYPE_DYNAMIC_CONFIGURE, _onDynamicConfig);

	if (ret < 0) {
		HAGGLE_ERR("Could not register event handler\n");
		return false;
	}
	
	// Register filter for node descriptions
	registerEventTypeForFilter(configEType, "Manager Configuration Filter Event", _onConfig, FILTER_CONFIG);
	
	if (!kernel->registerManager(this)) {
		HAGGLE_ERR("Could not register %s with kernel\n", name.c_str());
		return false;
	} else {
                registered = true;
        }

	return init_derived();
}

void Manager::signalIsReadyForStartup()
{
	readyForStartup = true;
	kernel->signalIsReadyForStartup(this);
}

void Manager::signalIsReadyForShutdown()
{
	readyForShutdown = true;
	kernel->signalIsReadyForShutdown(this);
}

bool Manager::isReadyForStartup()
{
	return readyForStartup;
}

bool Manager::isReadyForShutdown()
{
	return readyForShutdown;
}

bool Manager::registerWithKernel()
{
	if (!registered) {
		kernel->registerManager(this);
		registered = false;
		return true;
	}
	return false;
}

bool Manager::unregisterWithKernel()
{
        // state = MANAGER_STATE_STOPPED; // MOS - better stay in SHUTDOWN at the end
	
	if (registered) {
		kernel->unregisterManager(this);
		registered = false;
		return true;
	}
	return false;
}

void Manager::_onPrepareStartup(Event *e)
{
	state = MANAGER_STATE_PREPARE_STARTUP;
	onPrepareStartup();
}

void Manager::_onStartup(Event *e)
{
	state = MANAGER_STATE_STARTUP;
	onStartup();
	state = MANAGER_STATE_RUNNING;
}

void Manager::_onPrepareShutdown(Event *e)
{
	state = MANAGER_STATE_PREPARE_SHUTDOWN;
	onPrepareShutdown();
}

void Manager::_onShutdown(Event *e)
{
	state = MANAGER_STATE_SHUTDOWN;
	onShutdown();
}

void Manager::_onConfig(Event *e)
{
	DataObjectRefList& dObjs = e->getDataObjectList();

	while (dObjs.size()) {
                DataObjectRef dObj = dObjs.pop();

		// Get the metadata matching the manager
		Metadata *m = dObj->getMetadata()->getMetadata(this->getName());

		if (m) {
			onConfig(m);
		}
	}
}

// CBMEN, HL, Begin
// This is ugly but we don't want to include the whole header
#define DATAOBJECT_METADATA_APPLICATION_CONTROL_DYNAMIC_CONFIGURATION "DynamicConfiguration"
void Manager::_onDynamicConfig(Event *e)
{
	if (!e || !e->hasData())
		return;

	DataObjectRef dObj = e->getDataObject();
	if (!dObj)
		return;

	Metadata *m = dObj->getMetadata();
	if (!m)
		return;

	m = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_DYNAMIC_CONFIGURATION);
	if (!m)
		return;

	m = m->getMetadata(getName());
	if (!m)
		return;

	onDynamicConfig(m);

}
// CBMEN, HL, End
