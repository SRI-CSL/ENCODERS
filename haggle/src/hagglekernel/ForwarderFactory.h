/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _FORWARDERFACTORY_H
#define _FORWARDERFACTORY_H

#include "Forwarder.h"

class ForwarderFactory {
public:
    // this method is for backwards compatiblity w/ Haggle to support
    // the Prophet default when there is no configuration file
    static Forwarder* getSingletonForwarder(
        ForwardingManager* manager,
        const EventType type);

    static Forwarder* getNewForwarder(
        ForwardingManager* manager,
        const EventType type,
        const Metadata* config,
        bool *o_resolutionDisabled);

};

#endif /* FORWARDERFACTORY */
