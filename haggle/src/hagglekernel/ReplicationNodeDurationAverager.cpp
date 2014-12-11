/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ReplicationNodeDurationAverager.h"

ReplicationNodeDurationAverager::~ReplicationNodeDurationAverager()
{
    node_duration_data_t::iterator it = node_duration_data.begin();
    for (; it != node_duration_data.end(); it++) {
        node_duration_data.erase(it);
    }
}

void
ReplicationNodeDurationAverager::update(Timeval duration)
{
    if (node_duration_data.size() >= (unsigned int) maxDataPoints) {
       node_duration_data.pop_front();
    }
    node_duration_data.push_back(duration.getTimeAsMilliSeconds());
    numConnections++;
} 

unsigned long
ReplicationNodeDurationAverager::getAverageConnectMillis()
{
    node_duration_data_t::iterator it = node_duration_data.begin();
    unsigned long count = 0;
    unsigned long total_duration = 0;
    for (; it != node_duration_data.end(); it++) {
          count++;
          total_duration += (*it);
    }
    return (count == 0) ? 0 : (unsigned long) total_duration/count;
}

bool
ReplicationNodeDurationAverager::runSelfTest()
{
    ReplicationNodeDurationAverager *nodeRateAvger = new ReplicationNodeDurationAverager();
    if (0 != nodeRateAvger->getAverageConnectMillis()) {
        HAGGLE_ERR("self test 1 failed.\n");
        return false;
    }
    //Timeval casts 'x' as 'x seconds', we use miliseconds,
    //thus 3  = 3000ms.
    nodeRateAvger->update(3);

    if (3000 != nodeRateAvger->getAverageConnectMillis()) {
        HAGGLE_ERR("Self test 2 failed.   Expected 3000ms, found %dms\n", nodeRateAvger->getAverageConnectMillis());
        return false;
    }

    nodeRateAvger->update(9);
    if (6000 != nodeRateAvger->getAverageConnectMillis()) {
        HAGGLE_ERR("Self test 3 failed.   Expected 6000ms, found %dms\n",nodeRateAvger->getAverageConnectMillis());
        return false;
    }

    int i;
    for (i = 0; i < REPL_MANAGER_NODE_DURATION_MAX_DATAPOINTS; i++) {
        nodeRateAvger->update(9);
    }

    if (9000 != nodeRateAvger->getAverageConnectMillis()) {
        HAGGLE_ERR("Self test 4 failed.  Expected 9000ms, found %dms.\n", nodeRateAvger->getAverageConnectMillis());
        return false;
    }

    delete nodeRateAvger;
    return true;
}
