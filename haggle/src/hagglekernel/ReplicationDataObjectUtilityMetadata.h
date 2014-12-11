/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _REPL_META_H
#define _REPL_META_H

class ReplicationDataObjectUtilityMetadata;

#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/Reference.h>
#include "DataObject.h"

/**
 * The `ReplicationDataObjectUtilityMetadata` class is responsible for
 * storing utiltiy state for each <node, data object> pair. This state
 * is used by the `ReplicationKnapsackOptimizer` and the `ReplicationOptimizer`
 * to determine which data objects to replicate to which nodes.
 */
class ReplicationDataObjectUtilityMetadata {

protected:

    string id; /**< The id of the data object. */

    string nodeId; /**< The id of the node. */

    int cost; /**< The cost associated with replicating the data object to the node. */

    double utility; /**< The utility of replicating the data object to the node. */

    Timeval compute_date; /**< The time that this utility was computed. */

    Timeval create_date;  /**< When the first utility was computed for this <node, data object> pair. */

public:

    /**
     * Construct a new data object to store <node, data object> utility state.
     */
    ReplicationDataObjectUtilityMetadata(string _id /**< The id of the data object. */, string _nodeId /**< The id of the node. */, int _cost /**< The associated cost with this replication. */, double _utility /**< The utility associated with this replication. */, Timeval _compute_date /**< The time that this utility was computed. */, Timeval _create_date /**< The original creation time of this utility. */) :
        id(_id),
        nodeId(_nodeId),
        cost(_cost),
        utility(_utility),
        compute_date(_compute_date),
        create_date(_create_date) {}

    /**
     * Get the id of the node associated with this <node, data object> utility pair.
     * @return The ID of the node for the <node, data object> utility pair.
     */
    string getNodeId() { return nodeId; }

    /**
     * Get the id of the data object associated with this <node, data object> utiltiy pair.
     * @return the ID of the data object for the <node, data object> utility pair.
     */
    string getId() { return id; }

    /**
     * Get the cost associated with replication this data object to this node.
     * @return The associated cost for the replication..
     */
    int getCost() { return cost; }

    /**
     * Set the utility computed for performing this replication.
     */
    void setUtility(double _utility /**< The utility of replicating the data object to the node. */, Timeval _compute_date /**< The time that this was computed. */) { utility = _utility; compute_date = _compute_date; }

    /**
     * Get the computed utility for the replication.
     * @return The computed utility for the replication.
     */
    double getUtility() { return utility; }

    /**
     * Get the time that the utility was computed.
     * @return The time that the utility was computed.
     */
    Timeval getComputedTime() { return compute_date; }

    /**
     * Get the original creation time for this <node, data object> utility
     * state.
     * @return The original creation time.
     */
    Timeval getCreateTime() { return create_date; }
};

typedef Reference<ReplicationDataObjectUtilityMetadata> ReplicationDataObjectUtilityMetadataRef;

#endif
