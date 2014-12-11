/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

/** @file InterestManager.h
 * This manager is responsible for aggregating local application subscriptions 
 * and inserting them into an interest data object which is subsequently
 * amenable to forwarding and caching. 
 * Interest data objects are pushed out periodically (with an incrementing
 * time stamp) or when an interest changes (i.e. an interest is added or
 * removed). 
 * By making interests a first class data object, they are subject to the
 * standard routing and caching policies. For example, one could use
 * total order replacement and absolute ttl caching policies, with
 * epidemic routing for interest dissemination.
 * Upon receiving an interest data object, the encoded application node
 * descriptions are extracted and inserted. 
 * If the interest data object is deleted from the database, then the
 * application node descriptions that it contained will also be deleted.
 * 
 * This manager uses the semantics branch to separate the dissemination
 * of bloomfilters from interests. The purpose of this separation is because 
 * ENCODERS performs exact match queries using the Drexel code (via a cuid) 
 * and does not need multihop bloomfilters to inhibit sending data objects
 * a node has already received (they would unsubscribe to the cuid).
 *
 * Interest data objects have the following attributes:
 * 
 *  1. `Interests=Aggregate`
 *  2. `SeqNo=sequence_no`
 *  3. `ATTL_MS=duration_ms`
 *
 * And they have the following metdata that encodes the interests for a
 * particular application node: 
 *
 *     <VNode an="`node_name`" mm="`max_matches`" mt="`match_threshold`" ct="`create_time`" id="`node_id`" >
 *         <a n="`attr_key=attr_val`" w="`attr_weight`" />
 *         ...
 *     </VNode>
 *
 * This class uses a new event type raised by the Application Manager: 
 *   EVENT_TYPE_APP_NODE_CHANGED 
 * that is fired whenever an application's interest changes. 
 *
 * An example Interest Manager configuration is:
 * 
 *     <InterestManager enable="true" interest_refresh_period_ms="30000" interest_refresh_jitter_ms="500" absolute_ttl_ms="90000" consume_interests="true" />
 *
 * Here, interests will be periodically refreshed after 30 seconds, with half a second of jitter. After
 * 90 seconds the interests will be removed (if the appropiate caching settings are enabled).
 *
 * The following configuration options are supported:
 * 
 * - `enable` 
 *   
 *   Set to `true` to enable the interest manager
 *
 * - `interest_refresh_period_ms` 
 * 
 *   Interest refresh period in milliseconds.
 *
 * - `interest_refresh_jitter_ms`
 *  
 *   Interest jitter in milliseconds.
 * 
 * - `absolute_ttl_ms`
 * 
 *   Absolute ttl in milliseconds for an interest data object.
 *
 * - `debug` 
 *
 *    Set to `true` to enable extra verbose debug messages.
 *
 * - `run_self_tests`
 * 
 *    Set to `true` to run basic unit tests.
 *
 * - `consume_interests`
 *
 *    Set to `true` to consume interests on data object match and forward.
 * 
 * Example caching policies for the interest manager are as follows:
 *
 *      <CacheStrategy name="CacheStrategyUtility">
 *            <CacheStrategyUtility knapsack_optimizer="CacheKnapsackOptimizerGreedy" global_optimizer="CacheGlobalOptimizerFixedWeights" utility_function="CacheUtilityAggregateMin" max_capacity_kb="10240" watermark_capacity_kb="5120" compute_period_ms="200" purge_poll_period_ms="1000" purge_on_insert="true" publish_stats_dataobject="false" keep_in_bloomfilter="true" handle_zero_size="true"> 
 *               <CacheKnapsackOptimizerGreedy />
 *               <CacheGlobalOptimizerFixedWeights min_utility_threshold="0.1">
 *                   <Factor name="CacheUtilityAggregateMin" weight="1" />
 *                   <Factor name="CacheUtilityReplacementTotalOrder" weight="1" />
 *                   <Factor name="CacheUtilityPurgerAbsTTL" weight="1" />
 *               </CacheGlobalOptimizerFixedWeights>
 *               <CacheUtilityAggregateMin name="CacheUtilityAggregateMin">
 *                   <Factor name="CacheUtilityReplacementTotalOrder">
 *                           <CacheUtilityReplacementTotalOrder metric_field="SeqNo" id_field="Origin" tag_field="Interests" tag_field_value="Aggregate" />
 *                   </Factor>
 *                   <Factor name="CacheUtilityPurgerAbsTTL">
 *                       <CacheUtilityPurgerAbsTTL purge_type="ATTL_MS" tag_field="Interests" tag_field_value="Aggregate" />
 *                   </Factor>
 *               </CacheUtilityAggregateMin>
 *           </CacheStrategyUtility>
 *      </CacheStrategy>
 *
 * This will use total order replacement on interest data objects (fresher interest data objects
 * replace staler interest data objects), and it will use the absolute TTL specified 
 * in the `<InterestManager>` settings to purge old interest data objects.
 *
 * Example forwarder policies for interest data objects are as follows:
 *
 *      <ForwardingManager max_nodes_to_find_for_new_dataobjects="30" max_forwarding_delay="2000"
 *             node_description_retries="0" dataobject_retries="1" dataobject_retries_shortcircuit="2" 
 *             query_on_new_dataobject="true" periodic_dataobject_query_interval="0" 
 *             enable_target_generation="false" push_node_descriptions_on_contact="false"
 *             load_reduction_min_queue_size="500" load_reduction_max_queue_size="1000" enable_multihop_bloomfilters="false">
 *          <ForwardingClassifier name="ForwardingClassifierAttribute">
 *	            <ForwardingClassifierAttribute class_name="flood" attribute_name="Interests" attribute_value="Aggregate" />
 *          </ForwardingClassifier>
 *          <Forwarder protocol="Flood" contentTag="flood" />
 *	        <Forwarder protocol="AlphaDirect" />
 *      </ForwardingManager>
 *
 * This will propagate interest data objects epidemically 
 * (make sure `enable_mutlihop_bloomfilters="false"`!).
 *
 * We strongly suggest that the following configuration is set:
 * 
 *     <ApplicationManager delete_state_on_deregister="true" />
 *
 * This will enable applications to register and deregister, and trigger the appropriate
 * interest refresh.
 *
 * We have added several new event types that _ONLY_ the InterestManager uses:
 *
 *   'EVENT_TYPE_ROUTE_TO_APP' : Triggered when forwarding routes to a 
 *      remote application node (direction of interest). Used for the 
 *      `consume_interest` option.
 *
 *   'EVENT_TYPE_APP_NODE_CHANGED' : Triggered whenever an application adds or 
 *      removes an interest. Also triggered when an application deregisters.
 *
 * The `NodeManager` should have the following parameter set for `Node` to disable
 * forwarding of application node descriptions:
 *
 *    <Node send_app_node_descriptions="false" >
 *
 */
#ifndef _INTERESTMANAGER_H
#define _INTERESTMANAGER_H

/**
 * Manager responsible for efficient interest propagation. 
 */
class InterestManager;

#include "Event.h"
#include "Manager.h"
#include "Filter.h"

#define FILTER_INTERESTS "Interests=*" // interest data objects contain this attribute

class InterestManager : public Manager
{	
private:
    bool enabled; /**< Flag to enable or disable the interest manager & aggregation. */
    bool debug; /**< Flag to enable or disable verbose debug printing. */
    bool consumeInterest; /**< Flag to enable or disable consuming the interest when forwarding towards the subscriber. */
    EventType interestEType; /**<  Event type that is fired when an interest data object is received. */
    EventType periodicInterestRefreshEventType; /**< Event type that is fired periodically create new interest data objects. */
    Event *periodicInterestRefreshEvent; /**< Actual event for `periodicInterestRefreshEventType`. */
    int interestRefreshJitterMs; /**< Jitter in ms for periodic interest refresh. */
    int interestRefreshPeriodMs; /**< Period in ms for periodic interest refresh. */
    int absoluteTTLMs; /**< absolute TTL in ms for the interest data object. */
    long TTL; /**< Sequence number for the next interest data object to be sent out. */
    int prevNodes; /**< How many application nodes where sent in the last interest data object, this is is used to suppress empty interest data objects in some situations. **/

    DataObjectRef pushObj; /**< The next interest data object to push out to the network. */

    /**
     * Create node description data objects from an interest data object. 
     * @return List of application node descriptions embedded in the data object.
     */
    DataObjectRefList parseReceiveDataObjects(DataObjectRef &dObj /**< [in] Parse an incoming data object for application node descriptions. */); 

    /**
     * Create a new interest data object from a list of local application nodes. 
     * @return An interest data object containing local application node descriptions.
     */
    DataObjectRef createInterestDataObjectFromApplications(NodeRefList &localApplications /**< [in] Local applications to include in the interest data object. */); 

    /**
     * Handler for when an interest data object is received. 
     * It is parsed and the local application node descriptions are inserted.
     */
    void notifyReceiveDataObjects(DataObjectRef &dObj /**< [in] The received interest data object. */); 

    // statistics:
    long interestDataObjectsCreated; /**< Number of interest data objects created. */
    long interestNodesCreated; /**< Number of interest nodes created. */
    long interestDataObjectsReceived; /**< Number of interest data objects received (remote). */
    long interestNodesReceived; /**< Number of application node descriptions received. */

    /**
     * A helper function to convert application node information into metadata. 
     * @return Metadata describing the node.
     */
    Metadata *createAppNodeMetadata(
        Timeval ct,  /**< Create time. */
        string appname, /**< Application name. */
        string id,  /**< Application ID. */
        int maxMatches,  /**< Maximum nuber of matches. */
        int matchThreshold, /**< Matching threshold. */
        string parentId, /**< Parent ID. */
        Attributes attrs /**< Node interests. */); 

    bool selfTest1(); /**< Simple unit test. */
    bool selfTests(); /**< Run all unit tests. */
protected:
    /**
     * Event handler called when a new interest data object is received. 
     */
    void onReceiveInterestDataObject(Event *e /**< [in] Event containing the received data object. */); 

    /** 
     * Event handler called when a local application's interests change.
     */
    void onInterestUpdate(Event *e /**< [in] Event indicating the interest change. */);

    /**
     * Event handler called when a data object is deleted.
     */
    void onDeletedDataObject(Event *e /**< [in] Event indicating that the data object was deleted. */);

    /**
     * Event handler for a periodic interest push.
     */ 
    void onPeriodicInterestRefresh(Event * e /**< [in] Event triggering the refresh. */); 

    /**
     * Event handler for when a data object is forwarded destined for a node.
     * This event is used to consume matching interests upon forwarding.
     */ 
    void onForwardDataObject(Event *e /**< [in] Event describing the forward. */); 

    /**
     * Event handler called during shutdown. Clears data structures.
     */
    void onPrepareShutdown();

    /**
     * Initialize a new InterestManager during start-up. 
     * @return True if initializion was successful, false otherwise.
     */
    bool init_derived();

    /**
     * Configuration config.xml handler during start-up.
     */
    void onConfig(Metadata *m /**< [in] Metadata representation of config.xml. */); 

    /**
     * Shutdown handler to clear remaining data structures.
     */
    void onShutdown();
public:
    /** 
     * Construct an InterestManager.
     */
    InterestManager(HaggleKernel *_haggle = haggleKernel); 
    /**
     * Deconstruct the InterestManager.
     */
    ~InterestManager();
};

#endif /* _INTERESTMANAGER_H */
