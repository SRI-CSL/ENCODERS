/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ForwarderFactory.h"
#include "ForwarderAggregate.h"
#include "ForwardingClassifierFactory.h"
#include "ForwarderProphet.h"
#include "ForwarderAlphaDirect.h"
#include "ForwarderFlood.h"
#include "ForwarderNone.h"

Forwarder* ForwarderFactory::getSingletonForwarder(
    ForwardingManager* manager,
    const EventType eventType)
{
    // create a shared queue for the forwarders
    ForwarderAggregate *aggregateForwarder 
        = new ForwarderAggregate(manager, eventType, NULL, "ForwarderAggregateSingleton");
    aggregateForwarder->addDefaultForwarder(new ForwarderProphet(aggregateForwarder));
    return aggregateForwarder;
}

Forwarder* ForwarderFactory::getNewForwarder(
    ForwardingManager* manager,
    const EventType eventType,
    const Metadata* config,
    bool *o_resolutionDisabled)
{

    const Metadata* classifierConfig = config->getMetadata("ForwardingClassifier");

    // BEGIN LOADING CLASSIFIER:
    
    const char *param = NULL;

    ForwardingClassifier *classifierModule = NULL;

    if (classifierConfig) {
        param = classifierConfig->getParameter("name");
        if (param) {
            classifierModule = ForwardingClassifierFactory::getClassifierForName(manager, param);
        } else {
            HAGGLE_ERR("Classifier tag must specify classifier name.\n");
        }
    }

    if (classifierModule) {
        classifierModule->onConfig(*classifierConfig);
        HAGGLE_DBG("Loaded classifier: %s\n", param);
    } else {
        HAGGLE_DBG("No classifier loaded.\n");
    }

    // DONE LOADING CLASSIFIER.

    // START LOADING FORWARDERS:

    ForwarderAggregate *aggregateForwarder = NULL;

    int numForwarders = 0;

    const Metadata *forwarderConfig;

    int i = 0;

    // SW: START: handle no forwarder case better (don't create aggregate)
    forwarderConfig = config->getMetadata("Forwarder", 0);
    if (forwarderConfig) {
        param = forwarderConfig->getParameter("protocol");
        if (param) {
            string protocol = param;
            bool resolutionDisabled = false;
            if (0 == protocol.compare("noResolution")) {
                resolutionDisabled = true;
            }
            if (0 == protocol.compare("noForward")) {
                HAGGLE_DBG("Module loaded without forwarding.\n");
                *o_resolutionDisabled = resolutionDisabled;
                return NULL;
            }
        }
    }
    // SW: END: handle no forwarder case better (don't create aggregate)

    aggregateForwarder = new ForwarderAggregate(manager, eventType, classifierModule);

    while (NULL != (forwarderConfig = config->getMetadata("Forwarder", i++))) { 
        param = forwarderConfig->getParameter("protocol");

        Forwarder *newForwarder = NULL;
        
        if (!param) {
            HAGGLE_ERR("No forwarder protocol specified.\n");
            continue;
        }
        string protocol = param;

        if (0 == protocol.compare("noResolution")) {
            HAGGLE_ERR("noResolution defined outside of no forwarder\n");
            continue;
        } else if ((0 == protocol.compare("noForward")) || (0 == protocol.compare(NONE_NAME))) {
            newForwarder = new ForwarderNone(manager);
        } else if (0 == protocol.compare(PROPHET_NAME)) {
            newForwarder = new ForwarderProphet(aggregateForwarder);
        } else if (0 == protocol.compare(ALPHADIRECT_NAME)) {
            newForwarder = new ForwarderAlphaDirect(aggregateForwarder);
        } else if (0 == protocol.compare(FLOOD_NAME)) {
            newForwarder = new ForwarderFlood(manager); 
        } else {
            HAGGLE_DBG("configured forwarding module <%s> not known. No change applied.", protocol.c_str()); 
        }

        if (newForwarder) {

            param = forwarderConfig->getParameter("contentTag");
        
            if (!param) {
                HAGGLE_DBG("Forwarder specified without a content tag, setting to default\n");
                aggregateForwarder->addDefaultForwarder(newForwarder); 
            } else {
                string className = param;
                aggregateForwarder->addForwarderForClassName(newForwarder, className);
            }

            numForwarders++;
            newForwarder->onConfig(*forwarderConfig);
        }

    } /* end of forwarder config while */

    // DONE LOADING FORWARDERS.
    
    HAGGLE_DBG("Loaded %d forwarding modules.\n", numForwarders);

    return aggregateForwarder;
}
