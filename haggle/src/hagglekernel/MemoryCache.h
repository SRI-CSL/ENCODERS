/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _MEMORYCACHE_H
#define _MEMORYCACHE_H

class AbstractNode;
class AbstractDataObject;
class MemoryCache;

#include "Node.h"
#include "DataObject.h"
#include "RepositoryEntry.h"
#include <libcpphaggle/Reference.h>

/**
 * @file MemoryCache.h
 * 
 * An in-memory data structure composed of hash maps and lists for fast 
 * indexing of haggle queries. 
 *
 * This file defines a class that is responsible for storing all the database state and
 * performing the queries. 
 */

/**
 * Node wrapper to make unit testing easier, either it can wrap a real
 * node or abstract a fake node.
 *
 * This class avoids the need to instantiate a full node data object during testing,
 * it abstracts a node.
 */
class AbstractNode
{
private:

    /** 
     * A struct for fake node data, used during unit testing.
     */
    typedef struct {
        string id; /**< Fake node id. */
        List<string> interfaces; /**< Fake interface ids. */
        List<Pair<string, int> > interests; /**< Fake interest, weight. */
        int maxDataObjectsInMatch; /**< Fake maximum data objects in match. */
        int matchThreshold; /**< Fake match threshold. */
        Node::Type_t type; /**< Fake node type. */
    } fake_node_t; 

    NodeRef node; /**< The underlying node (not a fake node). */
    fake_node_t *fake_node; /**< Fake node state for unit testing. */

public:
    /** 
     * Construct a node abstraction from an existing node.
     */
    AbstractNode(NodeRef *_node = NULL /**< The wrapped real node. */) : node(_node == NULL ? NodeRef(NULL) : *_node), fake_node(NULL) {};
    /**
     * Construct a fake node with a specific id.
     */
    AbstractNode(const string &_id /**< The id for the fake node.*/) : node(NodeRef(NULL)), fake_node(new fake_node_t()) { fake_node->id = _id; };
    /** 
     * Deconstruct an abstract node.
     */
    ~AbstractNode() { if (fake_node) delete fake_node; }
    /** 
     * Getter fort he maximum number of data objects per match.
     * @return The maximum number of data objects per match for the node.
     */
    int getMaxDataObjectsInMatch() { return node ? node->getMaxDataObjectsInMatch() : fake_node ? fake_node->maxDataObjectsInMatch : 0; };
    /**
     * Getter for the node's matching threshold.
     * @return The matching threshold.
     */
    int getMatchThreshold() { return node ? node->getMatchingThreshold() : fake_node ? fake_node->matchThreshold : 0; };
    /** 
     * Getter for the node's type.
     * @return The node type.
     */
    Node::Type_t getType() { return node ? node->getType() : fake_node ? fake_node->type : Node::TYPE_UNDEFINED; };
    /**
     * Getter for the node's ID.
     * @return The node's ID.
     */
    string getId() { return node ? Node::idString(node) : fake_node ? fake_node->id : ""; };
    /** 
     * Getter for the underling node (NULL if fake).
     * @return The underlying node.
     */
    NodeRef getNode() { return node; };
    /** 
     * Getter for the underlying data object (NULL if fake).
     * @return The data object for the node.
     */
    DataObjectRef getDataObject() { return node ? node->getDataObject() : DataObjectRef(NULL); };
    /**
     * Get a list of interface IDs associated with the node.
     */
    void getInterfaces(List<string> *interfaces /**< [out] The list of interface ids. */);
    /**
     * Get a list of <interest, weight> pairs, where interest is of the form key=value. 
     */
    void getInterests(List<Pair <string, int> > *interests /**< [out] The list of interest,weight pairs. */);
    /**
     * Set the maximum data objects in the match.  Only used with fake nodes.
     */
    void setMaxDataObjectsInMatch(int _maxDataObjectsInMatch /**< [in] Maximum data objects in the match. */) { if (fake_node) { fake_node->maxDataObjectsInMatch = _maxDataObjectsInMatch; } };
    /**
     * Set the matching threshold (0-100). Only used with fake nodes.
     */
    void setMatchThreshold(int _matchThreshold /**< [in] The matching threshold, 0-100. */) { if (fake_node) { fake_node->matchThreshold = _matchThreshold; } };
    /**
     * Add a fake interface id. 
     */
    void addInterface(const string &_id /**< [in] The fake interface id. */) { if (fake_node) { fake_node->interfaces.push_back(_id); } };
    /** 
     * Add a fake interest, and weight. The interest is of the form key=val.
     */
    void addInterest(const string &keyval /**< [in] The fake key=val interest string. */, int weight /**< [in] The interest weight. */) {  if (fake_node) { fake_node->interests.push_back(make_pair(keyval, weight)); } };
    /**
     * Set the type of a fake node. 
     */
    void setType(Node::Type_t _type /**< [in] The fake node type. */) { if (fake_node) { fake_node->type = _type; } };
};

/**
 * Data object to make unit testing easier, can either wrap a real data
 * object or abstract a fake data object.
 *
 * This class avoids the need to instantiate a full data object during testing. 
 */
class AbstractDataObject
{
private:
    string id; /**< Fake data object id. */
    List<string> attributes; /**< Fake attributess in key=val format. */
    DataObjectRef dObj; /**< The real underlying data object. */
public:
    /**
     * Construct an abstract data object that wraps a real data object. 
     */
    AbstractDataObject(DataObjectRef &_dObj /**< [in] The real wrapped data object. */) : id(""), dObj(_dObj) {};
    /** 
     * Construct a fake data object. 
     */ 
    AbstractDataObject(const string &_id /**< [in] The fake data object's id. */) : id(_id), dObj(NULL) {};
    /**
     * Deconstruct an abstract data object.
     */
    ~AbstractDataObject() {};
    /**
     * Get the data object's ID.
     * @return The abstract data object's id.
     */
    string getId() { return dObj ? dObj->getIdStr() : id; };
    /**
     * Get the attributes in key=val format for the abstract data object.
     */
    virtual void getAttributes(List<string> *new_attrs /**< [out] The data object's attributes. */);
    /**
     * Get the underlying data object. This is NULL if it is a fake data object.
     * @return The underlying data object.
     */
    DataObjectRef getDataObject() { return dObj; };
    /**
     * A helper function to add fake attributes during testing. 
     */
    void _addAttribute(const string &keyval /**< [in] The fake attribute to add. */) { attributes.push_back(keyval); };
};

/**
 * A fast in-memory database made from hash tables. 
 * 
 * Each hash table keeps a long integer that counts the maximum number
 * of elements in the table since last compaction, we use this value to
 * determine when the compact a hash table.  Compaction is supported where
 * we create new hashtables and copy the entries over if the size of the hash 
 * table shrinks by a factor of 2 and a small fudge (NOTE: The hash table 
 * implementation uses an array doubling algorithm. 
 */
class MemoryCache 
{
private:
    bool excludeZeroWeightAttributes;  /**< Exclude zero weight attributes from the database if set to `true`, include otherwise. */
    bool enableCompaction; /**< Shrink the database to save memory if set to `true`, do not otherwise. */

    // primary indicies

    long do_id_to_do_max_size; /**< The maximum number of entries that have been stored in the hashtable for `do_id_to_do`. */
    long do_id_to_do_freed; /**< The number of freed entries for `do_id_to_do`. */
    typedef HashMap<string, AbstractDataObject *> do_id_to_do_t; /**< A map type for `do_id_to_do`. */
    do_id_to_do_t *do_id_to_do; /**< A map to go from a data object id to an abstract data object. This is _NOT_ a multi-map. */

    long node_id_to_node_max_size; /**< The maximum number of entries that have been stored in the hashtable for `node_id_to_node`. */
    long node_id_to_node_freed; /**< The number of freed entries for `node_id_to_node`. */
    typedef HashMap<string, AbstractNode *> node_id_to_node_t; /**< A map type for `node_id_to_node_t`. */
    node_id_to_node_t *node_id_to_node; /**< A map to go from a node id to an abstract node. This is _NOT_ a multi-map. */

    long node_id_to_do_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `node_id_to_do_id`. */
    long node_id_to_do_id_freed; /**< The number of freed entries for `node_id_to_do_id`. */
    typedef HashMap<string, string> node_id_to_do_id_t; /**< A map type for `node_id_to_do_id`. */
    node_id_to_do_id_t *node_id_to_do_id; /**< A map to go from a node id to a data object id. This _is_ a mult-map.  This is only used if in memory node descriptions are enabled. */

    // secondary indices

    long iface_to_node_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `iface_to_node_id`. */
    long iface_to_node_id_freed; /**< The number of freed entries for `iface_to_node_id`. */
    typedef HashMap<string, string> iface_to_node_id_t; /**< A map type for `iface_to_node_id`. */
    iface_to_node_id_t *iface_to_node_id; /**<  A map to go from the interface id to a node id. This is _NOT_ a mult-map. */

    long keyval_to_node_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `keyval_to_node_id`. */
    long keyval_to_node_id_freed; /**< The number of freed entries for `keyval_to_node_id`. */
    typedef HashMap<string, string> keyval_to_node_id_t;  /**< A map type for `keyval_to_node_id`. */
    keyval_to_node_id_t *keyval_to_node_id; /**<  A map to go from the interest of form `key=val` to a node id with that interest. This _is_ a mult-map. */

    long key_star_to_node_ids_max_size; /**< The maximum number of entries that have been stored in the hashtable for `key_star_to_node_ids`. */
    long key_star_to_node_ids_freed; /**< The number of freed entries for `key_star_to_node_ids`. */
    typedef HashMap<string, HashMap<string, int> *> key_star_to_node_ids_t; /**< A map type for `key_star_to_node_ids`. */
    key_star_to_node_ids_t *key_star_to_node_ids; /**<  A map to go from a `key=*` attribute to a node id with that interest. This _is_ a mult-map. */

    long type_to_node_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `type_to_node_id`. */
    long type_to_node_id_freed; /**< The number of freed entries for `type_to_node_id`. */
    typedef HashMap<string, string> type_to_node_id_t; /**< A map type for `type_to_node_id`. */
    type_to_node_id_t *type_to_node_id; /**<  A map to go from node types to node ids with that type. This _is_ a mult-map. */

    long keyval_to_do_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `keyval_to_do_id`. */
    long keyval_to_do_id_freed; /**< The number of freed entries for `keyval_to_do_id`. */
    typedef HashMap<string, string> keyval_to_do_id_t; /**< A map type for `keyval_to_do_id`. */
    keyval_to_do_id_t *keyval_to_do_id; /**< A map to go from a key=val attribute to data object ids with that attribute. This _is_ a mult-map. */

    long key_star_to_do_ids_max_size;  /**< The maximum number of entries that have been stored in the hashtable for `key_star_to_do_ids`. */
    long key_star_to_do_ids_freed; /**< The number of freed entries for `key_star_to_do_ids`. */
    typedef HashMap<string, HashMap<string, string> *> key_star_to_do_ids_t;  /**< A map type for `key_star_to_do_ids`. */
    key_star_to_do_ids_t *key_star_to_do_ids; /**< A map to go from a key=* attribute to data object ids with any attributes with key `key`. This _is_ a mult-map. */

    /**
     * Filter properties. 
     */
    typedef struct {
        long matchThreshold; /**< The match threshold for the filter (0-100). */
        long maxMatches; /**< The maximum number of matches for the filter. */
    } filter_t;

    long filters_max_size; /**< The maximum number of entries that have been stored in the hashtable for `key_star_to_do_ids`. */
    long filters_freed; /**< The number of freed entries for `filters_freed`. */
    typedef HashMap<long, filter_t> filters_t; /**< A map type for `filters`. */
    filters_t *filters;  /**< A map to go from a filter id to a filter struct w/ that filter's properties. This is _NOT_ a mult-map. */

    /**
     * The attributes for a filter.
     */
    typedef struct {
        string keyval; /**< The key=val attribute belonging to the filter. */
        int weight; /**< The weight of that attribute belonging to the filter. */
    } attribute_t;

    long filter_id_to_attribute_max_size; /**< The maximum number of entries that have been stored in the hashtable for `filter_id_to_attribute`. */
    long filter_id_to_attribute_freed; /**< The number of freed entries for `filter_id_to_attribute`. */
    typedef HashMap<long, attribute_t> filter_id_to_attribute_t; /**< A map type for `filter_id_to_attribute`. */
    filter_id_to_attribute_t *filter_id_to_attribute; /**< A map to go from a filter id to a filter struct w/ that filter's attributes. This _is_ a mult-map. */

    long keyval_to_filter_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `keyval_to_filter_id`. */
    long keyval_to_filter_id_freed; /**< The number of freed entries for `keyval_to_filter_id`. */
    typedef HashMap<string, long> keyval_to_filter_id_t; /**< A map type for `keyval_to_filter_id`. */
    keyval_to_filter_id_t *keyval_to_filter_id; /**< A map to go from a key=val attribute to filter ids. This _is_ a mult-map. */

    long key_star_to_filter_id_max_size; /**< The maximum number of entries that have been stored in the hashtable for `key_star_to_filter_id`. */
    long key_star_to_filter_id_freed; /**< The number of freed entries for `key_star_to_filter_id`. */
    typedef HashMap<string, HashMap<long, int> *> key_star_to_filter_id_t; /**< A map type for `keyval_to_filter_id`. */
    key_star_to_filter_id_t *key_star_to_filter_id; /**< A map to go from a key=* attribute to filter ids. This _is_ a mult-map. */

    long availableFilterId; /**< Counter to generate temporary filter ids. */

    long errorCount; /**< The number of errors that have occurred. */
    int maxNodesToMatch; /**< The maximum number of nodes to match for a data object. */
    long repositoryRows; /**< The number of repository row ids, used to generate new ids on repository insertion. */ 

    long authority_to_repository_max_size; /**< The maximum number of entries that have been stored in the hashtable for `authority_to_repository`. */
    long authority_to_repository_freed; /**< The number of freed entries for `authority_to_repository`. */
    typedef HashMap<string, const RepositoryEntryRef> authority_to_repository_t; /**< A map type for `authority_to_repository`. */
    authority_to_repository_t *authority_to_repository; /**< A map to go from a repository authority to repository entries. This _is_ a mult-map. */

    /**
     * Basic unit tests for zero weight option.
     * @return `true` on PASS, `false` on failure.
     */
    static bool runZeroWeightTest();

    /**
     * Basic unit tests for compaction.
     * @return `true` on PASS, `false` on failure.
     */
    static bool runCompactionTest();

    /**
     * Basic unit tests for bare functionality. 
     * @return `true` on PASS, `false` on failure.
     */
    static bool runSelfTest1();

    /** 
     * Compute the degree of satisfaction between a filter and a data object.
     * @see computeSatisfaction
     * @return The filter degree of satisfaction, between 0 and 1. 
     */
    double computeFilterSatisfaction(long filter_id /**< [in] The filter id to match with. */, const string &obj_id /**< [in] The data object id to match against. */, int *o_attr_matched = NULL /**< [out] The number of attributes that matched. */);

    /** 
     * Compute the degree of satisfaction between a node description and  a data object, o_attr_matched is the number of matched attributes
     * @return The node degree of satisfaction, between 0 and 1. 
     */
    double computeSatisfaction(const string &node_id, const string &obj_id, int *o_attr_matched = NULL);

    /**
     * Compact a hash table by copying it into a new hash table.
     */ 
    template <typename X, typename Y>
    void compactHashTable(HashMap<X, Y> *big /**< [in] The large hashtable to compact. */, HashMap<X, Y> *small /** [out] The compacted hashtable is copied into this parameter. */) {
        if (!big || !small || big == small) {
            HAGGLE_ERR("Cannot compact hash table\n");
            return;
        }
        for (typename HashMap<X, Y>::iterator it = big->begin(); it != big->end(); it++) {
            small->insert(make_pair((*it).first, (*it).second));
        }
    }

public: 
    /** 
     * Construct an empty memory cache.
     */
    MemoryCache();

    /**
     * Free all the data structures for the memory cache.
     */
    ~MemoryCache();

    /**
     * Shrink all of the hashtables by copying them to a new hash table.
     *
     * Compaction only shrinks hashtables whose size has at least halved.
     * This constraint is because the underlying hash table uses the array doubling
     * technique, compacting earlier would not result in less memory usage. 
     */
    void compact(); 

    /**
     * Set the maximum number of nodes to match per data object.
     */
    void setMaxNodesToMatch(int _maxNodesToMatch /**< [in] The maximum number of nodes to match per data object. */) { maxNodesToMatch = _maxNodesToMatch; };

    /**
     * Enable or disable compaction. 
     * @see compact()
     */
    void setCompaction(bool enabled /**< [in] Set to `true` to enable compaction, `false` to disable */) { enableCompaction = enabled; };

    /**
     * Enable or disable excluding attributes with 0 weight from matching (and storing in the database). 
     */
    void setExcludeZeroWeightAttributes(bool status /**< [in] Set to `true` to exclude zero weight attributes from the database and matching, `false` to include.` */) { excludeZeroWeightAttributes = status; };

    // basic data object functions:

    /**
     * Store a data object into the database.
     * @see cacheDataObject() 
     * @return 0 on success, -1 on failure. 
     */
    int cacheDataObject(DataObjectRef &dObj /**< [in] The data object to insert into the database. */);

    /**
     * Store an abstract data object into the database.
     * @see cacheDataObject() 
     * @return 0 on success, -1 on failure. 
     */
    int cacheDataObject(AbstractDataObject *ad /**< [in] The abstract data object to insert into the database. */);

    /**
     * Check if a data object is in the database.
     * @return `true` if the data object is in the database, `false` otherwise.
     */
    bool isDataObjectCached(const string &dobj_id /**< [in] The data object id to check for in the database. */);

    /**
     * Get a data object from the database given its ID.
     * @return The data object with the passed ID.
     */ 
    DataObjectRef getDataObjectFromId(const string &dobj_id /**< [in] The ID of the data object to retrieve. */);

    /**
     * Remove a cached data object, give its ID.
     * @return 0 on success, -1 on failure. 
     */
    int unCacheDataObject(const string &dobj_id /**< [in] The ID of the data object to remove from the database. */);

    /**
     * Get all the data objects from the cache. 
     *
     * This function is mainly used when saving the database to disk during shutdown. 
     * It can be costly so we recommend using the other getters. 
     * @return A list of all of the data objects in the database.
     */
    DataObjectRefList getAllDataObjects();

    /**
     * Get all of the data objects from the database that do not have any nodes interested 
     * in them, and that are older than a specified age. 
     *
     * This function is useful for removing stale data objects from the database, but in
     * general it has been superceded by the utiltiy caching framework. 
     * @return The old data objects to purge.
     */
    DataObjectRefList getDataObjectsToAge(int maxAgeS /**< [in] All of the data objects returned will be at least this age. */, int maxToAge /**< [in] The maximum number of data objects to return for purging. */);

    /**
     * Get all of the node data object IDs for a particular node id.
     *
     * This function should only be used with in memory node descriptions enabled. 
     * @see getNodeDataObjectsForNode()
     * @return A list of data object ids for data objects that represent a node.
     */
    List<string> getNodeDataObjectIdsForNode(string &node_id /**< [in] The node id whose data object IDs should be fetched. */);
    /**
     * Get all of the node data objects for a particular node id.
     *
     * This function should only be used with in memory node descriptions enabled.
     * @see getNodeDataObjectIdsForNode()
     * @return A list of data objects that represent a node.
     */
    DataObjectRefList getNodeDataObjectsForNode(string &node_id /**< [in] The node id whose data objects should be fetched. */);

    // basic node functions:

    /**
     * Insert an abstract node into the database.
     * @see cacheNode()
     * @return -1 on failure, 0 on success. 
     */
    int cacheNode(AbstractNode *node);

    /**
     * Insert a node into the database.
     * @see cacheNode()
     * @return -1 on failure, 0 on success. 
     */
    int cacheNode(NodeRef &node);

    /**
     * Check if a node is in the database, given its ID.
     * @return `true` if it is in the database, `false` if it is not.
     */ 
    bool isNodeCached(const string &node_id /**< [in] The ID of the node to check to see is in the database. */);

    /**
     * Get a list of all of the nodes in the database that have a particular type.
     * @return The list of nodes that have the passed type.
     */
    NodeRefList getNodesFromType(Node::Type_t type /**< [in] The node type whose nodes are fetched. */);

    /** 
     * Get a node from the database that has a particular ID.
     * @return The node with the ID (or NULL if the node is missing).
     */
    NodeRef getNodeFromId(const string &node_id /**< [in] The node ID of the node that is fetched. */);

    /**
     * Get a node that has a particular interface ID.
     * @return The node with the interface ID (or NULL if the node is missing).
     */
    NodeRef getNodeFromIfaceId(const string &iface_id /**< [in] The interface ID of the node to fetch. */);

    /**
     * Remove a node from the database. 
     * @return -1 on failure, 0 on success.
     */
    int unCacheNode(const string &node_id);

    /**
     * Get all of the nodes from the database. 
     * 
     * This function is useful when saving the database to disk during shutdown. It
     * is an expensive operation and should not be used otherwise (use the other getters).
     * @return A list of all of the nodes in the database. 
     */
    NodeRefList getAllNodes();

    // basic filter functions:

    /**
     * Get a temporary filter ID to construct a new filter with.
     *
     * This function is mainly used for quick one-off filters.
     * @return The unused and newly allocated filter ID.
     */
    long getTemporaryFilterId() { return availableFilterId--; }

    /**
     * Check if a filter is in the database, given its ID.
     * @return `true` if the filter is in the database, `false` otherwise.
     */
    bool isFilterCached(long filter_id /**< [in] The ID of the filter to see is in the database. */);

    /**
     * Remove a filter from the cache.
     * @return -1 on failure, 0 on success. 
     */
    int unCacheFilter(long filter_id /**< [in] The ID of the filter to remove from the database. */);

    /** 
     * Set the parameters for a filter.
     */
    void setFilterSettings(long filter_id /**< [in] The ID of the filter whose parameters are to be set. */, long matchThreshold=0 /**< [in] The matching threshold of the filter. */, long maxMatches=0 /**< [in] The maximum number of matches for the filter. */);

    /**
     * Add an attribute to a filter.
     * @return -1 on failure (the filter id is invalid), 0 on success. 
     */
    int cacheFilter(long filter_id /**< [in] The ID of the filter whose attribute we are adding. */, string key /**< [in] The attribute key to add to the filter. */, string val /**< [in] The attribute value to add to the filter. */, int weight=0 /**< [in] The attribute weight. */);

    /** 
     * Get all the filter IDs that match a particular data object. 
     * @return The list of filter IDs that match the data object.
     */ 
    List<long> getFiltersForDataObjectId(string dobj_id /**< [in] The ID of the data object for which the returned filters are interested in. */);

    /**
     * Get a list of all of the data object IDs that match a particular filter.
     * @see getDataObjectsForFilter()
     * @return The list of data object IDs that match the filter.
     */
    List<string> getDataObjectIdsForFilter(long filter_id /**< [in] The ID of the filter whose data objects match are returned. */);

    /**
     * Get a list of all of the data objects that match a particular filter.
     * @see getDataObjectIdsForFilter()
     * @return The list of data objects that match the filter.
     */
    DataObjectRefList getDataObjectsForFilter(long filter_id);

    // Node -> Data Objects

    /**
     * Get a list of all of the data object ids that match a particular node with a specific ID.
     * @see getDataObjectsForNode()
     * @return The list of all of the data object ids that the node matches. 
     */ 
    List<string> getDataObjectIdsForNode(
        const string &node_id /**< [in] The node ID of the node that is interested in all of the returned data objects.*/, 
        int min_attr_match = 0, /**< [in] The minimum number of attributes that should match. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */
        int max_matches = -1, /**< [in] The maximum number of data objects to match the node with. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */
        int threshold = -1 /**< [in] The minimum threshold of the degree of satisfaction match between the nodes and data objects. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */);

    /**
     * Get a list of all of the data objects that match a particular node.
     * @see getDataObjectIdsForNode()
     * @return The list of all of the data objects that the node matches. 
     */
    DataObjectRefList getDataObjectsForNode(
        NodeRef &node, /**< [in] The node that is interested in all of the returned data objects. */
        int min_attr_match = 0,  /**< [in] The minimum number of attributes that should match. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */
        int max_matches = -1, /**< [in] The maximum number of data objects to match the node with. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */
        int threshold = -1 /**< [in] The minimum threshold of the degree of satisfaction match between the nodes and data objects. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */);

    // Data object -> Nodes

    /**
     * Get a list of all of the node ids that match a particular data object id.
     * @see getNodesForDataObject()
     * @return The list of all of the node ids that the data object matches.
     */
    List<string> getNodeIdsForDataObject(
        const string &dobj_id, /**< [in] The ID of the data object that matches all of the returned nodes. */
        int min_attr_match = 0,  /**< [in] The minimum number of attributes that should match. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */
        int max_matches = -1  /**< [in] The maximum number of nodes to match the data object with. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the class's default will be used.*/);

    /**
     * Get a list of all of the nodes that match a particular data object.
     * @see getNodeIdsForDataObject()
     * @return The list of all of the nodes that the data object matches.
     */
    NodeRefList getNodesForDataObject(
        DataObjectRef &dObj, /**< [in] The data object that matches all of the returned nodes. */
        int min_attr_match = 0, /**< [in] The minimum number of attributes that should match. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the node's default limit will be applied. */
        int max_matches = -1 /**< [in] The maximum number of nodes to match the data object with. If this parameter is 0 then the limit is not applied. If this parameter is -1 then the class's default will be used.*/);

    // repository functions:

    /**
     * Insert a repository entry into the database.
     * @return 0 on success, -1 on failure.
     */
    int insertRepository(const RepositoryEntryRef &ref /**< [in] The repository entry to insert. */);

    /**
     * Retrieve all of the repository entries from the database that belong to a particular
     * authority and have an optional key and an optional ID.
     * @return The matched repository entries.
     */
    RepositoryEntryList readRepository(
        string authority, /**< [in] The authority name used to match repository entries. */
        string key = "",  /**< [in] The (optional) repository entry key that returned entries must have. */
        unsigned int id = 0 /**< [in] The ID that the returned repository entry must have (NOT CURRENTLY IMPLEMENTED). */); 

    /**
     * Delete a repository entry from the database.
     * @return 0 on success, -1 on failure. 
     */
    int deleteRepository(
        string authority, /**< [in] The authority of the repository entry to be deleted. */
        string key, /**< [in] The key of the repository entry to be deleted. */
        unsigned int id = 0 /**< [in] The ID of the repository entry to be deleted (NOT CURRENTLY IMPLEMENTED) */); 

    /**
     * Get all of the repository entries in the database.
     *
     * This function is primarily used when saving the database to disk during shutdown. 
     * @return All of the repository entries in the database.
     */
    RepositoryEntryList getAllRepository();

    /**
     * Run the max matches (unit test). 
     * @return `true` on test PASS, `false` on FAIL.
     */
    static bool runMaxMatchesTest();


    /**
     * Run the self tests (unit tests). 
     * @return `true` on test PASS, `false` on FAIL.
     */
    static bool runSelfTests();

    /**
     * Get the number of errors that occurred in the database throughout execution.
     * @return The number of errors that occurred. 
     */
    long getErrorCount() { return errorCount; };

    /**
     * Print database statistics to the file.
     */
    void printStats();

    /**
     * Get the number of entries freed due to compaction. 
     * @return The number of hash table entries freed due to compaction. 
     */
    long getCompactionFreed() { return do_id_to_do_freed + node_id_to_node_freed + node_id_to_do_id_freed + iface_to_node_id_freed + keyval_to_node_id_freed + key_star_to_node_ids_freed + type_to_node_id_freed + keyval_to_do_id_freed + key_star_to_do_ids_freed + filters_freed + filter_id_to_attribute_freed + keyval_to_filter_id_freed + key_star_to_filter_id_freed + authority_to_repository_freed; };
};

#endif /* _MEMORYCACHE_H */
