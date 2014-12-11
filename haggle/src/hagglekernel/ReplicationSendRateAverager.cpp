/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#include "ReplicationSendRateAverager.h"

ReplicationSendRateAverager::~ReplicationSendRateAverager()
{
    send_rate_data_t::iterator it;
    while ((it = send_rate_data.begin()) != send_rate_data.end()) {
        Pair<long, long> *datum = send_rate_data.front();
        send_rate_data.pop_front();
        if (!datum) {
            HAGGLE_ERR("NULL datum\n");
            continue;
        }
        delete datum;
    }
}

void
ReplicationSendRateAverager::update(long send_delay_ms, long send_bytes)
{
    if (send_rate_data.size() >= maxDataPoints) {
        Pair<long, long> *datum = send_rate_data.front();
        send_rate_data.pop_front();
        if (!datum) {
            HAGGLE_ERR("NULL datum\n");
            return;
        }
        delete datum;
    }
    send_rate_data.push_back(new Pair<long, long>(send_delay_ms, send_bytes));
}

long
ReplicationSendRateAverager::getAverageBytesPerSec()
{
    send_rate_data_t::iterator it = send_rate_data.begin();
    long total_delay_ms = 0;
    long total_bytes = 0;
    for (; it != send_rate_data.end(); it++) {
        total_delay_ms += (*it)->first;
        total_bytes += (*it)->second;
    }
    if (total_bytes == 0) {
        return 0;
    }
    if (total_delay_ms == 0) {
        return total_bytes * 1000;
    }
    return (total_bytes*1000 / total_delay_ms); //bytes/ms -> bytes/sec
}

bool
ReplicationSendRateAverager::runSelfTest()
{
    ReplicationSendRateAverager *sendRateAvger = new ReplicationSendRateAverager();
    if (0 != sendRateAvger->getAverageBytesPerSec()) {
        HAGGLE_ERR("self test 1 failed.\n");
        return false;
    }

    sendRateAvger->update(1000, 3000);

    if (3000 != sendRateAvger->getAverageBytesPerSec()) {
        HAGGLE_ERR("Self test 2 failed.  Expected 3KB/s, found %d B/s\n",sendRateAvger->getAverageBytesPerSec() );
        return false;
    }

    int i;
    for (i = 0; i < REPL_MANAGER_SEND_RATE_MAX_DATAPOINTS; i++) {
        sendRateAvger->update(1000, 9000);
    }

    if (9000 != sendRateAvger->getAverageBytesPerSec()) {
        HAGGLE_ERR("Self test 3 failed.   Expected 9KB/s, found %d B/s\n", sendRateAvger->getAverageBytesPerSec());
        return false;
    }

    delete sendRateAvger;
    return true;
}
