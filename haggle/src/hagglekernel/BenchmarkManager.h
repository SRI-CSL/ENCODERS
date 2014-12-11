/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BENCHMARKMANAGER_H
#define _BENCHMARKMANAGER_H

#include <libcpphaggle/Platform.h>

class BenchmarkManager;

#include "Manager.h"
#include "Event.h"
#include "DataObject.h"
#include "Node.h"

/**
 * @file BenchmarkManager.h
 *
 * File containing classes for benchmarking and profiling the database. 
 */

/**
 * A random number generator that keeps its state within the class to ensure
 * consistency across runs (no static state like srand that might be modified 
 * elsewhere in the code).
 */
class CustomRandom {
private:
	unsigned short xsubi[3]; /**< Random number generator state. */
public:
    /**
     * Construct a new random number generator with its own local state.
     */
	CustomRandom();
    /**
     * Cleanup the random number generator and its state.
     */
	~CustomRandom() {};

    /**
     * Generate a random integer.
     * @return A random integer. 
     */
	int next(); 
};

/**
 * The benchmark manager collects performance metrics for the database by 
 * inserting random nodes and data objects, and saves this state to a
 * log file.
 * The following flag must be enabled when compiling in order to use 
 * this code: 
 *
 *    ./configure --enable-benchmark
 * 
 * The benchmark has the following stages:
 * 1. create and insert nodes
 * 2. create and insert data objects
 * 3. issue data object queries for each node (record statistcs to log file)
 * 4. issue the same queries and delete the returned data objects and node 
 * 
 * The benchmark log file only contains query latency data, while the
 * other delays are recorded in log messages in the DataStore implementation.
 */
class BenchmarkManager : public Manager
{
    CustomRandom crandom; /**< Random number generator. */
    unsigned int numDataObjectsAttr; /**< Number of data object attributes. */
    unsigned int numNodesAttr; /**< Number of node attributes. */
    unsigned int attrPoolSize; /**< Pool size of the attributes to draw from. */
    unsigned int numDataObjects; /**<  Number of inserted data objects. */
    unsigned int numNodes; /**< Number of inserted nodes. */
    EventCallback<EventHandler> *queryCallback; /**< Callback to continue insertions after each query. */
    NodeRefList queryNodes; /**< Nodes to query. */
    NodeRefList oldQueryNodes; /**< Nodes to delete (copy of queryNodes). */
    EventType evaluateEType; /**< Filter to kick off queries. */
    /**
     * Called during initialization of the manager.
     * @return `false` if an error occurred during initialization, `true` otherwise.
     */
    bool init_derived();
public:
    /**
     * Construct a new benchmark manager for stress testing and profiling.
     */
    BenchmarkManager(
        HaggleKernel *_haggle = haggleKernel, /**< [in] The haggle kernel used to interface with the other managers. */
        unsigned int _numDataObjectsAttr = 0,  /**< [in] The number of attributes per data object (chosen randomly from the attribute pool.). */
        unsigned int _numNodesAttr = 0, /**< [in] The number of attributes per node (chosen randomly from the attribute pool.). */
        unsigned int _attrPoolSize = 0, /**< [in] The size of the attribute pool. */
        unsigned int _numDataObjects = 0, /**< [in] The number of data objects to insert. */
        unsigned int _numNodes = 0 /**< [in] The number of nodes to insert. */); 

    /**
     * Cleanup any state from the benchmarking.
     */
    ~BenchmarkManager();

    /**
     * The first stage of the test where the nodes are created and inserted
     * into the database. 
     */
    void createNodes(Event* e /**< [in] The event that triggered the callback to create the nodes (raised in init). */); 

    /**
     * Continusly insert data objects until all of the data objects have been
     * inserted, creating a random data object with each call. 
     * @see createNodes()
     */
    void insertDataobject(Event* e /**< [in] The event that triggered the callback to insert the data objects (raised in createNodes()).*/ ); 

    /**
     * Issue node queries for all of the inserted nodes.
     * After all of the queries have been issued, the queries will
     * be issued again and the resulting data objects will be deleted. 
     * @see onQueryResult()
     * @see insertDataobject()
     */
    void onEvaluate(Event *e /**< [in] The event that triggered this function (raised in insertDataobject()). */); 

    /**
     * Callbacks fired for each query.
     * @see onEvaluate()
     */
    void onQueryResult(Event* e /**< [in] The event that triggered this function (raised in onEvaluate()). */); 

    /**
     * Generate a fake node with a certain number of interests drawn randomly.
     * @return A fake node used for benchmarking. 
     */
    static NodeRef createNode(
        unsigned int numAttr, /**< [in] The number of interests for the node. */
        unsigned int attrPoolSize, /**< [in] The size of the pool to draw the interests from. */
        CustomRandom *crandom = NULL /**< [in] The random number generator used to pick the interests. */);

    /**
     * Generate a fake data object with a certain number of attributes drawn randomly.
     * @return A fake data object. 
     */
    static DataObjectRef createDataObject(
        unsigned int numAttr, /**< [in] The number of attributes for the data object. */
        unsigned int attrPoolSize, /**< [in] The size of the pool to draw the attributes from. */
        CustomRandom *crandom = NULL  /**< [in] The random number generator used to pick the attributes. */);
};

#endif /* _BENCHMARKMANAGER_H */
