/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#include <libcpphaggle/Platform.h>
#include "ForwarderNone.h"

ForwarderNone::ForwarderNone(ForwardingManager *m, const string name) :
	Forwarder(m, name),
    useShortCircuit(false)
{
    HAGGLE_DBG("Forwarding module \'%s\' initialized\n", getName()); 
}

ForwarderNone::~ForwarderNone()
{
    HAGGLE_DBG("ForwarderNone deconstructor.\n");
}

void
ForwarderNone::onForwarderConfig(const Metadata& m)
{
    if (strcmp(getName(), m.getName().c_str()) != 0) {
        return;
    }

    const char *param;
    param = m.getParameter(FORWARDER_NONE_USE_SHORTCIRCUIT);

    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling short circuit\n");
            useShortCircuit = true;
        }
        else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling short circuit\n");
            useShortCircuit = false;
        }
    }
}

void ForwarderNone::getRoutingTableAsMetadata(Metadata *m) {
    Metadata *dm = m->addMetadata("Forwarder");
    if (!dm) {
        return;
    }

    dm->setParameter("type", "ForwarderNone");
    dm->setParameter("name", getName());
}
