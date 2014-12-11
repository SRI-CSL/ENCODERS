/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_CONNECT_STATE_H
#define _REPL_CONNECT_STATE_H

/**
 * The ReplicationConnectState is responsible for keeping track of the average 
 * send rate rate and average connection duration of neighbor nodes. This
 * state is updated during neighbor discovery and when a node updates
 * its node description. Node descriptions contain the average connection 
 * duration and average send rate. This class takes the estimated time left
 * and multiplies it by the estimated send rate to determine the estimated
 * bytes that can be sent (similar to the bandwidth delay product metric).
 */
class ReplicationConnectState;

#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>
#include "HaggleKernel.h"

using namespace haggle; 


/**
 * State for neighbor's average connection duration and average send rate. 
 */
class ReplicationConnectState {

private:

    typedef struct {
        unsigned long avg_byte_sec; /**< Latest neighbor's average bytes per second. */
        unsigned long avg_connect_ms; /**< Latest neighbor's average connection duration in milliseconds. */
        Timeval nodeCreateTime; /**< Creation of the node description that populated this state. */
        Timeval firstCreateTime; /**< The time we initially connected to the node. */
    } node_info_t; /**< Neighbor's send state. */

    typedef HashMap<string, node_info_t> connect_map_t; /**< A data structure that maps a node id to its latest state. */

    connect_map_t connection_map; /**< A neighbor's connection state. */

public:

    /**
     * Construct a new data structure to keep track of neighbor's state.
     */
    ReplicationConnectState() {};

    /**
     * Free the resources used by this data structure.
     */
    ~ReplicationConnectState();

    /**
     * Report that a neighbor node disconnected and remove its state.
     */
    void notifyDownContact(string node_id /**< The id of the node that disconnected. */);

    /**
     * Update the state of a neighbor node, given it's average send rate and connection time. 
     * We use the creation time to make sure that the state is up-to-date.
     */
    void update(string node_id /**< The id of the node whose state to update. */, unsigned long avg_bytes_per_sec /**< The average send rate in bytes per second. */, unsigned long avg_connect_millis /** The average connection duration, in milliseconds */, Timeval &nodeCreateTime /** The creation time of the node description that set this state. */);

    /**
     * Get the estimated bytes that can be sent by multiplying the estimated
     * connection time duration left by the estimated bandwidth.
     * @return The estimated bytes left that can be sent to the neighbor with id `node_id`.
     */
    unsigned long getEstimatedBytesLeft(string node_id /**< The node id whose estimated bytes left is computed. */);

    /**
     * Run the unit tests.
     * @return `true` if the unit tests PASSED, `false` otherwise.
     */
    bool runSelfTest();
};

#endif
