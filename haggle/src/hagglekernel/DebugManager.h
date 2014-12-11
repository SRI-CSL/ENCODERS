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
#ifndef _DEBUGMANAGER_H
#define _DEBUGMANAGER_H

#include <libcpphaggle/Platform.h>

#if defined(DISABLE_DEBUG_MANAGER)
#if defined(ENABLE_DEBUG_MANAGER)
#undef ENABLE_DEBUG_MANAGER
#endif
#elif defined(ENABLE_DEBUG_MANAGER)
#elif defined(DEBUG)
#define ENABLE_DEBUG_MANAGER
#endif

#if defined(ENABLE_DEBUG_MANAGER)

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class DebugManager;

#include "Manager.h"

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/stat.h>
#include <fcntl.h>
//#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

using namespace haggle;

#define DATABUF_LEN 2048

/** */
class DebugManager : public Manager
{
private:
	EventCallback<EventHandler> *onFindRepositoryKeyCallback;
	EventCallback<EventHandler> *onDumpDataStoreCallback;
        SOCKET server_sock;
	List<SOCKET> client_sockets;
	bool interactive;
#if defined(OS_LINUX) || defined(OS_MACOSX)
#define INVALID_STDIN -1
	int console;
	//struct termios org_opts, new_opts;
#elif defined(OS_WINDOWS)
#define INVALID_STDIN INVALID_HANDLE_VALUE
	HANDLE console;
#endif
        EventType eventTestEType;
#ifdef DEBUG
        EventType debugEType;
#endif
	void dumpTo(SOCKET client_sock, DataStoreDump *dump);
	bool init_derived();
public:
        DebugManager(HaggleKernel *_kernel = haggleKernel, bool interactive = true);
        ~DebugManager();
        void onWatchableEvent(const Watchable& wbl);
        void publicEvent(Event *e);
#ifdef DEBUG
        void onDebugReport(Event *e);
#endif
	void onFindRepositoryKey(Event *e);
        void onDumpDataStore(Event *e);
	
	void onShutdown();
	void onConfig(Metadata *m);
};

#endif
#endif /* _DEBUGMANAGER_H */
