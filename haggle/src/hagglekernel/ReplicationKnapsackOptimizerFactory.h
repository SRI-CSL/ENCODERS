/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_KS_OPT_FACTORY_H
#define _REPL_KS_OPT_FACTORY_H

class ReplicationKnapsackOptimizerFactory;

#include "ReplicationKnapsackOptimizer.h"

/**
 * This factory is used to instantiate `ReplicationKnapsackOptimizer` objects.
 */
class ReplicationKnapsackOptimizerFactory {

public:

    /**
     * Get a knapsack optimizer object given the class name.
     * @return The knapsack optimizer object for the passed class name.
     */
    static ReplicationKnapsackOptimizer *getNewKnapsackOptimizer(
        HaggleKernel *kernel /**<The Haggle Kernel used to post events and access global data structures. */,
        string name = string("") /**< The name of the knapsack optimizer to instantiate. */ );
};

#endif /* _REPL_KS_OPT_FACTORY_H */
