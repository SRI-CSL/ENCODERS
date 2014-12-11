/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _MEMORYDATASTORE_H
#define _MEMORYDATASTORE_H

/**
 * @file MemoryDataStore.h
 *
 * An in-memory DataStore to replace the SQL DataStore that is optimized for
 * queries with a few attributes (~2) that match a few data objects (~10), with
 * a large number of data objects (~30,000). 
 *
 * We do this by using hash tables to index the queries, and storing 
 * everything in-memory. 
 * A 30k data base with 150 nodes consumes roughly ~200MB of memory. 
 * 
 * The original SQL matching is un-optimized for the kidnds of queries that
 * the Drexel code will issue (single matching data objects identified by
 * cuid, for example). 
 *
 * To support persistent data objects (saved / loaded on shutdown/startup),
 * we dump the data base to a SQLDataStore which is saved on to disk.
 * This can be a very slow process for databases with 1000s of data objects.
 *
 * To enable MemoryDataStore, please start haggle with the "-m" option and
 * include options in config.xml, i.e.:
 *
 *     <DataManager ...>
 *               ...
 *              <DataStore>
 *                      <MemoryDataStore run_self_tests="false" in_memory_node_descriptions="true" shutdown_save="false" />
 *              </DataStore>
 *     </DataManager>
 *
 *
 * There are 3 main objects for matching in Haggle: 
 * 1. data objects
 * 2. node descriptions
 * 3. filters
 * 
 * Node descriptions have interests which are (key, val, weight) triples,
 * along with a matching threshold and a maximum number of matches.
 * They have an underlying data object that encapsulates them. 
 *
 * Data objects have (key, val) tuples.

 * Data objects and node descriptions are propgated throughout the network.
 *
 * Filters are similar to node descriptions, but are used internally for
 * managers to listen to specific events (i.e. data object insertions with
 * a certain attribute). 
 *
 * Matching 
 * ----
 * Matching occurs as follows:
 *
 * Node description to data objects: find all the data objects that a node
 * is interested in. Usually, the matched data objects do NOT include
 * node description data objects, since these are propagated using a different
 * mechanism that does not use matching. 
 *
 * Data object to node descriptions: find all the nodes that are interested
 * in a particular data object. Usually, the data object is NOT a node
 * description data object, since these are propagated using a different
 * mechanism that does not use matching.
 *
 * Data object to filter: find all the filters that are interested in a 
 * data object. These MAY include node description data objects since
 * internal managers will be interested in these events.
 *
 * Filter to data objects: find all the data objects that a particular 
 * filter is interested in.
 *
 * Configuration options
 * -----
 *
 * - *count_node_descriptions* = *true* or *false*
 *
 *   Count node descriptions when matching against *max_matches* parameter.
 *   Default is *false*.
 *
 * - *shutdown_save* = *true* or *false*
 * 
 *   Persist the DB to disk during shutdown. Default is *true*.
 *
 * - *exclude_node_descriptions* = *true* or *false*
 *
 *   Remove node descriptions data objects from result set when finding all data 
 *   objects for a particular node description. 
 *   Default is *false*.
 *
 * - *in_memory_node_descriptions* = *true* or *false*
 *
 *   Set to *true* to NOT keep node description data objects in the database 
 *   (implies exclude_node_descriptions="true").  
 *   Default is *true*.
 *
 * - *exclude_zero_weight_attributes* = *true* or *false* 
 *
 *   Do not use attributes with weight 0 in matching. 
 *   Default is *false*.
 *
 * - *max_nodes_to_match* = 30 
 *   
 *   Maximum number of nodes to match for a particular data object.
 *   Default is MEMORY_DATA_STORE_MAX_NODE_MATCH (in this file)
 *
 * - *enable_compaction* = *true* or *false*
 *
 *   Shrink the data base upon sufficient deletions of nodes and data objects. 
 *   NOTE: compaction can be slow!
 *   Default is *false*.
 *
 * - *run_self_tests* = *true* or *false* 
 *   
 *   Run unit tests and report success/failure.
 *   Default is *false*.
 *
 * NOTE: The following are features supported in SQLDataStore but NOT
 * in this DataStore:
 *
 *  - *_dump*
 * 
 *    Used to dump the database as XML to memory or a file.
 *    This is only called by the DebugManager if DEBUG_DATASTORE is enabled.
 */

#define MEMORY_DATA_STORE_MAX_NODE_MATCH 30  // maximum number of nodes to match for a dataobject 

class MemoryDataStore;

#include <libcpphaggle/Platform.h>
#include "DataStore.h"
#include "Node.h"

#include "DataObject.h"
#include "Metadata.h"

#include "MemoryCache.h"
#include "HaggleKernel.h"

/**
 * An in-memory DataStore that is optimized for exact match queries on a small
 * number of attributes.  
 *
 * This class stores all of its state in the MemoryCache, which uses hash
 * tables for fast lookup. 
 */
class MemoryDataStore : public DataStore
{
private:
    MemoryCache *mc; /**< State of in-memory data base (stores hash tables). */
    string filepath; /**< Where on disk to store files. */
    bool recreate;   /**< Flag to create database from fresh and ignore existing file. */
    bool inMemoryNodeDescriptions; /** Flag to store node description data objects in the database. */
    bool shutdownSave; /**< Flag to save database to SQL on disk during shutdown. */
    long monitor_filter_event_type; /**< Event type used for monitor manager to receive all events. */
    List<long> nd_filters; /**< Filters for listening to node description events. */

    // statistics:

    long dataObjectsInserted; /**< The number of data objects inserted. */
    long dataObjectsDeleted; /**< The number of data objects deleted. */
    long persistentDataObjectsDeleted; /**< The number of persistent data objects deleted. */
    long blocksSkipped; /**< The number of network encoded blocks skipped when matching due to sharing a parent data object. */

    bool excludeNodeDescriptions; /**< Flag to not include node descriptions for node queries. */
    bool countNodeDescriptions; /**< Flag to count node descriptions in max matches. */

    /**
     * Fire all the filter events that are interested in a particular data object. 
     * @return -1 on failure, 0 on success.
     */
    int evaluateFilters(const DataObjectRef dObj /**< [in] The data object to find and fire matching filters for. */); 

    /**
     * Find all the data objects that match a filter and fire the event with
     * these data objects.
     * @return -1 on failure, 0 on success. 
     */
    int evaluateDataObjects(long eventType /**< [in] The filter id to find matching data objects for. */); 

    /**
     * A helper function to delete a data object using its ID.
     * @return -1 on failure, 0 on success.
     */ 
    int _deleteDataObjectHelper(
        string id_str, /**< [in] The ID of the data object to delete from the database. */
        bool shouldReportRemoval,  /**< [in] Flag to enable firing the DATAOBJECT_DELETED event for the deleted data object. */
        bool keepInBloomfilter /**< [in] Flag to keep deleted data objects in the bloomfilter. Should only be used when `shouldReportRemoval` is `true`. */); 

    /**
     * A helper function to retrieve all the data objects that match a particular 
     * node.
     * @return -1 on failure, 0 on success. 
     */
    int _doDataObjectQueryStep2(
        NodeRef &node, /**< [in] The node whose matching data objects we are querying. */
        NodeRef delegate_node,  /**< [in] The delegate node whom we are forwarding the matching data objects through. */
        DataStoreQueryResult *qr, /**< [out] The query result that this function appends results to. */
        int max_matches, /**< [in] The maximum number of matches. Local applications have no limit. */
        unsigned int threshold,  /**< [in] The matching threshold. */
        unsigned int attrMatch /**< [in] The minimum number of attribute that need to match. */); 

    /**
     * Call the monitor filter event with the passed data object.
     * The monitor filter listens for all data objects.
     */
    void _evaluateMonitorFilter(DataObjectRef dObj /**< [in] The data object to call the monitor filter with. */); 

    /**
     * Delete obsolete node description data objects.
     * 
     * This function is only useful when in memory node descriptions are enabled.
     * @return -1 on failure, 0 if the node description of `dObj` is not the newest, and 1 if it is the newest. 
     */
    int deleteDataObjectNodeDescriptions(
        DataObjectRef dobj /**< [in] The node description data object. */, 
        string& node_id /**< [in] The ID of the node description data object. */, 
        bool reportRemoval /**< [in] Flag to enable raising the DATAOBJECT_DELETE event. */); 

protected:
    /**
     * Insert a node into the database and call callback. Optionally merge 
     * The new node's bloomfilter with the existing node's bloomfilter. 
     * @return -1 on failure, 1 on success. 
     */
    int _insertNode(
        NodeRef& node, /**< [in] The node to be inserted. */
        const EventCallback<EventHandler> *callback = NULL,  /**< [in] The callback to be called upon insertion. */
        bool mergeBloomfilter = false /**< [in] Flag to enable merging bloomfilter of new node and old node. */); 

    /**
     * Delete a node from the database. 
     * @return -1 on failure, 0 on success. 
     */
    int _deleteNode(NodeRef& node /**< [in] The node to remove from the database. */);

    /**
     * Retrieve a node from the database, given a partial node. 
     * @return -1 on failure, 0 on success. 
     */
    int _retrieveNode(
        NodeRef& node,  /**< [in] Partial node to retrieve from the database. */
        const EventCallback<EventHandler> *callback, /**< [in] Callback that is called with the retrieved node. */ 
        bool forceCallback /**< [in] Flag if set to `true` then the interfaces will be merged (needed by the ApplicationManager) AND the callback will be called.*/);

    /**
     * Retrieve all of the nodes that have a specific type.
     * @return -1 on failure, 0 on success. 
     */
    int _retrieveNode(
        Node::Type_t type, /**< [in] Node type of retrieved nodes. */
        const EventCallback<EventHandler> *callback /**< [in] Callback to pass retrieved nodes too. */ ); 

    /**
     * Retrieve the node with a specific interface.
     * @return -1 on failure, 0 on success. 
     */
    int _retrieveNode(
        const InterfaceRef& iface, /**< [in] The interface of the node to retrieve. */
        const EventCallback<EventHandler> *callback,  /**< [in] The callback to call with the retrieved node. */
        bool forceCallback /**< [in] Flag to force the callback when no matching node was found. */);

    /**
     * Insert a data object into the database.
     * @return -1 on failure, 0 on success. 
     */
    int _insertDataObject(
        DataObjectRef& dObj, /**< [in] The data object to insert. */
        const EventCallback<EventHandler> *callback = NULL,  /**< [in] The callback to call upon successful insertion. */
        bool reportRemoval = true /**< [in] Flag to report removal if the insertion triggered a removal (i.e. an insertion of a new node description that subsumes an old node description. */); 

    /**
     * Retrieve a data object given an ID. 
     * @return -1 on failure, 0 on success. 
     */
    int _retrieveDataObject(
        const DataObjectId_t &id, /**< [in] The ID of the data object to retrieve. */
        const EventCallback<EventHandler> *callback = NULL /**< [in] The callback to call upon retrieval. */); 

    /**
     * Delete a data object from the database given an ID.
     * @return -1 on failure, 0 on success. 
     */
    int _deleteDataObject(
        const DataObjectId_t &id, /**< [in] The ID of the data object to delete. */
        bool shouldReportRemoval = true,  /**< [in] Flag to report the removal via the DATAOBJECT_DELETED event. */
        bool keepInBloomfilter = false /**< [in] Flag that when set to `true` will keep the deleted data object in the bloomfilter, and remove it otherwise. */); 

    /**
     * Delete a data object from the database given the data object.
     * @return -1 on failure, 0 on success. 
     */
    int _deleteDataObject(
        DataObjectRef& dObj, /**< [in] The data object to be deleted. */
        bool shouldReportRemoval = true,  /**< [in] Flag that when set to `true` will report the removal via the DATAOBJECT_DELETED event. */
        bool keepInBloomfilter = false /**< [in] Flag that when set to `true` will keep the deleted data object in the bloomfilter, and remove it otherwise. */); 

    /**
     * Delete all the data objects that have no interest and are past a certain age.
     * @return -1 on failure, 0 on success. 
     */
    int _ageDataObjects(
        const Timeval& minimumAge, /**< [in] The age after which data objects will be purged. */
        const EventCallback<EventHandler> *callback = NULL, /**< [in] Call the callback with the purged data objects. */
        bool keepInBloomfilter = false /**< [in] Flag that when set to `true` will keep the deleted data object in the bloomfilter, and remove it otherwise. */); 

    /**
     * Insert a filter into the database to match on certain data objects. 
     * @return -1 on failure, 0 on success. 
     */
    int _insertFilter(
        Filter *f, /**< [in] Filter to insert into the database. */
        bool matchFilter = false,  /**< [in] Flag that when set to `true` will evaluate the filter upon insertion. */
        const EventCallback<EventHandler> *callback = NULL /**< [in] Callback to fire upon insertion of the filter. */); 
    
    /**
     * Delete a filter from the database given its event ID. 
     * @return -1 on failure, 0 on success. 
     */
    int _deleteFilter(long eventtype /**< [in] ID of the filter to delete. */); 

    // main query functions:

    /**
     * Find all the data objects that match a filter and fire a callback.
     * @return -1 on failure, 0 on success. 
     */
    int _doFilterQuery(DataStoreFilterQuery *q /**< [in] Object that contains the callback and result set to be returned. */); 

    /**
     * Find all the data objects that match a particular node, and fire a callback.
     * @see _doDataObjectForNodesQuery()
     * @return -1 on failure, 0 on success. 
     */
    int _doDataObjectQuery(DataStoreDataObjectQuery *q /**< [in] Object that contains the callback and the result set to be returned. */); 

    /**
     * Find all the data objects that match a set of nodes, and fire a callback.
     * @see _doDataObjectForNodesQuery()
     * @return -1 on failure, 0 on success. 
     */
    int _doDataObjectForNodesQuery(DataStoreDataObjectForNodesQuery *q /**< [in] Object that contains the callback and the result set to be returned. */); 

    /**
     * Find all of the nodes that match a particular data object, and fire a callback.
     * @return -1 on failure, 0 on success. 
     */
    int _doNodeQuery(
        DataStoreNodeQuery *q, /**< [in] Object that contains the callback and the result set to be returned. */
        bool localAppOnly /**< [in] Flag that when set to `true` will only match local application nodes. */); 

    /**
     * Insert a repository entry into the database for persistent storage.
     * @return -1 on failure, 0 on success. 
     */
    int _insertRepository(DataStoreRepositoryQuery *q /**< [in] Object that contains the repository entry to inserted. */); 

    /**
     * Read a repository entry from the database. 
     * @return -1 on failure, 0 on success. 
     */
    int _readRepository(
        DataStoreRepositoryQuery *q, /**< [in] Object containing the repository entry information to find in the database. */
        const EventCallback<EventHandler> *callback = NULL /**< [in] Callback to fire upon finding the repository entry. */);

    /**
     * Delete a repository entry from the database.
     * @return -1 on failure, 0 on success. 
     */
    int _deleteRepository(DataStoreRepositoryQuery *q /**< [in] Object containing the repository entry information to delete from the database. */);

    /**
     * Generate a XML representation of the database and store it in a callback that is fired.
     *
     * NOT CURRENTLY IMPLEMENTED!
     * @return -1 on failure, 0 on success. 
     */
    int _dump(const EventCallback<EventHandler> *callback = NULL); 

    /**
     * Generate a XML representation of the database and dump it to a file.
     *
     * NOT CURRENTLY IMPLEMENTED!
     * @return -1 on failure, 0 on success. 
     */
    int _dumpToFile(const char *filename /**< [in] Name of file to dump the XML. */); 

    /**
     * Retrieve all the data objects for the TotalOrderReplacement caching query.
     * @return -1 on failure, 0 on success. 
     */
    int _doDataObjectQueryForReplacementTotalOrder(DataStoreReplacementTotalOrderQuery *q /**< [in] Object containing the query for the total order replacement, as well as the callback. */);

    /**
     * Retrieve all of the data objects for the TimedDelete caching query.
     * @return -1 on failure, 0 on success. 
     */
    int _doDataObjectQueryForReplacementTimedDelete(DataStoreReplacementTimedDeleteQuery *q /**< [in] Object containing the query for the timed delete, as well as the callback. */); 

    /**
     * Handler that is called during initialization with the configuration information.
     */
    void _onDataStoreConfig(Metadata *m = NULL /**< [in] Contains configuration parameters that are read to configure this DataStore. */); 

    /**
     * Handler that is called during shutdown. Saves state to disk (if enabled) 
     * and cleans up data structures.
     */
    void _exit(); 

    /**
     * Unit test to load/save from disk.
     * @returns `true` on test PASS, `false` otherwise.
     */
    static bool runSaveLoadTest();

    /**
     * Helper function for unit tests to retrieve all data objects in the database.
     * @return List of all data objects in the data base.
     */
    DataObjectRefList _MD_getAllDataObjects();

    /**
     * Helper function for unit tests to retrieve all nodes in the database..
     * @return List of all nodes in the database.
     */
    NodeRefList _MD_getAllNodes();

    /**
     * Helper function for unit tests to retrieve all repository entries
     * from the data base.
     */
    RepositoryEntryList _MD_getAllRepositoryEntries();

public:

    /**
     * A helper function get the ID or the parent ID (if network encoded block) 
     * of a data object. 
     * @return The ID of the data object, or of its parent if the data object is a network encoded block.
     */
    string getIdOrParentId(DataObjectRef dObj /**< [in] The data object to get the ID from. */); 

    /**
     * Construct a new MemoryDataStore.
     */
    MemoryDataStore(
        bool _recreate = false, /**< [in] Flag that when set to `true` will reconstruct the data base from scratch. */
        const string = DEFAULT_DATASTORE_FILEPATH, /**< [in] File path for persistent database (if enabled). Used to save to or load from. */
        const string name = "MemoryDataStore" /**< [in] Class name used for debugging and logging. */);

    /**
     * Cleanup and free any data structures used by the DataStore.
     */
    ~MemoryDataStore();

    /**
     * Called during start-up, initializes the class.
     * @return `true` if initialization was sucessful, `false` otherwise.
     */
    bool init();
};

#endif /* _MEMORYDATASTORE_H */
