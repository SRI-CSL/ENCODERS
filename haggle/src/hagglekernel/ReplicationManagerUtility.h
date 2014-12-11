/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _REPL_MANAGER_UTILITY_H
#define _REPL_MANAGER_UTILITY_H

/**
 * This class implements the utility-based replication pipeline. 
 * The purpose of this class is replicate data objects to neighbors, given
 * certain bandwidth constraints and link duration constraints, so that
 * the overall data object delivery is increased. 
 * The key problem is which data objects should be sent to which neighbors. 
 * We frame the problem as a 0-1 knapsack optimization problem, where we 
 * compute a utility that is a function of both the data object to be
 * replicated, and the node to receive the replica. The cost is the 
 * size of the data object in bytes, and the bag size is the estimated
 * bandwidth delay product between the node and its neighbors. 
 * Users can specify a threshold utility so that <data objects, node> must have
 * at least that utility for the data object to be eligilble for replication
 * to the node.
 * We estimate the bandwidth delay product by nodes exchanging their
 * average connection duration and their average bandwidth, when they meet.
 * These averages are maintained using the `ReplicationNodeDurationAverager`
 * and `ReplicationSendRateAverager` classes.
 * The state of other neighbors' durations and rates are kept in the 
 * `ReplicationConnectState` class.
 * By multiplying the average connection duration and the average bandwidth,
 * we have an estimate for the average bandwidth delay product. 
 * If some other manager decides to send data objects (e.g., the forwarding
 * manager decides to route a data object or flood node descriptions) then
 * the bag size will be reduced to reflect the fact that this data object
 * is sent.
 * In the case where the estimate is too short, the knapsack solver will 
 * switch to a mode where it sends the data object with highest utility
 * first--note that this is solution (highest utility first) is optimal 
 * if there is an infinite bag size.
 * All of the optimization code is implemented in the `ReplicatonOptimizer`
 * class.
 *
 * This manager only decides `what` data objects to replicate, and does not
 * specify the order to replicate them in. This manager is designed to work
 * with the SendPriority manager by using the `ReplicationConcurrentSend` 
 * class. This class ensures that at most certain number of data objects are 
 * queued up to be sent, enabling the SendPriorityManager to re-order this
 * queue according to a partial order. 
 *
 */

class ReplicationManagerUtility;

#include "ReplicationManagerAsynchronous.h"
#include "ReplicationConnectState.h"
#include "ReplicationSendRateAverager.h"
#include "ReplicationNodeDurationAverager.h"
#include "ReplicationConcurrentSend.h"
#include "ReplicationOptimizer.h"

#define REPL_MANAGER_UTILITY_NAME "ReplicationManagerUtility" /**< The name of the utility replication manager. */

#define REPL_MANAGER_DEFAULT_POLL_PERIOD_MS 1200 /**< The default number of ms to poll the utilities and trigger replication. */

#define REPL_MANAGER_AVG_BYTES_PER_SEC "avg_bytes_per_sec" /**< The attribute name used by this manager to send / receive avg send rate statistics. This is included in the node description. */

#define REPL_MANAGER_AVG_CONNECT_MILLIS "avg_connect_millis" /**< The attribute name used by this manager to send /receive avg connection time statistics. This is included in the node description. */

typedef BasicMap<string, Pair<string, int> > ext_send_map_t; /**< The type for a map to keep track of what data objects are in-flight by other managers (e.g., NodeManager and ForwardingManager). */

/**
 * The utility replication manager. Replicates data objects according to
 * a utility optimization problem (0-1 knapsack).
 */
class ReplicationManagerUtility : public ReplicationManagerAsynchronous {

private:

    ReplicationConnectState *connectState; /**< Object that stores the send rate, node duration state for neighbors. */

    ReplicationSendRateAverager *sendRateAverager; /**< Object that stores the send rate satistics for the local node. */

    ReplicationNodeDurationAverager *nodeDurationAverager; /**< Object that stores the node duration statistics for the local node. */

    ReplicationConcurrentSend *concurrentSend; /**< Object to limit the number of data objects that are replicated concurrently (giving the SendPriorityManger time to sort them). */

    ReplicationOptimizer *replicationOptimizer; /**< Object to solve the 0-1 knapsack problem. */

    EventType periodicEventType; /**< Event type for periodic polling. When this event fires the utilities are recomputed and replication may occur. */

    Event *periodicEvent; /**< The periodic polling event. */

    int pollPeriodMs; /**< The period to poll. */

    bool enabled; /**< Flag to enable/disable this manager. */

    Timeval lastReplication;
    int replCooldownMs;

    // statistics
    long dataObjectsInserted; /**< The number of data objects inserted. */

    long dataObjectsDeleted; /**< The number of data objects deleted. */

    long errorCount; /**< The number of errors that occurred. */

    bool runSelfTest; /**< Flag to enable/disable unit tests. */

    ext_send_map_t ext_send_map; /**< Data structure to keep track of the data objects that are in-flight and were sent by another manager (e.g, NodeManager or ForwardingManager). */

    /**
     * Trigger replication towards a neighbor node, if possible.
     */
    void replicateToNeighbor(string nbr_id /**< The id of the neighbor node to trigger replication towards. */); 

    /**
     * Trigger replication towards all neighbors.
     */
    void replicateAllNeighbors();

    /**
     * Helper function to determine which nodes this manager is responsible for.
     * @return `true` if this manager is responsible for maintaining state for
     * this node, `false` otherwise.
     */
    bool isInterestingNode(NodeRef node /**< The node which the manager determines is responsible for. */, bool availableCheck = true /** Check if node is available. */);

    /** 
     * Helper function to determine which data objects this manager is responsible
     * for.
     * @return `true` if this manager is responsible for maintaining state for
     * this data object, `false` otherwise.
     */
    bool isInterestingDataObject(DataObjectRef dObj /**< The data object which the manager determines is responsible for.*/);

public:
    /**
     * Construct a new utility replication manager module.
     */
    ReplicationManagerUtility(ReplicationManager *m /**< The parent manager. */);

    /**
     * Deconstruct the utility replication manager and free its allocated
     * resources.
     */
    ~ReplicationManagerUtility();

    /**
     * Handler for periodic events that trigger utility computation and
     * replication.
     */
    void onPeriodicEvent(Event *e /**< The event that triggered this function to be called. */);

    /**
     * Called during shutdown to perform clean-up.
     */
    void onExit();

    /**
     * Helper function to replicate a data object to a neighbor node.
     */
    void sendDataObjectToNode(NodeRef node /**< The node that the data object is being replicated to. */, DataObjectRef dObj /**< The data object to be replicated. */);

    /**
     * Get the total cost in bytes of the data objects that are in-flight,
     * being sent by other managers (e.g., node manager or forwarding manager. 
     * We reduce the bag size by this amount when performing knapsack optimization.
     * @return The cost in bytes for these data objects to be sent.
     */
    int getExtSendCost(string node_id /**< The node id which has data objects being replicated towards it whose costs in bytes are summed. */);

    /**
     * Determine whether a data object is being replicated towards a node by
     * a separate manager.
     * @return `true` if the data object with id `dobj_id` is being sent
     * towards the neighbor node with id `node_id`.
     */
    bool inExtSendMap(string node_id /**< The node that possibly has a data object being replcated to it. */, string dobj_id /**< The data object that is possibly being replicated. */);

    /**
     * Called when a data object is being replicated towards a node, when
     * triggered by an outside manager (e.g., NodeManager or ForwardingManager).
     * This state is kept track of when computing the bag size.
     */
    void addToExtSendMap(string node_id /**< The node id which is receiving a replica. */, string dobj_id /**< The data object being replicated. */, int dobj_cost /**< The cost of replicating the data object. */);

    /**
     * Remove the data object from the external send map upon sucessful send or
     * failure.
     */
    void removeFromExtSendMap(string node_id /**< The id of the node that received the replica. */, string dobj_id /**< The id of the data object that was replicated. */);

protected:

    /**
     * Configuration handler that is fired when config.xml is loaded.
     * Sets the parameters to the replication manager.
     */
    void _onConfig(const Metadata& m /**< The metadata configuration to initialize the utility replication manager. */);

    /**
     * Handler that is called when a node is first discovered.
     */
    void _notifyNewContact(NodeRef node /**< The newly discovered node. */);

    /**
     * Handler that is called when a node's state is changed.
     */
    void _notifyUpdatedContact(NodeRef node /**< The updated node. */);

    /**
     * Handler that is called when a node is disconnected. 
     */
    void _notifyDownContact(NodeRef node /**< The node that was disconnected. */);

    /**
     * Handler that is called when a new node description is received.
     */
    void _notifyNewNodeDescription(NodeRef n /**< The newly received node description. */);

    /**
     * Handler that is called when a data object is sent.
     */
    void _notifySendDataObject(DataObjectRef dObj /**< The data object that is being sent. */, NodeRefList nl /**< The list of nodes receiving the data object. */);

    /**
     * Handler that is called when a data object is successfully sent.
     */
    void _notifySendSuccess(DataObjectRef dObj /**< The data object that was successfully sent. */, NodeRef n /**< The node that received the data object. */, unsigned long flags /**< Flags to indicate how it was sent. */);

    /**
     * Handler that is called when a data object fails to send.
     */
    void _notifySendFailure(DataObjectRef dObj /**< The data object that failed to send. */, NodeRef n /**< The node that failed to receive the data object. */);

    /**
     * Handler that is called when a data object is inserted.
     */
    void _notifyInsertDataObject(DataObjectRef dObj /**< The data object that was inserted. */);

    /**
     * Handler that is called when a data object is deleted.
     */
    void _notifyDeleteDataObject(DataObjectRef dObj /**< The data object that was deleted. */);

    /**
     * Handler that is called when send statistics are generated by another manager.
     */
    void _notifySendStats(NodeRef node /**< The node that received the data object. */, DataObjectRef dObj /**< The data object that was sent. */, long send_delay /**< The time in ms it took to send. */, long send_bytes /**< The size of the transmission. */);

    /**
     * Handler that is called when a node disconnects.
     */
    void _notifyNodeStats(NodeRef node /**< The node that disconnected. */, Timeval duration /**< The duration of the connection. */);

    /**
     * Handler that is called to trigger replication to all neighbors, if possible.
     */
    void _replicateAll();

    /**
     * Handler that is called when sending a node description--adds the send rate
     * and connection duration statistics. 
     */
    void _sendNodeDescription(DataObjectRef dObj /**< The node description data object. */, NodeRefList n /**< The list of nodes to receive the node description. */);

    /**
     * Handler for the periodic function to compute utilites and replicate data objects. 
     */ 
    void _handlePeriodic();
};

#endif
