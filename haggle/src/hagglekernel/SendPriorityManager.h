/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _SENDPRIORITYMANAGER_H
#define _SENDPRIORITYMANAGER_H

/**
 * The SendPriorityManager class enables the user to specify an ordering
 * of priority for data objects to be sent. This ordering is formulated
 * as a partial order, specified in the configuration file. Partial orders
 * can be composed of other partial orders, and multiple different types
 * of partial orders can be combined to define flexible prioritization. 
 *
 * This mechanism is important when links have limited bandwidth and are 
 * connected for small durations--the most important data should be sent
 * first. This manager intercepts data objects before they are placed on the
 * protocol queue, and it maintains its own queues which are drained into the
 * protocol queue. 
 * When sending a data object, a manager will traditionally raise the 
 * DATAOBJECT_SEND event. Instead, this manager (e.g., ForwardingManager) 
 * sends the EVENT_TYPE_SEND_PRIORITY which is received by the PriorityManager. 
 * The received data object is placed on a queue that constructs a topological
 * order of set of queued data objects according to the partial order 
 * specification. 
 * The manager maintains a number of send stations which are data objects that
 * are presently being sent by a protocol. If all of the send stations are in 
 * use, then the data object is queued--otherwise, it is sent immediately. 
 * If a data object is sent successfully, or fails to send, then the Protocol
 * will raise the SEND_SUCCESS/SEND_FAILURE event, as well as the new event
 * SEND_PRIORITY_SUCCESS/SEND_PRIORITY_FAILURE to indicate to the manager
 * that the data object can be removed from the queue, and a new data object
 * can be added. 
 * The send stations are maintained for each node destination.
 *
 * To construct a topological order given the partial order, we implemented
 * a heap whose elements higher in the heap must be sent before elements
 * lower in the heap. This is effectively a Hesse Diagram or Graph.
 * Upon element insertion, it is pushed down (placed at the greatest depth) 
 * as far as possible, while respecting the partial order specification.
 * This algorithm has O(n) insertion, and O(1) deletion.  
 *
 * To enable the SendPriorityManager, one needs to enable it in config.xml.
 * The configuration options are as follows:
 *
 *  <SendPriorityManager enable="true" run_self_tests="true" partial_order_class="PartialOrderFIFO">
 *    <PartialOrderFIFO />
 *  </SendPriorityManager>
 *
 * where:
 * enable - "true" to enable the send priority manager, "false" otherwise.
 * run_self_tests - "true" to run the unit tests, "false" otherwise.
 * partial_order_class - The name of the partial order class, whose subsequent 
 * metadata is used to initialize the class.
 *
 * The following partial order specifications are supported:
 *
 * PartialOrderFIFO : a FIFO queue that behaves similarly as how haggle 
 * originally behaved, without the send stations. No options
 *
 * PartialOrderAttribute : data objects with the specified attribute have
 * priority (>) over data objects without the attribute. Data objects either
 * both with the attribute, or without the attribute are incomparable (|). 
 * Has the following options:
 *  `attribute_name`:
 *    Set to the attribute name which gives priority to data objects with that
 *    attribute over data objects without. 
 *
 * PartialOrderPriorityAttribute : uses the integer value of attributes with a 
 * specific name to order said data objects--higher values have greater priority
 * than lower values. If one (or both) of the data objects do not have the
 * specified attribute, then they are incomparable (|).
 * Has the following options:
 * `priority_attribute_name`:
 *   Set to the attribute name whose value is interpreted as an interger for
 *   prioritization.
 *
 */

class SendPriorityManager;

#include "Event.h"
#include "Manager.h"
#include "Protocol.h"

class SendPriorityElement; /**< The class to store the state necessary to send a data object. */
class TopologicalSorter; /**< The class responsible for generating a topological sorted order given a partial order. */
class SendQueue;

/**
 * The manager responsible for ordering the send queues by a Partial Order.
 */
class SendPriorityManager : public Manager
{	
private:
    bool enabled; /**< Enable sending data objects by priority. */
    bool debug; /**< Enable verbose debugging messages. */

    long numSendSuccess; /**< The number of data objects that successfully sent. */
    long numSendFailure; /**< The number of data objects that failed to send. */

    SendQueue *sendQueue; /**< Maintains the queues for each receiving node, ordered by the partial order. */
    
    /**
     * Verifies that the Partial Order prioritization works correctly. 
     * Data objects with higher prioritization are sent before data objects
     * with lower prioritization. 
     * @return True if the Partial Order prioritization unit test passes, 
     *  False otherwise.
     */
    bool selfTest1();

    /**
     * Verifies that the default FIFO prioritization works correctly.
     * Data objects will be sent in order of their insertion into the send queue.
     * @return True if the FIFO prioritization unit tests passes, False otherwise.
     */
    bool selfTest2();

    /**
     * Verifies that the ordered combiner partial order works correctly.
     * @return True if the ordered combiner unit tests passes, False otherwise.
     */
    bool selfTest3();

    /**
     * Verifies that the attribute partial order works correctly.
     * @return True if the attribute unit tests passes, False otherwise.
     */
    bool selfTest4();

    /**
     * Verifies that the complex partial order works correctly.
     * @return True if the attribute unit tests passes, False otherwise.
     */
    bool selfTest5();

    /**
     * Runs all the unit tests if the `run_self_tests` parameter is set to
     * `true` in the configuration file. 
     * @return True if all of the tests PASSED, False otherwise. 
     */
    bool selfTests();

    /**
     * Shutdown handler to clear remaining data structures.
     */
    void onShutdown();
protected:
    /** 
     * Handler for a send success. Removes the DO that was sent from the send
     * queue and queues up another DO to send.
     */
    void onSendSuccessful(Event *e /**< [in] The event that triggered the send success. */);

    /** 
     * Handler for a send failure. Removes the DO to be sent from the send
     * queues and queues up another DO to send.
     */
    void onSendFailure(Event *e /**< [in] The event that triggered the send failure. */);

    /**
     * Handler for the EVENT_TYPE_SEND_PRIORITY event raised by the ProtocolManager
     * to send a data object.  Places the data object on the send queues which
     * are ordered by priority.
     */
    void onSendPriority(Event *e /**< [in] The event that triggered the send. */);


    /**
     * Configuration config.xml handler during start-up.
     */
    void onConfig(Metadata *m /**< [in] Metadata representation of config.xml. */); 

    /**
     * Initialize a new SendPriorityManager during start-up. 
     * @return True if initializion was successful, false otherwise.
     */
    bool init_derived();
public:
    /** 
     * Construct a SendPriorityManager.
     */
    SendPriorityManager(HaggleKernel *_haggle = haggleKernel); 

    /**
     * Deconstruct the SendPriorityManager.
     */
    ~SendPriorityManager();

    /**
     * Get the event used to queue a data object for sending.
     * @return The event containing information used by the SendPriorityManager
     *  to process the request.
     */
    static Event *getSendPriorityEvent(DataObjectRef dObj, NodeRef target, EventType sendCallback);

    /**
     * Get the event used to dequeue a data object that was sucessfully sent, 
     * and queue up another data object to send.
     * @return The event containing information used by the SendPriorityManager
     *  to process the request.
     */
    static Event *getSendPrioritySuccessEvent(DataObjectRef dObj, NodeRef target);

    /**
     * Get the event used to dequeue a data object that was unsucessfully sent, 
     * and queue up another data object to send.
     * @return The event containing information used by the SendPriorityManager
     *  to process the request.
     */
    static Event *getSendPriorityFailureEvent(DataObjectRef dObj, NodeRef target);
};

/**
 * Stores a Hesse Graph for the partial order, which is used as a heap to fetch
 * elements off in topological order.
 */
class PartialOrderElement
{
private:
    List<SendPriorityElement *> equivObjs; /**< The set of equivalent data objects (equivlance class). */
    List<PartialOrderElement *> children; /**< The set of elements that are less than the current data object. */
    int parentCount; /**< The set of elements D such that ``d in D iff d > c and there does not exist an element x such that x > c and x < d. */
    bool is_inf; /**< True if the partial order element represents the root of the diagram (> all other elements), False otherwise. */
public:

    /**
     *  Construct a new node in the Hesse Graph, given a list of equivalent
     *  data objects as send priority elements. 
     */
    PartialOrderElement(List<SendPriorityElement *> *objs /**< [in] The set of equivalent send priority elements to be wrapped. **/);

    /**
     * Construct a new node in the Hesse Graph, given an individual send priorty
     * element.
     */
    PartialOrderElement(SendPriorityElement *se /**< [in] An individual send priorty element to be wrapped. **/);

    /**
     * Get the list of send priority elements whose data objects are equivalent.
     */
    void getEquivObjects(List<SendPriorityElement *> *o_equivObjects /**< [out] The list to fill with equivalent send priority elements. */);

    /**
     * Add a child partial order element--the child must be immediately 
     * succeeding the current element.
     */
    void addChild(PartialOrderElement *childToAdd /**< [out] The child to add to the children list. */);

    /** 
     * Add a list of children.
     */
    void addChildren(List<PartialOrderElement *> *childrenToAdd /**< [in] Add a list of immediate children to this element. */);

    /**
     * Add an equivalent send priority element. 
     */
    void addEquivalent(SendPriorityElement *se /**< [in] An individual equivalent send priority element to be added. */);

    /**
     * Add a list of equivalent send priorty elements. 
     */
    void addEquivalents(List<SendPriorityElement *> *seles /**< [in] Add a list of equivalent send priority elements. */);

    /**
     * Get the number of equivalent data objects.
     */
    int getNumEquivalent();

    /**
     * Remove a send priorty element from the equivalent list and return it.
     * @return The popped equivalent send priority element.
     */ 
    SendPriorityElement *popEquivalent();

    /**
     * Remove a send priorty element from the equivalent list and return it.
     * @return The popped equivalent send priority element.
     */ 
    SendPriorityElement *peekEquivalent();

    /** 
     * Get the list of immediate children.
     */
    void getChildren(List<PartialOrderElement *> *o_children /**< [out] The list to populate with immediate children of the current element. */);

    /**
     * The list of child partial order elements to remove.
     */
    void removeChild(PartialOrderElement *childToRemove /**< [in] The children to remove. */);

    /**
     * Remove a child from the children list and return it.
     */
    PartialOrderElement *popChild();

    /**
     * Remove children that are in a list of elements from the children list.
     */
    void removeChildren(List<PartialOrderElement *> *childrenToRemove);

    /** 
     * Get the number of children.
     */
    int getNumChildren();

    /**
     * Clone the current element and all of its children.
     */
    PartialOrderElement *makeCopyWithoutParent(); 

    /**
     * Increment the parent count, used when this element is added as a child
     * for another parent.
     */
    void incrementParentCount();

    /**
     * Decrement the parent count, used when this element is removed as a child
     * from another parent.
     */
    void decrementParentCount();
    
    /**
     * Get the parent count.
     * @return The number of parents this element has.  
     */
    int getParentCount();

    /**
     * Check with the element is an infinity element.
     * @return True if it is an infinity element, False othewise.
     */
    bool isInf();

    /**
     * Get the create time of the element.
     * @return The time at which the element was created.
     */
    Timeval getCreateTime();
};

/**
 * Interface for classes that define a Partial Order.
 * Recall that a partial order is a relation "<=" such that is 
 * 1) reflxive,
 * 2) antisymmetric 
 * 3) transitive 
 *
 * We also define an element Inf, such that for all elements x, X < Inf.
 * Inf serves as the root of our Hasse Graph. 
 */
class PartialOrder 
{
public:
    static const int LT = -1; /**< Strictly less than <. */
    static const int GT = 1;  /**< Strictly greater than >. */
    static const int EQ = 0;  /**< Equal =. */
    static const int NC = -2; /**< Not comparable. */

    /**
     * Construct the partial order.
     */
    PartialOrder();
    /**
     * Free the partial order.
     */
    virtual ~PartialOrder();

    /** 
     * Check if an element is Inf.
     * @return True if the element is Inf, False otherwise.
     */
    bool isInf(PartialOrderElement *a /**< The element being checked if Inf. */);

    /**
     * Compare two elements.
     * @return LT if a < b, GT if a > b, EQ if a = b, NC if a is not comparable to b
     */
    virtual int compare(PartialOrderElement *a /**< [in] The first element to the binary relation. */, PartialOrderElement *b /** [in] The second element to the binary relation. */);
};

/**
 * Class responsible for initializing and instantiating partial orders.
 */
class PartialOrderFactory
{
public:
/**
 * Instantiate and initialize a partial order from its class name 
 * and metadata.
 * @return The initialized partial order.
 */
static PartialOrder *getPartialOrderForConfig(
        string name /**< [in] The name of the partial order class to load. */, 
        Metadata *m /**< [in] Metadata containing configuration setttings for the partial order. */);
};

/**
 * The number of parallel data objects to be sent to a receiving node.
 */
typedef List<SendPriorityElement *> send_stations_t; 

/**
 * The send queue state for each receiving node.
 */
typedef struct {
    TopologicalSorter *topSorter; /** The class used to re-order the queues. */
    send_stations_t *sendStations; /** The data objects currently being sent. */
    int numActiveSendStations; /** The number of data objects currently being sent. */
} send_state_t;

/**
 * Queues sorted by priority for sending data objects to neighbors.
 */
class SendQueue {
private:

    HaggleKernel *kernel; /**< The haggle kernel used to post events. */
    string partialOrderName; /**< The name of the partial order that order the queues. */
    Metadata *partialOrderMetdata; /**< The metadata to initialize the partial order. */

    int parallelFactor; /**< Maximum number of concurrent data object transmissions. */

    typedef HashMap<string, send_state_t*> senders_t;
    senders_t senders; /**< Concurrent send queues. */

    /**
     * Get the send state responsible for the <data object, node> pair. 
     * @return The send state responsible for the <data object, node> pair.
     */
    send_state_t *getSendState(
        DataObjectRef dObj, /**< The data object whose queue state is fetched. */
        NodeRef node /**< The node whose queue state is fetched. */);

    /**
     * Create send state responsible for the <data object, node> pair.
     * @return The send state responsible for the <data object, node> pair.
     */
    send_state_t *createSendState(
        DataObjectRef dObj, /**< The data object whose send state is created. */
        NodeRef node /**< The node whose send state is created. */);

    /**
     * Remove send state responsible for the <data object, node> pair.
     */
    void removeSendState(
        DataObjectRef dObj, /**< The data object whose state is being removed. */
        NodeRef node /**< The node whose state is being removed. */);

    /** 
     * Queue and send data objects for a send state.
     */
    void startSend(send_state_t *sendState /**< The send state whose data objects will be sent. */);
        
public:
    /**
     * Construct a new send queue.
     */
    SendQueue(
        HaggleKernel *_kernel, /**< The haggle kernel used to post events. */
        string partialOrderName,  /**< The name of the partial order used to order the queues. */
        Metadata *partialOrderMetdata, /**< The metadata used to initialize the partial order. */
        int _parallelFactor /**< The max number of concurrent data object transmissions. */);

    /**
     * Deconstruct a send queue.
     */
    ~SendQueue();

    /**
     * Add a send priority element to be sent.
     */
    void addSendPriorityElement(
        SendPriorityElement *se /**< The send priority element to be sent. */);

    /**
     * Remove a send priority element from the send queue. 
     * @return true if the element was successfully removed, false otherwise.
     */
    bool removeSendPriorityElement(
        SendPriorityElement *se /**< The send priority element to be removed. */);

    void handleEndNeighbor(NodeRef nbr);
};

#endif /* _SENDPRIORITYMANAGER_H */
