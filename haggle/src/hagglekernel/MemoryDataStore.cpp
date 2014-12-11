/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#include "MemoryDataStore.h"

#include "SQLDataStore.h"

#include "BenchmarkManager.h" // we use this for unit tests to create fake nodes/DOs

/*
 * In-memory data base that is persisted to disk using the SQLDataStore.
 * This code very closely mimics the SQLDataStore, and supports most of
 * the SQLDataStore options.
 * 
 * This class uses the MemoryCache object heavily to maintain all database
 * state. 
 */
MemoryDataStore::MemoryDataStore(
    bool _recreate,      // `true` to create a new data base (not load from disk)
    const string path,   // directory of persisted data base
    const string name) : // class name
        DataStore(name), // parent class constructor
        mc(new MemoryCache()), // init in-memory state
        filepath(path), // path to persist DB
        recreate(_recreate), // if `true` then create a fresh db
        inMemoryNodeDescriptions(true), // don't store node description data objects in the DB
        shutdownSave(true), //  save on shutdown
        monitor_filter_event_type(-1), // filter that listens to everything
        dataObjectsInserted(0), // count number of DOs inserted
        dataObjectsDeleted(0), // count number of DOs deleted
        persistentDataObjectsDeleted(0), // count number of persistent DOs deleted 
        blocksSkipped(0), // count NC blocks skipped (only record blocks for same DO as 1 in the max matches)
        excludeNodeDescriptions(false), // don't match node descriptions. 
        countNodeDescriptions(false) // don't count node descriptions towards max matches
{
    mc->setMaxNodesToMatch(MEMORY_DATA_STORE_MAX_NODE_MATCH);
}

MemoryDataStore::~MemoryDataStore()
{
    if (mc) {
        delete mc;
    }
}

/*
 * Read a database from disk and initialize this data store. Uses SQLDataStore
 * to load/store to disk in SQL format. 
 */
bool
MemoryDataStore::init()
{
    if (recreate) {
        return true;
    }

    Timeval before = Timeval::now();
    HAGGLE_DBG("Loading database from disk\n");
    SQLDataStore sql = SQLDataStore(false, filepath);
    sql.init();
    sql.switchToInMemoryDatabase();

    // Load data objects 

    int dobjsLoaded = 0;
    {
        DataObjectRefList dobjs = sql._MD_getAllDataObjects();
        for (DataObjectRefList::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
            _insertDataObject(*it);
            dobjsLoaded++;
        }
    }

    // Load nodes

    Timeval b1 = Timeval::now();
    int nodesLoaded = 0;
    {
        NodeRefList nodes = sql._MD_getAllNodes();
        for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
            _insertNode(*it);
            nodesLoaded++;
        }
    }

    // Load repoistory entries

    Timeval b2 = Timeval::now();
    int repoLoaded = 0;
    {
        RepositoryEntryList repos = sql._MD_getAllRepositoryEntries();
        for (RepositoryEntryList::iterator it = repos.begin(); it != repos.end(); it++) {
            DataStoreRepositoryQuery *q = new DataStoreRepositoryQuery(*it);
            _insertRepository(q);
            delete q;
            repoLoaded++;
        }
    }

    Timeval now = Timeval::now();

    HAGGLE_DBG("Loaded, duration: %d ms, repos (%d): %d ms, nodes (%d): %d ms, dobjs (%d): %d ms\n", (now - before).getTimeAsMilliSeconds(), repoLoaded, (now-b2).getTimeAsMilliSeconds(), nodesLoaded, (b2-b1).getTimeAsMilliSeconds(), dobjsLoaded, (b1-before).getTimeAsMilliSeconds());

    return true;
}

/*
 * Called on shutdown. Persist the data objects to disk using a SQLDataStore.
 */
void
MemoryDataStore::_exit()
{
    Timeval before = Timeval::now();
    mc->printStats(); 
    if (!shutdownSave) {
        HAGGLE_DBG("Skipping save on shutdown\n");
        return;
    }
    HAGGLE_DBG("Saving database to disk\n");
    // delete the old db
    SQLDataStore sql = SQLDataStore(true, filepath);
    sql.init();
    sql.switchToInMemoryDatabase();

    // persist the data objects 

    int dobjInserted = 0;
    {
        DataObjectRefList dobjs = mc->getAllDataObjects();
        for (DataObjectRefList::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
            sql._insertDataObject(*it, NULL, false);
            dobjInserted++;
        }
    }

    Timeval b1 = Timeval::now();

    // persist the nodes

    int nodesInserted = 0;

    {
        NodeRefList nodes = mc->getAllNodes();
        for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
            sql._insertNode(*it);
            nodesInserted++;
        }
    }

    Timeval b2 = Timeval::now();

    // persist the repo

    int repoInserted = 0;

    {
        RepositoryEntryList repos = mc->getAllRepository();
        for (RepositoryEntryList::iterator it = repos.begin(); it != repos.end(); it++) {
            DataStoreRepositoryQuery *q = new DataStoreRepositoryQuery(*it);
            sql._insertRepository(q);
            delete q;
            repoInserted++;
        }
    }
    Timeval now = Timeval::now();

    HAGGLE_DBG("Saved, total duration: %d ms, repo (%d): %d ms, nodes (%d): %d ms, dobjs (%d): %d ms\n", (now - before).getTimeAsMilliSeconds(), repoInserted, (now-b2).getTimeAsMilliSeconds(), nodesInserted, (b2-b1).getTimeAsMilliSeconds(), dobjInserted, (b1-before).getTimeAsMilliSeconds());
}

/*
 * Insert a node into the database, optionally merge the bloomfilter of
 * the passed node with that in the database (if it exists).
 */
int
MemoryDataStore::_insertNode(
    NodeRef& node, 
    const EventCallback<EventHandler> *callback, 
    bool mergeBloomfilter)
{
    if (!node) {
        HAGGLE_ERR("No node\n");
        return -1;
    }

    if (node->getType() == Node::TYPE_UNDEFINED) {
        HAGGLE_DBG("Node type undefined. Ignoring INSERT of node %s\n", 
            node->getName().c_str());
        return -1;
    }

    Timeval before = Timeval::now();

    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);
    int ret = 0;

    HAGGLE_DBG("Inserting node %s, num attributes=%lu num interfaces=%lu\n", 
        node->getName().c_str(), 
        node->getAttributes()->size(), 
        node->getInterfaces()->size());

    if (mc->isNodeCached(node_id)) {
        if (mergeBloomfilter) {
            NodeRef existing_node = mc->getNodeFromId(node_id);
            if (!existing_node) {
                HAGGLE_ERR("Could not finding existing node: %s\n", node_id.c_str());
                return -1;
            }
            node->getBloomfilter()->merge(*existing_node->getBloomfilter());
        }
        ret = mc->unCacheNode(node_id);
        if (ret < 0) {
            HAGGLE_ERR("Could not delete node %s\n", node_name.c_str());
            return -1;
        }
    }

    ret = mc->cacheNode(node);

    if (ret < 0) {
        HAGGLE_ERR("Could not insert node: %s\n", node_name.c_str());
        return -1;
    }

    if (callback) {
        HAGGLE_DBG("Scheduling callback for inserted node: %s\n", node_name.c_str());
        kernel->addEvent(new Event(callback, node));
    }

    Timeval now = Timeval::now();

    HAGGLE_DBG("Node %s inserted successfully (duration: %ld us)\n", node_name.c_str(), (now-before).getMicroSeconds());

#if defined(BENCHMARK)
    BENCH_TRACE(BENCH_TYPE_NODE_INSERT_DELAY, (now-before).getMicroSeconds(), 0);
#endif

    return 1;
}

/*
 * Delete a node from the data base. 
 */
int
MemoryDataStore::_deleteNode(
    NodeRef &node)
{
    Timeval before = Timeval::now();
    Timeval now;
    string node_id = Node::idString(node);
    if (!mc->isNodeCached(node_id)) {
        HAGGLE_DBG("Could not delete node : %s\n", node_id.c_str());
        return -1;
    }
    mc->unCacheNode(node_id);
	now = Timeval::now();
    HAGGLE_DBG("Node %s deleted successfully (duration: %ld us)\n", 
        node->getName().c_str(), (now-before).getMicroSeconds());

#if defined(BENCHMARK)
    BENCH_TRACE(BENCH_TYPE_NODE_DELETE_DELAY, (now-before).getMicroSeconds(), 0);
#endif

    return 0;
}

/*
 * Retrieve a node from the database.  Set `forceCallback` to true to merge
 * the interfaces from the refNode with that in the database (this is used
 * by the ApplicationManager).
 */
int
MemoryDataStore::_retrieveNode(
    NodeRef &refNode,
    const EventCallback<EventHandler> *callback,
    bool forceCallback)
{
    if (!callback) {
        HAGGLE_ERR("No callback specified\n");
        return -1;
    }

    string node_id = Node::idString(refNode);
    string node_name = Node::nameString(refNode);

    HAGGLE_DBG("Retrieve Node %s\n", node_name.c_str());

    NodeRef node;

    if (mc->isNodeCached(node_id)) {
        node = mc->getNodeFromId(node_id);
    }
    
    if (!node) {
        HAGGLE_DBG("No node %s in data store\n", node_name.c_str());
        if (forceCallback) {
            HAGGLE_DBG2("Forcing callback\n");
            kernel->addEvent(new Event(callback, refNode));
            return 0;
        }
        else {
            return -1;
        }
    }

    /*
     FIXME: This is done to allow an application's new UDP port number to be
     moved to it's old node. This should really be somehow done in the
     application manager, or have some other way of triggering it, rather
     than relying on forceCallback to be true only when it's the application
     manager that caused this function to be called.
     */
    if (forceCallback) {
        refNode.lock();
        const InterfaceRefList *lst = refNode->getInterfaces();
		
        for (InterfaceRefList::const_iterator it = lst->begin(); 
             it != lst->end(); it++) {
            node->addInterface(*it);
        }
        refNode.unlock();
    }

	HAGGLE_DBG("Node %s retrieved successfully\n", node_name.c_str());
    kernel->addEvent(new Event(callback, node));
    return 1;
}

/*
 * Get all the nodes matching a certain type from the database. 
 */
int
MemoryDataStore::_retrieveNode(
    Node::Type_t type, 
    const EventCallback<EventHandler> *callback)
{
    if (!callback) {
        HAGGLE_ERR("No callback specified\n");
        return -1;
    }
    NodeRefList *node_ptr = new NodeRefList();
    NodeRefList nodes = mc->getNodesFromType(type);
    *node_ptr = nodes;
    kernel->addEvent(new Event(callback, node_ptr));
    
    return 0;
}

/*
 * Grab a node from the database that has a certain interface. 
 */
int
MemoryDataStore::_retrieveNode(
    const InterfaceRef& iface, 
    const EventCallback<EventHandler> *callback, 
    bool forceCallback)
{
    if (!callback) {
        HAGGLE_ERR("No callback specified\n");
        return -1;
    }

    string iface_name = iface->getIdentifierStr();
    string iface_id = Interface::idString(iface).c_str();
    HAGGLE_DBG("Retrieving node based on interface [%s]\n", iface_id.c_str());

    NodeRef node = mc->getNodeFromIfaceId(iface_id);
    
    if (!node) {
        HAGGLE_DBG("No node with interface [%s] in data store\n", iface_name.c_str());
        if (forceCallback) {
            HAGGLE_DBG("Forcing callback\n");
            kernel->addEvent(new Event(callback, iface));
            return 0;
        }
        else {
            return -1;
        }
    }

    string node_id = Node::idString(node);
    string node_name = Node::nameString(node);

    // CBMEN, HL - Begin: Update node interface. This is needed after restarts since we may just have an empty interface.
    if (iface) {
        if (node->hasInterface(iface)) {
            HAGGLE_DBG("Updating interface for Node %s [%s]\n", node->getName().c_str(), node->getIdStr());
            if (node->removeInterface(iface)) {
                if (node->addInterface(iface)) {
                    HAGGLE_DBG("Updated interface for Node %s [%s]\n", node->getName().c_str(), node->getIdStr());
                } else {
                    HAGGLE_ERR("Error adding interface for Node %s [%s] for interface update!\n", node->getName().c_str(), node->getIdStr());
                }
            } else {
                HAGGLE_ERR("Error removing interface for Node %s [%s] for interface update!\n", node->getName().c_str(), node->getIdStr());
            }
        }
    }
    // CBMEN, HL - End

    // carry over from the SQLDataStore which did not persist status to db
    if (iface->isUp()) {
        node->setInterfaceUp(iface);
    }

    HAGGLE_DBG("Node %s retrieved successfully based on interface [%s]\n", node_name.c_str(), iface_name.c_str());

    kernel->addEvent(new Event(callback, node));
    
    return 1;
}

/*
 * Call the monitor filter event w/ the passed data object.
 * The monitor filter listens for all data objects.
 */
void
MemoryDataStore::_evaluateMonitorFilter(DataObjectRef dObj)
{
    if (!dObj->isPersistent() || dObj->isNodeDescription() || (monitor_filter_event_type < 0)) {
        HAGGLE_DBG2("Not valid for monitor filter.\n");
        return; 
    }
    
    DataObjectRefList dObjs;
    dObjs.push_back(dObj);
    kernel->addEvent(new Event(monitor_filter_event_type, dObjs));
    HAGGLE_DBG("Evaluating using monitor filter.\n");
}

/*
 * Remove obsolete node description data objects from data store.
 * Only useful when in_memory_node_descriptions=false
 * Returns 0 if the node description of `dObj` is not the newest one,
 * 1 if it is, and -1 on failure. 
 * `node_id` is set to the node's id for the data object
 */
int
MemoryDataStore::deleteDataObjectNodeDescriptions(
    DataObjectRef dObj, 
    string& node_id,
    bool reportRemoval)
{
    NodeRef node = Node::create(dObj);  
    if (!node) {
        return -1;
    }

    node_id = node->getIdStr();
    DataObjectRefList dobjs = mc->getNodeDataObjectsForNode(node_id);

    DataObjectRefList to_delete;
    unsigned int stored = 0;
    int result = 1;

    DataObjectRef new_do = dObj;
    // we get the data objects back unsorted
    for (DataObjectRefList::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
        DataObjectRef tmp = (*it);
        if (!tmp) {
            HAGGLE_ERR("Somehow got null data object\n");
            continue;
        }
        if (tmp->getCreateTime() > new_do->getCreateTime()) {
            new_do = tmp;
        }
    }

    for (DataObjectRefList::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
        DataObjectRef tmp = (*it);
        // FIXME: we probably don't want to replace if timestamps are equal, but 
        // MO has added this in SQLDataStore...
        if (tmp->getCreateTime() <= new_do->getCreateTime()) {
            to_delete.push_back(tmp);
        }
    }

    if (new_do != dObj) {
        result = 0;
    }

    HAGGLE_DBG("%u node descriptions from same node [%s] already in datastore\n", 
        stored, node_id.c_str());

    for (DataObjectRefList::iterator it = to_delete.begin(); it != to_delete.end(); it++) {
        _deleteDataObject(*it, reportRemoval);
    }

    return result;
}

/*
 * Match the data object to their respective filters and fire the filter
 * events. 
 */
int
MemoryDataStore::evaluateFilters(DataObjectRef dObj)
{
    if (!dObj) {
        HAGGLE_ERR("Null data object\n");
        return -1;
    }

    HAGGLE_DBG("Evaluating filters\n");

    if (inMemoryNodeDescriptions && dObj->isNodeDescription()) {
        DataObjectRefList dObjs;
        dObjs.push_back(dObj);
        for (List<long>::iterator it = nd_filters.begin(); it != nd_filters.end(); it++) {
            HAGGLE_DBG("Evaluating using filter cache.\n");
            kernel->addEvent(new Event((*it), dObjs));    
        }
        return 1;
    }
    
    DataObjectRefList dObjs;

    _evaluateMonitorFilter(dObj);

    HAGGLE_DBG("Data object [%s] filter match\n", DataObject::idString(dObj).c_str());

    dObjs.add(dObj);

    int eventType = -1;

    string dobj_id = DataObject::idString(dObj);
    List<long> filter_ids = mc->getFiltersForDataObjectId(dobj_id);
    for (List<long>::iterator it = filter_ids.begin(); it != filter_ids.end(); it++) {
        eventType = (*it);
        HAGGLE_DBG("Filter with event type %d matches!\n", eventType);
        kernel->addEvent(new Event(eventType, dObjs));
    }
    
    return 0;
}

/*
 * Find all the data objects that match the filter `eventType` and fire the
 * event with these data objects. 
 */
int
MemoryDataStore::evaluateDataObjects(long eventType)
{
    HAGGLE_DBG("Evaluating filters\n");

    if (eventType < 0) {
        HAGGLE_ERR("Trying to evaluate negative filter\n");
        return -1;
    }

    DataObjectRefList dObjs;
    if (eventType == monitor_filter_event_type) {
        dObjs = mc->getAllDataObjects();
    }
    else if (!mc->isFilterCached(eventType)) {
        HAGGLE_ERR("Unknown filter: %d\n", (int) eventType);
        return -1;
    }
    else {
        dObjs = mc->getDataObjectsForFilter(eventType);
    }

    if (dObjs.size()) {
        kernel->addEvent(new Event(eventType, dObjs));
    }
        
    return dObjs.size();
}

/*
 * Insert a data object and evaluate the filters.
 */
int 
MemoryDataStore::_insertDataObject(
    DataObjectRef& dObj, 
    const EventCallback<EventHandler> *callback,
    bool reportRemoval)
{
    if (!dObj) {
        return -1;
    }

    Timeval before = Timeval::now();

    string dobj_id = DataObject::idString(dObj);

    HAGGLE_DBG("DataStore insert data object [%s] with num_attributes=%d\n", dobj_id.c_str(), dObj->getAttributes()->size());

    if (inMemoryNodeDescriptions && dObj->isNodeDescription()) {
        evaluateFilters(dObj);
        if (callback) {
            kernel->addEvent(new Event(callback, dObj));
        }
        return 0;
    }

    if (!inMemoryNodeDescriptions && dObj->isNodeDescription()) {
        string node_id;
        // NOTE: this is only useful if in_memory_node_descriptions is disabled
        int ret = deleteDataObjectNodeDescriptions(dObj, node_id, reportRemoval);
        if (ret == 0) {
            HAGGLE_DBG("There are already newer node descriptions for"
                       " the same node [%s] in the data store.\n", 
				       node_id.c_str());
            return -1;
        }
        else if (ret == -1) {
            HAGGLE_ERR("Bad node description, ignoring insert.\n");
            return -1;
        }

        NodeRef node = Node::create(dObj);
        if (node->getNodeDescriptionAttribute() == Node::NDATTR_NONE) {
            // if you turn in memory node descriptions off, then please add 
            // node description attribute for proper matching (the filters
            // won't match otherwise).
            HAGGLE_ERR("in memory node descriptions is disabled but no node attributes, this will interfer with matching! setting attribute as a work around...\n");
            dObj->addAttribute(NODE_DESC_ATTR, "", 0);
        }
    }


    DataObjectRef oldObj;
    if (mc->isDataObjectCached(dobj_id)) {
        oldObj = mc->getDataObjectFromId(dobj_id);
    }

    if (!dObj->isPersistent() && oldObj) {
        _deleteDataObject(dObj, false);
    }
    else if (oldObj) {
        HAGGLE_ERR("DataObject [%s] already in datastore\n", dobj_id.c_str());
        dObj->setDuplicate();
        dObj->setStored();
        if (callback) {
            kernel->addEvent(new Event(callback, dObj));
        }
        return 0;
    }
    
    mc->cacheDataObject(dObj);

    dataObjectsInserted++;
    
    evaluateFilters(dObj);

    if (!dObj->isPersistent()) {
        _deleteDataObject(dObj, false);
    }
    else {
    }

    Timeval now = Timeval::now();

    HAGGLE_DBG("Data object [%s] successfully inserted (duration: %ld us)\n", dobj_id.c_str(), (now-before).getMicroSeconds());

#if defined(BENCHMARK)
    BENCH_TRACE(BENCH_TYPE_DOBJ_INSERT_DELAY, (now-before).getMicroSeconds(), 0);
#endif


    if (callback) {
        kernel->addEvent(new Event(callback, dObj));
    }

    return 0;
}

/*
 * Retrieve a data object using its ID.
 */
int 
MemoryDataStore::_retrieveDataObject(
    const DataObjectId_t &id, 
    const EventCallback<EventHandler> *callback)
{
    if (!callback) {
        HAGGLE_ERR("No callback to retrieve data object\n");
        return -1;
    }

    string dobj_id = DataObject::idString(id);
    DataObjectRef dObj;
    if (mc->isDataObjectCached(dobj_id)) {
        dObj = mc->getDataObjectFromId(dobj_id);
    }

    kernel->addEvent(new Event(callback, dObj));
    return 0;
}

/*
 * Helper to delete a data object using its ID string.
 */
int
MemoryDataStore::_deleteDataObjectHelper(
    string id_str,
    bool shouldReportRemoval, 
    bool keepInBloomfilter)
{

    Timeval before = Timeval::now();
    Timeval now;

    if (!mc->isDataObjectCached(id_str)) {
        HAGGLE_ERR("DataObject: %s is not in cache\n", id_str.c_str());
        return -1;
    }

    DataObjectRef cachedObj = mc->getDataObjectFromId(id_str);
    
    if (shouldReportRemoval) {
        cachedObj->setStored(false);
        cachedObj->deleteData();
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, cachedObj, keepInBloomfilter));
        persistentDataObjectsDeleted++;
    }

    dataObjectsDeleted++;

    mc->unCacheDataObject(id_str);

    now = Timeval::now();
    HAGGLE_DBG("Data object %s deleted successfully (duration: %ld us)\n",
        id_str.c_str(), (now-before).getMicroSeconds());

#if defined(BENCHMARK)
    BENCH_TRACE(BENCH_TYPE_DOBJ_DELETE_DELAY, (now-before).getMicroSeconds(), 0);
#endif

    return 0;
}

/*
 * Delete a data object from DB using its ID. `shouldReportRemoval` will fire an event 
 * indicating the deletion.
 * `keepInBloomfilter` will keep the deleted data object in the bloomfilter.
 */
int 
MemoryDataStore::_deleteDataObject(
    const DataObjectId_t &id, 
    bool shouldReportRemoval, 
    bool keepInBloomfilter)
{
    return _deleteDataObjectHelper(DataObject::idString(id), shouldReportRemoval, keepInBloomfilter);
}

/*
 * Delete a data object from DB using a data object reference. 
 */
int 
MemoryDataStore::_deleteDataObject(
    DataObjectRef& dObj,
    bool shouldReportRemoval, 
    bool keepInBloomfilter)
{
    if (inMemoryNodeDescriptions && dObj->isNodeDescription()) {
        return 0;
    }
    return _deleteDataObjectHelper(DataObject::idString(dObj), shouldReportRemoval, keepInBloomfilter);
}

/*
 * Age all data objects from the database that are older than `minimumAge`
 * DEPRECATED: the utility caching framework obsoletes this mechanism, these
 * queries are VERY slow so we recommend NOT using this function. 
 */
int
MemoryDataStore::_ageDataObjects(
    const Timeval& minimumAge,
    const EventCallback<EventHandler> *callback,
    bool keepInBloomfilter)
{
    int ret = 0;
    DataObjectRefList dObjs = mc->getDataObjectsToAge(minimumAge.getTimeAsMilliSeconds() / 1000, DATASTORE_MAX_DATAOBJECTS_AGED_AT_ONCE);

    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++)  {
        DataObjectRef dObj = (*it);
        dObj->setStored(false);
        _deleteDataObject(dObj, false);
        persistentDataObjectsDeleted++;
    }

    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, dObjs, keepInBloomfilter));

    if (callback) {
        kernel->addEvent(new Event(callback, dObjs));
    }

    return ret;
}

/*
 * Insert a filter from a filter object.
 * If the filter is a monitor filter or a node description filter then we
 * treat them slightly differently for efficiency reasons.
 */
int
MemoryDataStore::_insertFilter(
    Filter *f,
    bool matchFilter, 
    const EventCallback<EventHandler> *callback)
{
    if (!f) {
        HAGGLE_ERR("Null filter\n");
        return -1;
    }

    long etype = f->getEventType();

    HAGGLE_DBG("Insert filter: %s (%ld)\n", f->getFilterDescription().c_str(), etype);

    if (etype < 0) {
        HAGGLE_ERR("Cannot insert negative event type\n");
        return -1;
    }

    bool insertIntoDB = true;

    // node description filter bypass
    string description = f->getFilterDescription();
    if (description == "NodeDescription:* ") {
        HAGGLE_DBG("Handling cached filter.\n");
        nd_filters.push_back(etype);
    }

    // monitor filter bypass
    if (description == "*:* ") {
        HAGGLE_DBG("Handling monitor filter.\n");
        monitor_filter_event_type = etype;
        insertIntoDB = false;
    }

    if (mc->isFilterCached(etype)) {
        HAGGLE_DBG("Filter exists, updating...\n");
        int ret = _deleteFilter(etype);
        if (ret < 0) {
            HAGGLE_ERR("Could not delete filter\n");
            return -1;
        }
    }

    const Attributes *attrs = f->getAttributes();

    bool is_empty = true;
    for (Attributes::const_iterator it = attrs->begin(); 
         insertIntoDB && it != attrs->end();
         it++) {
        const Attribute& a = (*it).second;
        mc->cacheFilter(etype, a.getName(), a.getValue(), a.getWeight());
        is_empty = false;
    }

    if (is_empty && insertIntoDB) {
        // insert a bogus filter since some apps later update this filter
        mc->cacheFilter(etype, "", "", 0);
    }

    if (callback) {
        kernel->addEvent(new Event(callback, f));
    }

    if (matchFilter) {
        evaluateDataObjects(etype);
    }

    return 0;
}

/*
 * Delete a filter using its ID.
 */
int
MemoryDataStore::_deleteFilter(
    long eventtype)
{
    bool wasCached = false;
    for (List<long>::iterator it = nd_filters.begin(); it != nd_filters.end(); it++) {
        if ((*it) == eventtype) {
            nd_filters.erase(it);
            wasCached = true;
        }
    }

    if (wasCached) {
        return 1;
    }

    if (eventtype == monitor_filter_event_type) {
        monitor_filter_event_type = -1;
        return 1;
    }

    if (!mc->isFilterCached(eventtype)) {
        HAGGLE_ERR("Filter is not cached for event type: %ld\n", eventtype);
        return -1;
    }

    int ret = mc->unCacheFilter(eventtype);
    if (ret < 0) {
        HAGGLE_ERR("Could not uncache filter for event type: %ld\n", eventtype);
        return -1;
    }

    return 0;
}

/*
 * Find all the data objects that match a particular filter, and call 
 * the callback with those data objects.
 */
int
MemoryDataStore::_doFilterQuery(DataStoreFilterQuery *q)
{
    if (!q) {
        HAGGLE_ERR("NULL filter query\n");
        return -1;
    }

    DataStoreQueryResult *qr = new DataStoreQueryResult();
    if (!qr) {
        HAGGLE_ERR("Could not allocate query result\n");
        return -1;
    }

    Filter *filter = (Filter *)q->getFilter();
    if (!filter) {
        HAGGLE_ERR("Could not get filter\n");
        return -1;
    }

    if (_insertFilter(filter) < 0) {
        HAGGLE_ERR("Could not insert filter\n");
        return -1;
    }

    long etype = filter->getEventType();

    DataObjectRefList dObjs = mc->getDataObjectsForFilter(etype);

    int num_match = 0;
    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        DataObjectRef dObj = (*it);
        if (!dObj) {
            HAGGLE_ERR("Filter retrieved NULL data object\n");
            continue;
        }
        qr->addDataObject(dObj);
        num_match++;
    }

    qr->setQuerySqlEndTime();

    if (num_match) {
        kernel->addEvent(new Event(q->getCallback(), qr));
    }
    else {
        delete qr;
    }

    return 0;
}

/*
 * Helper function to get the ID (or parent ID if fragmented / NC block).
 */
string 
MemoryDataStore::getIdOrParentId(DataObjectRef dObj)
{
    if (networkCodingConfiguration->isNetworkCodingEnabled(dObj, NULL)) {
        if (networkCodingDataObjectUtility->isNetworkCodedDataObject(dObj)) {
            return networkCodingDataObjectUtility->getOriginalDataObjectId(dObj); 
        }
    }

    if (fragmentationConfiguration->isFragmentationEnabled(dObj, NULL)) {
        if (fragmentationDataObjectUtility->isFragmentationDataObject(dObj)) {
            return fragmentationDataObjectUtility->getOriginalDataObjectId(dObj);
        }
    }

    return DataObject::idString(dObj);
}

/* 
 * Helper function to retrieve all the data objects that match a particular
 * node, where `delegate_node` will forward the data objects towards that node.
 * `max_matches` is the maximum number of matches for the node
 * `threshold` is the matching threshold 
 * `attrMatch` is the minimum number of matching attributes  (DEPRECATED)
 */
int
MemoryDataStore::_doDataObjectQueryStep2(
    NodeRef &node,
    NodeRef delegate_node,
    DataStoreQueryResult *qr,
    int max_matches,
    unsigned int threshold,
    unsigned int attrMatch)
{
    if (kernel->firstClassApplications() && node->isLocalApplication()) {
        max_matches = 0;
    }

    int num_match = 0;
    int num_match_dobjs = 0;

    Map<string, int> uniques;

    // NOTE: since MemoryCache does not perform bloom filter checks, grab everything
    // that matches and perform sort here
    DataObjectRefList dObjs = mc->getDataObjectsForNode(node, attrMatch, 0, threshold);

    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        DataObjectRef dObj = (*it);
        if (!dObj) {
            HAGGLE_ERR("Retrieved null data object\n");
            continue;
        }

        bool delegate_has_dataobject = delegate_node ? delegate_node->hasThisOrParentDataObject(dObj) : false;
        if (node->hasThisOrParentDataObject(dObj) || delegate_has_dataobject) {
            continue;
        }
        if (kernel->firstClassApplications() && !node->isLocalApplication() && node->getType() == Node::TYPE_APPLICATION && !node->hasProxyId(kernel->getThisNode()->getId())) {
            Node::Id_t proxyId;
            memcpy(proxyId, node->getProxyId(), NODE_ID_LEN); // careful to avoid locking due to reference lock proxies
            NodeRef proxy = kernel->getNodeStore()->retrieve(proxyId);
            if (proxy && proxy->has(dObj)) {
                continue;
            }
        }

        bool include = true;

        if (dObj->isNodeDescription()) {
            if (excludeNodeDescriptions) {
                continue;
            }

            NodeRef desc_node = Node::create(dObj);
            if (desc_node == node || (delegate_node && delegate_node == desc_node)) {
                continue;
            }
            if (!countNodeDescriptions) {
                include = false;
            }
        }

        HAGGLE_DBG("Data object [%s] retrieved from data base (node match)\n", DataObject::idString(dObj).c_str());

        string idOrParentId = getIdOrParentId(dObj);
        Map<string, int>::iterator itu = uniques.find(idOrParentId);
        if (itu != uniques.end()) {
            uniques.erase(itu);
            include = false;
            blocksSkipped++;
        }
        uniques.insert(make_pair(idOrParentId, 1));

        qr->addDataObject(dObj);
        qr->addNode(node);
        if (include) {
            num_match_dobjs++;
        }
        num_match++;

        if (max_matches != 0 && (num_match_dobjs >= max_matches)) {
            break;
        }
    }

    return num_match;
}
    
/*
 *  Find all the data objects that match a particular node.
 */
int 
MemoryDataStore::_doDataObjectQuery(
    DataStoreDataObjectQuery *q)
{
    NodeRef node = q->getNode();
    if (!node) {
        HAGGLE_ERR("Could not get node\n");
        return -1;
    }
    DataStoreQueryResult *qr = new DataStoreQueryResult();
    if (!qr) {
        HAGGLE_DBG("Could not allocate query result object\n");
        return -1;
    }

    qr->addNode(node);
    qr->setQuerySqlStartTime();
    qr->setQueryInitTime(q->getQueryInitTime());

    unsigned long maxDataObjectsInMatch = node->getMaxDataObjectsInMatch();
    unsigned long matchingThreshold = node->getMatchingThreshold();
    
    unsigned int num_match = _doDataObjectQueryStep2(
        node, 
        NULL, 
        qr, 
        maxDataObjectsInMatch, 
        matchingThreshold, 
        q->getAttrMatch());

    if (num_match == 0) {
        qr->setQuerySqlEndTime();
    }
    qr->setQueryResultTime();

#if defined(BENCHMARK)
    kernel->addEvent(new Event(q->getCallback(), qr));
#else 
    if (num_match) {
        kernel->addEvent(new Event(q->getCallback(), qr));
    }
    else {
        delete qr;
    }
#endif

    HAGGLE_DBG("%u data objects matched query\n", num_match);

    return num_match;
}

/*
 * Find all the data objects that match a set of nodes, we use the maximum
 * data objects to match and the matching threshold on the first node
 * in this set (typically the delegate).
 */
int
MemoryDataStore::_doDataObjectForNodesQuery(
    DataStoreDataObjectForNodesQuery *q)
{
    unsigned int num_match = 0;
    unsigned int total_match = 0;

    long num_left;
    bool has_maximum = false;
    NodeRef node = q->getNode();
    NodeRef delegateNode = node;
    unsigned int threshold = 0;

    HAGGLE_DBG("DataStore DataObject (for multiple nodes) Query for node=%s\n", 
        node->getIdStr());

    DataStoreQueryResult *qr = new DataStoreQueryResult();

    if (!qr) {
        HAGGLE_DBG("Could not allocate query result object\n");
        return -1;
    }

    qr->addNode(node);
    qr->setQuerySqlStartTime();
    qr->setQueryInitTime(q->getQueryInitTime());

    num_left = node->getMaxDataObjectsInMatch();
    if (num_left > 0) {
        has_maximum = true;
    }

    threshold = node->getMatchingThreshold();
    node = q->getNextNode();

    while (node && (!has_maximum || (num_left >= 0))) {
        HAGGLE_DBG("DataStore DataObject (for multiple nodes) Query for target=%s\n", node->getIdStr());
        num_match = _doDataObjectQueryStep2(node, delegateNode, qr, num_left, threshold, q->getAttrMatch());
        if (has_maximum) {
            num_left -= num_match;
        }
        total_match += num_match;
        node = q->getNextNode();
    }
    qr->setQuerySqlEndTime();
    qr->setQueryResultTime();

#if defined(BENCHMARK)
    kernel->addEvent(new Event(q->getCallback(), qr));
#else
    if (num_match) {
        kernel->addEvent(new Event(q->getCallback(), qr));
    }
    else {
        delete qr;
    }
#endif
    HAGGLE_DBG("%u data objects matched query\n", total_match);

    return num_match;
}

/*
 * Find all the nodes that match a particular data object. 
 */
int
MemoryDataStore::_doNodeQuery(
    DataStoreNodeQuery *q,
    bool localAppOnly)
{
    DataObjectRef dObj = q->getDataObject();
    if (!dObj) {
        HAGGLE_ERR("No data object in query\n");
        return -1;
    }

    string dobj_id = DataObject::idString(dObj);

    HAGGLE_DBG("Node query for data object [%s]\n", dobj_id.c_str());

    DataStoreQueryResult *qr = new DataStoreQueryResult();

    if (!qr) {
        HAGGLE_DBG("Could not allocate query result object\n");
        return -1;
    }

    qr->addDataObject(dObj);
    qr->setQuerySqlStartTime();
    qr->setQueryInitTime(q->getQueryInitTime());

    int maxResponse = q->getMaxResp() > 0 ? q->getMaxResp() : 0;
    int attrMatch = q->getAttrMatch();

    // support for in-memory node descriptions and no routing protocol:
    if (inMemoryNodeDescriptions && dObj->isNodeDescription()) {
        mc->cacheDataObject(dObj);
    }

    // get back all of the nodes, and apply max match constraint later
    // (MemoryCache is unaware of bloomfilters)
    NodeRefList nodes = mc->getNodesForDataObject(dObj, attrMatch, 0);

    if (inMemoryNodeDescriptions && dObj->isNodeDescription()) {
        mc->unCacheDataObject(dobj_id);
    }

    int num_match = 0;
    for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
        if (num_match == 0) {
            qr->setQuerySqlEndTime();
        }
        NodeRef node = (*it);
        if (!node) {
            HAGGLE_ERR("Retrieved empty node\n");
            continue;
        }
        if (localAppOnly && !node->isLocalApplication()) {
            continue;
        }
        if (node->getType() == Node::TYPE_PEER || node->getType() == Node::TYPE_GATEWAY || node->getType() == Node::TYPE_APPLICATION) {
            qr->addNode(node);
            num_match++;
        }

        if ((maxResponse > 0) && (num_match >= maxResponse)) {
            break;
        }
    }

    if (num_match == 0) {
        qr->setQuerySqlEndTime();
    }

#if defined(BENCHMARK)
    kernel->addEvent(new Event(q->getCallback(), qr));
#else
    if (num_match) {
        kernel->addEvent(new Event(q->getCallback(), qr));
    } else {
        delete qr;
    }
#endif
	
    HAGGLE_DBG("%u nodes matched data object [%s]\n", num_match, dobj_id.c_str());
	
    return num_match;
}

/*
 * Store an entry into the database. This is typically used to persist
 * information to disk.
 */
int
MemoryDataStore::_insertRepository(
    DataStoreRepositoryQuery *q)
{
    const RepositoryEntryRef query = q->getQuery();

    HAGGLE_DBG("Inserting repository \'%s\' : \'%s\'\n", query->getAuthority(), query->getKey() ? query->getKey() : "-");

    string auth = query->getAuthority();
    string key = query->getKey();

   int ret = mc->insertRepository(query);
   if (ret < 0) {
       HAGGLE_ERR("Problems inserting repository\n");
       return -1;
   }
    
    return 0;
}

/*
 * Read information from disk.
 */
int
MemoryDataStore::_readRepository(
    DataStoreRepositoryQuery *q, 
    const EventCallback<EventHandler> *callback)
{	
    const RepositoryEntryRef query = q->getQuery();
    DataStoreQueryResult *qr = new DataStoreQueryResult();
	
    HAGGLE_DBG("Reading repository \'%s\' : \'%s\'\n", query->getAuthority(), query->getKey() ? query->getKey() : "-");

    if (!qr) {
        HAGGLE_ERR("Could not allocate query result object\n");
        return -1; 
    }
	
    if (!query->getAuthority()) {
        HAGGLE_ERR("Error: No authority in repository entry\n");
        return -1;
    }

    RepositoryEntryList rl = mc->readRepository(query->getAuthority(), query->getKey(), query->getId());

    for (RepositoryEntryList::iterator it = rl.begin(); it != rl.end(); it++) {
        qr->addRepositoryEntry(*it);
    }

    kernel->addEvent(new Event(q->getCallback(), qr));

    return 1;
}

/*
 * Delete information from the repository (don't persist to disk)
 */
int
MemoryDataStore::_deleteRepository(
    DataStoreRepositoryQuery *q)
{
    const RepositoryEntryRef query = q->getQuery();

    int ret = mc->deleteRepository(query->getAuthority(), query->getKey(), query->getId());
    if (ret < 0) {
        HAGGLE_ERR("Problems deleting from repository\n");
        return -1;
    }

    return 1;
}

/*
 * Dump the databse to XML format in memory (passed in callback).
 */
int
MemoryDataStore::_dump(
    const EventCallback<EventHandler> *callback)
{
    //TODO
    HAGGLE_ERR("NOT IMPLEMENTED\n");
    return 0;
}

/*
 * Dump the database in XML format to a file.
 */
int
MemoryDataStore::_dumpToFile(
    const char *filename)
{
    //TODO
    HAGGLE_ERR("NOT IMPLEMENTED\n");
    return 0;
}

/*
 * Botique query for TotalOrderReplacement function in Utility Caching.
 * Finds all the data objects that have a particular set of attributes.
 */
int
MemoryDataStore::_doDataObjectQueryForReplacementTotalOrder(
    DataStoreReplacementTotalOrderQuery *q)
{
    if (!q || !q->getCallback()) {
        HAGGLE_ERR("No callback specified\n");
        return -1;
    }

    string tagFieldName = q->getTagFieldName();
    string tagFieldValue = q->getTagFieldValue();
    string metricFieldName = q->getMetricFieldName();
    string idFieldName = q->getIdFieldName();
    string idFieldValue = q->getIdFieldValue();

    long tempFilterId = mc->getTemporaryFilterId();
    mc->setFilterSettings(tempFilterId, 100);
    mc->cacheFilter(tempFilterId, tagFieldName, tagFieldValue, 1);
    mc->cacheFilter(tempFilterId, metricFieldName, "*", 1);
    mc->cacheFilter(tempFilterId, idFieldName, idFieldValue, 1);
    DataObjectRefList dObjs = mc->getDataObjectsForFilter(tempFilterId); 
    mc->unCacheFilter(tempFilterId);

    DataStoreReplacementTotalOrderQueryResult *qr = new DataStoreReplacementTotalOrderQueryResult();

    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        DataObjectRef dObj = (*it);
        const Attribute *attr = dObj->getAttribute(metricFieldName);
        if (!attr) {
            HAGGLE_ERR("Missing metric field\n");
            continue;
        }
        unsigned long metric = atol(attr->getValue().c_str());
        qr->addDataObject(dObj, metric);
    }

    kernel->addEvent(new Event(q->getCallback(), qr));
    return 0;
}

/*
 * Botique query for TimedDelete function in utility caching. 
 * Finds all the data objects that have a partiuclar set of attributes.
 */
int
MemoryDataStore::_doDataObjectQueryForReplacementTimedDelete(
    DataStoreReplacementTimedDeleteQuery *q)
{
    if (!q || !q->getCallback()) {
        HAGGLE_ERR("No callback specified\n");
        return -1;
    }

    string tagFieldName = q->getTagFieldName();
    string tagFieldValue = q->getTagFieldValue();

    long tempFilterId = mc->getTemporaryFilterId();
    mc->cacheFilter(tempFilterId, tagFieldName, tagFieldValue);
    DataObjectRefList dObjs = mc->getDataObjectsForFilter(tempFilterId); 
    mc->unCacheFilter(tempFilterId);

    kernel->addEvent(new Event(q->getCallback(), dObjs));
    return 0;
}

/**
 * Unit test to save/load from disk. 
 */
bool
MemoryDataStore::runSaveLoadTest()
{
    MemoryDataStore *md = new MemoryDataStore(false, "/tmp/test.db");
    if (!md) {
        HAGGLE_ERR("Could not allocate memory data store.\n");
        return false;
    }

    RepositoryEntryRef re = new RepositoryEntry("key", "val");

    DataStoreRepositoryQuery *q = new DataStoreRepositoryQuery(re);
    if (!q) {
        HAGGLE_ERR("Could not allocate repository query.\n");
        return false;
    }

    if (0 != md->_insertRepository(q)) {
        HAGGLE_ERR("Could not insert into repository.\n");
        return false;
    }

    NodeRef fakeNode = BenchmarkManager::createNode(1, 1);

    if (!fakeNode) {
        HAGGLE_ERR("Could not create fake node.\n");
        return false;
    }

    if (1 != md->_insertNode(fakeNode)) {
        HAGGLE_ERR("Could not insert fake node.\n");
        return false;
    }

    DataObjectRef fakeDataObject = BenchmarkManager::createDataObject(1, 1);

    if (!fakeDataObject) {
        HAGGLE_ERR("Could not create fake data object.\n");
        return false;
    }

    if (0 != md->_insertDataObject(fakeDataObject)) {
        HAGGLE_ERR("Could not insert data object.\n");
        return false;
    }

    delete q;

    md->_exit();

    delete md;

    md = new MemoryDataStore(true, "/tmp/test.db");

    if (!md || !md->init()) {
        HAGGLE_ERR("Could not init new MemoryDataStore\n");
        return false;
    }

    DataObjectRefList dobjs = md->_MD_getAllDataObjects();
    if (dobjs.size() != 1) {
        HAGGLE_ERR("Wrong data object list size: got %d != 1\n", dobjs.size());
        return false;
    }

    NodeRefList nodes = md->_MD_getAllNodes();
    if (nodes.size() != 1) {
        HAGGLE_ERR("Wrong node list size, got %d != 1\n", nodes.size());
        return false;
    }

    RepositoryEntryList rel = md->_MD_getAllRepositoryEntries();
    if (rel.size() != 1) {
        HAGGLE_ERR("Wrong repository size, got: %d, expected 1\n", rel.size());
        return false;
    }

    RepositoryEntryRef re_new = *(rel.begin());

    if (re == re_new) {
        HAGGLE_ERR("Repository entry was not saved correctly\n");
        return false;
    }

    delete md;

    return true;
}


/**
 * Helper function for unit tests to retrieve all data objects in the database.
 * @return List of all data objects in the data base.
 */
DataObjectRefList 
MemoryDataStore::_MD_getAllDataObjects()
{
    return mc->getAllDataObjects();
}

/**
 * Helper function for unit tests to retrieve all nodes in the database..
 * @return List of all nodes in the database.
 */
NodeRefList 
MemoryDataStore::_MD_getAllNodes()
{
    return mc->getAllNodes();
}

/**
 * Helper function for unit tests to retrieve all repository entries
 * from the data base.
 */
RepositoryEntryList 
MemoryDataStore::_MD_getAllRepositoryEntries()
{
    return mc->getAllRepository();
}


/*
 * Read configuration parameters for database from config.xml
 */
void
MemoryDataStore::_onDataStoreConfig(
    Metadata *m )
{
    if ((NULL == m) || (0 != strcmp(getName(), m->getName().c_str()))) {
        return;
    }

    HAGGLE_DBG("MemoryDataStore configuration.\n");

    const char *param = m->getParameter("count_node_descriptions");

    if (param) {
        if (0 == (strcmp(param, "true"))) {
            countNodeDescriptions = true;
        } else if (0 == (strcmp(param, "false"))) {
            countNodeDescriptions = false;
        } else {
            HAGGLE_ERR("Unknown parameter for count_node_descriptions, need `true` or `false`.\n");    
        }
    }

    param = m->getParameter("shutdown_save");

    if (param) {
        if (0 == (strcmp(param, "true"))) {
            shutdownSave = true;
        } else if (0 == (strcmp(param, "false"))) {
            shutdownSave = false;
        } else {
            HAGGLE_ERR("Unknown parameter for shutdown_save, need `true` or `false`.\n");    
        }
    }

    param = m->getParameter("exclude_node_descriptions");

    if (param) {
        if (0 == (strcmp(param, "true"))) {
            excludeNodeDescriptions = true;
        } else if (0 == (strcmp(param, "false"))) {
            excludeNodeDescriptions = false;
        } else {
            HAGGLE_ERR("Unknown parameter for exclude_node_descriptions, need `true` or `false`.\n");           }
    }

    param = m->getParameter("in_memory_node_descriptions");

    if (param) {
        if (0 == (strcmp(param, "true"))) {
            inMemoryNodeDescriptions = true;
        } else if (0 == (strcmp(param, "false"))) {
            inMemoryNodeDescriptions = false;
        } else {
            HAGGLE_ERR("Unknown parameter for in_memory_node_descriptions, need `true` or `false`.\n");    
        }
    }

    param = m->getParameter("exclude_zero_weight_attributes");

    if (param) {
        bool excludeZeroWeightAttributes = false;
        if (0 == (strcmp(param, "true"))) {
          excludeZeroWeightAttributes = true;
        } else if (0 == (strcmp(param, "false"))) {
          excludeZeroWeightAttributes = false;
        } else {
          HAGGLE_ERR("Unknown parameter for exclude_zero_weight_attributes, need `true` or `false`.\n");
        }
        mc->setExcludeZeroWeightAttributes(excludeZeroWeightAttributes);
    }

    param = m->getParameter("enable_compaction");

    if (param) {
        bool enableCompaction = false;
        if (0 == (strcmp(param, "true"))) {
            enableCompaction = true;
        } else if (0 == (strcmp(param, "false"))) {
            enableCompaction = false;
        } else {
            HAGGLE_ERR("Unknown parameter for enable_compaction, need `true` or `false`.\n");
        }
        mc->setCompaction(enableCompaction);
    }

    param = m->getParameter("max_nodes_to_match");

    if (param) {
        char *endptr = NULL;
        int maxNodesToMatch = (int)strtoul(param, &endptr, 10);
			
        if (endptr && endptr != param && maxNodesToMatch >= 0) {
            HAGGLE_DBG("set max nodes to match: %d\n", maxNodesToMatch);
            mc->setMaxNodesToMatch(maxNodesToMatch);
        }
    }

    param = m->getParameter("run_self_tests");
    bool runSelfTests = false;

    if (param) {
        if (0 == (strcmp(param, "true"))) {
            runSelfTests = true;
        } else if (0 == (strcmp(param, "false"))) {
            runSelfTests = false;
        } else {
            HAGGLE_ERR("Unknown parameter for run_self_tests, need `true` or `false`.\n");           }
    }

    if (runSelfTests) {
        bool success = true;
        if (!MemoryCache::runSelfTests()) {
            HAGGLE_ERR("An error occurred running MemoryCache tests.\n");
            success = false;
        }
        else if (!MemoryDataStore::runSaveLoadTest()) {
            HAGGLE_ERR("An error occurred in save load test.\n");
            success = false;
        }
        HAGGLE_STAT("Summary Statistics - Memory Data Store - Unit tests %s\n", success ? "PASSED" : "FAILED");
    }

    HAGGLE_DBG("Loaded memory data store, in memory node descriptions: %s\n", inMemoryNodeDescriptions ? "enabled" : "disabled");
}
