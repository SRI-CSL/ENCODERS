/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */


#ifndef _REPL_MANAGER_ASYNC_H
#define _REPL_MANAGER_ASYNC_H

class ReplicationManagerTask;
class ReplicationManagerAsynchronous;

typedef enum {
    REPL_MANAGER_CONFIG, /**< Type for configuration events. */
    REPL_MANAGER_QUIT, /**< Type for shutdown event. */
    REPL_MANAGER_NEW_CONTACT, /**< Type for new contact event. */
    REPL_MANAGER_UPDATED_CONTACT, /**< Type for updated contact event. */
    REPL_MANAGER_DOWN_CONTACT, /**< Type for node disconnection event. */
    REPL_MANAGER_NEW_NODE_DESCRIPTION, /**< Type for new node description event. */
    REPL_MANAGER_SEND_SUCCESS, /**< Type for when data object is successfully sent. */
    REPL_MANAGER_SEND_FAILURE, /**< Type for when a data object fails to send. */
    REPL_MANAGER_INSERT_DATAOBJECT, /**< Type for when a data object is inserted. */
    REPL_MANAGER_DELETE_DATAOBJECT, /**< Type for when a data object is deleted. */
    REPL_MANAGER_SEND, /**< Type when a data object is to be sent. */
    REPL_MANAGER_SEND_STATS, /**< Type for when send stats are generated. */
    REPL_MANAGER_NODE_STATS, /**< Type for when node connection stats are generated. */
    REPL_MANAGER_SEND_NODE_DESCRIPTION, /**< Type for when a node description is sent. */
    REPL_MANAGER_PERIODIC, /**< Type for periodic computations and replication. */
    REPL_MANAGER_REPLICATE_ALL, /**< Type to trigger replication to all neighbors, if possible. */
} ReplicationManagerTaskType_t; 

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"
#include "ReplicationManagerModule.h"

/**
 * This class is used for threads to post asynchronous events to communicate
 * with a ReplicationManagerModule that runs in a separate thread.
 * This class is essentially a bag of objects that are passed to the thread.
 */
class ReplicationManagerTask {

private:

    const ReplicationManagerTaskType_t type; /**< The asynchronous event type. */

    Metadata *m; /**< Metadata state for an event. */

    DataObjectRef dObj; /**< Data object state for an event. */

    NodeRef node; /**< Node state for an event. */

    NodeRefList nodeList; /**< Node list state for an event. */

    long flags; /**< Flag state for an event. */

    long send_delay; /**< Send delay state for an event. */

    long send_bytes; /**< Send bytes state for an event. */

    Timeval duration; /**< Send duration state for an event. */

public:

    /**
     * Construct a bag for the configuration event.
     */
    ReplicationManagerTask(const Metadata &_m /**< The configuration metadata. */) :
            type(REPL_MANAGER_CONFIG),
            m(_m.copy()),
            dObj(NULL),
            node(NULL),
            nodeList(NULL),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     *  Construct a bag for an event which takes a node.
     */
    ReplicationManagerTask(const ReplicationManagerTaskType_t t /**< The event type. */, NodeRef n /**< Node for the event. */) :
            type(t),
            m(NULL),
            dObj(NULL),
            node(n),
            nodeList(NULL),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Construct a bag for the periodic event.
     */
    ReplicationManagerTask() :
            type(REPL_MANAGER_PERIODIC),
            m(NULL),
            dObj(NULL),
            node(NULL),
            nodeList(NULL),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Construct a bag for an event that has no state.
     */
    ReplicationManagerTask(const ReplicationManagerTaskType_t t /**< The event type. */) :
            type(t),
            m(NULL),
            dObj(NULL),
            node(NULL),
            nodeList(NULL),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Construct a bag for an event that has a data object.
     */
    ReplicationManagerTask(const ReplicationManagerTaskType_t t /**< The event type. */, DataObjectRef _dObj /**< The data object for that event. */) :
            type(t),
            m(NULL),
            dObj(_dObj),
            node(NULL),
            nodeList(NULL),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Construct a bag for an event that has a data object, node and optional
     * flag. 
     */
    ReplicationManagerTask(const ReplicationManagerTaskType_t t /**< The event type. */, DataObjectRef _dObj /**< The data object for that event. */, NodeRef _node /**< The node for the event. */, int _flags = 0 /**< The flags for the event. */) :
            type(t),
            m(NULL),
            dObj(_dObj),
            node(_node),
            nodeList(NULL),
            flags(_flags),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Construct a bag for a send stats event.
     */
    ReplicationManagerTask(NodeRef _node /**< The node that was sent to. */, DataObjectRef _dObj /**< The data object that was sent. */, long _send_delay /**< The send delay, in ms. */, long _send_bytes /**< The bytessent. */) :
            type(REPL_MANAGER_SEND_STATS),
            m(NULL),
            dObj(_dObj),
            node(_node),
            nodeList(NULL),
            flags(0),
            send_delay(_send_delay),
            send_bytes(_send_bytes),
            duration(0) {}

    /**
     * Construct a bag for the send node description event. 
     */
    ReplicationManagerTask(DataObjectRef _dObj /**< The data object for the node. */, NodeRefList _n /**< The nodes to send the data object to. */) :
            type(REPL_MANAGER_SEND_NODE_DESCRIPTION),
            m(NULL),
            dObj(_dObj),
            node(NULL),
            nodeList(_n),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Construct a bag for a node stats event.
     */
    ReplicationManagerTask(NodeRef _node /**< The node that disconnected. */, Timeval _duration /**< The connection duration. */ ) :
            type(REPL_MANAGER_NODE_STATS),
            m(NULL),
            dObj(NULL),
            node(_node),
            nodeList(NULL),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(_duration) {}

    /**
     * Construct a bag for the send event.
     */
    ReplicationManagerTask(NodeRefList _nodes /**< The nodes that are to receive the data object. */, DataObjectRef _dObj /**< The data object that was sent. */) :
            type(REPL_MANAGER_SEND),
            m(NULL),
            dObj(_dObj),
            node(NULL),
            nodeList(_nodes),
            flags(0),
            send_delay(0),
            send_bytes(0),
            duration(0) {}

    /**
     * Get the event type.
     * @return The event type.
     */
    ReplicationManagerTaskType_t getType() {
        return type;
    }

    /**
     * Get the configuration for the event.
     * @return The configuration metadata.
     */
    Metadata *getConfig() {
        return m;
    }

    /** 
     * Get the data object for the event.
     * @return The data object.
     */
    DataObjectRef getDataObject() {
        return dObj;
    }

    /**
     * Get the node for the event.
     * @return The node.
     */
    NodeRef getNode() {
        return node;
    }

    /**
     * Get the node list for the event.
     * @return The node list.
     */
    NodeRefList getNodeList() {
        return nodeList;
    }

    /**
     * Get the flags for the event.
     * @return The flags.
     */
    long getFlags() {
        return flags;
    }

    /**
     * Get the send delay for the event.
     * @return The send delay in milliseconds.
     */
    long getSendDelay() {
        return send_delay;
    }

    /**
     * Get the send bytes for the event.
     * @return The send bytes.
     */
    long getSendBytes() {
        return send_bytes;
    }

    /**
     * Get the duration for the event.
     * @return The duration.
     */
    Timeval getDuration() {
        return duration;
    }

    /**
     * Deconstruct the bag and free its resources.
     */
    ~ReplicationManagerTask() {
        if (m) {
            delete m;
        }
    }
};

/**
 * The `ReplicationManagerAsynchronous` runs in its own thread and proxies
 * thread-safe calls to an event queue that its thread pulls from to process
 * requests. This is an abstract class for other classes to derive from that
 * require asynchronous support.
 */
class ReplicationManagerAsynchronous : public ReplicationManagerModule {

private:

    Mutex mutex; /**< The lock that guards the thread safe functions. */

    bool _isDone; /**< A flag that indicates whether the thread is running. */

protected:

    GenericQueue<ReplicationManagerTask *> *taskQ; /**< The task queue for events to be processed. */

    /**
     * Run loop for the thread.
     * @return false on exit.
     */
    bool run(void);

    /**
     * onConfig handler that runs in the module thread.
     */
    virtual void _onConfig(const Metadata& m /**< Configuration metadata to configure the module. */) {};

    /**
     * notifyNewContact handler that runs in the module thread.
     */
    virtual void _notifyNewContact(NodeRef node /**< The newly connected node. */) {};

    /**
     * notifyUpdateContact handler that runs in the module thread.
     */
    virtual void _notifyUpdatedContact(NodeRef node /**< The node that was updated. */) {};

    /**
     * notifyDownContact handler that runs in the module thread.
     */
    virtual void _notifyDownContact(NodeRef node /**< The node that was disconnected. */) {};

    /**
     * notifyNewNodeDescription handler that runs in the module thread.
     */
    virtual void _notifyNewNodeDescription(NodeRef n /**< The node that we just received a new node description from. */) {};

    /**
     * notifySendDataObject handler that runs in the module thread.
     */
    virtual void _notifySendDataObject(DataObjectRef dObj /**< The data object being sent. */, NodeRefList nodes /**< The nodes receiving the data object. */) {};

    /**
     * notifySendSuccess handler that runs in the module thread.
     */
    virtual void _notifySendSuccess(DataObjectRef dObj /**< The data object that was successfully sent. */, NodeRef n /**< The node that received the data object. */, unsigned long flags /**< The send flags. */) {};

    /**
     * notifySendFailure handler that runs in the module thread.
     */
    virtual void _notifySendFailure(DataObjectRef dObj /**< The data object that failed to send. */, NodeRef n /**< The node that failed to send the data object. */) {};

    /**
     * notifyInsertDataObject handler that runs in the module thread.
     */
    virtual void _notifyInsertDataObject(DataObjectRef dObj /**< The data object that was inserted. */) {};

    /**
     * notifyDeleteDataObject handler that runs in the module thread.
     */
    virtual void _notifyDeleteDataObject(DataObjectRef dObj /**< The data object that was deleted. */) {};

    /**
     * notifySendStats handler that runs in the module thread.
     */
    virtual void _notifySendStats(NodeRef node /**< The node that received the data object. */, DataObjectRef dObj /**< The data object that was sent. */, long send_delay /**< The delay in milliseconds to send it. */, long send_bytes /**< The number of bytes sent. */) {};

    /**
     * notifyNodeStats handler that runs in the module thread.
     */
    virtual void _notifyNodeStats(NodeRef node /**< The node that disconnected. */, Timeval duration /**<  The duration of the contact. */) {};

    /**
     * sendNodeDescription handler that runs in the module thread.
     */
    virtual void _sendNodeDescription(DataObjectRef dObj /**< The data object of the node description. */, NodeRefList n /**< The nodes receiving the node description. */) {};

    /**
     * replicateAll handler that runs in the module thread.
     */
    virtual void _replicateAll() {};

    /**
     * handlePeriodic handler thatn runs in the module thread.
     */
    virtual void _handlePeriodic() {};

public:
    /**
     * Construct a new asynchronous replication manager.
     */
    ReplicationManagerAsynchronous(
        ReplicationManager *m,
        string name,
        GenericQueue<ReplicationManagerTask *> *_taskQ 
            = new GenericQueue<ReplicationManagerTask *>())
        : ReplicationManagerModule(m, name),
          _isDone(false),
          taskQ(_taskQ) { };

    /**
     * Deconstruct the replication manager, free the queues and allocated
     * data structures.
     */
    virtual ~ReplicationManagerAsynchronous();

    /**
     * Get the task queue with the events to be processed.
     */
    GenericQueue<ReplicationManagerTask *> *getTaskQueue() {
        return taskQ;
    }
    
    /**
     * Code to stop the running thread.
     */
    virtual void quit();

    /**
     * Code executed within the thread during thread shutdown.
     */
    void cleanup();

    /**
     * A handler that subclasses can use to cleanup when the thread is shutting down.
     */
    virtual void onExit() {};

    /**
     * Determine whether it is time to shutdown.
     * @return `true` if it's time to shutdown, `false` otherwise.
     */
    virtual bool isDone() { return _isDone; }
    
    /**
     * Handler for when a configuration file is received. The metadata
     * from the file configures the manager.
     */
    void onConfig(const Metadata& m /**< The metadata from the configuration file. */);

    /** 
     * Handler for when a new node becomes a neighbor.
     */
    void notifyNewContact(NodeRef n /**< The newly discovered neighbor. */);

    /**
     * Handler for when a neighbor node is updated.
     */
    void notifyUpdatedContact(NodeRef n /**< The newly updated node. */);

    /**
     * Handler for when a neighbor node is disconnected. 
     */
    void notifyDownContact(NodeRef n /**< The node that was disconnected. */);

    /**
     * Handler for when a new node description is received. 
     */
    void notifyNewNodeDescription(NodeRef n /**< The node for the new node description. */);

    /**
     * Handler for when a data object is scheduled to be sent. 
     */
    void notifySendDataObject(DataObjectRef dObj /**< The data object that was sent. */, NodeRefList nodes /**< The nodes that received the data object. */);

    /**
     * Handler for when a data object is sent successfully.
     */
    void notifySendSuccess(DataObjectRef dObj /**< The data object that was sent successfully. */, NodeRef n /**< The node that recevied the data object. */, unsigned long flags /**< Send flags. */);

    /**
     * Handler for when a data object fails to send.
     */
    void notifySendFailure(DataObjectRef dObj /**< The data object that failed to send. */, NodeRef n /**< The node that failed to receive the data object. */);

    /**
     * Handler for when a data object is inserted. 
     */
    void notifyInsertDataObject(DataObjectRef dObj /**< The data object that was inserted. */);

    /**
     * Handler for when a data object is deleted.
     */
    void notifyDeleteDataObject(DataObjectRef dObj /**< The data object that was deleted. */);

    /**
     * Handler for when send statistics are generated. 
     */
    void notifySendStats(NodeRef node /**< The node that sent the data object. */, DataObjectRef dObj /**< The data object that was received. */, long send_delay /**< The delay in milliseconds to send the data object. */, long send_bytes /**< The bytes sent. */);

    /**
     * Handler for when new node statistics are generated.
     */
    void notifyNodeStats(NodeRef node /**< The node that disconnected. */, Timeval duration /**< The time connection duration. */);

    /**
     * Handler for when a node description should be sent. 
     */
    void sendNodeDescription(DataObjectRef dObj /**< The data object of the node description to send. */, NodeRefList n /**< The neighbors to receive the data object. */);

    /**
     * Replicate the data object to all neighbors.
     */
    void replicateAll();

    /**
     * Handler that is periodically fired.
     */
    void firePeriodic();
};

#endif /* _REPL_MANAGER_ASYNC_H */
