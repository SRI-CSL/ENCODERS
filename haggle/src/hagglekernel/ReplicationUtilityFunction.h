/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#ifndef _REPL_UTIL_FUNC_H
#define _REPL_UTIL_FUNC_H

/**
 * The replicaiton utility function is responsible for computing a 
 * utility value for replicating a data object with id `dobjd_id` to
 * a node with id `node_id`. Utility functions can be composed arbitrarily,
 * and are specified by the user in the configuration file `config.xml`.
 * A utility returns a value between 0 and 1, where 1 has the highest
 * possible utility, and 0 has no utility.
 * A global optimizer may be passed which adjusts the weights given to
 * each utility function.
 * This utility is used by the `ReplicationOptimizer` and 
 * `ReplicationKnapsackOptimizer` when computing which data objects to
 * replicate.
 */
class ReplicationUtilityFunction;
class ReplicationUtilityAggregate;
class ReplicationUtilityAggregateSum;
class ReplicationUtilityAggregateMin;
class ReplicationUtilityAggregateMax;
class ReplicationUtilityRandom;
class ReplicationUtilityNOP;
class ReplicationUtilityDOExists;
class ReplicationUtilityWait;
class ReplicationUtilityAttribute;
class ReplicationUtilityLocal;
class ReplicationUtilityNeighborhoodOtherSocial;

#include "HaggleKernel.h"
#include "ReplicationGlobalOptimizer.h"
#include "ReplicationManager.h"

/**
 * The main base class that all replication utility functions derive from.
 */
class ReplicationUtilityFunction {

private:
    
    ReplicationManager *manager; /**< The parent replication manager. */ 

    string name; /**< The name of the utility function. */

    ReplicationGlobalOptimizer *globalOptimizer; /**< The global optimizer which may adjust the weights of the utility function. */

public:

    /**
     * Construct a new utility function.
     */
    ReplicationUtilityFunction(ReplicationManager *_manager /**< The parent manager. */, ReplicationGlobalOptimizer *_globalOptimizer /**< The global optimizer to adjust the utility weights. */, string _name /**< The name of the utility function. */) :
        manager(_manager),
        name(_name),
        globalOptimizer(_globalOptimizer) {};

    /**
     * Deconstruct and free the allocated resources.
     */
    virtual ~ReplicationUtilityFunction() {}

    /**
     * Compute a utility between 0 (no utiltiy) and 1 (highest utility) for 
     * replicating data object with id `do_id` to neighbor node with id 
     * `node_id`.
     * @return A value between 0 and 1 representing the utility.
     */
    virtual double compute(string do_id /**< The id of the data object whose utility is to be comptued. */, string node_id /** The id of the node which whose utility for replicating the data object is being computed. */) = 0;

    /**
     * Initialize the utility function given a configuration file.
     */
    virtual void onConfig(const Metadata& m /**< The metadata used to configure the utility function. */);

    /**
     * Update the utility function given a new data object insertion.
     */
    virtual void notifyInsertion(DataObjectRef dObj /**< The data object that was inserted. */) {};

    /**
     * Update the utility function given that a data object was deleted.
     */
    virtual void notifyDelete(DataObjectRef dObj /**< The data object that was deleted. */) {};

    /**
     * Update the utility function given that a data object was replicated
     * successfully.
     */
    virtual void notifySendSuccess(DataObjectRef dObj /**< The data object that was successfully replicated. */, NodeRef node /**< The node that successfully received the data object. */) {};

    /**
     * Update the utility function given that a data object failed to replicate
     * successfully.
     */
    virtual void notifySendFailure(DataObjectRef dObj /**< The data object that failed to replicate. */, NodeRef node /**< The node that failed to receive the data object. */) {};

    /**
     * Update the utility function given that a new neighbor node was discovered. 
     */
    virtual void notifyNewContact(NodeRef node /**< The new node that was discovered. */) {};

    /**
     * Update the utility function given that the neighbor node was updated.
     */
    virtual void notifyUpdatedContact(NodeRef node /**< The node that was updated. */) {};

    /** 
     * Update the utility function given that a neighbor node is no longer connected. 
     */
    virtual void notifyDownContact(NodeRef node /**< The node that is no longer connected. */) {};

    /**
     * Get the parent manager. 
     * @return The parent manager.
     */
    ReplicationManager *getManager() { return manager; }

    /**
     * Get the HaggleKernel to post events and access global data structures.
     * @return The HaggleKernel.
     */
    HaggleKernel *getKernel() { return manager ? manager->getKernel() : NULL; }

    /**
     * Get the global optimizer to compute weights.
     * @return The global optimizer.
     */
    ReplicationGlobalOptimizer *getGlobalOptimizer() { return globalOptimizer; }

    /**
     * Get the name of the function.
     * @return The name of the function.
     */
    string getName() { return name; }

    /**
     * Print the name of the function in a pretty format.
     * @return The name of the function in a pretty format.
     */
    virtual string getPrettyName() { return name; }
};

// START: aggregate functions:

/**
 * The base class that serves for utiliy functions that are composed
 * of other utility functions. Mostly proxies the calls to the composite 
 * functions.
 */
class ReplicationUtilityAggregate : public ReplicationUtilityFunction {

protected:

    List<ReplicationUtilityFunction *> factors; /**< The encompassed utility functions. */

public:
    /**
     * Construct a new aggregate.
     */
    ReplicationUtilityAggregate(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer, string _name) 
        : ReplicationUtilityFunction(_manager, _globalOptimizer, _name) {};

    /**
     * Free the resources allocated for the aggregate.
     */
    virtual ~ReplicationUtilityAggregate();

    /**
     * Notify the composed utility functions that a new data object was inserted. 
     */
    void notifyInsertion(DataObjectRef dObj /**< The data object that was inserted. */);

    /**
     * Notify the composed utility functions that a data object was deleted. 
     */
    void notifyDelete(DataObjectRef dObj /**< The data object that was deleted. */);

    /**
     * Notify the composed utility functions that a data object was replicated 
     * successfully.
     */
    void notifySendSuccess(DataObjectRef dObj /**< The data object that was successfully replicated. */, NodeRef node /** The node that successfully received the data object. */);

    /**
     * Notify the composed utility functions that a data object was replicated
     * unsucessfully. 
     */
    void notifySendFailure(DataObjectRef dObj /**< The data object that failed to replicate.*/, NodeRef node /**< The node that failed to receive the data object. */);

    /**
     * Notify the composed utility functions that a new neighbor node was discovered.
     */
    void notifyNewContact(NodeRef node /**< The new node that was discovered. */);

    /**
     * Notify the composed utiltiy functions that a neighbor disconnected.
     */
    void notifyDownContact(NodeRef node /**< The node that disconnected. */);

    /**
     * Notify the composed utility functions that a neighbor node was updated.
     */
    void notifyUpdatedContact(NodeRef node /**< The node that was updated. */);

    /**
     * Initialize the utiltiy function and the composed utiltiy functions
     * from a config file.
     */
    void onConfig(const Metadata& m /**< Metadata from the config file used to instantiate and initialize the composed utility functions. */);

    /**
     * Use the composed functions to compute an aggregate function for storing
     * the data object with `do_id` at node with id `node_id`.
     */
    virtual double compute(string do_id /**< Id of data object to be replicated.*/ , string node_id /**< Id of node to replicate data object to.*/) = 0;

    /**
     * Get the pretty name of the composed utility functions.
     */
    virtual string getPrettyName() = 0;
};

/**
 * Compute a sum over the composed utility functions.
 */
class ReplicationUtilityAggregateSum : public ReplicationUtilityAggregate {

#define REPL_UTIL_AGGREGATE_SUM_NAME "ReplicationUtilityAggregateSum"

public:
    /**
     * Construct a new sum utility function.
     */    
    ReplicationUtilityAggregateSum(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityAggregate(_manager, _globalOptimizer, REPL_UTIL_AGGREGATE_SUM_NAME) {};

    /**
     * Sum the composed utiltiy functions and return the result, normalizing 
     * between 0 and 1.
     * @return The normalized sum of the composed utility functions.
     */
    double compute(string do_id /**< The data object to be replicated. */, string node_id /**< The node to replicate the data object to. */);

    /**
     * Pretty print by including the names of the composed utility functions.
     * @return The pretty string of the composed utiltiy functions and sum +.
     */
    string getPrettyName();
};

/**
 * Compute the minimum utility over the composed utiltiy functions.
 */
class ReplicationUtilityAggregateMin : public ReplicationUtilityAggregate {

#define REPL_UTIL_AGGREGATE_MIN_NAME "ReplicationUtilityAggregateMin"

public:
    /**
     * Construct an aggregate min.
     */
    ReplicationUtilityAggregateMin(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityAggregate(_manager, _globalOptimizer, REPL_UTIL_AGGREGATE_MIN_NAME) {};


    /**
     * Compute the minimum utility across the composed utility functions.
     * @return The minimum utility.
     */
    double compute(string do_id /**< The id of the data object to be replicated. */, string node_id /**< The id of the node to replicate the data object to. */);

    /**
     * Get the pretty name of the minimum over the composed utility functions.
     * @return The pretty min name.
     */
    string getPrettyName();
};

/**
 * Compute the maxmimum utility over the composed utiltiy functions.
 */
class ReplicationUtilityAggregateMax : public ReplicationUtilityAggregate {

#define REPL_UTIL_AGGREGATE_MAX_NAME "ReplicationUtilityAggregateMax"

public:

    /** 
     * Construct an aggregate max.
     */
    ReplicationUtilityAggregateMax(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityAggregate(_manager, _globalOptimizer, REPL_UTIL_AGGREGATE_MAX_NAME) {};

    /**
     * Compute the maximum utility across the composed utility functions.
     * @return The maximum utility.
     */
    double compute(string do_id /**< The id of the data object to be replicated. */, string node_id /**< The id of the node to replicate the data object to. */);

    /**
     * Get the pretty name of the maximum over the composed utility functions.
     * @return The pretty max name.
     */
    string getPrettyName();
};

// END: aggregate functions.

/**
 * Compute a utility that is random.
 */
class ReplicationUtilityRandom : public ReplicationUtilityFunction {

#define REPL_UTIL_RANDOM_NAME "ReplicationUtilityRandom" /**< Name of the utility function. */

public:

    /**
     * Construct a new random utility function.
     */
    ReplicationUtilityRandom(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityFunction(_manager, _globalOptimizer, REPL_UTIL_RANDOM_NAME) {};

    /**
     * Deconstruct the utility function and free its resources.
     */
    ~ReplicationUtilityRandom() {}

    /**
     * Compute the utility of replicating data object with id `do_id` to
     * the node with id `node_id`, by generating a random number.
     * @return A random number between 0 and 1.
     */
    double compute(string do_id /**< The id of the data object being replicated. */, string node_id /**< The id of the node to replicate the data object to. */);

    /**
     * Initialize and configure the random utility function.
     */
    void onConfig(const Metadata& m /**< The configuration file used to initialize the utiltiy function. */);

    /**
     * Get the pretty name of the utility function.
     * @return The pretty name.
     */
    string getPrettyName() { return string("random(d)"); };
};

/**
 * A NOP (no operation) utiltiy function (does nothing).
 */
class ReplicationUtilityNOP : public ReplicationUtilityFunction {

#define REPL_UTIL_NOP_NAME "ReplicationUtilityNOP"  /**< Name of the utility function. */

public:

    /**
     * Construct a new NOP utility function.
     */
    ReplicationUtilityNOP(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityFunction(_manager, _globalOptimizer, REPL_UTIL_NOP_NAME) {};

    /**
     * Deconstruct the NOP utiltiy function and free its resources.
     */
    ~ReplicationUtilityNOP() {}

    /**
     * Compute the utility for replicating a data object with id `do_id` to
     * the node with id `node_id`.
     * Always returns the value determined by the global optimizer.
     * @return The utility specified by the global optimizer.
     */
    double compute(string do_id /**< The id of the data object to be replicated .*/, string node_id /** The id of the node to replicate the data object to. */);

    /**
     * Confgure the NOP utility function.
     */
    void onConfig(const Metadata& m /** The configuration metadata to configure the NOP utility function. */);

    /**
     * Get the pretty name of the utility function.
     * @return The pretty name of the utility function.
     */
    string getPrettyName() { return string("NOP"); };
};

/**
 * Gives a utility of 0 for replication to a node until a certain time
 * has elapsed since insertion.
 */
class ReplicationUtilityWait : public ReplicationUtilityFunction {

#define REPL_UTIL_DEFAULT_WAIT_MS 15000 /*< Default time to wait in milliseconds after insertion before assigning utiltiy. */

#define REPL_UTIL_WAIT_NAME "ReplicationUtilityWait" /**< Name of the utility function. */

private:

    typedef HashMap<string, Timeval> time_map_t; /**< Map type from the data object id to the time before which it has utility 0. */

    time_map_t time_map; /**< The map that keeps track of when data objects can have positive utility. */

    Timeval expireTimeval; /**< The duration to wait from creation time when the data object utility is 0. */

public:

    /**
     * Construct a new wait utility function.
     */
    ReplicationUtilityWait(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityFunction(_manager, _globalOptimizer, REPL_UTIL_WAIT_NAME), expireTimeval(REPL_UTIL_DEFAULT_WAIT_MS / (double) 1000) {};

    /**
     * Deconstruct the utility function and free its resources.
     */
    ~ReplicationUtilityWait();

    /**
     * Get the utility of replicating data object with id `do_id` to node 
     * with id `node_id`. This value will be 0 from the data object insertion 
     * until the specified time has elapsed.
     * @return The utility that is defined by the global optimizer if the time has elpased since insertion, 0 otherwise.
     */
    double compute(string do_id /**< The id of the data object whose utility is being computed. */, string node_id /**< The id of the node to replicate to. */);

    /**
     * Notify the utility function that a data object was deleted. 
     */
    void notifyDeletion(DataObjectRef dObj /**< The data object that was deleted. */);

    /**
     * Notify that a data object was inserted. 
     */
    void notifyInsertion(DataObjectRef dObj /**< The data objec that was inserted. */);
    
    /**
     * Initialize and configure the utility function.
     */
    void onConfig(const Metadata& m /**< The configuration file to initialize the utility function. */);

    /**
     * Pretty print the utility function.
     * @return The pretty representation of the function name.
     */
    string getPrettyName() { return string("WAIT"); };
};

/**
 * Given an attribute name and a maximum attribute value, this utility 
 * function will assign the replication utility to be the value specified
 * in the value of an attribute of the data object, normalized by
 * the maximum attribute value.
 */
class ReplicationUtilityAttribute : public ReplicationUtilityFunction {

#define REPL_UTIL_ATTR_NAME "ReplicationUtilityAttribute" /**< Name of the utility function. */

private:

    string attr_name; /**< The attribute name whose value determines the replication utility. */

    int attr_max_value; /**< Normalization constant that divides the attribute value. */

    typedef HashMap<string, double> map_attribute_t; /**< Map type from do_id to utility. */

    map_attribute_t attribute_map; /**< The map used to map data object ids to utility. */

public:

    /**
     * Construct a new attribute utility function.
     */
    ReplicationUtilityAttribute(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityFunction(_manager, _globalOptimizer, REPL_UTIL_ATTR_NAME),
        attr_name(""), 
        attr_max_value(1) {};

    /**
     * Deconstruct an attribute utility function, and free its resources.
     */
    ~ReplicationUtilityAttribute();

    /**
     * Compute the utility of replicating the dataobject with do_id `do_id`
     * to the node with ide `node_id`, using the attribute in the data object.
     * @return The value specified in the data object attribute, 0 otherwise.
     */
    double compute(string do_id /**< The data object id whose data object attribute is used to determine the utility. */, string node_id /**< Id of the node to replicate to--this is actually not used in this function. */);

    /**
     * Initialize the attribute name and normalization value from the
     * configuration file.
     */
    void onConfig(const Metadata& m /**< The configuration to initialize the utility function. */);

    /**
     * Notify the utiltiy function that a new data object was inserted.
     */
    void notifyInsertion(DataObjectRef dObj /**< The inserted data object. */);

    /**
     * Notify the utility function that a data object was deleted.
     */
    void notifyDelete(DataObjectRef dObj /**< The data object that was deleted. */);

    /**
     * Get the pretty name.
     * @return The pretty name of the attribute utility function.
     */
    string getPrettyName() { return string("attr(d)"); };
};

/**
* ReplicationUtilityNeighborOtherSocial differs from CacheUtilityNeighborOtherSocial in the following ways:
* 1.  We ONLY look at the diversity of the social group of the (replication) target node.   For caching,
* we look at ALL social groups to computer diversity.    This is due to difference in function.
* 2.  We only consider neighbors for replication, as opposed to caching (which can be neighbors or entire network).
* Thus, we dont have a local option.
* 3.  LessIsMore is removed, as it is unnecessary.   In effect, this method returns how much you want to replicate.
* So, by default, it is on (high diversity = low replication, and vice versa).
* 4.   Like #1, #2, we still use ALL nodes of the same social group for calculation.   In caching, we only consider local neighbors, or entire network.
* 5.  Since we compare immediate nodes, if we have 2+ neighbors in the same social group, we will send to each.   This is a side effect
* of evaluating each DO per node, as opposed to computer a generic value per DO, then trying to figure out where to send it.
**/
class ReplicationUtilityNeighborhoodOtherSocial : public ReplicationUtilityFunction {
#define REPL_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME "ReplicationUtilityNeighborhoodOtherSocial" /**< \fn Full name for configuration */
private:
//protected:
    int max_group_count; /**< Maximum nodes per same social group to consider, as we give a value of 1-1/(2^#), where # is the number of nodes having the content.   By limiting this to '1', you can make a discrete value with a weight of 2 (0,1), or give it a higher value, to add more weight to more copies in other groups */

    bool exclude_my_group;  /**<  Exclude all nodes in my social group for calculation purpose, if true. */  
public:
    ReplicationUtilityNeighborhoodOtherSocial(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityFunction(_manager, _globalOptimizer, REPL_UTIL_NEIGHBORHOOD_OTHER_SOCIAL_NAME),
        max_group_count(1),
        exclude_my_group(true)
	 {};
    ~ReplicationUtilityNeighborhoodOtherSocial() {}
    double compute(string do_id, string node_id); //, string &strResult);
    void onConfig(const Metadata& m);
    /**
    *  Return a string for the formulaic printing.
    */
    string getPrettyName() { return string("nbrhoodothersocial(d)"); }; 
    /**
    *  Return a string for the individual DO value computation.
    */
    string getShortName() { return string("NHOTHERSOCIAL"); };  
};

class ReplicationUtilityLocal : public ReplicationUtilityFunction {
#define REPL_UTIL_LOCAL_NAME "ReplicationUtilityLocal" /**< \fn Full name for configuration */

private:
    bool localImmunity; /**< This option, if true, will give all locally published content a value of 1.   If false, it is treated as other content. */
    typedef HashMap<string, int> local_map_t;
    local_map_t local_map;  /**< map between DO (which is local) and size */
    //unsigned long long local_size;
public:
    ReplicationUtilityLocal(ReplicationManager *_manager, ReplicationGlobalOptimizer *_globalOptimizer) :
        ReplicationUtilityFunction(_manager, _globalOptimizer, REPL_UTIL_LOCAL_NAME), //local_size(0), 
	localImmunity(false) {}
    ~ReplicationUtilityLocal() {}
    double compute(string do_id, string node_id); //string &strResult);
    void onConfig(const Metadata& m);
    void notifyInsertion(DataObjectRef dObj);
    void notifyDelete(DataObjectRef dObj);
    /**
    *  Return a string for the formulaic printing.
    */
    string getPrettyName() { return string("Local(d)"); };
    /**
    *  Return a string for the individual DO value computation.
    */
    string getShortName() { return string("LCL"); };
};


#endif /* _REPL_UTIL_FUNC_H */
