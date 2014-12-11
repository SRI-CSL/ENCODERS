/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _REPL_CON_SEND_H
#define _REPL_CON_SEND_H

/**
 * The ReplicationConcurrentSend is responsible for keeping track of the 
 * number of data objects that the replication manager is currently sending
 * to a particular neighbor. This allows the replication manager to bound
 * the number of in flight data objects, giving the SendPriorityManager 
 * an opportunity to reorganize the data objects in the send queue according
 * to separate policy. 
 */
class ReplicationConcurrentSend;

#include <libcpphaggle/String.h>
#include <libcpphaggle/Map.h>
#include "Trace.h"

#define REPL_CONN_SEND_DEFAULT_CONN 5 /**< The default max number of in-flight data objects sent by the replication manager. */

typedef BasicMap<string, string> node_tokens_t; /**< Data structure used to keep track of what data objects are scheduled to be sent. */

/**
 * The data structure used to keep track of the number of in-flight data
 * objects being replicated towards a particular node.
 */
class ReplicationConcurrentSend {
private:
    node_tokens_t node_tokens; /**< The tokens used to cap the number of data objets being replicated to a neighbor. */
    int maxTokens; /**< The maximum number of in-flight data objects replicated to a particular node. */
    bool tokens_per_node; /**< Should tokens be per node maximum, or total DO's sent maximum. */
public:
    /** 
     * Constructor to keep track of data objects in flight.
     */
    ReplicationConcurrentSend() : 
	tokens_per_node(true), 
        maxTokens(REPL_CONN_SEND_DEFAULT_CONN) {};

    void setMaxTokens(int numTokens) { maxTokens = numTokens; }; 
    int getMaxTokens() { return maxTokens; }; 
    void setTokensPerNode(bool setToken) { tokens_per_node = setToken; }; 
    bool getTokensPerNode() { return tokens_per_node; }; 

    /**
     * Free the reserved data structure.
     */
    ~ReplicationConcurrentSend();

    /**
     * Check if the replication manager can send a data object towards the
     * neighbor with node id `node_id`.
     * @return `true` iff there is a token available for a particular
     * neighbor node `node_id`.
     */
    bool tokenAvailable(string node_id /**< The node id of the neighbor the caller wishes to replicate a data object to. */);

    /**
     * Reserve a token from the `node_id` neighbor token bucket to send
     * data object with id `dobj_id`.
     */ 
    void reserveToken(string node_id /**< The node id that the data object will be replicated to. */, string dobj_id /**< The data object id of the data object being replicated. */);

    /**
     * Free a used token when a data object is either successfully sent or
     * fails to send. 
     */
    void freeToken(string node_id /**< The node id that had a data object successfully replicated to. */, string dobj_id /**< The data object that was successfully replicated. */);

    /**
     * Check if a data object has been scheduled to be replicated to a node.
     * @return `true` and `false` respectively if and only if there
     * a token reserved to send data object with id `dobj_id` to node
     * with id `node_id`.
     */
    bool tokenExists(string node_id /**< The node id of the node who the caller is checking if the data object was replicated to. */, string dobj_id /**< The data object id for the data object that the caller is checking is scheduled for replication. */);

    /**
     *  Return the data object ids for data objects that are currently being
     * sent to the node with node id `node_id`.
     */
    void getReservedDataObjectsForNode(string node_id /**< The id of the node whose data objects are being sent. */, List<string> &dobjs /**< The data object ids for data objects queued to send to the node. */);

    /** 
     * Runs the unit tests. 
     * @return `true` if the unit tests passed, `false` otherwise.
     */
    bool runSelfTest();
};

#endif
