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
#ifndef _MANAGER_H
#define _MANAGER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class Manager;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Watch.h>

#include "Event.h"
#include "Metadata.h"
#include "MetadataParser.h"
#include "HaggleKernel.h"
#include "ManagerModule.h"
#include "HaggleKernel.h"

using namespace haggle;

extern HaggleKernel *haggleKernel;

/*
  For some reason, Windows (mobile) does not handle multiple
  inheritence properly. If 'Manager' derives from both EventHandler
  and MetadataParser it seems the windows compiler cannot resolve
  virtual functions that are overridden in the specific managers that
  derive from the 'Manager' class..
 */
class Manager : public EventHandler
#if defined(ENABLE_METADATAPARSER)
	, public MetadataParser
#endif
{
public:
	typedef enum {
		MANAGER_STATE_STOPPED,
		MANAGER_STATE_PREPARE_STARTUP,
		MANAGER_STATE_STARTUP,
		MANAGER_STATE_RUNNING,
		MANAGER_STATE_PREPARE_SHUTDOWN,
		MANAGER_STATE_SHUTDOWN
	} ManagerState_t;
private:
	friend class HaggleKernel;
	string name;
	ManagerState_t state;
	bool registered;
	bool readyForStartup;
	bool readyForShutdown;
	bool startupComplete;
	void _onPrepareStartup(Event *e);
	void _onStartup(Event *e);
	void _onPrepareShutdown(Event *e);
	void _onShutdown(Event *e);
	void _onConfig(Event *e);
	void _onDynamicConfig(Event *e); // CBMEN, HL
	EventType configEType;
protected:
        HaggleKernel *kernel;
	virtual void onWatchableEvent(const Watchable& wbl) {}
	
	bool isStartupComplete() { return state >= MANAGER_STATE_RUNNING; }
	void signalIsReadyForStartup();
	void signalIsReadyForShutdown();
	
	/**
		This function can be overriden by a derived Manager if it wants to do pre 
		startup preparations. A manager should generally not generate any public 
		events that may affect other managers until onStartup() has been called.
	 
		When the manager is ready for startup, it MUST call signalIsReadyForStartup().
		This call does not have to happen in onPrepareStartup(), but at some point
		in the future.
	 */
	virtual void onPrepareStartup() { signalIsReadyForStartup(); }
	/**
		This function is called when all managers have signaled that they are prepared
		for startup. After this function has been called, all managers should be ready to 
		operate fully and may generate public events.
	 */
	virtual void onStartup() {}
	/**
		This function can be overriden by a derived Manager if it wants to initiate 
		preparation for shutdown, but is not yet ready for a full shutdown. The kernel
		event queue will be guaranteed to continue its operation until onShutdown() is
		called. Other managers should also run essential services until onShutdown() has
		occured.
		When the manager is ready for shutdown, it MUST call signalIsReadyForShutdown().
		This does not have to happen in onPrepareShutdown(), but at some point
		in the future.
	 */
	virtual void onPrepareShutdown() { signalIsReadyForShutdown(); }
	/**
		This function is called when all managers have signaled that they are prepared
		for shutdown. In onShutdown() there is no guarantee that other managers are still
		running, or that the event queue will reliably process any generated events.
		
		If onShutdown() is overridden, the Manager MUST itself ensure that it calls
		unregisterWithKernel() once it is ready, but it does not have to be in
		onShutdown();
	 */
	virtual void onShutdown() { unregisterWithKernel(); }

	/**
	 This function is called when a configuration data object is sent to a haggle node.
	 If implemented, the Manager might read the metadata to retrieve configuration
	 information. 
	 */
	virtual void onConfig(Metadata *m) { }

	/**
	 This function is called when a dynamic configuration data object is sent to a haggle node.
	 If implemented, the Manager might read the metadata to retrieve configuration
	 information. 
	 */
	virtual void onDynamicConfig(Metadata *m) { }

	bool unregisterWithKernel();
	bool registerWithKernel(); 
	/**
	  This function should be overridden in those managers that need to do 
	  initializations after object construction. It is called automatically
	  by init().
	  
	  Returns: true if the initialization is successful, or false otherwise.
	*/
	virtual bool init_derived() { return true; }
public:
        Manager(const char *_name, HaggleKernel *_kernel = haggleKernel);
        ~Manager();
       
	/**
	  The init() function should be called after the manager has been 
	  created in order to initialize it.

	  Returns: true if the initialization is successful, or false otherwise.
	*/
	bool init();

        const char *getName() const {
                return name.c_str();
        }
	HaggleKernel *getKernel() {
		return kernel;
	}
	/**
		This function is called by the haggle kernel when the haggle kernel 
		wishes to go into the shutdown sequence. 
	*/
	bool isReadyForStartup();
	bool isReadyForShutdown();
	ManagerState_t getState() const { return state; }
};

#endif /* _MANAGER_H */

