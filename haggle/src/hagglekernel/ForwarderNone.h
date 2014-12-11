/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#ifndef _FORWARDERNONE_H
#define _FORWARDERNONE_H

class ForwarderNone;

#include "Forwarder.h"

#define FORWARDER_NONE_USE_SHORTCIRCUIT "use_shortcircuit"

/*
 * A NOP forwarder. 
 */
class ForwarderNone : public Forwarder {
#define NONE_NAME "None"
private:
    bool useShortCircuit;
public:
    ForwarderNone(ForwardingManager *m = NULL, const string name = NONE_NAME);
    ~ForwarderNone();
    bool useNewDataObjectShortCircuit(const DataObjectRef& dObj) { return useShortCircuit; }
    void doNewDataObjectShortCircuit(const DataObjectRef& dObj) { return; }
    void onForwarderConfig(const Metadata& m);
    void getRoutingTableAsMetadata(Metadata *m); // CBMEN, HL
};

#endif /* FORWARDERNONE */
