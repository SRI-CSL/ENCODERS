/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _REPL_MANAGER_SEND_RATE_AVGER_H
#define _REPL_MANAGER_SEND_RATE_AVGER_H

/**
 * The ReplicationSendRateAverager is responsible for keeping track of 
 * the send rates of the local node to its neighbors. 
 * It collects these rates when a data object is sent, and computes an
 * average send rate in bytes per second.
 *
 * This class is used within the ReplicationManagerUtility to estimate
 * the bandwidth delay product between two nodes--the average send rate
 * in bytes is exchanged during neighbor discovery.
 */
class ReplicationSendRateAverager;

#include <libcpphaggle/List.h>
#include "Trace.h"

#define REPL_MANAGER_SEND_RATE_MAX_DATAPOINTS 200 /**< The default number of data points to average over. */

typedef List<Pair<long, long> *> send_rate_data_t; /**< Keeps tuples of (delay, bytes sent) for successful sends to average over and compute a rate. */

/**
 * Computes the average send rate of the local node.
 */
class ReplicationSendRateAverager {

private:

    send_rate_data_t send_rate_data; /**< Contains the data points to compute the average send rate. */

    unsigned int maxDataPoints; /**< The maximum number of data points to average over. */

    long totalDataPoints; /**< The total data points that have been observed since launch. */

public:

    /**
     * Constructor to create a new send rate averager. 
     */
    ReplicationSendRateAverager() : 
        maxDataPoints(REPL_MANAGER_SEND_RATE_MAX_DATAPOINTS),
        totalDataPoints(0) {};

    /**
     * Deconstruct the averager and free its resources.
     */
    ~ReplicationSendRateAverager();

    /**
     * Record a data object send success to later compute an average.
     */
    void update(long send_delay_ms /**< The send delay, or duration in milliseconds. */, long send_bytes /**< The total bytes send. */);

    /**
     * Get the average number of bytes sent per second.
     * @return The average number of bytes sent per second for the past `maxDataPoints` data objects.
     */
    long getAverageBytesPerSec();

    /**
     * Get the total observed data points.
     * @return The total observed data points.
     */
    unsigned long getTotalDataPoints() { return totalDataPoints; }

    /**
     * Run the module's unit tests.
     * @return `true` on success, `false`, otherwise.
     */
    bool runSelfTest();
};

#endif
