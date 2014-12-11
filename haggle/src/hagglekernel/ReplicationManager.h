/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_MANAGER_H
#define _REPL_MANAGER_H

/**
 * The ReplicationManager is responsible for replicating data objects to
 * neighbors. It works in conjunction with the existing ForwardingManager
 * and NodeManager, and is not a replacement. 
 *
 * This manager is a base class and runs a module, `ReplicationManagerModule`
 * which runs in a separate thread and implements most of the replication
 * logic.
 */

class ReplicationManager; /**< The base class that replication managers derive from. */
class ReplicationManagerModule; /**< The asynchronous module that implements the main replication functions. */

#include "Manager.h"
#include "Event.h"

/**
 * This manager is a base class that is responsible for replicating data
 * objects to neighbors. It proxies most of the calls to the asynchronous
 * module.
 */
class ReplicationManager : public Manager { 
private:
    ReplicationManagerModule *module; /**< The module that runs in a separate thread. */
public:
    /**
     * Construct a replication manager. 
     */
    ReplicationManager(HaggleKernel *_kernel) :
        Manager("ReplicationManager", _kernel),
        module(NULL) {};

    /**
     * Deconstruct the replication manager and free the module. 
     */
    ~ReplicationManager();

    /**
     * Fired when loading the configuration file. Sets the parameters
     * for the manager.
     */
    void onConfig(Metadata *m /**< The metadata from the config. */);

    /** 
     * Initialization that occurs after instantiation. 
     * @return  `True` iff initialization was sucessful.
     */
    bool init_derived();

    /**
     * Handler for the `EVENT_TYPE_NODE_UPDATED` event. This occurs when we
     * receive a new node description. 
     */
    void onNodeUpdated(Event *e /**< The event that triggered the handler. */);

    /**
     * Handler for the `EVENT_TYPE_NODE_CONTACT_NEW` event.  This occurs when
     * we learn of a new node.
     */
    void onNodeContactNew(Event *e /**< The event that triggered the new contact. */);

    /**
     * Handler for the `EVENT_TYPE_NODE_CONTACT_END` event. This occurs when
     * a node becomes disconnected. 
     */
    void onNodeContactEnd(Event *e /**< The event that triggered the down contact. */);

    /**
     * Handler for the `EVENT_TYPE_DATAOBJECT_SEND` event. This occurs when
     * a data object is sent.
     */
    void onSend(Event *e /**< The event that triggered the send. */);

    /**
     * Handler for the `EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL` event. This
     * occurs when a data object is successfully sent.
     */
    void onSendSuccessful(Event *e /**< The event that triggered the send success. */);

    
    /**
     * Handler for the `EVENT_TYPE_DATAOBJECT_SEND_FAILURE` event. This occurs
     * when a data object fails to send. 
     */
    void onSendFailure(Event *e /**< The event that triggered the send failure. */);

    /**
     * Handler for the `EVENT_TYPE_DATAOBJECT_DELETED` event. This occurs
     * when a data object is deleted.
     */
    void onDeletedDataObject(Event *e /**< The event that triggered the delete. */);

    /**
     * Handler for the `EVENT_TYPE_DATAOBJECT_NEW` event. This occurs when
     * a new data object is inserted. 
     */
    void onInsertedDataObject(Event *e /**< The event that triggered the insertion. */);

    /**
     * Handler for `EVENT_TYPE_REPL_MANAGER` event. This occurs when a replication
     * event is fired.
     */
    void onReplicationManagerEvent(Event *e /**< The event that triggered the handler. */);

    /**
     * Called prior to calling shutdown.
     */
    void onPrepareShutdown();

    /** 
     * Called when the manager is shut down.
     */
    void onShutdown();

    /**
     * Set the manager module.
     */
    void setModule(ReplicationManagerModule *m /**< The manager module. */);
    
    /**
     * Get the manager module.
     * @return The manager module.
     */ 
    ReplicationManagerModule *getModule();
};

#endif /* _REPL_MANAGER_H */
