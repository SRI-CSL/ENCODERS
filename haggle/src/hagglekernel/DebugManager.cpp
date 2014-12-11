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
#include <libcpphaggle/Platform.h>

#include "DebugManager.h"

#if defined(ENABLE_DEBUG_MANAGER)
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libcpphaggle/Platform.h>

#include <haggleutils.h>
#include <libcpphaggle/Thread.h>

#include "DataObject.h"
#include "Node.h"
#include "Interface.h"
#include "Event.h"
#include "XMLMetadata.h"
#include "Debug.h"
#include "DataStore.h"

#include "ForwardingManager.h"

SOCKET openSocket(int port);

#define DEFAULT_DEBUG_PORT 50901

// #ifdef DEBUG_LEAKS
// #undef DEBUG_LEAKS
// #endif

#define ADD_LOG_FILE_TO_DATASTORE 0

DebugManager::DebugManager(HaggleKernel * _kernel, bool _interactive) : 
	Manager("DebugManager", _kernel), onFindRepositoryKeyCallback(NULL), 
	onDumpDataStoreCallback(NULL), server_sock(-1), interactive(_interactive), console(INVALID_STDIN)
{
}

DebugManager::~DebugManager()
{
	if (onFindRepositoryKeyCallback)
		delete onFindRepositoryKeyCallback;

        if (onDumpDataStoreCallback)
                delete onDumpDataStoreCallback;
}

bool DebugManager::init_derived()
{
#define __CLASS__ DebugManager
#if defined(DEBUG)
/*
    // SW: START: this code assumes multiple handlers per event, a feature
    // that haggle does not currently support.
	int i;

	for (i = EVENT_TYPE_PUBLIC_MIN; i < MAX_NUM_PUBLIC_EVENT_TYPES; i++) {
		setEventHandler(i, publicEvent);
		HAGGLE_DBG("Listening on %d:%s\n", i, Event::getPublicName(i));
	}
    // SW: END
*/
#endif

	server_sock = openSocket(DEFAULT_DEBUG_PORT);

	if (server_sock == INVALID_SOCKET || !kernel->registerWatchable(server_sock, this)) {
		CLOSE_SOCKET(server_sock);
		HAGGLE_ERR("Could not register socket\n");
		return false;
	}

	HAGGLE_DBG("Server sock is %d\n", server_sock);

#if (defined(OS_LINUX) && !defined(OS_ANDROID)) || (defined(OS_MACOSX) && !defined(OS_MACOSX_IPHONE))
	if (interactive) {
		console = open("/dev/stdin", O_RDONLY);
		
		if (console == -1 || !kernel->registerWatchable(console, this)) {
			HAGGLE_ERR("Unable to open STDIN!\n");
			CLOSE_SOCKET(server_sock);
                        return false;
		}
	}
#elif defined(OS_WINDOWS_DESKTOP)
//#if 0 //Disabled
	if (interactive) {
		console = GetStdHandle(STD_INPUT_HANDLE);

		if (console == INVALID_HANDLE_VALUE || !kernel->registerWatchable(console, this)) {
			HAGGLE_ERR("Error - %s\n", StrError(GetLastError()));
			CLOSE_SOCKET(server_sock);
			return false;
		}
		// This will reset the console mode so that getchar() returns for 
		// every character read
		SetConsoleMode(console, 0);
		// Flush any existing events on the console
		FlushConsoleInputBuffer(console);
	}
//#endif // Disabled
#endif
#ifdef DEBUG_LEAKS
	debugEType = registerEventType("DebugManager Debug Event", onDebugReport);

	if (debugEType < 0) {
		HAGGLE_ERR("Could not register debug report event type...\n");
		CLOSE_SOCKET(server_sock);
		return false;
	}
#if defined(OS_WINDOWS_MOBILE) || defined(OS_ANDROID) 
	kernel->addEvent(new Event(debugEType, NULL, 10));
#endif
#endif
	
#if ADD_LOG_FILE_TO_DATASTORE
	/*
		Log file distribution code:
		
		This uses the repository to save information about wether or not we have
		already added a data object to the source code. This is necessary in 
		order not to insert more than one data object. Also, the data object
		will need to include the node ID of this node, which may not have been
		created yet, so this code delays the creation of that data object (if
		it needs to be created) until that is done. 
	*/
	onFindRepositoryKeyCallback = newEventCallback(onFindRepositoryKey);
	kernel->getDataStore()->readRepository(new RepositoryEntry("DebugManager", "has saved log file data object"), onFindRepositoryKeyCallback);
#endif

	onDumpDataStoreCallback = newEventCallback(onDumpDataStore);

	return true;
}

void DebugManager::onFindRepositoryKey(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	DataStoreQueryResult *qr = static_cast < DataStoreQueryResult * >(e->getData());
	
	RepositoryEntryRef re = qr->detachFirstRepositoryEntry();
	
	if (!re) {
		// No repository entry: no data object.
		DataObjectRef dObj;

		// Name the log so that the files are more easily readable on the 
		// machine that receives them:
		char filename[128];
		sprintf(filename, "log-%s.txt", kernel->getThisNode()->getIdStr());

		// Create data object:
		
		// Empty at first:
		dObj = DataObject::create(LogTrace::ltrace.getFile(), filename);
		
		if (!dObj) {
			HAGGLE_ERR("Could not create data object\n");
			return;
		}
		// Add log file attribute:
		Attribute a("Log file","Trace");
		dObj->addAttribute(a);
		
		// Add node id of local node, to make sure that two logs from different 
		// nodes don't clash:
		Attribute b("Node id", kernel->getThisNode()->getIdStr());
		dObj->addAttribute(b);
		
		// Insert data object:
		kernel->getDataStore()->insertDataObject(dObj);
		
		// Insert a repository entry to show the data object exists:
		kernel->getDataStore()->insertRepository(new RepositoryEntry("DebugManager", "has saved log file data object", "yes"));
	}
	
	delete qr;
}

static bool sendBuffer(SOCKET sock, const void *data, size_t toSend)
{
	size_t i = 0;
	
	do {
		ssize_t ret = send(sock, (char *)data + i, toSend, 0);
        
		if (ret == -1) {
#if defined(OS_WINDOWS)
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
				HAGGLE_ERR("Could not write HTTP to socket err=%d\n", ERRNO);
				goto out;
			}
#else
			if (errno != EAGAIN) {
				HAGGLE_ERR("Could not write HTTP to socket err=%d\n", errno);
				goto out;
			}
#endif
		} else {
			toSend -= ret;
			i += ret;
		}
	} while (toSend > 0);
out:
	return toSend == 0;
}

static bool sendString(SOCKET sock, const char *str)
{
	return sendBuffer(sock, str, strlen(str));
}

static size_t skipXMLTag(const char *data, size_t len)
{
	size_t i = 0;
	
	// Skip over the <?xml version="1.0"?> tag:
	while (strncmp(&(data[i]), "?>", 2) != 0)
		i++;
	i += 2;
	return i;
}

void DebugManager::dumpTo(SOCKET client_sock, DataStoreDump *dump)
{
	size_t toSend = dump->getLen();
	const char *data = dump->getData();
	size_t i = 0;
	
	i = skipXMLTag(data, toSend);
	toSend -= i;
	// Send the <?xml version="1.0"?> tag:
	if (!sendString(client_sock, "<?xml version=\"1.0\"?>\n"))
		return;
	// Send the root tag:
	if (!sendString(client_sock, "<HaggleInfo>"))
		return;
	// Send the data:
	if (!sendBuffer(client_sock, &(data[i]), toSend))
		return;
	
        DataObjectRef dObj = kernel->getThisNode()->getDataObject(false);
        unsigned char *buf;
        size_t len;
        if (dObj->getRawMetadataAlloc(&buf, &len)) {
                i = skipXMLTag((char *)buf, len);
                len -= i;
                if (!sendString(client_sock, "<ThisNode>\n")) {
			free(buf);
			return;
		}
                if (!sendBuffer(client_sock, &(buf[i]), len)) {
			free(buf);
			return;
		}
                if (!sendString(client_sock, "</ThisNode>\n")) {
			free(buf);
			return;
		}
                free(buf);
        }
	
	/*
	 
	 FIXME: With the new forwarding this thing is broken.
	 
        Manager *mgr = kernel->getManager((char *)"ForwardingManager");
	
        if (mgr) {
                ForwardingManager *fmgr = (ForwardingManager *) mgr;
		
                DataObjectRef dObj = fmgr->getForwarder()->myMetricDO;
                if (dObj) {
                        char *buf;
                        size_t len;
                        if (dObj->getRawMetadataAlloc(&buf, &len)) {
                                i = skipXMLTag(buf, len);
                                len -= i;
                                if (!sendString(client_sock, "<RoutingData>\n")) {
					free(buf);
					return;
				}
                                if (!sendBuffer(client_sock, &(buf[i]), len)) {
					free(buf);
					return;
				}
                                if (!sendString(client_sock, "</RoutingData>\n")) {
					free(buf);
					return;
				}
                                free(buf);
                        }
                }
        }
	*/
        NodeRefList nl;
	
        kernel->getNodeStore()->retrieveNeighbors(nl);
        if (!nl.empty()) {
                if (!sendString(client_sock, "<NeighborInfo>\n"))
                        return;
                for (NodeRefList::iterator it = nl.begin(); it != nl.end(); it++) {
                        if (!sendString(client_sock, "<Neighbor>"))
                                return;
                        if (!sendString(client_sock, (*it)->getIdStr()))
                                return;
                        if (!sendString(client_sock, "</Neighbor>\n"))
                                return;
                }
                if (!sendString(client_sock, "</NeighborInfo>\n"))
                        return;
        }
	
	// Send the end of the root tag:
	sendString(client_sock, "</HaggleInfo>");
}

void DebugManager::onDumpDataStore(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	DataStoreDump *dump = static_cast <DataStoreDump *>(e->getData());
	
	for (List<SOCKET>::iterator it = client_sockets.begin(); it != client_sockets.end(); it++) {
		dumpTo(*it, dump);
		kernel->unregisterWatchable(*it);
		CLOSE_SOCKET(*it);
	}
	client_sockets.clear();
	
	delete dump;
}

void DebugManager::onShutdown()
{
	if (server_sock != INVALID_SOCKET) {
		kernel->unregisterWatchable(server_sock);
		CLOSE_SOCKET(server_sock);
	}
	if (!client_sockets.empty()) {
		for (List<SOCKET>::iterator it = client_sockets.begin(); it != client_sockets.end(); it++) {
			kernel->unregisterWatchable(*it);
			CLOSE_SOCKET(*it);
		}
	}
	
#if defined(OS_LINUX) || defined(OS_MACOSX)
	if (console != -1) {
		kernel->unregisterWatchable(console);
		//CLOSE_SOCKET(console);
	}
#endif
	
#ifdef DEBUG_LEAKS
	Event::unregisterType(debugEType);
#endif
	unregisterWithKernel();
}

#if defined(DEBUG_LEAKS) && defined(DEBUG)
void DebugManager::onDebugReport(Event *e)
{
	LOG_ADD("%s: kernel event queue size=%lu\n", 
		Timeval::now().getAsString().c_str(), kernel->size()); 
	kernel->getNodeStore()->print();
	kernel->getInterfaceStore()->print();

#ifdef DEBUG_DATASTORE
	kernel->getDataStore()->print();
#endif
	kernel->addEvent(new Event(debugEType, NULL, 30));
}
#endif

#define BUFSIZE 4096

void DebugManager::onWatchableEvent(const Watchable& wbl)
{
	if (!wbl.isValid())
		return;
	
	if (wbl == server_sock) {
		struct sockaddr cliaddr;
		socklen_t len;
		SOCKET client_sock;
		
		len = sizeof(cliaddr);
		client_sock = accept(server_sock, &cliaddr, &len);
	
		if (client_sock != INVALID_SOCKET) {
			HAGGLE_DBG("Registering client socket: %ld\n", client_sock);
			if (!kernel->registerWatchable(client_sock, this)) {
				CLOSE_SOCKET(client_sock);
				return;
			}
			if (client_sockets.empty())
				kernel->getDataStore()->dump(onDumpDataStoreCallback);

			client_sockets.push_back(client_sock);
		} else {
			HAGGLE_DBG("accept failed: %ld\n", client_sock);
		}
	}
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_WINDOWS_DESKTOP)
#if defined(DEBUG)
	else if (wbl == console) {
		char *raw = NULL;
                size_t rawLen;
		int res = 0;
		unsigned char c;
		DebugCmdRef dbgCmdRef;

#if defined(OS_WINDOWS_DESKTOP)
		INPUT_RECORD *inrec; 
		DWORD i, num;

		if (!GetNumberOfConsoleInputEvents(console, &num)) {
			fprintf(stderr, "Failed to read number of console input events\n");
			return;
		}

		inrec = new INPUT_RECORD[num];

		if (!inrec)
			return;

		if (!ReadConsoleInput(console, inrec, num, &num)) {
			fprintf(stderr, "Failed to read console input\n");
			delete[] inrec;
			return;
		}
		
		for (i = 0; i < num; i++) {
			switch (inrec[i].EventType) {
				case KEY_EVENT:
					c = (char)inrec[i].Event.KeyEvent.uChar.UnicodeChar;
					res = 1;
					break;
				default:
					// Ignore
					//printf("unkown event %u\n", inrec[i].EventType);
					break;
			}
		}

		delete [] inrec;

		if (res == 0)
			return;
#else
		res = getchar();
		c = res;
		
		if (res == EOF) {
			if (ferror(stdin) != 0) {
				fprintf(stderr, "ferror: Could not read character - %s\n", 
					strerror(errno));
				clearerr(stdin);
				return;
			} else if (feof(stdin) != 0) {
				fprintf(stderr, "EOF on stdin\n");
				clearerr(stdin);
				return;
			}
			fprintf(stderr, "Unknown error on stdin\n");
			return;
		}
#endif
		//fprintf(stderr, "Character %c pressed\n", c);

		trace_disable(true); // MOS - disable stdout logging

		switch (c) {
			case 'c':
				dbgCmdRef = new DebugCmd(DBG_CMD_PRINT_CERTIFICATES);
				kernel->addEvent(new Event(dbgCmdRef));
				break;
#ifdef DEBUG_DATASTORE
			case 'd':
				kernel->getDataStore()->print();
				break;
#endif
		        case 'g':
				dbgCmdRef = new DebugCmd(DBG_CMD_PRINT_DATAOBJECTS);
				kernel->addEvent(new Event(dbgCmdRef));
				break;
			case 'u':
				kernel->getThisNode()->getDataObject()->print();
				break;
			case 'i':
				// Interface list
				//dbgCmdRef = DebugCmdRef(new DebugCmd(DBG_CMD_PRINT_INTERNAL_STATE), "PrintInterfacesDebugCmd");
				//kernel->addEvent(new Event(dbgCmdRef));
				kernel->getInterfaceStore()->print();
				break;
#ifdef DEBUG_LEAKS
			case 'l':
				LeakMonitor::reportLeaks();
				break;
#endif
			case 'm':
				kernel->printRegisteredManagers();
				break;
			case 'n':
				kernel->getNodeStore()->print();
				break;
			case 'p':
				dbgCmdRef = new DebugCmd(DBG_CMD_PRINT_PROTOCOLS);
				kernel->addEvent(new Event(dbgCmdRef));
				break;
			case 'r':
				dbgCmdRef = new DebugCmd(DBG_CMD_PRINT_ROUTING_TABLE);
				kernel->addEvent(new Event(dbgCmdRef));
				break;
			case 's':
				kernel->shutdown();
				break;		
#ifdef DEBUG
			case 't':				
				printf("===============================\n");
				Thread::registryPrint();
				printf("===============================\n");
				break;
#endif
			case 'b':
				kernel->getThisNode()->getDataObject()->getRawMetadataAlloc((unsigned char **)&raw, &rawLen);
				if (raw) {
					printf("======= Node description =======\n");
						printf("%s\n", raw);
					printf("================================\n");
					free(raw);
				}
				break;
// SW: START: cache strat debug:
            case 'z':
            {
                dbgCmdRef = new DebugCmd(DBG_CMD_PRINT_CACHE_STRAT);
                kernel->addEvent(new Event(dbgCmdRef));
                break;
            }
// SW: END: cache strat debug:
			case 'h':
			default:
				printf("========== Console help ==========\n");
				printf("The keys listed below does the following:\n");
				printf("c: Certificate list\n");
				printf("b: Node description of \'this node\'\n");		
#ifdef DEBUG_DATASTORE
				printf("d: list data store tables\n");
#endif
				printf("g: list data data objects sent and received\n");
				printf("i: Interface list\n");
#ifdef DEBUG_LEAKS
				printf("l: Leak report\n");
#endif
				printf("m: Manager list with registered sockets\n");
				printf("n: Neighbor list\n");
				printf("p: Running protocols\n");
				printf("r: Current routing table\n");
				printf("s: Generate shutdown event (also ctrl-c)\n");
				printf("t: Print thread registry (running threads)\n");
// SW: START: cache strat debug:
                printf("z: Print utility based caching information (if available)\n");
// SW: END: cache strat debug:
				printf("any other key: this help\n");
				printf("==================================\n");
				break;
		}
	}
#endif
#endif
}

void DebugManager::publicEvent(Event *e)
{
	if (!e)
		return;
	
	HAGGLE_DBG("%s data=%s\n", e->getName(), e->hasData() ? "Yes" : "No");
}

SOCKET openSocket(int port)
{
	int ret;
	const int optval = -1;
	struct sockaddr_in addr;
	SOCKET sock;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("Could not create TCP socket\n");
		return INVALID_SOCKET;
	}
	
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(int));
	
	if (ret == SOCKET_ERROR) {
		HAGGLE_ERR("setsockopt SO_REUSEADDR failed: %s\n", STRERROR(ERRNO));
	}
		
	ret = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(int));
	
	if (ret == SOCKET_ERROR) {
		HAGGLE_ERR("setsockopt SO_KEEPALIVE failed: %s\n", STRERROR(ERRNO));
	}
	
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	ret = bind(sock, (struct sockaddr *) &addr, sizeof(addr));

	if (ret == SOCKET_ERROR) {
		HAGGLE_ERR("Could not bind debug TCP socket to port %d\n", port);
		return INVALID_SOCKET;
	}

	listen(sock, 1);

	return sock;
}

void DebugManager::onConfig(Metadata *m)
{
	Metadata *dm = m->getMetadata("DebugTrace");

	if (dm) {
		const char *parm = dm->getParameter("enable");

		if (parm) {
			if (strcmp(parm, "true") == 0) {
				HAGGLE_DBG("Enabling debug trace\n");
				Trace::trace.enableFileTrace();
			} else if (strcmp(parm, "false") == 0) {
				HAGGLE_DBG("Disabling debug trace\n");
				Trace::trace.disableFileTrace();
				//fclose(stdout); // MOS - keep open for debugging console
				//fclose(stderr); // MOS
			}
		}

		parm = dm->getParameter("flush");

		if (parm) {
			if (strcmp(parm, "true") == 0) {
				HAGGLE_DBG("Enabling debug trace flushing\n");
				Trace::trace.enableFileTraceFlushing();
			} else if (strcmp(parm, "false") == 0) {
				HAGGLE_DBG("Disabling debug trace flushing\n");
				Trace::trace.disableFileTraceFlushing();
			}
		}

		parm = dm->getParameter("type");

		if (parm) {
			if (strcmp(parm, "ERROR") == 0) {
				HAGGLE_DBG("Setting trace type to ERROR\n");
				Trace::trace.setTraceType(TRACE_TYPE_ERROR);
			} else if (strcmp(parm, "STAT") == 0) {
				HAGGLE_DBG("Setting trace type to STAT\n");
				Trace::trace.setTraceType(TRACE_TYPE_STAT);
			} else if (strcmp(parm, "DEBUG") == 0) {
				HAGGLE_DBG("Setting trace type to DEBUG\n");
				Trace::trace.setTraceType(TRACE_TYPE_DEBUG);
			} else if (strcmp(parm, "DEBUG1") == 0) {
				HAGGLE_DBG("Setting trace type to DEBUG1\n");
				Trace::trace.setTraceType(TRACE_TYPE_DEBUG1);
			} else if (strcmp(parm, "DEBUG2") == 0) {
				HAGGLE_DBG("Setting trace type to DEBUG2\n");
				Trace::trace.setTraceType(TRACE_TYPE_DEBUG2);
			}
		}

		parm = dm->getParameter("enable_stdout");
		Trace::trace.disableStdout();
		if(parm) {
			if(strcmp(parm,"true") == 0 ) {
				HAGGLE_DBG("Enabling stdout trace\n");
				Trace::trace.enableStdout();
			}
		}	
	}
}
#endif
