/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

 

#ifndef _FORWARDERALPHADIRECT_H
#define _FORWARDERALPHADIRECT_H

class ForwarderAlphaDirect;

#include "ForwarderAsynchronousInterface.h"

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

using namespace haggle;

/*
 * Proof-of-concept Alpha-DIRECT Forwarder module.
 * 
 * See ForwarderAlphaDirect.cpp for implementation details. 
 */
class ForwarderAlphaDirect : public ForwarderAsynchronousInterface {
#define ALPHADIRECT_NAME "AlphaDirect"
private:  

    HaggleKernel *kernel;
        
    size_t getSaveState(RepositoryEntryList& rel);

    bool setSaveState(RepositoryEntryRef& e);

    bool newRoutingInformation(const Metadata *m);
	
    bool addRoutingInformation(DataObjectRef& dObj, Metadata *parent);

    void _newNeighbor(const NodeRef &neighbor);

    void _endNeighbor(const NodeRef &neighbor);
	
    void _generateTargetsFor(const NodeRef &neighbor);
	
    void _generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets);
    void _printRoutingTable(void);
    void _getRoutingTableAsMetadata(Metadata *m); // CBMEN, HL
    void _onForwarderConfig(const Metadata& m);

public:
    ForwarderAlphaDirect(ForwarderAsynchronous* forwarderAsync);
    ~ForwarderAlphaDirect();
};

#endif /* FORWARDERALPHADIRECT */
