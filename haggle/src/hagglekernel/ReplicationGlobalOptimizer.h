/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_GLOBAL_OPT_H
#define _REPL_GLOBAL_OPT_H

class ReplicationGlobalOptimizer;
class ReplicationGlobalOptimizerFixedWeights;

#include "HaggleKernel.h"

/**
 * The `ReplicationGlobalOptimizer` base class is responsible for assigning
 * weights to utility functions. Right now this is really a placeholder,
 * but future versions could adjust these weights in response to network
 * feedback. The global optimizer also specifies a minimum threshold that
 * <data object, node> utility pairs must exceed in order to be eligible
 * for replication.
 */
class ReplicationGlobalOptimizer {

private:

    HaggleKernel *kernel; /**< The Haggle Kernel used to post events to. */

    string name; /**< The name of the global optimizer. */

public:

    /**
     * Construct a global optimzer.
     */
    ReplicationGlobalOptimizer(HaggleKernel *_kernel /**< The haggle kernel to post events to. */, string _name /**< The name of the global optimizer. */);

    /**
     * Deconstruct the global optimizer and free its resources.
     */
    virtual ~ReplicationGlobalOptimizer() {};

    /**
     * Get the minimum threshold which <node, data object> utility pairs must
     * exceed in order to be eligible for replication.
     * @return The minimum utility threshold.
     */
    virtual double getMinimumThreshold() = 0;

    /**
     * Get the weight of the utility function with name `name`.
     * @return The weight that is multiplied by the utility function's value
     * when computing the utility.
     */
    virtual double getWeightForName(string name /**< The name of the utiltiy function whose weight is computed. */) = 0;

    /**
     * Configure the global optimizer with the metadata from config.xml
     */
    virtual void onConfig(const Metadata& m /**< The metadata to configure the global optimizer. */) {};

    /**
     * Get the name of the global optimizer.
     * @return The name of the global optimizer.
     */
    string getName() { return name; }

    /**
     * Get the pretty name of the global optimizer.
     * @return The pretty name of the global optimizer.
     */
    virtual string getPrettyName() { return name; }
    
    /**
     * Get the Haggle Kernel to post events to.
     * @return The Haggle Kernel.
     */
    HaggleKernel *getKernel() { return kernel; }
};

#define REPL_GLOBAL_OPT_FIXED_WEGHTS_NAME "ReplicationGlobalOptimizerFixedWeights" /**< The fixed weight global optimizer name. */

#define REPL_GLOBAL_OPT_DEFAULT_THRESHOLD 0 /**< The default minimum threshold for the fixed weight global optimizer. */

/**
 * A fixed weight global optimizer simply assigns weights defined in the
 * configuration file for each utiltiy function. These weights are multiplied
 * by the value of the utility function's computation to compute a utility.
 * A minimum threshold is also specified in the configuration file.
 */
class ReplicationGlobalOptimizerFixedWeights : public ReplicationGlobalOptimizer {

private:

    double minThreshold; /**< The minimum threshold that must be exceeded for replication to occur. */ 

    typedef Map<string, double> weight_registry_t; /**< The type of the map to keep track of the weights for each utility function. */

    weight_registry_t weights; /**< The weights for each utility function that are multiplied by the computed value of the utility function. */

public:

    /**
     * Construct a new fixed weight global optimizer.
     */
    ReplicationGlobalOptimizerFixedWeights(HaggleKernel *_kernel /**< The haggle kernel to post events to. */);

    /**
     * Deconstruct the global optimizer and free its allocated resources.
     */
    ~ReplicationGlobalOptimizerFixedWeights();

    /**
     * Get the minimum threshold that must be exceeded for replication to occur.
     * @return The minimum threshold that must be exceeded.
     */
    double getMinimumThreshold();

    /**
     * Get the weight of the utility function--this value is between 0 and 1
     * and is multiplied by the value of the utility function to compute the
     * the utility.
     * @return The utility function's weight.
     */
    double getWeightForName(string utilityFunctionName);

    /**
     * Configure the global optimizer.
     */
    void onConfig(const Metadata& m /**< The configuration settings for the global optimizer. */);

    /**
     * Get the pretty name of the global optimizer.
     * @return The pretty name.
     */
    virtual string getPrettyName();
};

#endif /* _REPL_GLOBAL_OPT_H */
