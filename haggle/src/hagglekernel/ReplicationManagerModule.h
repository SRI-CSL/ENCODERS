/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _REPL_MANAGER_MODULE_H
#define _REPL_MANAGER_MODULE_H

class ReplicationManagerModule;

#include "ReplicationManager.h"
#include "ManagerModule.h"

/**
 * The `ReplicationManagerModule` is the base class for modules to
 * derive from that the ReplicationManager uses.
 */
class ReplicationManagerModule : public ManagerModule<ReplicationManager> {

protected:

    HaggleKernel *kernel; /**< The haggle kernel to access global data structures and post events. */

public:
    /**
     * Construct a new manager module.
     */
    ReplicationManagerModule(ReplicationManager *m = NULL, string name="ReplicationManagerModule") :
        ManagerModule<ReplicationManager>(m, name),
        kernel(m ? m->getKernel() : NULL) {};

    /**
     * Deconstruct the replication manager module.
     */
    virtual ~ReplicationManagerModule() {};

    /**
     * Handler a replication manager event, which is proxied to the other
     * handlers. 
     */
    void handleReplicationManagerEvent(Event *e /**< The replication manager event that triggered the handler. */);

    /**
     * Handler for when a configuration file is received. The metadata
     * from the file configures the manager.
     */
    virtual void onConfig(const Metadata& m /**< The metadata from the configuration file. */) { };

    /** 
     * Handler for when a new node becomes a neighbor.
     */
    virtual void notifyNewContact(NodeRef n /**< The newly discovered neighbor. */) {};

    /**
     * Handler for when a neighbor node is updated.
     */
    virtual void notifyUpdatedContact(NodeRef n /**< The newly updated node. */) {};

    /**
     * Handler for when a neighbor node is disconnected. 
     */
    virtual void notifyDownContact(NodeRef n /**< The node that was disconnected. */) {};

    /**
     * Handler for when a new node description is received. 
     */
    virtual void notifyNewNodeDescription(NodeRef n /**< The node for the new node description. */) {};

    /**
     * Handler for when a data object is scheduled to be sent. 
     */
    virtual void notifySendDataObject(DataObjectRef dObj /**< The data object that was sent. */, NodeRefList nodes /**< The nodes that received the data object. */) {};

    /**
     * Handler for when a data object is sent successfully.
     */
    virtual void notifySendSuccess(DataObjectRef dObj /**< The data object that was sent successfully. */, NodeRef n /**< The node that recevied the data object. */, unsigned long flags /**< Send flags. */) {};

    /**
     * Handler for when a data object fails to send.
     */
    virtual void notifySendFailure(DataObjectRef dObj /**< The data object that failed to send. */, NodeRef n /**< The node that failed to receive the data object. */) {};

    /**
     * Handler for when a data object is inserted. 
     */
    virtual void notifyInsertDataObject(DataObjectRef dObj /**< The data object that was inserted. */) {};

    /**
     * Handler for when a data object is deleted.
     */
    virtual void notifyDeleteDataObject(DataObjectRef dObj /**< The data object that was deleted. */) {};

    /**
     * Handler for when send statistics are generated. 
     */
    virtual void notifySendStats(NodeRef node /**< The node that sent the data object. */, DataObjectRef dObj /**< The data object that was received. */, long send_delay /**< The delay in milliseconds to send the data object. */, long send_bytes /**< The bytes sent. */) {};

    /**
     * Handler for when new node statistics are generated.
     */
    virtual void notifyNodeStats(NodeRef node /**< The node that disconnected. */, Timeval duration /**< The time connection duration. */) {};

    /**
     * Handler for when a node description should be sent. 
     */
    virtual void sendNodeDescription(DataObjectRef dObj /**< The data object of the node description to send. */, NodeRefList n /**< The neighbors to receive the data object. */) {};

    /**
     * Replicate the data object to all neighbors.
     */
    virtual void replicateAll() {};

    /**
     * Handler that is periodically fired.
     */
    virtual void firePeriodic(){};

    /**
     * Determine whether it is time to shutdown.
     * @return `true` if it's time to shutdown, `false` otherwise.
     */
    virtual bool isDone() { return false; };

    /**
     * Code to stop the running thread.
     */
    virtual void quit() {};

    /**
     * Code to stop the manager module.
     */
    virtual void shutdown() {};
};

#endif /* _REPL_MANAGER_MODULE_H */
