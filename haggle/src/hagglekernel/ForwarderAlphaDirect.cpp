/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

 

/*
 *
 * Forwarder Alpha-DIRECT implementation.
 * 
 * Alpha-DIRECT forwards along the path that it first received an
 * interest from. Unlike DIRECT, Alpha-DIRECT operates at the object
 * level, opposed to the chunk level. 
 *
 * Can be enabled by adding the following XML to your haggle configuration file:
 * 
 * <Haggle>
 *       <Attr name="ManagerConfiguration" />
 *       <Attr name="ForwardingManager" />
 *        
 *           <ForwardingManager>
 *               <Forwarder protocol="AlphaDirect" /> 
 *           </ForwardingManager>
 * </Haggle>
 */
#include <libcpphaggle/Platform.h>
#include "ForwarderAlphaDirect.h"
#include "XMLMetadata.h"

ForwarderAlphaDirect::ForwarderAlphaDirect(ForwarderAsynchronous *forwarderAsync) :
	ForwarderAsynchronousInterface(forwarderAsync, ALPHADIRECT_NAME),
	kernel(getManager()->getKernel())
{
    HAGGLE_DBG("Forwarding module \'%s\' initialized\n", getName()); 
}

ForwarderAlphaDirect::~ForwarderAlphaDirect()
{
    HAGGLE_DBG("Alpha-DIRECT deconstructor.\n");
}

/*
 * Save persistent state to `rel', for SQL database.
 * Alpha-DIRECT uses the existing node and data store, and does not
 * need to persist additional state.
 */
size_t ForwarderAlphaDirect::getSaveState(RepositoryEntryList& rel)
{
    HAGGLE_DBG("Alpha-DIRECT saving state.\n");

    return rel.size();
}

/*
 * Load persistent state from `e' to an Alpha-DIRECT object.
 * Alpha-DIRECT uses the existing node and data store, and does not
 * need to load additional state.
 */
bool ForwarderAlphaDirect::setSaveState(RepositoryEntryRef& e)
{
    HAGGLE_DBG("Alpha-DIRECT loading saved state.\n");

    return true;
}

/*
 * This function is called every time new routing information is received from
 * a neighbor. The input is the part of the metadata which contains the
 * routing information. This means that the metadata could originally be part 
 * of any data object.
 *
 * Alpha-DIRECT does not use additional routing information and only requires
 * the existing Node Description propagation.
 */
bool ForwarderAlphaDirect::newRoutingInformation(const Metadata *m)
{	
    if (!m || m->getName() != getName()) {
        return false;
    }

    HAGGLE_DBG("Alpha-DIRECT receiving new routing information.\n");

    return false;
}

/*
 * Add routing information to a data object.
 * The parameter "parent" is the location in the data object where the routing 
 * information should be inserted.
 *
 * Alpha-DIRECT does not use additional routing information and only requires
 * the existing node description propagation.
 */ 
bool ForwarderAlphaDirect::addRoutingInformation(DataObjectRef& dObj, Metadata *parent)
{
    if (!dObj || !parent) {
        return false;
    }
	
    return false;
}

/* 
 * Handle when a neighbor joins.
 *
 * Alpha-DIRECT does not need to populate any other data structures, and instead
 * uses the existing node and data stores.
 */
void ForwarderAlphaDirect::_newNeighbor(const NodeRef &neighbor)
{
    if (neighbor->getType() != Node::TYPE_PEER) {
        return;
    }

    HAGGLE_DBG("Alpha-DIRECT detected a neighbor joining.\n");
}

/* 
 * Handle when a neighbor leaves.
 *
 * Alpha-DIRECT does not need to populate any other data structures, and instead
 * uses the existing node and data stores.
 */
void ForwarderAlphaDirect::_endNeighbor(const NodeRef &neighbor)
{
    if (neighbor->getType() != Node::TYPE_PEER) {
        return;
    }

    HAGGLE_DBG("Alpha-DIRECT detected a neighbor leaving.\n");
}

/* 
 * Helper function to insert a node reference and a time-stamp in a sorted list of 
 * node reference and time-stamp pairs.
 */
static void sortedNodeListInsert(List<Pair<NodeRef, long> >& list, NodeRef& node, long time)
{
    List<Pair<NodeRef,long> >::iterator it = list.begin();
	
    for (; it != list.end(); it++) {
        if (time > (*it).second) {
            break;
        }
    }
    list.insert(it, make_pair(node, time));
}


/* 
 * Helper function to iterate across the node store and find all the target nodes
 * for a particular delegate node. 
 *
 * In other words, generate a list of all the node descriptions that were received 
 * via a particular node description.
 */
static void retrieveValidTargets(NodeStore* nodeStore, NodeRef delegate,  NodeRefList& targets) {
    // unfortunately we couldn't push the interface logic into the criteria due to
    // locking issues in ->retrieve(), thus we retrieve a list of all the nodes
    NodeRefList allNodesRefList;
    nodeStore->retrieveAllNodes(allNodesRefList);

    for (NodeRefList::iterator it = allNodesRefList.begin(); it != allNodesRefList.end(); it++) {
        NodeRef& node = *it;
        DataObjectRef nodeDataObj = node->getDataObject(false);
        InterfaceRef nodeRcvdInterface = nodeDataObj->getRemoteInterface();

        if (nodeRcvdInterface && delegate->hasInterface(nodeRcvdInterface)) {
            targets.add(node);
        }
    }
}

/*
 *  Generate a list of nodes for which `neighbor' is a good
 *  delegate for.
 *
 *  For Alpha-DIRECT, we find all the node descriptions that came through the neighbor, and rank
 *  them by decreasing age (newest first).
 */ 
void ForwarderAlphaDirect::_generateTargetsFor(const NodeRef &neighbor)
{
    HAGGLE_DBG("Alpha-DIRECT generating a list targets that this node is a delegate for.\n");

    List<Pair<NodeRef, long> > sorted_target_list;

    NodeRefList validRemoteNodes; 
    
    retrieveValidTargets(kernel->getNodeStore(), neighbor, validRemoteNodes);
  
    for (NodeRefList::iterator it = validRemoteNodes.begin(); it != validRemoteNodes.end(); it++) {
		NodeRef& node = *it;

        DataObjectRef nodeDataObj = node->getDataObject(false);
        long rxTime = nodeDataObj->getReceiveTime().getMicroSeconds();
        NodeRef target = Node::create_with_id(Node::TYPE_PEER, node->getId(), "Alpha-DIRECT target node");

        if (!target) {
            continue;
        }

        sortedNodeListInsert(sorted_target_list, target, rxTime);
        HAGGLE_DBG("Neighbor '%s' is a good delegate for target '%s' [rxtime=%lu]\n", Node::nameString(neighbor).c_str(), Node::nameString(target).c_str(), rxTime);
    }

    if (!sorted_target_list.empty()) {
        NodeRefList targets;
        unsigned long num_targets = max_generated_targets;
		
        while ((num_targets-- > 0) && (sorted_target_list.size() > 0)) {
            NodeRef target = sorted_target_list.front().first;
            sorted_target_list.pop_front();
            targets.push_back(target);
        }

        HAGGLE_DBG("Generated %lu targets for neighbor %s\n", 
            targets.size(), neighbor->getName().c_str());
        kernel->addEvent(new Event(EVENT_TYPE_TARGET_NODES, neighbor, targets));
    } else {
      HAGGLE_DBG("Could not find any targets for neighbor %s\n", neighbor->getName().c_str());
    }
}

/*
 * Helper function to retrieve the node reference `delegate` from which the node description 
 * of `target` was received. 
 */
static void retrieveDelegateForTarget(NodeStore* nodeStore, NodeRef target,  NodeRef& delegate) {

    NodeRefList nbrsRefList;
    nodeStore->retrieveNeighbors(nbrsRefList);

    DataObjectRef targetDataObj = target->getDataObject(false);
    InterfaceRef targetRcvdInterface = targetDataObj->getRemoteInterface();

    delegate = (NodeRef)0;

    for (NodeRefList::iterator it = nbrsRefList.begin(); it != nbrsRefList.end(); it++) {
        NodeRef& node = *it;
    
        if (node->hasInterface(targetRcvdInterface)) {
            delegate = node;
            break;
        }
    }
}

/* 
 * Generate a list of all the delegates for a particular target.
 * 'dObj' is the object that is being forwarded.
 * 'target' is the destination node.
 * 'other_targets' are other possible destinations.
 * 
 * For DIRECT, we want to forward to the neighbor from which we received the 
 * first target node description from.  
 */
void ForwarderAlphaDirect::_generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets)
{
    List<Pair<NodeRef, double> > sorted_target_list;

    HAGGLE_DBG("Alpha-DIRECT generating a list of delegates.\n");

    NodeRef delegate;
    retrieveDelegateForTarget(kernel->getNodeStore(), target , delegate);

    if (!delegate) {
        HAGGLE_DBG("Delegate is no longer a neighbor.\n");
        return;
    }

    /* MOS - we disable this check to support neighborForwardingShortcut = false

    if (other_targets && isTarget(delegate, other_targets)) {
        HAGGLE_DBG("Delegate is already a target.\n");
        return; 
    }

    */

    if (kernel->getThisNode() == delegate) {
        HAGGLE_DBG("Delegate is self.\n");
        return;
    }

    NodeRefList delegates;
    delegates.push_back(delegate);

    kernel->addEvent(new Event(EVENT_TYPE_DELEGATE_NODES, dObj, target, delegates));
    HAGGLE_DBG("Generated %lu delegates for target %s\n", delegates.size(), target->getName().c_str());
}

/* 
 * Handler that is called when a configuration object is received locally.
 *
 * Currently Alpha-DIRECT does not take any parameters. 
 */
void ForwarderAlphaDirect::_onForwarderConfig(const Metadata& m)
{
    if (strcmp(getName(), m.getName().c_str()) != 0) {
        return;
    }
	
    HAGGLE_DBG("Alpha-DIRECT forwarder configuration\n");
}

/*
 * Handler called when debugging the forwarder module.
 *
 * As Alpha-DIRECT does not have a routing table (it uses the existing 
 * node and data store) this function is a stub.
 */
void ForwarderAlphaDirect::_printRoutingTable(void)
{
    printf("%s printing routing table:\n", getName());

/*
    // print in tuple (delegate, target) form
    NodeRefList allNodes;
    kernel->getNodeStore()->retrieveAllNodes(allNodes);

    printf("delegate -> target\n", getName());
    for (NodeRefList::iterator it = allNodes.begin(); it != allNodes.end(); it++) {
        NodeRef& target = *it;
        NodeRef delegate;
        retrieveDelegateForTarget(kernel->getNodeStore(), target, delegate);
        if (target && delegate) {
            printf("[ %s -> %s ]\n", delegate->getName().c_str(), target->getName().c_str());
        }
    }
    printf("delegate -> targets\n", getName());
*/

    // print in (delegate, targets...) form
    NodeRefList nbrs;
    kernel->getNodeStore()->retrieveNeighbors(nbrs);
    for (NodeRefList::iterator it = nbrs.begin(); it != nbrs.end(); it++) {
        NodeRef& nbr = *it;
        printf("%s -> [", nbr->getName().c_str());
        NodeRefList targets; 
        retrieveValidTargets(kernel->getNodeStore(), nbr, targets);
        for (NodeRefList::iterator itt = targets.begin(); itt != targets.end(); itt++) {
            NodeRef& target = *itt;
            printf(" %s ", target->getName().c_str());
        }
        printf("]\n");
    }

    printf("%s done printing routing table.\n", getName());
}

// CBMEN, HL, Begin
void ForwarderAlphaDirect::_getRoutingTableAsMetadata(Metadata *m) {
    Metadata *dm = m->addMetadata("Forwarder");

    if (!dm) {
        return;
    }

    dm->setParameter("type", "ForwarderAlphaDirect");
    dm->setParameter("name", getName());

    NodeRefList nbrs;
    kernel->getNodeStore()->retrieveNeighbors(nbrs);
    for (NodeRefList::iterator it = nbrs.begin(); it != nbrs.end(); it++) {
        NodeRef& nbr = *it;

        Metadata *dmm = dm->addMetadata("Delegate");
        if (!dmm) {
            continue;
        }
        dmm->setParameter("name", nbr->getName());
        dmm->setParameter("id", nbr->getIdStr());

        NodeRefList targets; 
        retrieveValidTargets(kernel->getNodeStore(), nbr, targets);
        for (NodeRefList::iterator itt = targets.begin(); itt != targets.end(); itt++) {
            NodeRef& target = *itt;
            Metadata *dmmm = dmm->addMetadata("Target");
            if (!dmmm) {
                continue;
            }
            dmmm->setParameter("name", target->getName());
            dmmm->setParameter("id", target->getIdStr());
        }
    }
}
// CBMEN, HL, End

