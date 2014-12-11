/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */


#include "ReplicationManagerModule.h"

#include "ReplicationManagerFactory.h"

void 
ReplicationManagerModule::handleReplicationManagerEvent(Event *e) {
    // proxy the event to the proper rate manager function
    if (!e) {
        HAGGLE_ERR("No event\n");
        return;
    }
    if (e->getType() != EVENT_TYPE_REPL_MANAGER) {
        HAGGLE_ERR("Not rate manager event\n");
        return;
    }
    if (!e->getData()) {
        HAGGLE_ERR("Missing rate data\n");
        return;
    }
    ReplicationManagerEventData *data = static_cast<ReplicationManagerEventData *>(e->getData());
    switch(data->getType()) {
    case REPL_MANAGER_TYPE_NEW_NODE_DESCRIPTION:
    {
        notifyNewNodeDescription(data->getNode());
        break;
    }
    case REPL_MANAGER_TYPE_SEND_NODE_DESCRIPTION:
    {
        sendNodeDescription(data->getDataObject(), data->getNodeList());
        break;
    }
    case REPL_MANAGER_TYPE_SEND_STATS:
    {
        notifySendStats(data->getNode(), data->getDataObject(), data->getSendDelay(), data->getSendBytes());
        break;
    }
    case REPL_MANAGER_TYPE_NODE_STATS:
    {
        notifyNodeStats(data->getNode(), data->getDuration());
        break;
    }
    case REPL_MANAGER_TYPE_REPLICATE_ALL:
    {
        replicateAll();
        break;
    }
    case REPL_MANAGER_TYPE_INVALID:
    {
        break;
    }
    default: 
    {
        HAGGLE_ERR("Unknown type\n");
        break;
    }
    }

    delete data;
}
