/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
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
#include <string.h>

#include "NodeStore.h"
#include "Trace.h"
#include "XMLMetadata.h"

NodeStore::NodeStore() :
  preemptObsoleteNodeDesc(true), // MOS
  preemptLocalObsoleteNodeDesc(true), // MOS
  continuousBloomfilters(true), // MOS
  continuousBloomfilterUpdateInterval(5000) // MOS
{
}

NodeStore::~NodeStore()
{
	int n = 0;

	while (!empty()) {
		NodeRecord *nr = front();
		nr->node->setStored(false);
		pop_front();
		delete nr;
		n++;
	}
	HAGGLE_DBG("Deleted %d node records in node store\n", n);

	for (social_to_nodes_map_t::iterator it = social_to_nodes_map.begin(); it != social_to_nodes_map.end(); it++) {
		if ((*it).second) {
			delete ((*it).second);
		}
	}
}

bool NodeStore::_stored(const NodeRef &node, bool mustBeNeighbor)
{
	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (mustBeNeighbor && !nr->node->isNeighbor())
			continue;

		if (nr->node == node) {
			return true;
		}
	}
	return false;
}
bool NodeStore::_stored(const Node &node, bool mustBeNeighbor)
{

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (mustBeNeighbor && !nr->node->isNeighbor())
			continue;

		if (nr->node == node) {
			return true;
		}
	}
	return false;
}

bool NodeStore::_stored(const Node::Id_t id, bool mustBeNeighbor)
{
	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (mustBeNeighbor && !nr->node->isNeighbor())
			continue;

		if (memcmp(id, nr->node->getId(), NODE_ID_LEN) == 0) {
			return true;
		}
	}
	return false;
}

bool NodeStore::_stored(const string idStr, bool mustBeNeighbor)
{
	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (mustBeNeighbor && !nr->node->isNeighbor())
			continue;

		if (strcmp(idStr.c_str(), nr->node->getIdStr()) == 0) {
			return true;
		}
	}
	return false;
}

bool NodeStore::stored(const NodeRef& node, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	bool ret;

	if (!node)
		return false;

	node.lock();
	ret = _stored(node, mustBeNeighbor);
	node.unlock();

	return ret;
}

bool NodeStore::stored(const Node &node, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	return _stored(node, mustBeNeighbor);
}

bool NodeStore::stored(const Node::Id_t id, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	return (id ? _stored(id, mustBeNeighbor) : false);
}

bool NodeStore::stored(const string idStr, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	return _stored(idStr, mustBeNeighbor);
}

bool NodeStore::add(NodeRef &node)
{
        Mutex::AutoLocker l(mutex);

	if (!node)
		return false;;

	if (_stored(node)) {
		HAGGLE_DBG("Node %s is already in node store\n", node->getIdStr());
		return false;
	}

	HAGGLE_DBG("Adding new node to node store %s\n", node->getIdStr());
	node->setStored();
	push_back(new NodeRecord(node));

	return true;
}

NodeRef NodeStore::add(Node *node)
{
        Mutex::AutoLocker l(mutex);

	if (!node)
		return NULL;

	if (_stored(*node)) {
		HAGGLE_DBG("Node %s is already in node store\n", node->getIdStr());
		return NULL;
	}

	HAGGLE_DBG("Adding new node to node store %s\n", node->getIdStr());

	NodeRef nodeRef(node);
	nodeRef->setStored();
	push_back(new NodeRecord(nodeRef));

	return nodeRef;
}

NodeRef NodeStore::retrieve(const NodeRef &node, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	if (!node)
		return NULL;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (mustBeNeighbor && !nr->node->isNeighbor())
			continue;

		if (nr->node == node)
			return nr->node;
	}

	return NULL;
}

NodeRef NodeStore::retrieve(const Node& node, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (mustBeNeighbor && !nr->node->isNeighbor())
			continue;

		if (nr->node == node)
			return nr->node;
	}

	return NULL;
}

NodeRef NodeStore::retrieve(const Node::Id_t id, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;
		NodeRef node = nr->node;

		if (mustBeNeighbor && !node->isNeighbor())
			continue;

		if (memcmp(id, node->getId(), NODE_ID_LEN) == 0)
			return node;
	}

	return NULL;
}

NodeRef NodeStore::retrieve(const string &id, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;
		NodeRef node = nr->node;

		if (mustBeNeighbor && !node->isNeighbor())
			continue;

		if (memcmp(id.c_str(), node->getIdStr(), MAX_NODE_ID_STR_LEN) == 0)
			return node;
	}

	return NULL;
}

NodeRef NodeStore::retrieve(const InterfaceRef &iface, bool mustBeNeighbor)
{
        Mutex::AutoLocker l(mutex);

        if (!iface)
	        return NULL;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;
		NodeRef node = nr->node;

		if (mustBeNeighbor && !node->isNeighbor())
			continue;

		if (node->hasInterface(iface))
			return node;
	}

	return NULL;
}

NodeStore::size_type NodeStore::retrieve(Node::Type_t type, NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->getType() == type) {
                        n++;
			nl.add(nr->node);
		}
	}

	return n;
}

NodeStore::size_type NodeStore::retrieve(const Criteria& crit, NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (crit(nr->node)) {
                        n++;
			nl.add(nr->node);
		}
	}

	return n;
}


NodeStore::size_type NodeStore::retrieveNeighbors(NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->isNeighbor()) {
                        n++;
			nl.add(nr->node);
		}
	}

	return n;
}

NodeStore::size_type NodeStore::retrieveDefinedNeighbors(NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->isNeighbor() && nr->node->getType() != Node::TYPE_UNDEFINED) {
                        n++;
			nl.add(nr->node);
		}
	}

	return n;
}

// SW: START RET ALL: Useful for retrieving everything in the NodeStore
NodeStore::size_type NodeStore::retrieveAllNodes(NodeRefList& nl)
{
    Criteria allMatchCriteria = Criteria();
    return retrieve(allMatchCriteria, nl);
}
// SW: END RET ALL

NodeStore::size_type NodeStore::numNeighbors()
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->isNeighbor()) {
                        n++;
		}
	}

	return n;
}

// MOS
NodeStore::size_type NodeStore::retrieveLocalApplications(NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->isLocalApplication()) {
                        n++;
			nl.add(nr->node);
		}
	}

	return n;
}

// MOS
NodeStore::size_type NodeStore::retrieveApplications(NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->getType() == Node::TYPE_APPLICATION) {
                        n++;
			nl.add(nr->node);
		}
	}

	return n;
}

// MOS
NodeStore::size_type NodeStore::retrieveApplications(NodeRef target, NodeRefList& nl)
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->getType() == Node::TYPE_APPLICATION) {
		  if(nr->node->hasProxyId(target->getId())) {
		      n++;
		      nl.add(nr->node);
		    }
		}
	}

	return n;
}

// MOS
NodeStore::size_type NodeStore::numLocalApplications()
{
        Mutex::AutoLocker l(mutex);
	size_type n = 0;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		if (nr->node->isLocalApplication()) {
                        n++;
		}
	}

	return n;
}

bool NodeStore::update(NodeRef &node, NodeRefList *nl)
{
        Mutex::AutoLocker l(mutex);
	bool found = false;

	if (!node)
		return false;


	// There may be undefined nodes in the node store that should
	// be removed/merged with a 'defined' node that we create from a
	// node description. We loop through all nodes in the store and
	// compare their interface lists with the one in the 'defined' node.
	// If any interfaces match, we remove the matching nodes in the store
	// and eventually replace them with the new one.
	for (NodeStore::iterator it = begin(); it != end();) {
		NodeRecord *nr = *it;
		bool found_now = false;

		if(node == nr->node) { // MOS - added check - use node ids or interfaces if undefined

		  nr->node.lock();

		  const InterfaceRefList *ifaces = nr->node->getInterfaces();

		  for (InterfaceRefList::const_iterator it2 = ifaces->begin(); it2 != ifaces->end(); it2++) {
			InterfaceRef iface = *it2;

			if (node->hasInterface(iface)) {
				// Transfer all the "up" interface states to the updated node
				if (iface->isUp())
					node->setInterfaceUp(iface);
				// found_now = true; // MOS - see below
			}
		  }

		  nr->node.unlock();

		  found_now = true;
		}

		if (found_now) {
		        if(nr->node->isLocalApplication()) { // MOS - keep local app state
			   node->setLocalApplication(); // MOS
			   node->setProxyId(nr->node->getProxyId()); // MOS
			   Bloomfilter *bf = Bloomfilter::create(*(nr->node->getBloomfilter()));
			   if(bf) node->setBloomfilter(bf, false); // MOS
			   else HAGGLE_ERR("Creation of Bloom filter failed for node %s\n", node->getIdStr());
			   node->setMatchingThreshold(nr->node->getMatchingThreshold()); // MOS - important to avoid race on -c
			   node->setMaxDataObjectsInMatch(nr->node->getMaxDataObjectsInMatch()); // MOS - important to avoid race on -c
			   node->setDeleteStateOnDeRegister(nr->node->getDeleteStateOnDeRegister()); // SW: keep track of delete state
			   HAGGLE_DBG("Local application node %s has been updated in node store\n", node->getIdStr());
		        }
			else {
			  if(continuousBloomfilters) {
			    if(node->getNodeDescriptionCreateTime().getTimeAsMilliSeconds() >
			       nr->node->getNodeDescriptionCreateTime().getTimeAsMilliSeconds() + continuousBloomfilterUpdateInterval)
			      {
				HAGGLE_DBG2("Updating (replacing) Bloom filter of node %s\n", Node::nameString(nr->node).c_str());
				Bloomfilter *bf = Bloomfilter::create(*(nr->node->getBloomfilter()));
				if(bf) node->setBloomfilter2(bf); // MOS - keep old Bloom filter as secondary
			      }
			    else
			      {
				HAGGLE_DBG2("Updating (merging) Bloom filter of node %s\n", Node::nameString(nr->node).c_str());
				if(nr->node->getBloomfilter2()) {
				  Bloomfilter *bf2 = Bloomfilter::create(*(nr->node->getBloomfilter2()));
				  if(bf2) node->setBloomfilter2(bf2); // MOS - keep old Bloom filter
				}
				node->getBloomfilter()->merge(*(nr->node->getBloomfilter()));
			      }
			  }
			}

			if (nl)
				nl->push_back(nr->node);

			nr->node->setStored(false);
			if(preemptObsoleteNodeDescriptions() && node->getDataObject() != nr->node->getDataObject())
			  nr->node->getDataObject()->setObsolete(); // MOS

			node->setExchangedNodeDescription(nr->node->hasExchangedNodeDescription());

			it = erase(it);
			delete nr;
			found = true;

		} else {
			it++;
		}
	}
	if (found) {
		node->setStored(true);
		push_back(new NodeRecord(node));
	}

	return found;
}


// Remove neighbor with a specified interface
NodeRef NodeStore::remove(const InterfaceRef &iface)
{
        Mutex::AutoLocker l(mutex);

	if (!iface)
		return NULL;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (nr->node->hasInterface(iface)) {
			NodeRef node = nr->node;
			erase(it);
			node->setStored(false);
			delete nr;
			return node;
		}
	}

	return NULL;
}

// Remove all nodes of a specific type
int NodeStore::remove(const Node::Type_t type)
{
        Mutex::AutoLocker l(mutex);
	int n = 0;

	NodeStore::iterator it = begin();

	while (it != end()) {
		NodeRecord *nr = *it;

		if (nr->node->getType() == type) {
			it = erase(it);
			n++;
			nr->node->setStored(false);
			delete nr;
			continue;
		}
		it++;
	}

	return n;
}

// Remove neighbor with a specified interface
bool NodeStore::remove(const NodeRef &node)
{
        Mutex::AutoLocker l(mutex);

	if (!node)
		return false;

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		NodeRecord *nr = *it;

		if (nr->node == node) {
			erase(it);
			nr->node->setStored(false);
			delete nr;
			return true;
		}
	}

	return false;
}

//Social Node tracking
/**
* \param name is a string that has the new social group identifier for this node.
*/
void NodeStore::setMyNodeSocialGroupName( string name) {
     mySocialGroupName=name;
}

/**
* \return a string, containing this nodes social group name
*/

string NodeStore::returnMyNodeSocialGroupName() {
     return mySocialGroupName;
}

/**
* \return string containing the social ID name for the node.   A value of "" means the node has no social group name.
* \param node is a Node which we use to know its social group name.
*
* This function will actually examine a node for a metadata entry of 'SocialGroups', to
* identify if this node has an associated social group name.    If there is no social group
* name, it returns and empty string "".
*
* BUG: print statements for verification
*/
string NodeStore::returnNodeSocialGroupName( NodeRef node) {
        Metadata *m = NULL;
        DataObjectRef dObj = node->getDataObject();
        m = dObj->getMetadata()->getMetadata(string("SocialGroups"));
	if (m) {
           return m->getParameter("SocialId");
        }
	return "DefaultSocialGroup";

}

//print out all node info
/**
* This function will print out all relationships between nodes that have social group identifiers.
* It is used to verify correct internal storage of nodes and social group names, as well
* as assisting in debugging.
*
* BUG: print statements for verification
*/
void NodeStore::socialPrintGroups() {
   // printf("My social group: %s\n", (returnMyNodeSocialGroupName().c_str()));
    HAGGLE_DBG("My social group: %s\n", (returnMyNodeSocialGroupName().c_str()));

    List<string> socialNames = returnSocialGroups();
    for (List<string>::iterator it = socialNames.begin(); it != socialNames.end(); it++) {
        string name=*it;
       // printf("Social group %s has %d members: ", name.c_str(), returnNumInSocialGroup(name));
        HAGGLE_DBG("Social group %s has %d members: ", name.c_str(), returnNumInSocialGroup(name));
        NodeRefList nodeList = returnNodesPerSocialGroup(name);
        for (NodeRefList::iterator itt = nodeList.begin(); itt != nodeList.end(); itt++) {
            NodeRef node=*itt;
             HAGGLE_DBG("%s[%s],", Node::nameString(node).c_str(), returnNodeSocialGroup(node).c_str());
            // printf("%s[%s],", node->getName().c_str(), returnNodeSocialGroup(node).c_str() );

        }
	HAGGLE_DBG("\n");
	//printf("\n");
    }

}

/**
* \param node is a Node we wish to store it and its corresponding social group name.
*
* This routine identifies a nodes's social group name, and maintains appropriate
* data structures to identify associations between name and list of nodes, all unique group names,
* and one to one node<->social ID mapping.
*
* BUG: print statements for verification
*/

void NodeStore::insertNodeSocialGroup( NodeRef node) {
		Mutex::AutoLocker l(mutex); // CBMEN, HL

        string socialGroup=string(returnNodeSocialGroupName(node));
        if ( node->getType() != Node::TYPE_PEER) { return; }
	//node<->social group mapping
    	node_social_map_t::iterator it2 = social_node_map.find(node->getIdStr());
    	if (it2 == social_node_map.end() ) {
          string name = string(node->getIdStr());
          social_node_map.insert(make_pair(name, socialGroup));
    	} else {
	  social_node_map.erase(node->getIdStr());
          string name = string(node->getIdStr());
          social_node_map.insert(make_pair(name, socialGroup));
        }
	//map of (social group, list)
	//each list is nodes belonging to it
    	social_to_nodes_map_t::iterator it3 = social_to_nodes_map.find(socialGroup);
    	if (it3 == social_to_nodes_map.end() ) {
          list_nodes_id_in_social_group_t *newList = new list_nodes_id_in_social_group_t;
	  newList->clear();
	  newList->push_front(node->getIdStr());
	  // list_nodes_id_in_social_group_t::iterator it = newList->begin();
          // string& str = *it;
          // Pair < string, list_nodes_id_in_social_group_t * > a=make_pair(socialGroup, newList) ;
          // social_to_nodes_map.insert(make_pair(socialGroup, newList)); //a);
          social_to_nodes_map.insert(make_pair(socialGroup, newList));
    	} else {
          //add to group, or update it
          list_nodes_id_in_social_group_t *newList2 = (*it3).second ;
          for (list_nodes_id_in_social_group_t::iterator itt = newList2->begin();
             itt != newList2->end(); itt++) {
             string NodeId = (*itt);
             if (NodeId == node->getIdStr()) {
                newList2->erase(itt);
		break;
             }
          }
	  newList2->push_back(node->getIdStr());
	  	social_to_nodes_map_t::iterator it4 = social_to_nodes_map.find(socialGroup);
	  	if (it4 != social_to_nodes_map.end()) {
	  		social_to_nodes_map.erase(socialGroup);
	  	}
          social_to_nodes_map.insert(make_pair(socialGroup, newList2));
        }
}

/**
* \param node is a Node we wish to delete from our internal structures.
*
* This routine removes a nodes's appropriate
* data structures to free associations between name and list of nodes, all unique group names,
* and one to one node<->social ID mapping.
*
*
*/
void NodeStore::removeNodeSocialGroup( NodeRef node) {
	Mutex::AutoLocker l(mutex); // CBMEN, HL
	node_social_map_t::iterator it2 = social_node_map.find(Node::idString(node));
    	if (it2 != social_node_map.end() ) {
	  social_node_map.erase(Node::idString(node));
        }

        string socialGroup=returnNodeSocialGroupName(node);
    	social_to_nodes_map_t::iterator it3 = social_to_nodes_map.find(socialGroup);
    	if (it3 != social_to_nodes_map.end() ) {
          //add to group, or update it
          list_nodes_id_in_social_group_t *newList2 = (*it3).second ;
          for (list_nodes_id_in_social_group_t::iterator itt = newList2->begin();
             itt != newList2->end(); itt++) {
             string itNode = (*itt);
             if (itNode == node->getIdStr()) {
                newList2->erase(itt);
		break;
             }
          }
          social_to_nodes_map_t::iterator it4 = social_to_nodes_map.find(socialGroup);
		  	if (it4 != social_to_nodes_map.end()) {
		  		social_to_nodes_map.erase(socialGroup);
		  	}
          if (newList2->size() > 0) {
            social_to_nodes_map.insert(make_pair(socialGroup, newList2));
          }
        }
}

/**
* \return string containg the nodes social group name
* \param node takes a Node we wish to delete from our internal structures.
*
* This function returns the social group name associated with a node.   This
* differs from returnNodeSocialGroupName() as it uses the internal
* data structures from insertNodeSocialGroup()
*
* @see insertNodeSocialGroup(), @see returnNodeSocialGroupName()
*
*/
string NodeStore::returnNodeSocialGroup( NodeRef node) {
	Mutex::AutoLocker l(mutex); // CBMEN, HL
    	node_social_map_t::iterator it2 = social_node_map.find(Node::idString(node));
    	if (it2 != social_node_map.end() ) {
          return (*it2).second;
    	}
	//no match
        return "DefaultSocialGroup";
}

/**
* \return a List of nodes that belong to the same social group.
* \param socialGroup is the string of the social group name.
*
* This function will return a list of nodes that match the specified
* social group name.
*
*/
NodeRefList NodeStore::returnNodesPerSocialGroup(string socialGroup) {
	Mutex::AutoLocker l(mutex); // CBMEN, HL
        NodeRefList *nodeList = new NodeRefList;
    	social_to_nodes_map_t::iterator it = social_to_nodes_map.find(socialGroup);
    	if (it != social_to_nodes_map.end() ) {
          //add to group, or update it
          list_nodes_id_in_social_group_t *newList = (*it).second ;
          for (list_nodes_id_in_social_group_t::iterator itt = newList->begin();
             itt != newList->end(); itt++) {
             string itNode = (*itt);
             nodeList->push_back(retrieve(itNode));
          }
        }
        return *nodeList;
}

/**
* \return an integer of the number of nodes that match the social group name
* \param socialGroup is the string of the social group name
*
* This function returns the number of nodes that match the social group name.
*
*/

int NodeStore::returnNumInSocialGroup(string socialGroup) {
	Mutex::AutoLocker l(mutex); // CBMEN, HL
	int result = 0;
    	social_to_nodes_map_t::iterator it = social_to_nodes_map.find(socialGroup);
    	if (it != social_to_nodes_map.end() ) {
          //add to group, or update it
          list_nodes_id_in_social_group_t *newList = (*it).second ;
          result = newList->size();
        }
        return result;

}

/**
* \return a List of strings containing social group names.
*
* Returns a list of strings, holding all the unique node group names.
*
*/
List<string> NodeStore::returnSocialGroups() {
	Mutex::AutoLocker l(mutex); // CBMEN, HL
   List<string> retValue;
   	for (social_to_nodes_map_t::iterator it = social_to_nodes_map.begin(); it != social_to_nodes_map.end(); it++) {
	retValue.push_back((*it).first);
	}
   return retValue;
}


#ifdef DEBUG
void NodeStore::print()
{
        Mutex::AutoLocker l(mutex);
	int n = 0;

	printf("======== Node store list ========\n\n");

	if (empty()) {
		printf("No nodes in store\n");
		return;
	}
	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

                printf("Node: %d type=\'%s\' name=\'%s\' - %s stored=%s localApp=%s\n",
                       n++, nr->node->getTypeStr(),
                       Node::nameString(nr->node).c_str(),
                       (nr->node->isAvailable() && (nr->node->getType() == Node::TYPE_PEER || nr->node->getType() == Node::TYPE_UNDEFINED)) ? "Neighbor" : "Unconfirmed neighbor",
                       nr->node->isStored() ? "Yes" : "No",
		       nr->node->isLocalApplication() ? "Yes" : "No"); // MOS
		printf("Num objects in bloomfilter=%lu\n", nr->node->getBloomfilter()->numObjects());
                printf("id=%s proxyId=%s\n", nr->node->getIdStr(), nr->node->getProxyIdStr()); // MOS - added proxy
                printf("");
		nr->node->printInterfaces();

		const Attributes *attrs = nr->node->getAttributes();

		if (attrs->size()) {
			printf("Interests:\n");
			for (Attributes::const_iterator itt = attrs->begin(); itt != attrs->end(); itt++) {
				printf("\t\t%s\n", (*itt).second.getString().c_str());
			}
		}
	}

	printf("===============================\n\n");
}
#endif

// CBMEN, HL, Begin
void NodeStore::getNodesAsMetadata(Metadata *m) {

	Mutex::AutoLocker l(mutex);

	for (NodeStore::iterator it = begin(); it != end(); it++) {
		const NodeRecord *nr = *it;

		Metadata *dm = m->addMetadata("Node");
		if (!dm) {
			continue;
		}

		dm->setParameter("type", nr->node->getTypeStr());
		dm->setParameter("name", nr->node->getName());
		dm->setParameter("id", nr->node->getIdStr());
		dm->setParameter("proxy_id", nr->node->getProxyIdStr());
		dm->setParameter("confirmed", (nr->node->isAvailable() && (nr->node->getType() == Node::TYPE_PEER || nr->node->getType() == Node::TYPE_UNDEFINED)) ? "Neighbor" : "Unconfirmed neighbor");
		dm->setParameter("stored", nr->node->isStored() ? "true" : "false");
		dm->setParameter("local_application", nr->node->isLocalApplication() ? "true" : "false");
		dm->setParameter("num_objects_in_bloom_filter", nr->node->getBloomfilter()->numObjects());

		const Attributes *attrs = nr->node->getAttributes();
		for (Attributes::const_iterator it = attrs->begin(); it != attrs->end(); it++) {
			Metadata *dmm = dm->addMetadata("Attribute");
			if (dmm) {
				dmm->setParameter("name", (*it).second.getName());
				dmm->setParameter("value", (*it).second.getValue());
				dmm->setParameter("weight", (*it).second.getWeight());
			}
		}

		Metadata *dmm = dm->addMetadata("Interfaces");
		if (dmm) {
			nr->node->getInterfacesAsMetadata(dmm);
		}
	}

}
// CBMEN, HL, End
