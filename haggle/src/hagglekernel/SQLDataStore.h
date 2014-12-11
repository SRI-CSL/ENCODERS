/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM)
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
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
#ifndef _SQLDATASTORE_H
#define _SQLDATASTORE_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class SQLDataStore;

#include <libcpphaggle/Platform.h>
#include "DataStore.h"
#include "Node.h"

#define DEFAULT_DATASTORE_FILENAME "haggle.db"
#define INMEMORY_DATASTORE_FILENAME ":memory:"
#define DEFAULT_DATASTORE_FILEPATH DEFAULT_DATASTORE_PATH

#include <libxml/parser.h>
#include <libxml/tree.h> // For dumping to XML

#include <sqlite3.h>
#include "DataObject.h"
#include "Metadata.h"

//#if (SQLITE_VERSION_NUMBER >= 3007000) // MOS - we have a higher version, but not recognized
#define HAVE_SQLITE_BACKUP_SUPPORT 1
//#endif
#ifdef DEBUG_DATASTORE
#define DEBUG_SQLDATASTORE
#endif

/** */
class SQLDataStore : public DataStore
{
private:
// SW: START FILTER CACHE OPTION:
    bool inMemoryNodeDescriptions;
// SW: END FILTER CACHE OPTION.
// SW: START FILTER CACHE:
    int fc_node_description_event_type;
// SW: END FILTER CACHE.
// MOS: START MONITOR FILTER:
    int monitor_filter_event_type;
// MOS: END MONITOR FILTER.
// MOS: START DO CACHE:
    typedef BasicMap<sqlite_int64,DataObjectRef> do_cache_t;
    do_cache_t do_cache;
// MOS: END DO CACHE.
	sqlite3 *db; 
	bool excludeZeroWeightAttributes; // MOS - exclude from matching and attribute table
	bool excludeNodeDescriptions; // MOS - exclude from query result
	bool countNodeDescriptions;
	bool isInMemory;
	bool recreate;
	string filepath;

	int dataObjectsInserted, dataObjectsDeleted; // MOS
	int persistentDataObjectsInserted, persistentDataObjectsDeleted; // MOS

// SW: START: frag/block max match fix:
        long blocksSkipped; 
// SW: END: frag/block max match fix.

	int cleanupDataStore();
	int createTables();
	int sqlQuery(const char *sql_cmd);

	int setViewLimitedDataobjectAttributes(sqlite_int64 dataobject_rowid = 0);
	int setViewLimitedNodeAttributes(sqlite_int64 dataobject_rowid = 0);
	int evaluateDataObjects(long eventType);
	int evaluateFilters(const DataObjectRef& dObj, sqlite_int64 dataobject_rowid = 0);

	sqlite_int64 getDataObjectRowId(const DataObjectId_t& id);
	sqlite_int64 getAttributeRowId(const Attribute* attr);
	sqlite_int64 getNodeRowId(const NodeRef& node);
	sqlite_int64 getNodeRowId(const InterfaceRef& iface);
	sqlite_int64 getInterfaceRowId(const InterfaceRef& iface);

	DataObject *createDataObject(sqlite3_stmt *stmt);
	NodeRef createNode(sqlite3_stmt *in_stmt, bool fromScratch = false);

	Attribute *getAttrFromRowId(const sqlite_int64 attr_rowid, const sqlite_int64 node_rowid);
	DataObject *getDataObjectFromRowId(const sqlite_int64 dataObjectRowId);
	NodeRef getNodeFromRowId(const sqlite_int64 nodeRowId, bool fromScratch = false);
	Interface *getInterfaceFromRowId(const sqlite_int64 ifaceRowId);
	
	int findAndAddDataObjectTargets(DataObjectRef& dObj, const sqlite_int64 dataObjectRowId, const long ratio);
	int deleteDataObjectNodeDescriptions(DataObjectRef ref_dObj, string& node_id, bool reportRemoval);
#if defined(HAVE_SQLITE_BACKUP_SUPPORT)
	int backupDatabase(sqlite3 *pInMemory, const char *zFilename, int toFile = 1);
#endif
	string getFilepath();
		
	
#ifdef DEBUG_SQLDATASTORE
	void _print();
#endif

// SW: START FILTER CACHE:
    bool _useCacheToEvaluateFilters(const DataObjectRef& dObj, sqlite_int64 dataobject_rowid);
    void _evaluateFiltersWithCache(const DataObjectRef& dObj, sqlite_int64 dataobject_rowid);
    bool _useCacheToEvaluateDataObjects(long eventType);
    int _evaluateDataObjectsWithCache(long eventType);
    bool _isCachedFilter(long eventType);
    bool _isCachedFilter(Filter *f);
    void _insertCachedFilter(Filter *f, bool matchFilter, const EventCallback<EventHandler> *callback);
    void _deleteCachedFilter(long eventType); 
    NodeRefList *_retrieveAllNodes(); 
// SW: END FILTER CACHE.
        xmlDocPtr dumpToXML();

// MOS: START MONITOR FILTER:
    void _evaluateMonitorFilter(const DataObjectRef& dObj, sqlite_int64 dataobject_rowid);
    bool _isMonitorFilter(Filter *f);
    void _insertMonitorFilter(Filter *f, bool matchFilter, const EventCallback<EventHandler> *callback);
// MOS: END MONITOR FILTER.

// SW: function to switch to in memory db, specified in config.xml
protected:
// SW: START: MEMORY DATA STORE:
	friend class MemoryDataStore;
	bool switchToInMemoryDatabase();
	DataObjectRefList _MD_getAllDataObjects();
	NodeRefList _MD_getAllNodes();
	RepositoryEntryList _MD_getAllRepositoryEntries();
// SW: END: MEMORY DATA STORE.
//	Node *createNode(sqlite3 *db, sqlite3_stmt * in_stmt);
//	Node *getNodeFromRowId(sqlite3 * db, const sqlite_int64 nodeRowId);

	// These functions work through the task Queue
	// - insert implements update functionality
	int _insertNode(NodeRef& node, const EventCallback<EventHandler> *callback = NULL, bool mergeBloomfilter = false);
	int _deleteNode(NodeRef& node);
	int _retrieveNode(NodeRef& node, const EventCallback<EventHandler> *callback, bool forceCallback);
	int _retrieveNode(Node::Type_t type, const EventCallback<EventHandler> *callback);
	int _retrieveNode(const InterfaceRef& iface, const EventCallback<EventHandler> *callback, bool forceCallback);
	int _insertDataObject(DataObjectRef& dObj, const EventCallback<EventHandler> *callback = NULL, bool reportRemoval = true);
// SW: START GET DOBJ FROM ID:
    int _retrieveDataObject(const DataObjectId_t &id, const EventCallback<EventHandler> *callback = NULL);
// SW: END GET DOBJ FROM ID.
	int _deleteDataObject(const DataObjectId_t &id, bool shouldReportRemoval = true, bool keepInBloomfilter = false);
	int _deleteDataObject(DataObjectRef& dObj, bool shouldReportRemoval = true, bool keepInBloomfilter = false);
	int _ageDataObjects(const Timeval& minimumAge, const EventCallback<EventHandler> *callback = NULL, bool keepInBloomfilter = false);
	//int _timeDeleteDataObjects(DataObjectRef& dObj, const Timeval& minimumAge, const EventCallback<EventHandler> *callback = NULL, bool keepInBloomfilter = false);
	int _insertFilter(Filter *f, bool matchFilter = false, const EventCallback<EventHandler> *callback = NULL);
	int _deleteFilter(long eventtype);
	// matching Filters
	int _doFilterQuery(DataStoreFilterQuery *q);
	// matching Dataobject > Nodes
	/**
		Returns: The number of data objects filled in.
	*/
	int _doDataObjectQueryStep2(NodeRef &node, NodeRef alsoThisBF, DataStoreQueryResult *qr, int max_matches, unsigned int ratio, unsigned int attrMatch);
	int _doDataObjectQuery(DataStoreDataObjectQuery *q);
	int _doDataObjectForNodesQuery(DataStoreDataObjectForNodesQuery *q);
// SW: JM: START REPLACEMENT:
    int _doDataObjectQueryForReplacementTotalOrder(DataStoreReplacementTotalOrderQuery *q);
// SW: JM: END REPLACEMENT.
// JM: START REPLACEMENT:
    int _doDataObjectQueryForReplacementTimedDelete(DataStoreReplacementTimedDeleteQuery *q);

// JM: END REPLACEMENT.


	// matching Node > Dataobjects
	int _doNodeQuery(DataStoreNodeQuery *q, bool localAppOnly); // MOS - added localAppOnly
	int _insertRepository(DataStoreRepositoryQuery *q);
	int _readRepository(DataStoreRepositoryQuery *q, const EventCallback<EventHandler> *callback = NULL);
	int _deleteRepository(DataStoreRepositoryQuery *q);
	
	int _dump(const EventCallback<EventHandler> *callback = NULL);
	int _dumpToFile(const char *filename);
// SW: START DATASTORE CFG:
	virtual void _onDataStoreConfig(Metadata *m = NULL);
// SW: END DATASTORE CFG.

public:
// SW: START: frag/block max match fix:
    string getIdOrParentId(DataObjectRef dObj);
// SW: END: frag/block max match fix.
	SQLDataStore(const bool recreate = false, const string = DEFAULT_DATASTORE_FILEPATH, const string name = "SQLDataStore");
	~SQLDataStore();

	bool init();
};

#endif /* _SQLDATASTORE_H */
