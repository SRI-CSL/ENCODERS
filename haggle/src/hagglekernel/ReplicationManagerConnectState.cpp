/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "RateManagerConnectState.h"

RateManagerConnectState::RateManagerConnectState() :
    maxConnectMetricAgeMS(RATE_MANAGER_DEFAULT_MAX_CONNECT_METRIC_AGE_MS),
    connectOverride(0),
    connectBoost(0),
    maxPreviousConnectAgeMS(RATE_MANAGER_DEFAULT_MAX_ENCODING_COUNT_AGE_MS),
    avgConnectLowThreshS(RATE_MANAGER_DEFAULT_AVG_CONNECT_LOW_THRESHOLD_S),
    avgConnectHighThreshS(RATE_MANAGER_DEFAULT_AVG_CONNECT_HIGH_THRESHOLD_S),
    errorCount(0)
{
}

RateManagerConnectState::~RateManagerConnectState()
{
    for (Map<string, Timeval>::iterator it = currentConnections.begin(); it != currentConnections.end(); it++) {
        currentConnections.erase(it);
    }

    for (BasicMap<string, connection>::iterator it = previousConnections.begin(); it != previousConnections.end(); it++) {
        previousConnections.erase(it);
    }

    for (Map<string, connect_metric>::iterator it = connectMetric.begin(); it != connectMetric.end(); it++) {
        connectMetric.erase(it);
    }
}

/*
 * Record a neighbor's connect metric
 */
void
RateManagerConnectState::setConnectMetric(string node_id, int metric)
{
    Map<string, Timeval>::iterator itt = currentConnections.find(node_id);
    if (itt == currentConnections.end()) {
        HAGGLE_ERR("Setting a connect metric for node that is not currently connected: %s, metric: %d\n", node_id.c_str(), metric);
        errorCount++;
        return;
    }

    connect_metric m;
    m.connect_metric = metric;
    m.create_time = Timeval::now();

    Map<string, connect_metric>::iterator it = connectMetric.find(node_id);
    if (it != connectMetric.end()) {
        connectMetric.erase(it);
    }

    connectMetric.insert(make_pair(node_id, m));
}

/*
 * Get a neighbor's connect metric
 */
int
RateManagerConnectState::getConnectMetric(string node_id)
{
    Map<string, connect_metric>::iterator it = connectMetric.find(node_id);
    if (it != connectMetric.end()) {
        return (*it).second.connect_metric;
    }
    HAGGLE_ERR("Trying to get connect metric for unknown node: %s\n", node_id.c_str());
    errorCount++;
    return -1;
}

/*
 * Remove connect metrics that are too old
 */ 
void
RateManagerConnectState::purgeStaleConnectMetrics()
{
// TODO: we should probably never purge connect metrics, let nbr discovery do this
/*
    Timeval now = Timeval::now();
    for (Map<string, connect_metric>::iterator it = connectMetric.begin(); it != connectMetric.end(); it++) {
        string node_id = (*it).first;
        connect_metric m = (*it).second;
        if ((now - m.create_time).getTimeAsMilliSeconds() > (double) maxConnectMetricAgeMS) {
            // no point keeping state if we no longer have the popularity index
            Map<string, Timeval>::iterator itt = currentConnections.find(node_id);
            if (itt != currentConnections.end()) {
                errorCount++;
                HAGGLE_ERR("Trying to purge metric of node still connected (node: %s, metric: %d).\n", node_id.c_str(), m.connect_metric);
                continue;
            }
            BasicMap<string, connection>::iterator ittt = previousConnections.find(node_id);
            if (ittt != previousConnections.end()) {
                previousConnections.erase(ittt);
            }
            else {
                errorCount++;
                HAGGLE_ERR("Somehow the connect metric for node id: %s remained without any previous connection state\n", node_id.c_str());
            }
            connectMetric.erase(it);
        }
    }
*/
}

/*
 * Compute a connect metric based on our average connect time and number
 * of neighbors. 
 */
int
RateManagerConnectState::getMyConnectMetric()
{
    if (connectOverride > 0) {
        return connectOverride;
    }

    double avg_connect_time = computeAverageConnectTime();
    int Navt = 0;

    if (avg_connect_time >= avgConnectHighThreshS) {
        Navt = 2;
    } 
    else if (avg_connect_time >= avgConnectLowThreshS) {
        Navt = 1;
    }
    else {
        Navt = 0;
    }

    int num_node_connections = computeNumberNodeConnections();

    return Navt * num_node_connections + connectBoost;
}

/*
 * Compute the number of uniq noes encountered over a period of time
 */
int
RateManagerConnectState::computeNumberNodeConnections()
{
    // grab the uniq node ids from the current connections
    Map<string, int> uniqNodes;

    for (Map<string, Timeval>::iterator it = currentConnections.begin(); it != currentConnections.end(); it++) {
        string node_id = (*it).first;
        Map<string, int>::iterator itt = uniqNodes.find(node_id);
        int count = 0;
        if (itt != uniqNodes.end()) {
            count = (*itt).second;
            uniqNodes.erase(itt);
        }
        count++;
        uniqNodes.insert(make_pair(node_id, count));
    }

    // grab the uniq node ids from the prev. connections
    for (BasicMap<string, connection>::iterator it = previousConnections.begin(); it != previousConnections.end(); it++) {
        string node_id = (*it).first;
        Map<string, int>::iterator itt = uniqNodes.find(node_id);
        int count = 0;
        if (itt != uniqNodes.end()) {
            count = (*itt).second;
            uniqNodes.erase(itt);
        }
        count++;
        uniqNodes.insert(make_pair(node_id, count));
    }

    int num_uniq_neighbors = 0;
    for (Map<string, int>::iterator it = uniqNodes.begin(); it != uniqNodes.end(); it++) {
        num_uniq_neighbors++;
    }

    return num_uniq_neighbors;
}

double
RateManagerConnectState::computeAverageConnectTime()
{
    Timeval cur_time = Timeval::now();
    double total_duration = 0;

    int num_connections = 0;

    // include current connections
    for (Map<string, Timeval>::iterator it = currentConnections.begin(); it != currentConnections.end(); it++) {
        total_duration += (cur_time - (*it).second).getTimeAsMilliSeconds()/(double)1000;
        num_connections++;
    }

    // include previous connections within Tinterval_s
    for (BasicMap<string, connection>::iterator it = previousConnections.begin(); it != previousConnections.end(); it++) {
        connection c = (*it).second;
        total_duration += (c.end.getTimeAsMilliSeconds() - c.start.getTimeAsMilliSeconds()) / (double) 1000;
        num_connections++;
    }

    if (num_connections == 0) {
        return 0;
    }

    return total_duration / (double) num_connections;
}

void
RateManagerConnectState::purgeStalePreviousConnections()
{
    Timeval cur_time = Timeval::now();
    Map<string, int> deletedConnections;
    for (BasicMap<string, connection>::iterator it = previousConnections.begin(); it != previousConnections.end(); it++) {
        connection c = (*it).second;
        string node_id = (*it).first;
        if (c.end.getTimeAsMilliSeconds() >= (cur_time.getTimeAsMilliSeconds() - maxPreviousConnectAgeMS)) {
            continue;
        }
        previousConnections.erase(it);
        Map<string, int>::iterator itt = deletedConnections.find(node_id);
        int count = 0;
        if (itt != deletedConnections.end()) {
            count = (*itt).second;
            deletedConnections.erase(itt);
        }
        count++;
        deletedConnections.insert(make_pair(node_id, count));
    }

    for (Map<string, int>::iterator it = deletedConnections.begin(); it != deletedConnections.end(); it++) {
        string node_id = (*it).first;
        BasicMap<string, connection>::iterator itt = previousConnections.find(node_id);
        if (itt != previousConnections.end()) {
            continue;
        }
        Map<string, Timeval>::iterator ittt = currentConnections.find(node_id);
        if (ittt != currentConnections.end()) {
            continue;
        }
        // the node has no more state, remove the connect metric as well
        Map<string, connect_metric>::iterator itttt = connectMetric.find(node_id);
        if (itttt == connectMetric.end()) {
            errorCount++;
            HAGGLE_ERR("Somehow the previous connection remained without connect metric state for node: %s.\n", node_id.c_str());
        } 
        connectMetric.erase(itttt);
    }
}

void
RateManagerConnectState::setConnected(string node_id)
{
    Map<string, Timeval>::iterator it = currentConnections.find(node_id);
    
    if (it != currentConnections.end()) {
        // use the earliest date
        HAGGLE_ERR("Setting connected for node: %s when never received disconnect\n", node_id.c_str());
        errorCount++;
        return; 
    }

    Timeval cur_time = Timeval::now();
    currentConnections.insert(make_pair(node_id, cur_time));
}

void
RateManagerConnectState::setDisconnected(string node_id)
{
    Map<string, Timeval>::iterator it = currentConnections.find(node_id);
    
    if (it == currentConnections.end()) {
        HAGGLE_ERR("Setting disconnected for node: %s when never received conneted\n", node_id.c_str());
        errorCount++;
        return;
    }

    Timeval cur_time = Timeval::now();
    connection c;

    c.start = (*it).second;
    c.end = cur_time;

    previousConnections.insert(make_pair(node_id, c));
    currentConnections.erase(node_id);
}

void
RateManagerConnectState::purgeStaleState()
{
    //purgeStaleConnectMetrics();
    purgeStalePreviousConnections();
}

void
RateManagerConnectState::printDebug()
{
    for (Map<string, Timeval>::iterator it = currentConnections.begin(); it != currentConnections.end(); it++) {
        HAGGLE_ERR("RMDEBUG: currentConnections: %s, inserted: %s\n", (*it).first.c_str(), (*it).second.getAsString().c_str());
    }

    for (BasicMap<string, connection>::iterator it = previousConnections.begin(); it != previousConnections.end(); it++) {
        HAGGLE_ERR("RMDEBUG: prevConnection: %s, start: %s, end: %s\n", (*it).first.c_str(), (*it).second.start.getAsString().c_str(), (*it).second.end.getAsString().c_str());
    }

    for (Map<string, connect_metric>::iterator it = connectMetric.begin(); it != connectMetric.end(); it++) {
        HAGGLE_ERR("RMDEBUG: connect metric: %s\n", (*it).first.c_str());
    }
}

void
RateManagerConnectState::printStats()
{
    HAGGLE_STAT("Summary Statistics - RateManager Statistics - Error Count: %ld\n", errorCount);
    HAGGLE_STAT("Summary Statistics - RateManager Statistics - My Connect Metric: %d, avg connect time: %f, num connections: %d, boost: %d, override: %d\n", getMyConnectMetric(), computeNumberNodeConnections(), computeAverageConnectTime(), connectBoost, connectOverride);
}

/*
 * START: UNIT TESTS
 */

bool
RateManagerConnectState::runSelfTest()
{
    bool passed = true;
    HAGGLE_DBG("Running self tests...\n");
    // test 1
    if (!runSelfTest1()) {
        HAGGLE_ERR("Self test #1 FAILED.\n");
        passed = false;
    }
    
    return passed;
}

bool
RateManagerConnectState::runSelfTest1() 
{
    RateManagerConnectState *state = new RateManagerConnectState();
    state->setAvgConnectLowThreshS(0);
    state->setAvgConnectHighThreshS(1);
    state->setConnectBoost(1);
    state->setMaxPreviousConnectAgeMS(2000);
    
    string node_id_1 = "n1";
    string node_id_2 = "n2";

    state->setConnected(node_id_1);
    state->setConnectMetric(node_id_1, 1);

    sleep(1);

    if (1 != state->getConnectMetric(node_id_1)) {
        HAGGLE_ERR("getConnectMetric failed (n1)\n");
        goto FAIL;
    }

    state->setConnected(node_id_2);
    state->setConnectMetric(node_id_2, 2);

    if (2 != state->getConnectMetric(node_id_2)) {
        HAGGLE_ERR("getConnectMetric failed (n2)\n");
        goto FAIL;
    }

    state->setDisconnected(node_id_1);

    if (2 != state->computeNumberNodeConnections()) {
        HAGGLE_ERR("computeNumberNodeConnections failed\n");
        goto FAIL;
    }

    sleep(1);

    if (1 != (int) state->computeAverageConnectTime()) {
        HAGGLE_ERR("computeAverageConnectTime failed (2)\n");
        goto FAIL;
    }

    if (5 != state->getMyConnectMetric()) {
        HAGGLE_ERR("getMyConnectMetric failed\n");
        goto FAIL;
    }

    sleep(2);
    state->purgeStalePreviousConnections();

    if (1 != state->computeNumberNodeConnections()) {
        HAGGLE_ERR("computeNumberNodeConnections failed (2): got: %d, expected: %d \n", state->computeNumberNodeConnections(), 1);
        goto FAIL;
    }

    if (3 != (int) state->computeAverageConnectTime()) {
        HAGGLE_ERR("computeAverageConnectTime failed (2)\n");
        goto FAIL;
    }

    if (0 != state->getErrorCount()) {
        HAGGLE_ERR("an error occurred\n");
        goto FAIL;
    }

    delete state;
    return true;
FAIL:
    delete state;
    return false;
}

/*
 * END: UNIT TESTS
 */
