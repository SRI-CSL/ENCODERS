/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _REPL_OPT_H
#define _REPL_OPT_H

/**
 * This class is responsible for determining the optimal data objects to
 * replicate towards a particular neighbor, given the data objects that
 * are already inflight towards that neighbor, and the estimated bytes
 * left in the connection.
 * The problem of which data objects to send is posed as a 0-1 knapsack problem,
 * where the elements are data objects, the cost is the size of the data object
 * in bytes, and the bag size is the estimated available bytes left that can
 * be sent over the connection. The utilities are functions over
 * (data object, node) pairs, as specified in config.xml.
 */
class ReplicationOptimizer;

#include <libcpphaggle/String.h>
#include "ReplicationKnapsackOptimizer.h"
#include "ReplicationUtilityFunction.h"
#include "ReplicationGlobalOptimizer.h"
#include "ReplicationDataObjectUtilityMetadata.h"

typedef BasicMap<string, ReplicationDataObjectUtilityMetadataRef> neighbor_node_id_map_t; /**< The data structure to keep track of utilties for data objects, given a node id. */

typedef BasicMap<string, ReplicationDataObjectUtilityMetadataRef> dobj_id_map_t; /**< The data structure to keep track of utilities for nodes, given a data object id. */

typedef BasicMap<string, ReplicationDataObjectUtilityMetadataRef> meta_map_t; /**< The data structure to keep track of utilities for <node, dobj> pairs, by concatenating the the node_id and dobj_id as a key. */

typedef HashMap<string, DataObjectRef> DO_map_t; /**< A map from all inserted data object ids to a data object. */

typedef HashMap<string, NodeRef> node_map_t; /**< A map from all active neighbor node ids to node objects. */

/**
 * The class responsible for computing the optimal data objects to replicate
 * towards a node.
 */
class ReplicationOptimizer {

private:

    ReplicationKnapsackOptimizer *knapsack; /**< Object used to solve the 0-1 knapsack problem. */

    ReplicationUtilityFunction *utilFunction; /**< The utility function used to compute utilities for <node, data object> pairs. */

    ReplicationGlobalOptimizer *globalOptimizer; /**< Class to dynamically adjust utility function weights. */

    neighbor_node_id_map_t utils; /**< Map to go from a node ids to all data objects and their utilities for replicating to that node. */

    dobj_id_map_t dobjs; /**<  Map to go from a data object and all nodes and their utilites for replicating that data object to that node. */

    meta_map_t meta; /**< A map to go from a <data object id, node id> tuple (represented as a string) to a utility object. */

    DO_map_t do_map; /**< A map from a data object id to a data object. */

    node_map_t node_map; /**< A map from a node id to the node. */

    int computePeriodMs; /**< Min milliseconds between calls to compute the utility for a node, data object pair. */

    int errorCount; /**< The number of errors that have occurred. */

    /**
     * Helper function to construct a key to access the `meta` utility map.
     * @return The key to the `meta` map.
     */
    string getKey(string dobj_id /**< The data object id whose utility the caller wants. */, string node_id /**< The node id whose utility the caller wants. */);

public:

    /**
     * Construct a new optimizer.
     */
    ReplicationOptimizer();

    /**
     * Free the resources allocated to the optimizer.
     */
    ~ReplicationOptimizer();

    /**
     * Calculate the cost of storing data object with id `dobj_id` at node
     * `node_id`.
     * @return The cost used during knapsack optimization.
     */
    int getCost(string dobj_id /**< The id of the data object whose cost is determined. */, string node_id /**< The id of the node whose cost is determined. */);

    /** CBMEN, HL - Add this call
    * Calculate the cost of storing data object at node `node_id`
    * @return The cost used during the knapsack optimization.
    */
    int getCost(DataObjectRef dObj, string node_id);

    /**
     * Compute the next data object to send towards the neighbor with id
     * `node_id`, skipping data objects in the `dobj_blacklist`, given
     * an estimated `bytes_left` bytes left in the connection.
     * @return `true` if there was a data object to replicate, `false` otherwise.
     */
    bool getNextDataObject(string node_id /**< The node id to compute data objects to replicate to. */, List<string> &dobj_blacklist /**< Data objects that are in-flight to the node--these are not included when computing the next data object to send. */, int bytes_left /**< The estimated bytes left in the connection. */, DataObjectRef& o_dobj /**< The optimal data object to replicate. */, NodeRef& o_node /**< The node belonging to `node_id`.*/);

    /**
     * Set the solver used to solve the 0-1 knapsack problem.
     */
    void setKnapsackOptimizer(ReplicationKnapsackOptimizer *ks /**< The knapsack solver. */);

    /**
     * Set the utility function to compute utilties for <node, data object> pairs.
     */
    void setUtilFunction(ReplicationUtilityFunction *utilFunction /**< The utility function used to determine utilites when solving the knapsack. */);

    /**
     * Set the global optimizer that alters the weights to the utility function.
     */
    void setGlobalOptimizer(ReplicationGlobalOptimizer *globalOptimizer /**< The global optimizer that alters the weights of the utility function. */);

    /**
     * Compute all the utilities for each <node, data object> pair, if they
     * have not been computed within the last `computerPeriodMs` milliseconds.
     */
    void computeUtilities();

    /**
     * Set the minimum delay between calls to compute() when calculating utilites.
     * This is used to reduce CPU load due to compute() calls.
     */
    void setComputePeriodMs(int computerPeriodMs /**< The minimum period in miliseconds between calls to compute(). */);

    /**
     * Notify the utility functions and the optimizer that a new data object was
     * inserted.
     */
    void notifyInsertion(DataObjectRef dObj /**< The data object that was inserted. */);

    /**
     * Notify the utility functions and the optimizer that a data object was deleted.
     */
    void notifyDelete(DataObjectRef dObj /**< The data object that was deleted. */);

    /**
     * Notify that utility functions and the optimizer that a data object was
     * sent replicated.
     */
    void notifySendSuccess(DataObjectRef dObj /**< The data object that was successfully sent. */, NodeRef node /**< The node that successfully received the data object. */);

    /**
     * Notify the utility functions and the optimizer that a data object was
     * unsuccessfully replicated.
     */
    void notifySendFailure(DataObjectRef dObj /**< The data object that failed to replicate. */, NodeRef node /**< The node that failed to receive the data object. */);

    /**
     * Notify the utility function and the optimizer that a new neighbor node
     * was discovered.
     */
    void notifyNewContact(NodeRef node /**< The newly discovered node. */);

    /**
     * Notify the utility function and the optimizer that a node was updated.
     */
    void notifyUpdatedContact(NodeRef node /**< The node that was upated. */);

    /**
     * Notify the utility function and the optimizer that a node disconnected.
     */
    void notifyDownContact(NodeRef node /**< The node that disconnected. */);

    /**
     * Run the unit tests.
     * @return `true` on PASS, `false` otherwise.
     */
    bool runSelfTest();
};

#endif
