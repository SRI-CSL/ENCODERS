/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM)
 *   Sam Wood (SW)
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
#ifndef _DATASTORE_H
#define _DATASTORE_H

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/List.h>

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class DataStoreQueryResult;
class DataStoreFilterQuery;
class DataStoreDataObjectQuery;
class DataStoreDataObjectForNodesQuery;
class DataStoreRepositoryQuery;
class DataStoreTask;
class DataStore;


// SW: JM: START REPLACEMENT:
class DataStoreReplacementTotalOrderQueryResult;
class DataStoreReplacementTotalOrderQuery;
// SW: JM: END REPLACEMENT.

// JM: START REPLACEMENT:
class DataStoreReplacementTimedDeleteQuery;
// JM: END REPLACEMENT.

#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/Thread.h>

#include "Metadata.h"
#include "Filter.h"
#include "Event.h"
#include "Node.h"
#include "DataObject.h"
#include "Interface.h"
#include "Debug.h"
#include "Utility.h"
#include "RepositoryEntry.h"
#include "Attribute.h"

#include "ScratchpadManager.h"

// SW: START: frag/block max match fix:
#include "networkcoding/manager/NetworkCodingConfiguration.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"
// SW: END: frag/block max match fix.

//#define DEBUG_DATASTORE

#define DATASTORE_MAX_DATAOBJECTS_AGED_AT_ONCE 3

class HaggleKernel;

// SW: JM: START REPLACEMENT:
class DataStoreReplacementTotalOrderQueryResult
{
private:
    List<Pair<DataObjectRef, unsigned long> > *orderedObjs;
public:
    DataStoreReplacementTotalOrderQueryResult() : 
        orderedObjs(new List<Pair<DataObjectRef, unsigned long> >()) {}

    void addDataObject(DataObjectRef dObj, unsigned long metric) {
        // add in descending order
        List<Pair<DataObjectRef, unsigned long> >::iterator it = orderedObjs->begin();
        for (; it != orderedObjs->end(); it++) {
            if (metric < (*it).second) {
                break;
            }
        }
        orderedObjs->insert(it, make_pair(dObj, metric));
    }

    List<Pair<DataObjectRef, unsigned long> >* getOrderedObjects() {
        return orderedObjs;
    }
};
// SW: JM: END REPLACEMENT.

// Result returned from a query
/** */
class DataStoreQueryResult
{
        NodeRefList nodes;
        DataObjectRefList dataObjects;
	// These Timevals are used for benchmarking
	Timeval queryInit;
        Timeval querySqlStart;
        Timeval querySqlEnd;
        Timeval queryResult;
	RepositoryEntryList repositoryEntries;
public:
        DataStoreQueryResult();
        ~DataStoreQueryResult();
        void setQueryInitTime(const Timeval &t) {
		queryInit = t;
        }
        void setQuerySqlStartTime() {
                querySqlStart = Timeval::now();
        }
        void setQuerySqlEndTime() {
                querySqlEnd = Timeval::now();
        }
        void setQueryResultTime() {
                queryResult = Timeval::now();
        }
        const Timeval& getQueryInitTime() const {
                return queryInit;
	}
	const Timeval& getQuerySqlStartTime() const {
		return querySqlStart;
	}
	const Timeval& getQuerySqlEndTime() const {
		return querySqlEnd;
	}
	const Timeval& getQueryResultTime() const {
		return queryResult;
	}
	int addNode(NodeRef& n);
	int delNode(NodeRef& n);
	int addDataObject(DataObjectRef& dObj);
	int delDataObject(DataObjectRef& dObj);
	NodeRef detachFirstNode();
	DataObjectRef detachFirstDataObject();
	const NodeRefList *getNodeList() const;
	const DataObjectRefList *getDataObjectList() const;
	int countDataObjects();
	int countNodes();
	int addRepositoryEntry(RepositoryEntryRef& re);
	RepositoryEntryRef detachFirstRepositoryEntry();
	int countRepositoryEntries();
};

/** */
class DataStoreFilterQuery
{
        const Filter *f;
        const EventCallback<EventHandler> *callback;
public:
        const Filter *getFilter() const {
                return f;
        }
        const EventCallback<EventHandler> *getCallback() const {
                return callback;
        }
        DataStoreFilterQuery(const Filter *_f, const EventCallback<EventHandler> *_callback) : f(_f), callback(_callback) {}
        ~DataStoreFilterQuery() {
                delete f;
        }
};

/** */
class DataStoreDataObjectQuery
{
        NodeRef node;
        Timeval queryInitTime;
        const unsigned int attrMatch;
        const EventCallback<EventHandler> *callback;
public:
        const NodeRef getNode() const {
                return node;
        }
        unsigned int getAttrMatch() const {
                return attrMatch;
        }
        const Timeval& getQueryInitTime() const {
                        return queryInitTime;
                }
        const EventCallback<EventHandler> *getCallback() const {
                return callback;
        }
        DataStoreDataObjectQuery(NodeRef& n, const unsigned int _attrMatch, const EventCallback<EventHandler> *_callback, bool _noResult = false) : node(n), queryInitTime(Timeval::now()), attrMatch(_attrMatch), callback(_callback) {
        }
        ~DataStoreDataObjectQuery() {}
};

/** */
class DataStoreDataObjectForNodesQuery
{
        NodeRef node;
        NodeRefList nodes;
        Timeval queryInitTime;
        const unsigned int attrMatch;
        const EventCallback<EventHandler> *callback;
public:
        const NodeRef getNode() const {
                return node;
        }
        const NodeRef getNextNode() {
                return nodes.pop();
        }
        unsigned int getAttrMatch() const {
                return attrMatch;
        }
        const Timeval& getQueryInitTime() const {
                        return queryInitTime;
                }
        const EventCallback<EventHandler> *getCallback() const {
                return callback;
        }
        DataStoreDataObjectForNodesQuery(const NodeRef &n, const NodeRefList &ns, const unsigned int _attrMatch, const EventCallback<EventHandler> *_callback) : node(n), nodes(ns), queryInitTime(Timeval::now()), attrMatch(_attrMatch), callback(_callback) {
        }
        ~DataStoreDataObjectForNodesQuery() {}
};

/** */
class DataStoreNodeQuery
{
	DataObjectRef dObj;
	Timeval queryInitTime;
	const unsigned int maxResp;
	const unsigned int attrMatch;
	const EventCallback<EventHandler> *callback;
public:
	const DataObjectRef getDataObject() const {
		return dObj;
	}
	unsigned int getMaxResp() const {
		return maxResp;
	}
	unsigned int getAttrMatch() const {
		return attrMatch;
	}
	const Timeval& getQueryInitTime() const {
		return queryInitTime;
	}
	const EventCallback<EventHandler> *getCallback() const {
		return callback;
	}
	DataStoreNodeQuery(DataObjectRef& d, const unsigned int _maxResp, const unsigned int _attrMatch, const EventCallback<EventHandler> *_callback) : dObj(d), queryInitTime(Timeval::now()), maxResp(_maxResp), attrMatch(_attrMatch), callback(_callback) {
	}
	~DataStoreNodeQuery() {}
};

// SW: JM: START REPLACEMENT:
class DataStoreReplacementTotalOrderQuery 
{
    Timeval queryInitTime;
    string tagFieldName;
    string tagFieldValue;
    string metricFieldName; 
    string idFieldName;
    string idFieldValue;
    const EventCallback<EventHandler> *callback;
public:

    string getTagFieldName() const {
        return tagFieldName;
    }

    string getTagFieldValue() const {
        return tagFieldValue;
    }

    string getMetricFieldName() const {
        return metricFieldName;
    }

    string getIdFieldName() const {
        return idFieldName;
    }

    string getIdFieldValue() const {
        return idFieldValue;
    }

    const Timeval& getQueryInitTime() const {
        return queryInitTime;
    }

    const EventCallback<EventHandler> *getCallback() const {
        return callback;
    }

    DataStoreReplacementTotalOrderQuery(
        string _tagFieldName,
        string _tagFieldValue,
        string _metricFieldName, 
        string _idFieldName,    
        string _idFieldValue, 
        const EventCallback<EventHandler> *_callback) :
            tagFieldName(_tagFieldName),
            tagFieldValue(_tagFieldValue),
            metricFieldName(_metricFieldName),
            idFieldName(_idFieldName),
            idFieldValue(_idFieldValue),
            callback(_callback) {}

    ~DataStoreReplacementTotalOrderQuery() {}
};
// SW: JM: END REPLACEMENT.
// JM: START REPLACEMENT:
class DataStoreReplacementTimedDeleteQuery 
{
    string tagFieldName;
    string tagFieldValue;
	const EventCallback<EventHandler> *callback;
public:

	string getTagFieldName() const {
		return tagFieldName;
	}

	string getTagFieldValue() const {
		return tagFieldValue;
	}

	const EventCallback<EventHandler> *getCallback() const {
		return callback;
	}

    DataStoreReplacementTimedDeleteQuery(
        string _tagFieldName,
        string _tagFieldValue,
        const EventCallback<EventHandler> *_callback) :
            tagFieldName(_tagFieldName),
            tagFieldValue(_tagFieldValue),
            callback(_callback) {}
        
    ~DataStoreReplacementTimedDeleteQuery() {}
};
// JM: END REPLACEMENT.

/** */
class DataStoreRepositoryQuery
{
	const RepositoryEntryRef query;
	const EventCallback<EventHandler> *callback;
public:
	const RepositoryEntryRef getQuery() const;
	const EventCallback<EventHandler> *getCallback() const;
	DataStoreRepositoryQuery(const RepositoryEntryRef& _re, EventCallback<EventHandler> *_callback = NULL);
	~DataStoreRepositoryQuery() {};
};

class DataStoreDump
{
        char *data;
        size_t len;
    public:
	size_t getLen() { return len; }
        const char *getData() { return data; }
        DataStoreDump(char *_data, const size_t _len) : data(_data), len(_len) {}
        ~DataStoreDump() { if (data) free(data); }
};

/*
	If you add a task, also make sure that you handle object deletion in the 
	DataStore destructor. Also, do not forget to update the task names in the 
	static array in DataStore.cpp
*/
typedef enum {
	TASK_INSERT_DATAOBJECT,
// SW: START GET DOBJ FROM ID:
    TASK_RETRIEVE_DATAOBJECT_BY_ID,
// SW: END GET DOBJ FROM ID.
	TASK_DELETE_DATAOBJECT,
	TASK_DELETE_DATAOBJECT_BY_ID,
	TASK_AGE_DATAOBJECTS,
	TASK_INSERT_NODE,
	TASK_DELETE_NODE,
	TASK_RETRIEVE_NODE,
	TASK_RETRIEVE_NODE_BY_TYPE,
	TASK_RETRIEVE_NODE_BY_INTERFACE,
	TASK_ADD_FILTER,
	TASK_DELETE_FILTER,
	TASK_FILTER_QUERY,
	TASK_DATAOBJECT_QUERY,
	TASK_DATAOBJECT_FOR_NODES_QUERY,
	TASK_NODE_QUERY,
	TASK_INSERT_REPOSITORY,
	TASK_READ_REPOSITORY,
	TASK_DELETE_REPOSITORY,
	TASK_DUMP_DATASTORE,
	TASK_DUMP_DATASTORE_TO_FILE,
#ifdef DEBUG_DATASTORE
	TASK_DEBUG_PRINT,
#endif
// SW: START DATASTORE CFG:
    TASK_CONFIG,
// SW: END DATASTORE CFG.
	TASK_EXIT,
// SW: JM: START REPLACEMENT:
    TASK_REPLACEMENT_TOTAL_ORDER,
// SW: JM: END REPLACEMENT.
// JM: START TIME DELETE
    TASK_TIME_DELETE_DATAOBJECTS,
// JM: END TIME DELETE
	_TASK_MAX,
} TaskType;

/** */
class DataStoreTask
{
	/*
	 The priority decides how a task is sorted in the data store's task queue.
	 Higher priorities should be lower to be sorted first as our heap implementation
	 is a min-heap.
	 */
	typedef enum {
		TASK_PRIORITY_HIGH,
		TASK_PRIORITY_MEDIUM,
		TASK_PRIORITY_LOW
	} Priority_t;
	static unsigned long totNum;
	static const char *taskName[_TASK_MAX];
	friend class DataStore;
	TaskType type;
	Priority_t priority;
	unsigned long num;
	Timeval timestamp;
	union {
		void *data;
		Filter *f;
		DataStoreFilterQuery *query;
		DataStoreDataObjectQuery *DOQuery;
		DataStoreDataObjectForNodesQuery *DOForNodesQuery;
		DataStoreNodeQuery *NodeQuery;
		DataStoreRepositoryQuery *RepositoryQuery;
		NodeRef *node;
		DataObjectRef *dObj;
		InterfaceRef *iface;
		Node::Type_t nodeType;
		Timeval *age;
		DataObjectId_t id;
// SW: JM: REPLACEMENT START:
        DataStoreReplacementTotalOrderQuery *ReplacementTotalOrderQuery;     
// SW: JM: REPLACEMENT END.
// JM: REPLACEMENT START:
        DataStoreReplacementTimedDeleteQuery *ReplacementTimedDeleteQuery;     
// JM: REPLACEMENT END.
// SW: START DATASTORE CFG:
        Metadata *m;
// SW: END DATASTORE CFG.
	};
	const EventCallback<EventHandler> *callback;
	// Some tasks also take a boolean parameter. This is it:
	bool boolParameter;
	bool boolParameter2;
public:
// SW: START DATASTORE CFG:
    DataStoreTask(Metadata *m);
// SW: END DATASTORE CFG.
	DataStoreTask(DataObjectRef& _dObj, TaskType _type = TASK_INSERT_DATAOBJECT, const EventCallback<EventHandler> *_callback = NULL, 
		      bool keepInBloomfilter = false, bool shouldReportRemoval = true);
	DataStoreTask(const DataObjectId_t _id, TaskType _type = TASK_DELETE_DATAOBJECT_BY_ID, const EventCallback<EventHandler> *_callback = NULL,
		      bool keepInBloomfilter = false, bool shouldReportRemoval = true);
	DataStoreTask(const NodeRef& _node, TaskType _type = TASK_INSERT_NODE, const EventCallback<EventHandler> *_callback = NULL, bool _boolParameter = false);
        DataStoreTask(Node::Type_t _nodeType, TaskType _type = TASK_RETRIEVE_NODE_BY_TYPE, const EventCallback<EventHandler> *_callback = NULL);
	DataStoreTask(const InterfaceRef& _iface, TaskType _type = TASK_RETRIEVE_NODE_BY_INTERFACE, const EventCallback<EventHandler> *_callback = NULL, bool _boolParameter = false);
        DataStoreTask(DataStoreFilterQuery *q, TaskType _type = TASK_FILTER_QUERY);
	DataStoreTask(DataStoreDataObjectQuery *q, TaskType _type = TASK_DATAOBJECT_QUERY);
        DataStoreTask(DataStoreDataObjectForNodesQuery *q, TaskType _type = TASK_DATAOBJECT_FOR_NODES_QUERY);
	DataStoreTask(DataStoreNodeQuery *q, TaskType _type = TASK_NODE_QUERY, bool _boolParameter = false); // MOS - added bool app-only param
	DataStoreTask(DataStoreRepositoryQuery *q, TaskType _type);
	DataStoreTask(const Filter& _f, TaskType _type, const EventCallback<EventHandler> *_callback = NULL, bool _boolParameter = false);
	DataStoreTask(DataObjectRef& _dObj, TaskType _type, void *_data = NULL, const EventCallback<EventHandler> *_callback = NULL);
	DataStoreTask(TaskType _type, void *_data = NULL, const EventCallback<EventHandler> *_callback = NULL);
	DataStoreTask(const Timeval &_age, TaskType _type = TASK_AGE_DATAOBJECTS, const EventCallback<EventHandler> *_callback = NULL, bool keepInBloomfilter = false);
        DataStoreTask(const DataStoreTask &ii); // Not defined
    // JM: START REPLACEMENT:
    DataStoreTask(DataStoreReplacementTotalOrderQuery *q);
    DataStoreTask(DataStoreReplacementTimedDeleteQuery *q);
    // JM: END REPLACEMENT.
	~DataStoreTask();

	const TaskType& getType() const { return type; }
	const Priority_t& getPriority() const { return priority; }
	// getKey() is overridden from the HeapItem class and decides how the task
	// is sorted in the task queue.
	const Timeval& getTimestamp() const { return timestamp; }
// SW: START DATASTORE CFG:
   Metadata *getConfig() { return m; } 
// SW: END DATASTORE CFG.
};


// This is an abstract DataStore class. From this it should be
// possible to implement several backends, e.g., based on XML or SQL
/** */
class DataStore : 
#ifdef DEBUG_LEAKS
	LeakMonitor, 
#endif
	public Runnable
{
        // The runnable class's mutex protects the task Queue
	class TaskQueue : public List<DataStoreTask *> {
	public:
		TaskQueue() {}
		~TaskQueue() {}
		void insert(DataStoreTask *task);
	} taskQ;
        // run() is the function executed by the thread
        bool run();
        // cleanup() is called when the thread is stopped or cancelled
        void cleanup();
	
protected:
	friend class HaggleKernel;
	HaggleKernel *kernel;
    	ScratchpadManager *scratchpadManager;
 
// SW: START: frag/block max match fix:
        NetworkCodingConfiguration *networkCodingConfiguration;
        NetworkCodingDataObjectUtility *networkCodingDataObjectUtility;
        FragmentationConfiguration *fragmentationConfiguration;
        FragmentationDataObjectUtility *fragmentationDataObjectUtility;
// SW: END: frag/block max match fix.

	// Functions acting on the DataStore through the task queue
	virtual int _insertNode(NodeRef& node, const EventCallback<EventHandler> *callback = NULL, bool mergeBloomfilter = false) = 0;
	virtual int _deleteNode(NodeRef& node) = 0;
	virtual int _retrieveNode(NodeRef& node, const EventCallback<EventHandler> *callback, bool forceCallback = false) = 0;
	virtual int _retrieveNode(Node::Type_t type, const EventCallback<EventHandler> *callback) = 0;
	virtual int _retrieveNode(const InterfaceRef& iface, const EventCallback<EventHandler> *callback, bool forceCallback = false) = 0;
	virtual int _insertDataObject(DataObjectRef& dObj, const EventCallback<EventHandler> *callback = NULL, bool reportRemoval = true) = 0;
// SW: START GET DOBJ FROM ID:
    virtual int _retrieveDataObject(const DataObjectId_t &id, const EventCallback<EventHandler> *callback = NULL) = 0;
// SW: END GET DOBJ FROM ID.
	virtual int _deleteDataObject(const DataObjectId_t &id, bool shouldReportRemoval = true, bool keepInBloomfilter = false) = 0;
	virtual int _deleteDataObject(DataObjectRef& dObj, bool shouldReportRemoval = true, bool keepInBloomfilter = false) = 0;
	virtual int _ageDataObjects(const Timeval& minimumAge, const EventCallback<EventHandler> *callback = NULL, bool keepInBloomfilter = false) = 0;
	virtual int _insertFilter(Filter *f, bool matchFilter = false, const EventCallback<EventHandler> *callback = NULL) = 0;
	virtual int _deleteFilter(long eventtype) = 0;
	virtual int _doFilterQuery(DataStoreFilterQuery *q) = 0;
	virtual int _doDataObjectQuery(DataStoreDataObjectQuery *q) = 0;
	virtual int _doDataObjectForNodesQuery(DataStoreDataObjectForNodesQuery *q) = 0;
	virtual int _doNodeQuery(DataStoreNodeQuery *q, bool localAppOnly) = 0; // MOS - added localAppOnly
	virtual int _insertRepository(DataStoreRepositoryQuery* q) = 0;
	virtual int _readRepository(DataStoreRepositoryQuery* q, const EventCallback<EventHandler> *callback = NULL) = 0;
	virtual int _deleteRepository(DataStoreRepositoryQuery* q) = 0;
	virtual int _dump(const EventCallback<EventHandler> *callback = NULL) = 0;
	virtual int _dumpToFile(const char *filename) = 0;
// SW: JM: START REPLACEMENT:
    virtual int _doDataObjectQueryForReplacementTotalOrder(DataStoreReplacementTotalOrderQuery* q) = 0;
// SW: JM: END REPLACEMENT.
// JM: START REPLACEMENT:
    virtual int _doDataObjectQueryForReplacementTimedDelete(DataStoreReplacementTimedDeleteQuery* q) = 0;
// JM: END REPLACEMENT.
#ifdef DEBUG_DATASTORE
	virtual void _print() {};
#endif
// SW: START DATASTORE CFG:
    virtual void _onDataStoreConfig(Metadata *m) = 0;
// SW: END DATASTORE CFG.
    virtual void _exit() {};
	void hookCancel();
public:
        DataStore(const string name = "DataStore");
        virtual ~DataStore();

	/**
	  The init() function should be called after the DataStore has been
	  created in order to initialize it before startup.
	  Should be overridden by derived classes.

	  Returns: true if the initialization was successful, or false otherwise.
	*/
	virtual bool init() { return true; }

	// These functions provide the interface to interact with the
	// DataStore. They wrap the functions in the derived class and
	// provides thread locking. They interact with the data store
	// throught the task queue

        /**
           Dump the data store to memory which is returned in a
           callback as a DataStoreDump object. The dump object must be
           deleted in the callback to avoid memory leaks.

           @param callback the callback context to return the dump to
           @returns 0 on success, or negative on failure.
           
         */
        void dump(const EventCallback<EventHandler> *callback = NULL);
        /**
           Dump the data store to a file. The function works asynchronously.

           @param filename the filename to dump to
           @returns 0 on success, or negative on failure.
           
         */
        void dumpToFile(const char *filename);
	void insertInterface(InterfaceRef& iface);
	void insertNode(NodeRef& node, const EventCallback<EventHandler> *callback = NULL, bool mergeBloomfilter = false);
	void deleteNode(NodeRef& node);
	void retrieveNode(const NodeRef& node, const EventCallback<EventHandler> *callback, bool forceCallback = false);
	void retrieveNode(Node::Type_t type, const EventCallback<EventHandler> *callback);
	void retrieveNode(const InterfaceRef& iface, const EventCallback<EventHandler> *callback, bool forceCallback = false);
	void insertDataObject(DataObjectRef& dObj, const EventCallback<EventHandler> *callback = NULL);
// SW: START GET DOBJ FROM ID:
    void retrieveDataObject(const DataObjectId_t id, const EventCallback<EventHandler> *callback = NULL);
// SW: END GET DOBJ FROM ID.
        void deleteDataObject(const DataObjectId_t id, bool keepInBloomfilter = false, bool shouldReportRemoval = true); // MOS - added shouldReportRemoval
        void deleteDataObject(DataObjectRef& dObj, bool keepInBloomfilter = false, bool shouldReportRemoval = true); // MOS - added shouldReportRemoval
	void ageDataObjects(const Timeval& minimumAge, const EventCallback<EventHandler> *callback = NULL, bool keepInBloomfilter = false);
	void insertFilter(const Filter& f, bool matchFilter = false, const EventCallback<EventHandler> *callback = NULL);
	void deleteFilter(long eventtype);
	void doFilterQuery(const Filter *f, EventCallback<EventHandler> *callback);
	void doDataObjectQuery(NodeRef& n, const unsigned int match, EventCallback<EventHandler> *callback);
	void doDataObjectForNodesQuery(const NodeRef &n, const NodeRefList &ns, 
                                      const unsigned int match,
                                      const EventCallback<EventHandler> *callback);
	void doNodeQuery(DataObjectRef& d, const unsigned int maxResp, const unsigned int match, EventCallback<EventHandler> *callback, 
			 bool localAppOnly = false); // MOS - added localAppOnly param
#ifdef DEBUG_DATASTORE
	virtual void print();
#endif
	void insertRepository(RepositoryEntryRef re);
	void readRepository(RepositoryEntryRef re, EventCallback < EventHandler > *callback);
	void deleteRepository(RepositoryEntryRef re);

	// Query cancel functions. Returns the number of queries removed, or -1 on error.
	int cancelDataObjectQueries(const NodeRef& node);

	void onConfig();
	
    // SW: JM: START REPLACEMENT:
    void doDataObjectQueryForTotalOrderReplacement(
        string tagFieldName, 
        string tagFieldValue, 
        string metricFieldName, 
        string idFieldName, 
        string idFieldValue, 
        EventCallback<EventHandler> *callback);
    // SW: JM: END REPLACEMENT.
    // JM: START REPLACEMENT:
    void doDataObjectQueryForTimedDelete(
        const char *tagFieldName, 
        const char *tagFieldValue, 
        EventCallback<EventHandler> *callback);
    // JM: END REPLACEMENT.
// SW: START DATASTORE CFG:
	void onConfig(Metadata *m = NULL);
// SW: END DATASTORE CFG.
    ScratchpadManager *getScratchpadManager() { return scratchpadManager; }

// SW: START: frag/block max match fix:
    virtual string getIdOrParentId(DataObjectRef dObj) = 0; // CBMEN, HL
        void setNetworkCodingConfiguration(NetworkCodingConfiguration *_networkCodingConfiguration) { networkCodingConfiguration = _networkCodingConfiguration; }
        void setNetworkCodingDataObjectUtility(NetworkCodingDataObjectUtility *_networkCodingDataObjectUtility) { networkCodingDataObjectUtility = _networkCodingDataObjectUtility; }
        void setFragmentationConfiguration(FragmentationConfiguration *_fragmentationConfiguration) { fragmentationConfiguration = _fragmentationConfiguration; }
        void setFragmentationDataObjectUtility(FragmentationDataObjectUtility *_fragmentationDataObjectUtility) { fragmentationDataObjectUtility = _fragmentationDataObjectUtility; }
// SW: END: frag/block max match fix.
};

#endif /* _DATASTORE_H */
