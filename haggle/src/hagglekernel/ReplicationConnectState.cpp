/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#include "ReplicationConnectState.h"

ReplicationConnectState::~ReplicationConnectState() {
     connect_map_t::iterator it;
     while ((it = connection_map.begin()) != connection_map.end()) {
          connection_map.erase(it); 
     }
}

/*
 *
 * Notify when a node neighbor is no longer available.
 * Remove it from the internal database.
 *
 */
void
ReplicationConnectState::notifyDownContact(string node_id)
{
    connect_map_t::iterator it = connection_map.find(node_id);
    if (it != connection_map.end()) {
        connection_map.erase(it);
    }
}

/*
 *
 * update() updates the database with node information, about that
 * node's expected connect time (in ms), expected bandwidth (in bytes/sec),
 * and the update time.   The update time is entered here, in case we get
 * the node description before the notifyNewContact event.
 *
 */
void
ReplicationConnectState::update(string node_id, unsigned long avg_bytes_per_sec, unsigned long avg_connect_millis, Timeval &nodeCreateTime)
{
     connect_map_t::iterator it = connection_map.find(node_id);
     node_info_t node_info = { avg_bytes_per_sec, avg_connect_millis, nodeCreateTime, Timeval::now() };
     //new entry
     if (it == connection_map.end()) {
        connection_map.insert(make_pair(node_id, node_info));
        return;
     } 
     //new entry is newer than stored entry
     if (nodeCreateTime > (*it).second.nodeCreateTime) {
        connection_map.erase(it);
        connection_map.insert(make_pair(node_id, node_info));
     }
}

/*
 *
 * Return the estimated number of bytes, that can be sent,
 * to the specified node.   Using the timestamp from when the node
 * is first seen, to now, and the estimated amount of contact time,
 * we can get an approximate estimate of how many more bytes can be
 * sent.
 *
 */
unsigned long
ReplicationConnectState::getEstimatedBytesLeft(string node_id) {

     connect_map_t::iterator it = connection_map.find(node_id);
     //new entry
     if (it == connection_map.end()) {
        HAGGLE_ERR("Cant find node %s info!\n", node_id.c_str());
        return 0LL;
     } 
     node_info_t &node_info = (*it).second;

     Timeval timePast = Timeval::now() - node_info.firstCreateTime;

     if ((unsigned long)timePast.getTimeAsMilliSeconds() > node_info.avg_connect_ms) {
        return 0LL;
     }
     unsigned long timeLeft = node_info.avg_connect_ms -  timePast.getTimeAsMilliSeconds();
     unsigned long bytesLeft = timeLeft *  node_info.avg_byte_sec / 1000; //bytes per millisec

     return bytesLeft;

}


/*
 *
 * self test tests the internal logic, estimating number of bytes based upon
 * initial contact information, and time expired.
 *
 */
bool
ReplicationConnectState::runSelfTest()
{
    bool result = true;
    //add 2 nodes
    ReplicationConnectState *connectState = new ReplicationConnectState();
    Timeval now = Timeval::now();
    connectState->update(string("node A"), 5000UL, 900UL, now);
    connectState->update(string("node B"), 4000UL, 8000UL, now);
    unsigned long nodeABytes = connectState->getEstimatedBytesLeft(string("node A"));
    if (nodeABytes == 0 || nodeABytes > 4500)  {
         HAGGLE_ERR("self test 1 failed.   %lu bytes left\n", nodeABytes);
	 result= false;
    }  
    sleep(1);
    nodeABytes = connectState->getEstimatedBytesLeft(string("node A"));
    if (nodeABytes != 0) {
         HAGGLE_ERR("self test 2 failed.   %lu bytes left\n", nodeABytes);
	 result= false;
    }  
    unsigned long nodeBBytes = connectState->getEstimatedBytesLeft(string("node B"));
    if (nodeBBytes == 0 || nodeBBytes > 28001)  {
         HAGGLE_ERR("self test 3 failed.   %lu bytes left\n", nodeBBytes);
	 result= false;
    }  

    //This will cause an ERROR message: ERROR: Cant find node B connect time!
    //in the haggle log, but that is expected as part of the self test.
    connectState->notifyDownContact(string("node B"));
    nodeBBytes = connectState->getEstimatedBytesLeft(string("node B"));
    if (nodeBBytes != 0) {
         HAGGLE_ERR("self test 4 failed.   %lu bytes left\n", nodeBBytes);
	 result= false;
    }  


    return result;
}
