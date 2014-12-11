/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Joshua Joy (JJ, jjoy)
 *   Sam Wood (SW)
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
#ifndef _APPLICATIONMANAGER_H
#define _APPLICATIONMANAGER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ApplicationHandle;
class ApplicationManager;

#include <libcpphaggle/Pair.h>
#include <libcpphaggle/Map.h>

#include "Manager.h"
#include "Event.h"
#include "DataObject.h"
#include "Node.h"

#include "networkcoding/application/ApplicationManagerHelper.h"

#define MONITOR_APP_NAME "MONITOR" // MOS - name of passive monitoring applications

// CBMEN, HL, Begin
#ifdef OS_ANDROID
#define TEMP_OBSERVER_FILE_PATH "/data/data/org.haggle.kernel/files/haggletmpobserverdata.XXXXXX"
#else
#define TEMP_OBSERVER_FILE_PATH "/tmp/haggletmpobserverdata.XXXXXX"
#endif
// CBMEN, HL, End

/*
	We import the control attributes from libhaggle in order to make sure
	we use the same values here. It is not a nice solution, as it makes haggle
	dependent on the presence of libhaggle. It should be considered or a 
	temoporary solution until the API is stabalized.
*/
#include "../libhaggle/include/libhaggle/ipc.h"

typedef List< Pair<NodeRef, DataObjectRef> > SentToApplicationList;

/** */
class ApplicationHandle
{
        bool event_types[_LIBHAGGLE_NUM_EVENTS];
        ApplicationNodeRef app;
	// NOTE: Using the map here is not optimal as we do not
	// need the value. However, this works until we have a non-stl
	// version of set.
	typedef Map<EventType, char> eventTypeRegistry;
	typedef Pair<EventType, char> eventTypeRegistryPair;
        eventTypeRegistry eventTypes;
        int id;
public:
        const ApplicationNodeRef getNode() const {
                return app;
        }
        int getId() const {
                return id;
        }

        bool hasEventInterest(int eid) {
                return event_types[eid];
        }
        void addEventInterest(int eid) {
                event_types[eid] = true;
        }
        void addInterestAttribute(const string name, const string value) {
                app->addAttribute(name, value);
        }
        void addFilterEventType(const EventType etype) {
                eventTypes.insert(make_pair(etype,'\0'));
        }
        bool hasFilterEventType(const EventType etype) {
                return (eventTypes.find(etype) != eventTypes.end()) ? true : false;
        }
        const eventTypeRegistry *getEventTypes() const {
                return &eventTypes;
        }

        ApplicationHandle(ApplicationNodeRef& _app, int _id) : app(_app), id(_id) {
                for (int i = 0; i < _LIBHAGGLE_NUM_EVENTS; i++) event_types[i] = false;
        }
        ~ApplicationHandle() {}
};

/** */
class ApplicationManager : public Manager
{
	SentToApplicationList pendingDOs;
        unsigned long numClients;
	unsigned long sessionid;
	bool dataStoreFinishedProcessing;
	EventCallback<EventHandler> *onRetrieveNodeCallback;
	EventCallback<EventHandler> *onDataStoreFinishedProcessingCallback;
	EventCallback<EventHandler> *onRetrieveAppNodesCallback;
	EventCallback <EventHandler> *onInsertedDataObjectCallback;
	EventType ipcFilterEvent;
	void onDataStoreFinishedProcessing(Event *e);
	EventType prepare_shutdown_timeout_event; // MOS
	void onPrepareShutdownTimeout(Event *e); // MOS
// SW: added delete application state
        int deRegisterApplication(ApplicationNodeRef& app, bool deleteApplicationState=false);
        void sendToApplication(DataObjectRef& dObj, ApplicationNodeRef& app);
        int sendToAllApplications(DataObjectRef& dObj, long eid);
        int addApplicationEventInterest(ApplicationNodeRef& app, long eid);
        int updateApplicationInterests(ApplicationNodeRef& app);
	Metadata *addControlMetadata(const control_type_t type, const string app_name, Metadata *parent);

	void sendCopyToApplication(DataObjectRef dObj, ApplicationNodeRef target); // MOS

        /* Event handler functions */
        void onSendResult(Event *e);
	void onDeletedDataObject(Event *e);
        void onReceiveFromApplication(Event *e);
        void onNeighborStatusChange(Event *e);
	void onRetrieveNode(Event *e);
	void onAppNodeUpdated(Event *e); // MOS
	void onRetrieveAppNodes(Event *e);
        void onApplicationFilterMatchEvent(Event *e);
        void onInsertedDataObject(Event *e);
        void onSendDataObjectToApp(Event *e); // MOS
        
	void onPrepareShutdown();
	void onShutdown();
	void onStartup();
	bool init_derived();
	void onConfig(Metadata *m); // MOS
        void onDynamicConfig(Metadata *m); // CBMEN, HL

	Attributes defaultInterests; // MOS
	void addDefaultInterests(NodeRef node); // MOS
	ApplicationManagerHelper* applicationManagerHelper;
	bool resetBloomFilterAtRegistration; // MOS
	unsigned long forcedShutdownAfterTimeout; // MOS

	ApplicationNodeRef monitorAppNode; // MOS
	bool deleteStateOnDeregister; // SW: TODO

        // CBMEN, HL, Begin
        EventCallback<EventHandler> *observerCallback;
        double observerCallbackDelay; 
        bool waitingForObserverCallback;
        HashMap<string, bool> enabledObservables;
        size_t dumpedDBs;
        string baseTempFilePath;
        string tempFilePath;
        FILE *tempFile;
        size_t dumpedObservables;
        size_t observerDOPublishedCount;
        List<Pair<Attribute, size_t> > additionalObserverAttributes;
        void onObserverCallback(Event *e);
        void onSendObserverDataObject(Event *e); 
        void configureObserver(Metadata *m);
        void createNodeMetricsObserverDataObject();
        void createInterfacesObserverDataObject();
        void createCertificatesObserverDataObject();
        void createNodeDescriptionObserverDataObject();
        void createNodeStoreObserverDataObject();
        void createProtocolObserverDataObject();
        void createRoutingTableObserverDataObject();
        void createCacheStrategyObserverDataObject();
        void createDataStoreDumpObserverDataObject();
        void addAdditionalObserverAttributes(DataObjectRef& dObj);
        // CBMEN, HL, End
public:
        bool addDefaultInterest(const Attribute& attr); // CBMEN, HL
        ApplicationManager(HaggleKernel *_kernel = haggleKernel);
        ~ApplicationManager();
};


#endif /* _APPLICATIONMANAGER_H */
