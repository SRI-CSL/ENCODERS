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

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>

using namespace haggle;

#include <stdlib.h>
#include <stdio.h>

#include "EventQueue.h"
#include "BenchmarkManager.h"
#include "DataObject.h"
#include "Node.h"
#include "Interface.h"
#include "Event.h"
#include "Attribute.h"
#include "Filter.h"

#include <haggleutils.h>

#define FILTER_EVALUATE_KEY "benchmark"
#define FILTER_EVALUATE_VAL "evaluate"
#define FILTER_EVALUATE "benchmark=evaluate"
#define BENCHMARK_WARMUP_DELAY_S 30

// START CUSTOM RANDOM CODE

/*
 * Random number generator that stores state within class
 */

CustomRandom::CustomRandom()
{
	xsubi[0] = 1;
	xsubi[1] = 2;
	xsubi[2] = 3;
	seed48(xsubi);
}

int
CustomRandom::next()
{
	return nrand48(xsubi);
}

// END CUSTOM RANDOM CODE

BenchmarkManager::BenchmarkManager(
    HaggleKernel * _kernel, 
    unsigned int _numDataObjectsAttr, 
    unsigned int _numNodesAttr, 
    unsigned int _attrPoolSize, 
    unsigned int _numDataObjects, 
    unsigned int _numNodes) : 
        Manager("BenchmarkManager", _kernel),
        crandom(CustomRandom()),
        numDataObjectsAttr(_numDataObjectsAttr),
        numNodesAttr(_numNodesAttr),
        attrPoolSize(_attrPoolSize),
        numDataObjects(_numDataObjects),
        numNodes(_numNodes)
        
{
}

BenchmarkManager::~BenchmarkManager()
{
}

/*
 * Initialize event handlers and kick off the tests
 */
bool
BenchmarkManager::init_derived()
{
#define __CLASS__ BenchmarkManager

    // event to kick off the tests
    EventType timeoutEType = registerEventType("BenchmarkManager Timeout Event", createNodes);
	
    if (timeoutEType < 0) {
        HAGGLE_ERR("Could not register Timeout Event...\n");
        return false;
    }

    // wait for databases to warm up before we start tests (i.e. switch to in-memory DB)
    kernel->addEvent(new Event(timeoutEType, NULL, BENCHMARK_WARMUP_DELAY_S));
	
    queryCallback = newEventCallback(onQueryResult);

    if (!queryCallback) {
        HAGGLE_ERR("Could not create queryCallback\n");
        return false;
    }

    // filter to listen for when database has been populated
    evaluateEType = registerEventType("BenchmarkManager EvalFilter Event", onEvaluate);

    if (evaluateEType < 0) {
        HAGGLE_ERR("Could not register EvalFilter Event...");
        return false;
    }

    Filter evaluateFilter(FILTER_EVALUATE, evaluateEType);

    kernel->getDataStore()->insertFilter(evaluateFilter);

    return true;
}

/*
 * This is the very first stage in the test, where we create the nodes
 * and insert them into the database. 
 */
void
BenchmarkManager::createNodes(
    Event *e)
{
    HAGGLE_LOG("Starting benchmark\n");

    // Generate and insert nodes
    for (unsigned int n = 0; n < numNodes; n++) {
        HAGGLE_LOG("Generating and inserting node %d\n", n);

        NodeRef node = createNode(numNodesAttr, attrPoolSize, &crandom);

        if (!node) {
            HAGGLE_ERR("node creation failed\n");
            exit(-1);
            return;
        }

        kernel->getDataStore()->insertNode(node);
        queryNodes.push_back(node);
    }

    // Generate and insert data objects
    // insertDataobject() is called once from here, then with asynchronous callbacks from Datastore::insertDataobject() until numDataObjects is reached
    insertDataobject(NULL);
}

/*
 * Create a random node with 1 random MAC, and `numAttr` interests drawn
 * randomly from `crandom` from a pool of `attrPoolSize` attributes.
 */
NodeRef 
BenchmarkManager::createNode(
    unsigned int numAttr,
    unsigned int attrPoolSize,
    CustomRandom *crandom)
{
    CustomRandom trandom = CustomRandom();
    if (!crandom) {
        crandom = &trandom;
    }
    static unsigned long id = 0; // unique ID for each node

    id++;
	
    // generate the MAC
    unsigned char macaddr[6];
    macaddr[0] = (unsigned char) crandom->next() % 256; 
    macaddr[1] = (unsigned char) crandom->next() % 256; 
    macaddr[2] = (unsigned char) crandom->next() % 256; 
    macaddr[3] = (unsigned char) crandom->next() % 256; 
    macaddr[4] = (unsigned char) crandom->next() % 256; 
    macaddr[5] = (unsigned char) crandom->next() % 256; 
	
    EthernetAddress addr(macaddr);
    InterfaceRef iface = Interface::create<EthernetInterface>(macaddr, "eth", addr, 0);

    // generate the node
    char nodeid[128];
    char nodename[128];
    sprintf(nodeid, "%040lx", id);
    sprintf(nodename, "node %lu", id);
	
    HAGGLE_DBG("new node id=%s\n", nodeid);

    NodeRef node = Node::create_with_id(Node::TYPE_PEER, nodeid, nodename);

    node->setMaxDataObjectsInMatch(0); // unlimited objects in match
    // NOTE: we do not limit the matches, since ties may be broken arbitrarily
    // during matching, making debugging between two different data stores
    // difficult (we want them to return the same exact result sets for
    // for each query

    if (!node) {
        return NULL;
    }

    node->addInterface(iface);

    // generate the interests
    for (unsigned int i = 0; i < numAttr; i++) {
        int tries = 0;
        char name[128];
        char value[128];
        do {
            unsigned long r = (crandom->next() % attrPoolSize) + 1;
            sprintf(name, "name%d", (int) r);
            sprintf(value, "value%d", (int) r);
            if (tries++ > 10) {
                HAGGLE_ERR("WARNING: Cannot generate unique attributes in data object... check attribute pool size!\n");
                break;
            }
        } while (node->getAttribute(name, value));
        // limit the interest weights to be 1-10
        int weight = (crandom->next() % 11) + 1; 
        node->addAttribute(name, value, weight);
    }

    return node;
}

/*
 * Continusly inserts data objects until numDataObjects have been inserted,
 * creating a random data object with each call. 
 * Afterward, node queries are called to retrieve data objects for nodes.
 */
void
BenchmarkManager::insertDataobject(
    Event* e)
{
    static unsigned int n = 0;
    n++;

    DataObjectRef dObj = createDataObject(numDataObjectsAttr, attrPoolSize, &crandom);
    HAGGLE_LOG("Generating and inserting dataobject %d\n", n);

    if (n < numDataObjects) {
        kernel->getDataStore()->insertDataObject(dObj, newEventCallback(insertDataobject));
        return;
    }
	
    // mark last node to get evaluation started
    // after its insert
    dObj->addAttribute(FILTER_EVALUATE_KEY, FILTER_EVALUATE_VAL);
    HAGGLE_LOG("Inserted final data object... "
                "waiting for the data store to finish before starting test. "
                "This may take a while.\n");
    kernel->getDataStore()->insertDataObject(dObj);
}

/*
 * Create a random data object with `numAttr` attributes.
 */
DataObjectRef
BenchmarkManager::createDataObject(
    unsigned int numAttr,
    unsigned int attrPoolSize,
    CustomRandom *crandom)
{
    CustomRandom trandom = CustomRandom();
    if (!crandom) {
        crandom = &trandom;
    }
    char name[128];
    char value[128];
    unsigned int r;

    unsigned char macaddr[6];

    macaddr[0] = (unsigned char) crandom->next() % 256;
    macaddr[1] = (unsigned char) crandom->next() % 256;
    macaddr[2] = (unsigned char) crandom->next() % 256;
    macaddr[3] = (unsigned char) crandom->next() % 256;
    macaddr[4] = (unsigned char) crandom->next() % 256;
    macaddr[5] = (unsigned char) crandom->next() % 256;
	
    unsigned char macaddr2[6];
    macaddr2[0] = (unsigned char) crandom->next() % 256;
    macaddr2[1] = (unsigned char) crandom->next() % 256;
    macaddr2[2] = (unsigned char) crandom->next() % 256;
    macaddr2[3] = (unsigned char) crandom->next() % 256;
    macaddr2[4] = (unsigned char) crandom->next() % 256;
    macaddr2[5] = (unsigned char) crandom->next() % 256;
	
    EthernetAddress addr(macaddr);
    EthernetAddress addr2(macaddr2);
    InterfaceRef localIface = Interface::create<EthernetInterface>(macaddr, "eth", addr, 0);		
    InterfaceRef remoteIface = Interface::create<EthernetInterface>(macaddr2, "eth2", addr2, 0);		
    DataObjectRef dObj = DataObject::create(NULL, 0, localIface, remoteIface);

    for (unsigned int i = 0; i < numAttr; i++) {
        int tries = 0;
        do {
            r = (crandom->next() % attrPoolSize) + 1;
            sprintf(name, "name%d", (int) r);
            sprintf(value, "value%d", (int) r);
            if (tries++ > 10) {
                HAGGLE_ERR("WARNING: Cannot generate unique attributes in data object... check attribute pool size!\n");
                break;
            }
        } while (dObj->getAttribute(name, value));

        dObj->addAttribute(name, value, r);
    }

    return dObj;
}

/*
 * Kick off the data object for nodes queries, after pausing for a few seconds.
 */
void BenchmarkManager::onEvaluate(Event *e)
{
    HAGGLE_LOG("Got filter event: Starting evaluation in 3 secs...\n");
    kernel->addEvent(new Event(queryCallback, NULL, 3.0));
}

/*
 * Callback after doing a node query. If the test has entered the last phase,
 * then the retrieved data objects and nodes will be deleted.
 */
void BenchmarkManager::onQueryResult(Event *e)
{
    static int queries = 1; // number of node queries executed 
    static int delete_queries = 1; // number of nodes deleted
    static bool delete_mode = false; // the stage we're in for queries 

    DataStoreQueryResult *qr = static_cast < DataStoreQueryResult * >(e->getData());
    NodeRef prevNode = NodeRef(NULL);

    do {
        if (!qr) {
            break;
        }

        int n = 0;
        while (true) {
            DataObjectRef dObj = qr->detachFirstDataObject();
            if (!dObj) {
                break;
            }

            n++;

            if (delete_mode) {
                kernel->getDataStore()->deleteDataObject(dObj);
            }
        }

        prevNode = qr->detachFirstNode();
        oldQueryNodes.push_back(prevNode);

        if (!delete_mode) {
            BENCH_TRACE(BENCH_TYPE_INIT, qr->getQueryInitTime(), 0);
            BENCH_TRACE(BENCH_TYPE_QUERYSTART, qr->getQuerySqlStartTime(), 0);
            BENCH_TRACE(BENCH_TYPE_QUERYEND, qr->getQuerySqlEndTime(), 0);
            BENCH_TRACE(BENCH_TYPE_RESULT, qr->getQueryResultTime(), n);
            BENCH_TRACE(BENCH_TYPE_END, Timeval::now(), 0);
            // SW: ease post processing by printing the elapsed time:
            BENCH_TRACE(BENCH_TYPE_DELAY, (qr->getQueryResultTime() - qr->getQueryInitTime()).getMicroSeconds(), 0);
            HAGGLE_LOG("%d data objects in query response %d\n", n, queries++);
        }
        else {
            kernel->getDataStore()->deleteNode(prevNode);
            HAGGLE_LOG("%d data objects delete from response %d\n", n, delete_queries++);
        }

        delete qr;
    } while (false);

    if (!delete_mode) {
        HAGGLE_LOG("Doing query %d\n", queries);
    }
    else {
        HAGGLE_LOG("Doing delete query %d\n", delete_queries);
    }

    NodeRef node = queryNodes.pop();

    if (!node && delete_mode) {
        HAGGLE_LOG("finished test\n");
        BENCH_TRACE_DUMP(numDataObjectsAttr, numNodesAttr, attrPoolSize, numDataObjects);
        kernel->addEvent(new Event(EVENT_TYPE_PREPARE_SHUTDOWN, NULL, 3.0));
    } 
    else if (!node) {
        HAGGLE_LOG("finished queries\n");
        // go to the deletion stage now
        delete_mode = true;
        queryNodes = oldQueryNodes;
        // benchmark deleting nodes
        node = queryNodes.pop();
        if (!node) {
            HAGGLE_ERR("Somehow oldQueryNodes was empty?\n");
            return;
        }
        kernel->getDataStore()->doDataObjectQuery(node, 1, queryCallback);
    }
    else {
        kernel->getDataStore()->doDataObjectQuery(node, 1, queryCallback);
    }
}
