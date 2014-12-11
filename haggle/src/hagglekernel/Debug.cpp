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
#include <libcpphaggle/Thread.h>

#include "Debug.h"
#include "Event.h"
#include "Attribute.h"
#include "Node.h"
#include "Interface.h"
#include "DataObject.h"
#include "Filter.h"
#include "Policy.h"
#include "DataStore.h"
#include "ManagerModule.h"
#include "Bloomfilter.h"
#include "Trace.h"

#ifdef DEBUG_LEAKS

#ifdef LEAK_COUNT_FUNCTION_CALLS
#include <execinfo.h>
#include <string.h>
#endif

const char *LeakMonitor::leaktypestr[] = {
	"Undefined",
	"Event",
	"Attribute",
	"DataObject",
	"Node",
	"Interface",
	"Policy",
	"Filter",
	"DebugCmd",
	"Metadata",
	"Address",
	"DataStore",
	"ManagerModule",
	"InterfaceRecord",
	"Bloomfilter",
	"Certificate",
	NULL
};

unsigned long LeakMonitor::numAlloc[_LEAK_TYPE_MAX] = { 0 };
unsigned long LeakMonitor::numFree[_LEAK_TYPE_MAX] = { 0 };
LeakMonitor::registry_t LeakMonitor::registry;
Mutex LeakMonitor::registryMutex;

#ifdef LEAK_COUNT_FUNCTION_CALLS
LeakMonitor::func_registry_t LeakMonitor::functionCalls[_LEAK_TYPE_MAX];
#endif

LeakMonitor::LeakMonitor(const LeakType_t _type, const string _name) :
	type(_type), objectName(_name) 
{	
        Mutex::AutoLocker l(registryMutex);
	
	registry.insert(make_pair(type, this));
	
	numAlloc[type]++;
	
#ifdef LEAK_COUNT_FUNCTION_CALLS
	void *array[3];
	size_t size;
	char **strings;
	
	size = backtrace (array, 3);
	strings = backtrace_symbols (array, size);
	
	if (size < 3) 
		return;

	// Find the function call on the stack where this object was created.
	// The second position is the constructor for the inherited class, 
	// the third should be the function in which this object
	// was created (unless this class is inherited in multiple steps).
	string line =  strings[2];

	free (strings);

	// Find the function name in the string which is returned
	size_t endpos = line.find("+") - 2;
	size_t startpos = line.rfind(" ", endpos);
	
	string function = line.substr(startpos, endpos - startpos);
	
	// Insert the function call in our registry or update the count
	// if the key already exists
	func_registry_t::iterator it = functionCalls[type].find(function);
	
	if (it == functionCalls[type].end()) {
		functionCalls[type].insert(func_key(function, 1));
	} else {
		(*it).second++;
	}
#endif
}

LeakMonitor::~LeakMonitor()
{
        Mutex::AutoLocker l(registryMutex);
        Pair<registry_t::iterator, registry_t::iterator> p;

        p = registry.equal_range(type);
	        
	for (; p.first != p.second; ++p.first) {
		if ((*p.first).second == this) {
			registry.erase(p.first);
                        numFree[type]++;
                        return;
		}
	}
	
        HAGGLE_ERR("LeakMonitor object of type %s not found\n", leaktypestr[type]);
}

void LeakMonitor::listRegistry()
{
        Mutex::AutoLocker l(registryMutex);
	union {
		Event *ev;
		Attribute *attr;
		DataObject *dObj;
		Node *node;
		Interface *iface;
		Policy *p;
		Filter *f;
		DebugCmd *dbgCmd;
		Metadata *md;
		Address *a;
		DataStore *ds;
		ManagerModule<Manager> *mm;
		InterfaceRecord *ir;
		Bloomfilter *bf;
		void *raw;
	} u;

	printf("====== Leak Registry ======\n");

	for (registry_t::iterator it = registry.begin(); it != registry.end(); it++) {
		unsigned char *raw = NULL;
		size_t len = 0;
		u.raw = (*it).second;

		printf("%s: ", leaktypestr[(*it).first]);

		switch ((*it).first) {
		case LEAK_TYPE_UNDEFINED:
			break;
		case LEAK_TYPE_EVENT:
			break;
		case LEAK_TYPE_ATTRIBUTE:
			printf("%s=%s", u.attr->getName().c_str(), u.attr->getValue().c_str());
			break;
		case LEAK_TYPE_DATAOBJECT:
			printf("%lu",  u.dObj->getNum());
			break;
		case LEAK_TYPE_NODE:
			printf("%lu name=%s stored=%s", 
                               u.node->getNum(), 
                               u.node->getName().c_str(), 
                               u.node->isStored() ? "Yes" : "No");
			break;
		case LEAK_TYPE_INTERFACE:
			printf("type=%s name=%s id=%s flag[STORED]=%s", 
                               u.iface->getTypeStr(), 
                               u.iface->getName(), 
                               u.iface->getIdentifierStr(), 
                               u.iface->isStored() ? "yes" : "no");
			break;
		case LEAK_TYPE_POLICY:
			break;
		case LEAK_TYPE_FILTER:
			break;
		case LEAK_TYPE_DEBUGCMD:
			break;
		case LEAK_TYPE_METADATA:
			u.md->getRawAlloc(&raw, &len);
			printf("\n%s", raw);
			free(raw);
			break;
		case LEAK_TYPE_ADDRESS:
			printf("%s", u.a->getURI() ? u.a->getURI() : "No URI");
			break;
		case LEAK_TYPE_DATASTORE:
			printf("%s", u.ds->getName());
			break;
		case LEAK_TYPE_MANAGERMODULE:
			printf("%s", u.mm->getName());
			break;
		case LEAK_TYPE_INTERFACE_RECORD:
			printf("iface=%s parent=%s", 
                               u.ir->iface->getIdentifierStr(),
                               u.ir->parent ? u.ir->parent->getIdentifierStr() : "No parent");
			break;
		case LEAK_TYPE_BLOOMFILTER:
			break;
		default:
			break;
		}
                printf("\n");
	}
        
	printf("\n===========================\n");
}

void LeakMonitor::reportLeaks()
{
	int i;
	
        synchronized(registryMutex) {
                for (i = 0; i < _LEAK_TYPE_MAX; i++) {		
                        printf("%s alloc=%lu free=%u active=%u\n", leaktypestr[i], numAlloc[i], numFree[i], numAlloc[i] - numFree[i]);
#ifdef LEAK_COUNT_FUNCTION_CALLS
                        func_registry_t::iterator it = functionCalls[i].begin();
                        
                        while (it != functionCalls[i].end()) {
                                printf("\t%s %s\n", (*it).first, (*it).second);
                                it++;
                        }
#endif
                }
        }
	
	listRegistry();
}

string LeakMonitor::getLeakReport()
{
	return string("Nothing here yet");
}

#endif /* DEBUG_LEAKS */

DebugCmd::DebugCmd(DebugCmdType_t _type, string _msg) :
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_DEBUGCMD),
#endif
type(_type), msg(_msg)
{
}

DebugCmd::~DebugCmd()
{

}
