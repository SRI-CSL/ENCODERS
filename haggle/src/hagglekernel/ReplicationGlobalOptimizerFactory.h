/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _REPL_GLOBAL_OPT_FACTORY_H
#define _REPL_GLOBAL_OPT_FACTORY_H

class ReplicationGlobalOptimizerFactory;

#include "ReplicationGlobalOptimizer.h"

/**
 * Factory class to construct a new global optimizer.
 */
class ReplicationGlobalOptimizerFactory {
public:

    /**
     * Get a new global optimizer object given its class name and the 
     * Haggle Kernel.
     * @return The instantiated global optimizer object.
     */
    static ReplicationGlobalOptimizer *getNewGlobalOptimizer(
        HaggleKernel *kernel,
        string name = string(""));
};

#endif /* _REPL_GLOBAL_OPT_FACTORY_H */
