/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _RATE_MANAGER_CONNECT_STATE_H
#define _RATE_MANAGER_CONNECT_STATE_H

#define RATE_MANAGER_DEFAULT_MAX_CONNECT_METRIC_AGE_MS 300000
#define RATE_MANAGER_DEFAULT_MAX_ENCODING_COUNT_AGE_MS 360000
#define RATE_MANAGER_DEFAULT_AVG_CONNECT_LOW_THRESHOLD_S 3
#define RATE_MANAGER_DEFAULT_AVG_CONNECT_HIGH_THRESHOLD_S 6

class RateManagerConnectState;

#include "DataObject.h"

class RateManagerConnectState {
private:
    int maxConnectMetricAgeMS;
    int connectOverride;
    int connectBoost;
    int maxPreviousConnectAgeMS;
    int avgConnectLowThreshS;
    int avgConnectHighThreshS;

    long errorCount; 

    typedef struct connection {
        Timeval start;
        Timeval end;
    } connection_t;

    Map<string, Timeval> currentConnections;

    // we use a basic map here to allow multiple entries for the same key
    BasicMap<string, connection> previousConnections;

    typedef struct connect_metric {
        int connect_metric;
        Timeval create_time;
    } connect_metric_t;

    Map<string, connect_metric> connectMetric;

    static bool runSelfTest1();

protected:
    int computeNumberNodeConnections();
    double computeAverageConnectTime();
    void purgeStaleConnectMetrics();
    void purgeStalePreviousConnections();
public:
    RateManagerConnectState();
    ~RateManagerConnectState();
    void setConnectMetric(string node_id, int pop_index);
    int getConnectMetric(string node_id);
    int getMyConnectMetric();
    void setConnected(string node_id);
    void setDisconnected(string node_id);
    void purgeStaleState();
    void printDebug();
    void printStats();
    void setMaxConnectMetricAgeMS(int d) { maxConnectMetricAgeMS = d; }
    void setConnectOverride(int d) { connectOverride = d; }
    void setConnectBoost(int d) { connectBoost = d; }
    void setMaxPreviousConnectAgeMS(int d) { maxPreviousConnectAgeMS = d; }
    void setAvgConnectHighThreshS(int d) { avgConnectHighThreshS = d; }
    void setAvgConnectLowThreshS(int d) { avgConnectLowThreshS = d; }

    static bool runSelfTest();
    long getErrorCount() { return errorCount; }
};

#endif
