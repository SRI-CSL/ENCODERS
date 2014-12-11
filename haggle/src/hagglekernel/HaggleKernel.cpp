/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

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
#include <time.h>

#include <malloc.h>

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Watch.h>
#if defined(OS_UNIX)
#include <grp.h>
#endif
#include "HaggleKernel.h"
#include "Event.h"
#include "EventQueue.h"
#include "Interface.h"
#include "SQLDataStore.h"

#include <git_ref.h> // MOS
#include <sys/time.h> // CBMEN, HL
#include <sys/resource.h> // CBMEN, HL

#define EXEC_STAT 1 // MOS - gather execution statistics - don't use this if not needed !

using namespace haggle;

void HaggleKernel::setThisNode(NodeRef &_thisNode)
{
	thisNode = _thisNode;
}

#define HOSTNAME_LEN 100

HaggleKernel::HaggleKernel(string _configFile, DataStore *ds , const string _storagepath) :
// SW: START CONFIG PATH: allow users to specify config.xml path
    configFile(_configFile), 
// SW: END CONFIG PATH.
	dataStore(ds), starttime(Timeval::now()), shutdownCalled(false),
        prepareShutdown(false), // MOS
        fatalError(false), // MOS
        running(false), storagepath(_storagepath), firstClassApps(false),
        discardObsoleteNodeDesc(true),
	replManEnabled(false)
{
}

bool HaggleKernel::init()
{
	char hostname[HOSTNAME_LEN];

#if defined(DEBUG) && (defined(OS_WINDOWS_MOBILE) || defined(OS_ANDROID) || defined(OS_LINUX)) // MOS - added linux
	if (!Trace::trace.enableFileTrace()) {
	  // HAGGLE_ERR("Could not enable file tracing\n"); // MOS - enableFileTrace generates accurate message
	}
#endif

// MOS - disable trace file by default for better performance
#if defined(LOGTRACE)
	if (!LogTrace::init()) {
		HAGGLE_ERR("Could not open trace file\n");
	}
#endif

#ifdef OS_WINDOWS
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		HAGGLE_ERR("WSAStartup failed: %d\n", iResult);
                return false;
	}
#endif

	HAGGLE_LOG("Git reference is %s\n", GIT_REF);

	Timeval startupTime = Timeval::now(); // MOS
	HAGGLE_LOG("Startup Time is %s\n", startupTime.getAsString().c_str());

	HAGGLE_LOG("Storage path is %s\n", storagepath.c_str());
	HAGGLE_LOG("Datastore path is %s\n", DEFAULT_DATASTORE_PATH);
	
#if defined(OS_ANDROID)
	int num = getgroups(0, NULL);

	HAGGLE_LOG("Haggle has the permissions of the following groups:\n");

	if (num > 0) {
		gid_t *groups = new gid_t[num];
		
		if (groups) { 
			if (getgroups(num, groups) == num) {
				for (int i = 0; i < num; i++) {
					struct group *g = getgrgid(groups[i]);
					if (g) {
						HAGGLE_LOG("group: %u %s\n", g->gr_gid, g->gr_name);
					}
				}
			}
			delete [] groups;
		}
	}

	struct group *g = getgrnam("bluetooth");

	if (g) {
		HAGGLE_LOG("Haggle needs the permissions of group %u %s, but is currently not granted\n", g->gr_gid, g->gr_name);
	}
#endif

	if (dataStore) {
		dataStore->kernel = this;
		if (!dataStore->init()) {
			HAGGLE_ERR("Data store could not be initialized\n");
			return false;
		}
	} else {
		HAGGLE_ERR("No data store!!!\n");
		return false;
	}

	// The interfaces on this node will be discovered when the
	// ConnectivityManager is started. Hence, they will not be
	// part of thisNode when we insert it in the datastore. This
	// is a potential problem.
	//dataStore->insertNode(&thisNode);

	int res = gethostname(hostname, HOSTNAME_LEN);

	if (res != 0) {
		HAGGLE_ERR("Could not get hostname\n");
		return false;
	}
	
	// Always zero terminate in case the hostname didn't fit
	hostname[HOSTNAME_LEN-1] = '\0';
        
        HAGGLE_LOG("Hostname is %s\n", hostname);

	thisNode = nodeStore.add(Node::create(Node::TYPE_LOCAL_DEVICE, hostname));

	if (!thisNode) {
		HAGGLE_ERR("Could not create this node\n");
		return false;
	}
	currentPolicy = NULL;


	return true;
}

HaggleKernel::~HaggleKernel()
{
#ifdef OS_WINDOWS
	// Cleanup winsock
	WSACleanup();
#endif
#if defined(LOGTRACE)
	LogTrace::fini();
#endif
	// Now that it has finished processing, delete the data store:
	if (dataStore)
		delete dataStore;

	HAGGLE_DBG("Done\n");
}

int HaggleKernel::registerManager(Manager *m)
{
	wregistry_t wr;

	if (!m)
		return -1;
	
	/*
		Insert this empty wregistry_t. When we add it to the registry,
		the empty wregistry will be copied, so it doesn't matter 
		that the source object is stack-allocated.
	 */
	
	if (!registry.insert(make_pair(m, wr)).second) {
		HAGGLE_ERR("Manager \'%s\' already registered\n", m->getName());
		return -1;
	}

	HAGGLE_DBG("Manager \'%s\' registered\n", m->getName());

	return registry.size();
}

int HaggleKernel::unregisterManager(Manager *m)
{
	if (!m)
		return -1;
	
	if (registry.erase(m) != 1) {
		HAGGLE_ERR("Manager \'%s\' not registered\n", m->getName());
		return 0;
	}

#ifdef DEBUG
        registry_t::iterator it;
        string registeredManagers;
	
	for (it = registry.begin(); it != registry.end(); it++) {
                registeredManagers += String(" ").append((*it).first->getName());
        }
	HAGGLE_DBG("Manager \'%s\' unregistered. Still %lu registered:%s\n", 
		   m->getName(), registry.size(), registeredManagers.c_str());
#endif	
	/*
	if (registry.size() == 0) {
		// Tell the data store to cancel itself
		dataStore->cancel();
		HAGGLE_DBG("Data store cancelled!\n");
	}
	 */
	return registry.size();
}

int HaggleKernel::registerWatchable(Watchable wbl, Manager *m)
{
	if (!m)
		return -1;
	
	if (!wbl.isValid()) {
		HAGGLE_ERR("Manager \'%s\' tried to register invalid watchable\n", m->getName());
		return -1;
	}
	registry_t::iterator it = registry.find(m);
	
	if (it == registry.end()) {
		HAGGLE_ERR("Non-registered manager \'%s\' tries to register a watchable\n", m->getName());
		return -1;
	}
	
	wregistry_t& wr = (*it).second;
	
        if (!wr.insert(make_pair(wbl, 0)).second) {
		HAGGLE_ERR("Manager \'%s\' has already registered %s\n", m->getName(), wbl.getStr());
                return -1;
        }
	
	HAGGLE_DBG("Manager \'%s\' registered %s\n", m->getName(), wbl.getStr());

	return wr.size();
}

int HaggleKernel::unregisterWatchable(Watchable wbl)
{
	registry_t::iterator it;
	
	for (it = registry.begin(); it != registry.end(); it++) {
		wregistry_t& wr = (*it).second;
		
		if (wr.erase(wbl) == 1) {			
			HAGGLE_DBG("Manager \'%s\' unregistered %s\n", (*it).first->getName(), wbl.getStr());
			return wr.size();
		}
	}
	
	HAGGLE_ERR("Could not unregister %s, as it was not found in registry\n", wbl.getStr());

	return 0;
}

void HaggleKernel::signalIsReadyForStartup(Manager *m)
{
	for (registry_t::iterator it = registry.begin(); it != registry.end(); it++) {
		if (!(*it).first->isReadyForStartup())
			return;
	}
	HAGGLE_DBG("All managers are ready for startup, generating startup event!\n");
	
	running = true;
	addEvent(new Event(EVENT_TYPE_STARTUP));
}

void HaggleKernel::signalIsReadyForShutdown(Manager *m)
{
	HAGGLE_DBG("%s signals it is ready for shutdown\n", m->getName());
	
	for (registry_t::iterator it = registry.begin(); it != registry.end(); it++) {
		if (!(*it).first->isReadyForShutdown()) {
			HAGGLE_DBG("%s is not ready for shutdown\n", (*it).first->getName());
			return;
		}
	}
	
	HAGGLE_DBG("All managers are ready for shutdown, generating shutdown event!\n");
	running = false;
	enableShutdownEvent();
}

#ifdef DEBUG
void HaggleKernel::printRegisteredManagers()
{
	registry_t::iterator it;
	
	printf("============= Manager list ==================\n");
	
	for (it = registry.begin(); it != registry.end(); it++) {
		Manager *m = (*it).first;
		printf("%s:", m->getName());
		wregistry_t& wr = (*it).second;
		wregistry_t::iterator itt = wr.begin();
		
		for (; itt != wr.end(); itt++) {
			printf(" %s ", (*itt).first.getStr());
		}
		printf("\n");
	}
	
	printf("=============================================\n");

}
#endif

Manager *HaggleKernel::getManager(char *name)
{
	for (registry_t::iterator it = registry.begin(); it != registry.end(); it++) {
		if (strcmp((*it).first->getName(), name) == 0) {
			return (*it).first;
		}
	}
	return NULL;
}
void HaggleKernel::shutdown()
{ 
	if (!running || shutdownCalled) 
		return;
	
	shutdownCalled = true;
	
	// MOS - no mallocs in signal handlers please - this can lead to deadlocks
	// addEvent(new Event(EVENT_TYPE_PREPARE_SHUTDOWN));
        prepareShutdown = true; // MOS - defer event
}

// TODO: Allow us to read an XML file from stdin
bool HaggleKernel::readStartupDataObject(FILE *fp)
{
	long len;
	ssize_t	k;
	size_t i, j, items, total_bytes_read = 0;
	char *data;
	bool ret = false;
	
	if (!fp) {
		HAGGLE_ERR("Invalid file pointer\n");
		return false;
	}
	// Figure out how large the file is.
	fseek(fp, 0, SEEK_END);
	// No, this does not handle files larger than 2^31 bytes, but that 
	// shouldn't be a problem, and could be easily fixed if necessary.
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	// Allocate space to put the data in. +1 byte 
	// so that we make sure we read until EOF in 
	// the read loop.
	data = new char[++len];
	
	if (!data) {
		HAGGLE_ERR("Could not allocate data\n");
		return false;
	}
	// Read the data:
	do {
		items = fread(data + total_bytes_read, 1, len - total_bytes_read, fp);
		total_bytes_read += items;
	} while (items != 0 && total_bytes_read < (size_t)len);

	if (ferror(fp) != 0) {
		HAGGLE_ERR("File error when reading configuration\n");
		goto out;
	}

	// Check that we reached EOF
	if (feof(fp) != 0) {
		DataObject *dObj = NULL;
	
		// Success! Now create data object(s):
		i = 0;
		
		while (i < total_bytes_read) {
			if (!dObj)
				dObj = DataObject::create_for_putting();
			
			if (!dObj) {
				HAGGLE_ERR("Could not create data object for putting\n");
				ret = false;
				goto out;
			}

			k = dObj->putData(&(data[i]), total_bytes_read - i, &j);
			
			// Check return value:
			if (k < 0) {
				// Error occured, stop everything:
				HAGGLE_ERR("Error parsing startup data object "
					   "(byte %ld of %ld)\n", i, total_bytes_read);
				i = total_bytes_read;
			} else {
				// No error, check if we're done:
				if (k == 0 || j == 0) {
					// Done with this data object:
#ifdef DEBUG
				        dObj->print(NULL); // MOS - NULL means print to debug trace
#endif
					addEvent(new Event(EVENT_TYPE_DATAOBJECT_RECEIVED, DataObjectRef(dObj)));
					// Should absolutely not deallocate this, since it 
					// is in the event queue:
					dObj = NULL;
					// These bytes have been consumed:
					i += k;
					ret = true;
				} else if (k > 0) {
					// These bytes have been consumed:
					i += k;
				}
			}
		}
		// Clean up:
		if (dObj) {
			delete dObj;
		}
	}
out:
	delete [] data;
	
	return ret;
}

bool HaggleKernel::readStartupDataObjectFile(string cfgObjPath)
{
	FILE *fp;
	bool ret;
	int i = 0;

    //  SW: allow users to specify config.xml path, otherwise use default
    if (cfgObjPath.empty()) {
        cfgObjPath = CONFIG_DATAOBJECT_PATH;
    }
	
	// Open file, try two paths for backwards compatibility
	do {	
		HAGGLE_LOG("Trying to read configuration from \'%s\'\n", cfgObjPath.c_str());

		fp = fopen(cfgObjPath.c_str(), "rb");
		
		// Did we succeed?
		if (fp)
			break;
		
		cfgObjPath = STARTUP_DATAOBJECT_PATH;
	} while (i++ < 1);
	
	if (!fp)
		return false;
	
	ret = readStartupDataObject(fp);

	if (ret) {
		HAGGLE_LOG("Successfully read configuration\n");
	} else {
		HAGGLE_ERR("Could not read configuration\n");
		// MOS - we better stop here - forgotten configs caused too many headaches
		HAGGLE_ERR("FATAL ERROR - EMERGENCY SHUTDOWN INITIATED\n"); // MOS
		setFatalError();
		shutdown();
	}
	// Close the file:
	fclose(fp);
	
	return ret;
}

void HaggleKernel::log_memory() {
	struct mallinfo mi = mallinfo();
	HAGGLE_STAT("Statistics - Malloc Total: %d - NotMMapped: %d - MMapped: %d - Allocated: %d - Free: %d - Releasable: %d - FreeChunks: %d - FastBinBlocks: %d - AvailableInFastBins: %d - SQLite: %lld\n",
		    mi.arena + mi.hblkhd, mi.arena, mi.hblkhd, mi.uordblks, mi.fordblks, mi.keepcost, mi.ordblks, mi.smblks, mi.fsmblks, sqlite3_memory_used()); 
}

// CBMEN, HL, Begin
void HaggleKernel::log_rusage() {
	struct rusage usage;
	if (getrusage(RUSAGE_SELF, &usage) == 0) {
		HAGGLE_STAT("RUSAGE - User CPU Time: %.9f - System CPU Time: %.9f - Max RSS: %ld - Soft Page Faults: %ld - Hard Page faults: %ld - Block Input ops: %ld - Block Output ops: %ld - Voluntary Context Switches: %ld - Involuntary Context Switches: %ld\n", 
		            usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1.0e6), usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1.0e6), usage.ru_maxrss, usage.ru_minflt, usage.ru_majflt, usage.ru_inblock, usage.ru_oublock, usage.ru_nvcsw, usage.ru_nivcsw);
	}
}

void HaggleKernel::log_eventQueue(Metadata *m) {

	HashMap<string, size_t> counts;
	HashMap<string, size_t>::iterator cit;
	Heap theHeap;

	Metadata *dm = m->addMetadata("Events");
	if (!dm) {
		return;
	}

	List<Event *> list = getEventList();
	for (List<Event *>::iterator it = list.begin(); it != list.end(); it++) {
		Event *e = *it;
		theHeap.insert(*it);

		cit = counts.find(e->getName());
		if (cit == counts.end()) {
			counts.insert(make_pair(e->getName(), 1));
		} else {
			size_t count = (*cit).second;
			counts.erase(cit);
			counts.insert(make_pair(e->getName(), count+1));
		}
	}

	for (size_t i = 0; i < list.size(); i++) {

		Event *e = static_cast<Event *>(theHeap.extractFirst());

		if (e) {
			if (i < 200) {
				Metadata *dmm = dm->addMetadata("Event");
				dmm->setParameter("name", e->getName());
				dmm->setParameter("time", e->getTimeout().getAsString());
				dmm->setParameter("canceled", e->isCanceled() ? "true" : "false");
				
				if (e->getDataObject()) {
					dmm->setParameter("dObj", e->getDataObject()->getIdStr());
				}
				if (e->getNode()) {
					dmm->setParameter("node", e->getNode()->getIdStr());
				}
				if (e->getInterface()) {
					dmm->setParameter("iface", e->getInterface()->getIdentifierStr());
				}
				if (e->getNodeList().size() > 0) {
					dmm->setParameter("nodeListSize", e->getNodeList().size());
				}
				if (e->getDataObjectList().size() > 0) {
					dmm->setParameter("dataObjectListSize", e->getDataObjectList().size());
				}

				if (strcmp(e->getName(), "BFRemoveDelay") == 0) {
					const DataObjectId_t *id = (DataObjectId_t *) e->getData();
					if (id) {
						dmm->setParameter("dObj", DataObject::idString(*id));
					}
				}
			}

			delete e;
		}
	}

	dm = m->addMetadata("Counts");
	if (!dm) {
		return;
	}

	for (cit = counts.begin(); cit != counts.end(); cit++) {
		Metadata *dmm = dm->addMetadata("Event");

		if (dmm) {
			dmm->setParameter("name", (*cit).first);
			dmm->setParameter("count", (*cit).second);
		}
	}

}
// CBMEN, HL, End

void HaggleKernel::run()
{
	bool shutdownmode = false;
	int maxEventQueueSize = 0; // MOS
	int numEvents = 0; // MOS

#ifdef EXEC_STAT
  int eventsExecuted = 0; // MOS
  double eventsExecutionTime = 0; // MOS
#endif	
	// Start the data store
	dataStore->start();

	addEvent(new Event(EVENT_TYPE_PREPARE_STARTUP));
	
    // SW: allow users to specify config.xml path
	readStartupDataObjectFile(configFile);
	
	while (registry.size()) {
		Watch w;
		Timeval now = Timeval::now();
		int signalIndex, res;
		EQEvent_t ee;
		Timeval timeout, *t = NULL;
		Event *e = NULL;

		// MOS - deferred generation of event to avoid deadlock
		if(prepareShutdown) {
		  prepareShutdown = false;
		  addEvent(new Event(EVENT_TYPE_PREPARE_SHUTDOWN));
		}		  
		
		/*
		 We make a copy of the registry each time we loop. This is because a manager
		 can unregister sockets, or itself, in the event (or socket) it processes.
		 Therefore, if we'd use the original registry, it might become inconsistent as 
		 we iterate it in the event loop.
		 */
		registry_t reg = registry;

		/* 
		   Get the time until the next event and check the status of
		   the event.

		*/
		ee = getNextEventTime(&timeout);
		
		switch (ee) {
			case EQ_EVENT_SHUTDOWN:
				HAGGLE_LOG("\n\n****************** SHUTDOWN EVENT *********************\n\n");
				Trace::trace.enableFileTraceFlushing(); // MOS - flush from now on
				shutdownmode = true;
			case EQ_EVENT:
				if (shutdownmode)
					timeout = 0;
						
				/* Convert the timeout from absolute time to relative time.
				 We need to make sure we do not have a negative time, which sometimes can 
				 happen when there are events added with a timeout of zero.
				 */
				if (now >= timeout)
					timeout.zero();
				else
					timeout -= now;
				
				// Set the pointer to our Timeval
				t = &timeout;
				
				break;
			case EQ_EMPTY:
				// Timeval NULL pointer = no timeout
				t = NULL;
				break;
			case EQ_ERROR:
			default:
				// This should not really happen
				HAGGLE_ERR("Bad state in event loop\n");
				break;
		}
		
		/*
			Iterate through the registered sockets and add them to our Watch. The registry
			does not require locking, as only managers and manager modules running 
			in the same thread as the kernel may register sockets. Modules running
			in separate threads do not need to register sockets, as they can easily 
			implement their own run-loop.
		 
		 */
		registry_t::iterator it = reg.begin();
		
		for (; it != reg.end(); it++) {
			wregistry_t& wr = (*it).second;
			wregistry_t::iterator itt = wr.begin();
			for (; itt != wr.end(); itt++) {
				(*itt).second = w.add((*itt).first);
				//HAGGLE_DBG("watchable %s added to watch with index %d\n", (*itt).first.getStr(), (*itt).second);
			}
		}
		
		/*
		 Add the Signal that is raised whenever something is added to the event queue.
		 */
		signalIndex = w.add(signal);

		//HAGGLE_DBG("Waiting on kernel watch with timeout %s\n", t ? t->getAsString().c_str() : "INFINATE");
		
		res = w.wait(t);

		if (res == Watch::TIMEOUT) {
			// Timeout occurred -> Process event from EventQueue
			e = getNextEvent();
			numEvents++; // MOS

			if(e->getType() == EVENT_TYPE_PREPARE_SHUTDOWN) {
				HAGGLE_LOG("\n\n****************** PREPARE SHUTDOWN EVENT *********************\n\n");
				Trace::trace.enableFileTraceFlushing(); // MOS - flush from now on
			}

			// MOS - log event statistics, SW: type casting size() to (int)
			if(numEvents % 100 == 0 || (int)size() > maxEventQueueSize) {
			  HAGGLE_DBG("Event count: %d - Pending: %d - Max: %d\n", numEvents, size(), maxEventQueueSize); 
			  if((int)size() > maxEventQueueSize) maxEventQueueSize = (int)size();
			  log_memory();
			  log_rusage();
			}

			LOG_ADD("%s: %s\n", Timeval::now().getAsString().c_str(), e->getDescription().c_str());

#ifdef EXEC_STAT
			// MOS
			Timeval startTime = Timeval::now();
#endif			
			if (e->isPrivate()) {
				//HAGGLE_DBG("Doing private event callback: %s\n", e->getName());
				e->doPrivateCallback();
			} else if (e->isCallback()) {
				//HAGGLE_DBG("Doing callback\n");
				e->doCallback();
				//HAGGLE_DBG("Done with callback\n");
			} else {
				/* 
				 Loop through all registered managers and check whether they are 
				 interested in this event.
				 */
				registry_t::iterator it = reg.begin();
				
				//HAGGLE_DBG("Doing public event %s\n", e->getName());
				
				for (; it != reg.end(); it++) {
					Manager *m = (*it).first;
					EventCallback < EventHandler > *callback = m->getEventInterest(e->getType());
					if (callback) {
						(*callback) (e);
					}
				}
			}

#ifdef EXEC_STAT
			// MOS
			Timeval endTime = Timeval::now();
			eventsExecuted += 1;
			eventsExecutionTime += (endTime - startTime).getTimeAsSecondsDouble();
#endif			
			/*
				Delete the event object. This may also delete
				data associated with the event. Data passed in public events 
				should be reference counted with the Reference class. Data in
				private events and callback events may not be reference counted,
				but the associated event data will not be deleted in that case.
				It is up to the private handlers to manage that data.
			 */
			if (e->shouldDelete())
				delete e;
		} else if (res == Watch::FAILED) {
			HAGGLE_ERR("Main run-loop error on Watch : %s\n", STRERROR(ERRNO));
			continue;
		}
		
		//HAGGLE_DBG("%d objects are set\n", res);
		
		if (w.isSet(signalIndex)) {
			/* Something was added to the queue. We do not
			   need to do anyting here as this signal is
			   only a trigger for us to check the queue
			   again. */  

			//HAGGLE_DBG("Queue signal was set\n");
		}
		// Check and handle readable watchables. 
		it = reg.begin();
		
		for (; it != reg.end(); it++) {
			Manager *m = (*it).first;
			wregistry_t& wr = (*it).second;
			wregistry_t::iterator itt = wr.begin();
			
			for (; itt != wr.end(); itt++) {				
				//HAGGLE_DBG("Checking if watchable %s with watch index %d is set\n", (*itt).first.getStr(), (*itt).second);

				if (w.isSet((*itt).second)) {
					//HAGGLE_DBG("Watchable %s with watch index %d is set\n", (*itt).first.getStr(), (*itt).second);
					m->onWatchableEvent((*itt).first);
				}
			}
		}
	}
	HAGGLE_DBG("Kernel exits from main loop\n");

	// CBMEN, HL, Begin
	{
		HAGGLE_STAT("Event Queue\n");
		while (!empty()) {
			Event *e = getNextEvent();
			HAGGLE_STAT("Event %s at %s\n", e->getName(), e->getTimeout().getAsString().c_str());
		} 
	}
	// CBMEN, HL, End

#ifdef EXEC_STAT
	HAGGLE_STAT("Summary Statistics - Events Executed: %d - Avg. Execution Time: %.9f\n", eventsExecuted, eventsExecutionTime/(double)eventsExecuted); // MOS
#endif
	// stop the dataStore thread and try to join with its thread
	HAGGLE_DBG("Joining with DataStore thread\n");
	dataStore->stop();
	HAGGLE_DBG("Joined\n");

	log_memory();
	log_rusage();
}
