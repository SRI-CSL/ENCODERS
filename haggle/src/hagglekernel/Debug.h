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
#ifndef _DEBUG_H
#define _DEBUG_H

// Make sure DEBUG is defined if DEBUG_LEAKS is
#ifdef DEBUG_LEAKS
#ifndef DEBUG
#define DEBUG
#endif
#endif

#ifdef DEBUG_LEAKS

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/Mutex.h>

using namespace haggle;

/*
#ifdef OS_UNIX
#define LEAK_COUNT_FUNCTION_CALLS
#endif
*/

typedef enum {
	LEAK_TYPE_UNDEFINED,	
	LEAK_TYPE_EVENT,	
	LEAK_TYPE_ATTRIBUTE,
	LEAK_TYPE_DATAOBJECT,
	LEAK_TYPE_NODE,
	LEAK_TYPE_INTERFACE,
	LEAK_TYPE_POLICY,
	LEAK_TYPE_FILTER,
	LEAK_TYPE_DEBUGCMD,
	LEAK_TYPE_METADATA,
	LEAK_TYPE_ADDRESS,
	LEAK_TYPE_DATASTORE,
	LEAK_TYPE_MANAGERMODULE,
	LEAK_TYPE_INTERFACE_RECORD,
	LEAK_TYPE_BLOOMFILTER,
	LEAK_TYPE_CERTIFICATE,
	_LEAK_TYPE_MAX,
} LeakType_t;

/** */
class LeakMonitor {
	static const char *leaktypestr[];
	static unsigned long numAlloc[_LEAK_TYPE_MAX];
	static unsigned long numFree[_LEAK_TYPE_MAX];
#ifdef LEAK_COUNT_FUNCTION_CALLS
	typedef Pair<string, unsigned long> func_key;
	typedef HashMap<string, unsigned long> func_registry_t;
	static func_registry_t functionCalls[_LEAK_TYPE_MAX];
#endif
	const LeakType_t type;
	const string objectName;
protected:
	typedef Pair<LeakType_t, LeakMonitor *> registry_key;
	typedef HashMap<LeakType_t, LeakMonitor *> registry_t;
	typedef Pair<registry_t::iterator, registry_t::iterator> registry_range_t;
	static Mutex registryMutex; // A mutex that protects the registry
	static registry_t registry;
public:
	LeakMonitor(const LeakType_t type = LEAK_TYPE_UNDEFINED, const string name = "Unnamed LeakMonitor");
	virtual ~LeakMonitor();
	static void listRegistry();
	static void reportLeaks();
	static string getLeakReport();
};

#endif /* DEBUG_LEAKS */

#include <libcpphaggle/Reference.h>

typedef enum {
	DBG_CMD_PRINT_INTERNAL_STATE,
	DBG_CMD_PRINT_PROTOCOLS,
	DBG_CMD_PRINT_DATAOBJECTS,
	DBG_CMD_PRINT_ROUTING_TABLE,
	DBG_CMD_PRINT_CERTIFICATES,
// SW: START: cache strat debug:
    DBG_CMD_PRINT_CACHE_STRAT,
// SW: END: cache strat debug:
    // CBMEN, HL, Begin
    DBG_CMD_OBSERVE_CERTIFICATES,
    DBG_CMD_OBSERVE_PROTOCOLS,
    DBG_CMD_OBSERVE_ROUTING_TABLE,
    DBG_CMD_OBSERVE_CACHE_STRAT,
    // CBMEN, HL, End
	_DBG_CMD_MAX
} DebugCmdType_t;

/**
	DebugCmd is a simple class that defines objects that pass debug commands to Managers.
	A DebugCmd object can be passed in events through a refcounted DebugCmdRef reference.
	Managers that receive this event may print (or do) manager specific debug information
	depending on the type of the DebugCmd object passed in the event.
 
	The DebugManager will typically generate these DebugCmd objects.
 */
class DebugCmd 
#ifdef DEBUG_LEAKS
: public LeakMonitor
#endif
{
	const DebugCmdType_t type;
	string msg;
public:
	DebugCmd(DebugCmdType_t _type, string _msg = "No message");
	virtual ~DebugCmd();
	const char *getMsg() const { return msg.c_str(); } 
	DebugCmdType_t getType() const { return type; }
};

typedef haggle::Reference<DebugCmd> DebugCmdRef;

#endif /* _DEBUG_H */
