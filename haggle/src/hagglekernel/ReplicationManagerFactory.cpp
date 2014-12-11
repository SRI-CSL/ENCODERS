/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#include "ReplicationManagerFactory.h"

#include "ReplicationManagerUtility.h"

ReplicationManagerModule *
ReplicationManagerFactory::getNewReplicationManagerModule(
    ReplicationManager *m,
    string name)
{
    ReplicationManagerModule *utilManagerModule = NULL;

    if (name == REPL_MANAGER_UTILITY_NAME) {
        utilManagerModule = new ReplicationManagerUtility(m);
        if (!utilManagerModule) {
            HAGGLE_ERR("Could not instantiate replication manager\n");
            return NULL;
        }
    }

    return utilManagerModule;
}


Event *
ReplicationManagerFactory::getSendStatsEvent(NodeRef node, DataObjectRef dObj, long send_delay, long send_bytes)
{
    return new Event(EVENT_TYPE_REPL_MANAGER, (void *) new ReplicationManagerEventData(dObj, node, send_delay, send_bytes), 0);
}

Event *
ReplicationManagerFactory::getNewNodeDescriptionEvent(NodeRef node)
{
    return new Event(EVENT_TYPE_REPL_MANAGER, (void *) new ReplicationManagerEventData(node), 0);
}

Event *
ReplicationManagerFactory::getSendNodeDescriptionEvent(DataObjectRef dObj, NodeRefList nodeList)
{
    return new Event(EVENT_TYPE_REPL_MANAGER, (void *) new ReplicationManagerEventData(dObj, nodeList), 0);
}

Event *
ReplicationManagerFactory::getReplicateAllEvent(double send_delay_s)
{
    return new Event(EVENT_TYPE_REPL_MANAGER, (void *) new ReplicationManagerEventData(), send_delay_s);
}

Event *
ReplicationManagerFactory::getNodeStatsEvent(NodeRef node, Timeval duration)
{
    return new Event(EVENT_TYPE_REPL_MANAGER, (void *) new ReplicationManagerEventData(node, duration), 0);
}
