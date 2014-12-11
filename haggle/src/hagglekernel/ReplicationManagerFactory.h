/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */


#ifndef REPL_MANAGER_FACTORY_H
#define REPL_MANAGER_FACTORY_H

class ReplicationManagerFactory;

#include "DataManager.h"
#include "Metadata.h"
#include "ReplicationManager.h"
#include "ReplicationManagerModule.h"
#include "Event.h"
#include "Manager.h"

/**
 * Data type for events that other managers can use to communicate
 * with a replication manager.
 */
typedef enum {
    REPL_MANAGER_TYPE_NEW_NODE_DESCRIPTION /**< New node description event. */,
    REPL_MANAGER_TYPE_SEND_NODE_DESCRIPTION /**< Send node description event. */,
    REPL_MANAGER_TYPE_SEND_STATS /**<  Send statistics event. */,
    REPL_MANAGER_TYPE_NODE_STATS /**< Node duration event. */,
    REPL_MANAGER_TYPE_REPLICATE_ALL /**< Replicate all event. */,
    REPL_MANAGER_TYPE_INVALID /**< Invalid event. */,
} ReplicationManagerEventType;

/*
 * Bag for various event data for ReplicationManagers.
 */
class ReplicationManagerEventData {

private:

    ReplicationManagerEventType t; /**< The event type. */

    NodeRef node; /**< The node for the event. */

    NodeRefList nodeList; /**< The node list for the event. */

    DataObjectRef dObj; /**< The data object for the event. */
 
    int delay; /**< The delay for the event. */

    long send_delay; /**< The send delay for the event. */

    long send_bytes; /**< The send bytes for the event. */

    Timeval duration; /**< The duration for the event. */
public:

    /**
     * The bag for the send node description event.
     */
    ReplicationManagerEventData(DataObjectRef _dObj /**< Data object of the node description being sent. */, NodeRefList _nodeList /**< Nodes receiving the node description. */) :
        t(REPL_MANAGER_TYPE_SEND_NODE_DESCRIPTION),
        node(NULL),
        nodeList(_nodeList),
        dObj(_dObj),
        delay(0),
        send_delay(0),
        send_bytes(0),
        duration(0) {};

    /**
     * The bag for the send stats event.
     */
    ReplicationManagerEventData(DataObjectRef _dObj /**< The data object that was sent. */, NodeRef _node /**< The node that received the data object. */, long _send_delay /**< The send delay in milliseconds. */, long _send_bytes /**< The sent bytes. */) :
        t(REPL_MANAGER_TYPE_SEND_STATS),
        node(_node),
        nodeList(NULL),
        dObj(_dObj),
        delay(0),
        send_delay(_send_delay),
        send_bytes(_send_bytes),
        duration(0) { };

    /**
     * The bag for the node stats event.
     */
    ReplicationManagerEventData(NodeRef _node /**< The node that disconnected. */, Timeval _duration /**< The duration it was connected for. */) :
        t(REPL_MANAGER_TYPE_NODE_STATS),
        node(_node),
        nodeList(NULL),
        dObj(NULL),
        delay(0),
        send_delay(0),
        send_bytes(0),
        duration(_duration) {};

    /**
     * The bag for the new node description event.
     */
    ReplicationManagerEventData(NodeRef _node /**< The node object for the new node description. */) :
        t(REPL_MANAGER_TYPE_NEW_NODE_DESCRIPTION),
        node(_node),
        nodeList(NULL),
        dObj(NULL),
        delay(0),
        send_delay(0),
        send_bytes(0),
        duration(0) {};

    /**
     * The bag for the replicate all event.
     */
    ReplicationManagerEventData() :
        t(REPL_MANAGER_TYPE_REPLICATE_ALL),
        node(NULL),
        nodeList(NULL),
        dObj(NULL),
        delay(0),
        send_delay(0),
        send_bytes(0),
        duration(0) {};

    /**
     * Get the node list for the event.
     * @return The node list. 
     */
    NodeRefList getNodeList() { return nodeList; }

    /**
     * Get the event type.
     * @return The event type.
     */
    ReplicationManagerEventType getType() { return t; }

    /** 
     * Get the node for the event.
     * @return The node.
     */
    NodeRef getNode() { return node; }

    /**
     * Get the data object for the event.
     * @return The data object.
     */
    DataObjectRef getDataObject() { return dObj; }

    /**
     * Get the delay for the event.
     * @return The delay.
     */
    int getDelay() { return delay; }

    /**
     * Get the send delay for the event.
     * @return The send delay.
     */
    long getSendDelay() { return send_delay; }

    /** 
     * Get the send bytes for the event.
     * @return The send bytes for the event.
     */
    long getSendBytes() { return send_bytes; }

    /**
     * Get the duration for the event.
     * @return The duration.
     */
    Timeval getDuration() { return duration; }
};

/**
 * Factory to construct a new replication manager module.
 */
class ReplicationManagerFactory {

public:

    /**
     * Get a new replication manager module, given the ReplicationManager and
     * the name of the class to instantiate.
     * @return The newly instantiated replication manager module.
     */
    static ReplicationManagerModule *getNewReplicationManagerModule(ReplicationManager *m /**< The replication manager. */, string name /**< The name of the class to instantiate. */);

    /**
     * Get a send stats event to communicate send statistics from other managers
     * to the replication manager.
     * @return The send stats event.
     */
    static Event *getSendStatsEvent(NodeRef node /**< The node that received the data object. */, DataObjectRef dObj /**< The data object that was sent. */, long send_delay_ms /**< The duration to send, in milliseconds. */, long send_bytes /**< The number of bytes sent. */);

    /**
     * Get a new node description event--used by other managers to communicate
     * with the replication manager that a new node description was received.
     * @return The new node description event.
     */
    static Event *getNewNodeDescriptionEvent(NodeRef node /**< The node of which we received a new node description. */);

    /**
     * Get a new send node description event--used by the other managers to
     * communicate with the replication manager.
     * @return The send node description event.
     */
    static Event *getSendNodeDescriptionEvent(DataObjectRef dObj /**< The data object of the node description to send. */, NodeRefList nodeList /**< The nodes to receive the node description. */);

    /**
     * Get a new node stats event--used by the other managers to communicate
     * with the replication manager.
     * @return The node stats event.
     */
    static Event *getNodeStatsEvent(NodeRef node /**< The node that disconnected. */, Timeval duration /**< The duration of the contact. */);

    /**
     * Get a new replicate all event--used by the other managers to communicate
     * with the replication manager.
     * @return The replicate all event.
     */
    static Event *getReplicateAllEvent(double send_delay_s = 0 /**< The delay in seconds to wait before firing the event. */);
};

#endif /* REPL_MANAGER_FACTORY_H */
