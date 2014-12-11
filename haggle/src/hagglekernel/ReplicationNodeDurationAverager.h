/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_MANAGER_NODE_DURATION_AVGER_H
#define _REPL_MANAGER_NODE_DURATION_AVGER_H

/** 
 * The ReplicationNodeDurationAverager is responsible for keeping track
 * of the connection duration of the local node with other nodes. 
 * It collects these durations on when the link goes down and computes
 * an average connection duration in milliseconds. 
 *
 * This class is used within the ReplicationManagerUtility to estimate
 * the bandwidth delay product between two nodes--the average duration
 * is exchanged between nodes during neighbor discovery.
 */
class ReplicationNodeDurationAverager;

#include <libcpphaggle/Timeval.h>
#include <libcpphaggle/List.h>
#include "HaggleKernel.h"

#define REPL_MANAGER_NODE_DURATION_MAX_DATAPOINTS 200 /**< The default number of data points to average over. */

using namespace haggle;

typedef List<unsigned long> node_duration_data_t; /**< The duration of a connection that ended, in milliseconds. */

/**
 * Computes the average connection duration of the local node.
 */
class ReplicationNodeDurationAverager {

private:

    node_duration_data_t node_duration_data; /**< Contains the most recent connection durations in milliseconds, to be averaged. */

    int maxDataPoints; /**< The maximum number of data points to average over. */

    unsigned long numConnections; /**< The total number of connections that have been observed since launch. */

public:

    /**
     * Constructor to create a new node duration averager.
     */
    ReplicationNodeDurationAverager() : 
        maxDataPoints(REPL_MANAGER_NODE_DURATION_MAX_DATAPOINTS), 
        numConnections(0) {};
    
    /**
     * Deconstruct the averager and free its resources.
     */
    ~ReplicationNodeDurationAverager();

    /**
     * Add a new connection duration data point--this is called upon disconnection. 
     */
    void update(Timeval duration /**< The connection duration. */);

    /**
     * Get the average connection duration in milliseconds over the past data points. 
     * @return The average connection duration in milliseconds.
     */
    unsigned long getAverageConnectMillis();

    /**
     * Get the total number of connections seen since launch.
     * @return The total number of connections seen.
     */
    unsigned long getNumConnections() { return numConnections; }

    /**
     * Run the uni tests for this class.
     * @return `true` if the tests PASSED, `false` otherwise.
     */
    bool runSelfTest();
};

#endif
