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
#ifndef _NODESTORE_H
#define _NODESTORE_H

#include <libcpphaggle/List.h>
#include <libcpphaggle/Mutex.h>

#include "Node.h"

//JM Define social structures 
typedef HashMap< string, string > node_social_map_t;
typedef NodeRefList list_nodes_in_social_group_t;
typedef List< string > list_nodes_id_in_social_group_t;
typedef HashMap< string, list_nodes_id_in_social_group_t * > social_to_nodes_map_t;

/**
	The node record only holds a node now, but it may be expanded in the future.
*/
class NodeRecord {
public:
	NodeRef node;
	NodeRecord(const NodeRef &_node) : node(_node) {}
	~NodeRecord() {}
	bool operator==(const NodeRef &n) { return node == n; }
};

/** 
 DEADLOCK WARNING!

 READ CAREFULLY BEFORE USING THE NODE STORE API.
 
 It is forbidden to hold locks on reference objects, i.e., "Reference<MyType> obj" (or
 other locks for that matter), while accessing the node store.
 
 Holding such locks may cause deadlocks when another thread holding a lock on the same object
 accesses the node store.
 
 Consider the following scenario:
 
 Thread 1.
 
 obj.lock();
 
 NodeRef node = kernel->getNodeStore()->retreive(iface); 
 
 <--- Context switch on the first line in the retreive function.
 
 Thread 2.
 
 node.lock();
 
 obj.lock() <--- blocked since thread 1 holds this lock.

 <--- Context switch to thread 2.
 
 Thread 2.
 
 (while in NodeStore::retreive() )
 
 node.lock(); (same as calling a dereference, e.g., node->getIdStr()) <--- This will block!
 
 DEADLOCK!!!
 
 
 Actually, this type of dual object competition can happen anywhere in the code, but it is
 just more likely to happen while accessing the node store, for example, by dereferencing an
 object as NodeStore::retreive() is called as so:
 
 kernel->getNodeStore()->retreive(obj->getInterface());
 
 This is equivalent to:

 obj.lock();
 iface = obj->getInterface();
 
 NodeRef node = kernel->getNodeStore()->retreive(iface); 
 
 obj.unlock();
 
 
 A Good RULE is therefore to never dereference an object in the argument to 
 NodeStore::retreive(), or any other node store function.

 */

class NodeStore : protected List<NodeRecord *>
{
	Mutex mutex;
	/*
	  Internal, unlocked versions of functions below.
	 */
        bool _stored(const NodeRef &node, bool mustBeNeighbor = false);
        bool _stored(const Node &node, bool mustBeNeighbor = false);
        bool _stored(const Node::Id_t id, bool mustBeNeighbor = false);
	bool _stored(const string idStr, bool mustBeNeighbor = false);

	bool preemptObsoleteNodeDesc; // MOS
	bool preemptLocalObsoleteNodeDesc; // MOS
	bool continuousBloomfilters; // MOS
	unsigned long continuousBloomfilterUpdateInterval; // MOS
	// JM: Start social tracking
   	node_social_map_t social_node_map;
   	social_to_nodes_map_t social_to_nodes_map;
	string mySocialGroupName; //JM add Fn() to support this from node.
	// JM: End social tracking

public:
	/**
		The Critiera class is a matching functor that can be
		used as a generic way to retrieve nodes from the node
		store. The basic criteria matches all nodes. When retrieving
		nodes, the caller should inherit this class and implement
		their own matching criteria.
	*/
	class Criteria {
	public:
		virtual bool operator() (const NodeRef& n) const
		{
			return true;
		}
		virtual ~Criteria() {}
	};
	NodeStore();
	~NodeStore();

	// Locking for the store
	void lock() { mutex.lock(); }
	void unlock() { mutex.unlock(); }
	bool trylock() { return mutex.trylock(); }
	/**
		Check if a node is currently stored in the store. The caller
		may optionally specify whether the given node must be marked
		a neighbor in the store.
		Returns: boolean indicating whether the node was found or not.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        bool stored(const NodeRef& node, bool mustBeNeighbor = false);
	/**
		Check if a node is currently stored in the store. The caller
		may optionally specify whether the given node must be marked
		a neighbor in the store.
		Returns: boolean indicating whether the node was found or not.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        bool stored(const Node& node, bool mustBeNeighbor = false);
	/**
		Check if a node is currently stored in the store. The caller
		may optionally specify whether the given node must be marked
		a neighbor in the store.
		Returns: boolean indicating whether the node was found or not.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        bool stored(const Node::Id_t id, bool mustBeNeighbor = false);
	/**
		Check if a node is currently stored in the store. The caller
		may optionally specify whether the given node must be marked
		a neighbor in the store.
		Returns: boolean indicating whether the node was found or not.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	bool stored(const string idStr, bool mustBeNeighbor = false);
	/**
		Add a new node to the node store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or a//JM add Fn() to support this from node.
n interface reference while calling this function.
	*/
        bool add(NodeRef& node);
	/**
		Add a new node to the node store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        NodeRef add(Node *node);

	/**
		Retrieve a node from the node store. The caller may optionally 
		specify to only retrieve current neighbors.
		Returns: A node reference to the specified node or a null-reference
		if it was not found in the store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        NodeRef retrieve(const NodeRef& node, bool mustBeNeighbor = false);
	/**
		Retrieve a node from the node store. The caller may optionally 
		specify to only retrieve current neighbors.
		Returns: A node reference to the specified node or a null-reference
		if it was not found in the store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        NodeRef retrieve(const Node& node, bool mustBeNeighbor = false);
	/**
		Retrieve a node from the node store. The caller may optionally 
		specify to only retrieve current neighbors.
		Returns: A node reference to the specified node or a null-reference
		if it was not found in the store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        NodeRef retrieve(const Node::Id_t id, bool mustBeNeighbor = false);
	/**
		Retrieve a node from the node store. The caller may optionally 
		specify to only retrieve current neighbors.
		Returns: A node reference to the specified node or a null-reference
		if it was not found in the store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	NodeRef retrieve(const string& id, bool mustBeNeighbor = false);
	/**
		Retrieve a node from the node store. The caller may optionally 
		specify to onl//JM add Fn() to support this from node.
y retrieve current neighbors.
		Returns: A node reference to the specified node or a null-reference
		if it was not found in the store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        NodeRef retrieve(const InterfaceRef& iface, bool mustBeNeighbor = false);

	/**
		Retrieve a list of all nodes matching a specific criteria.
		Returns NULL if there are no nodes that match the given criteria.
		The returned list has to be freed by the caller.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	size_type retrieve(const Criteria& crit, NodeRefList& nl);
	/**JM add Fn() to support this from node.

		Retrieve a list of all nodes of a specific type.
		Returns NULL if there are no nodes that match the given type.
		The returned list has to be freed by the caller.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	size_type retrieve(Node::Type_t type, NodeRefList& nl);
	
	template<typename T>
	size_type retrieve(ReferenceList<T>& nl)
	{
		Mutex::AutoLocker l(mutex);
		size_type n = 0;
		
		for (NodeStore::iterator it = begin(); it != end(); it++) {
			const NodeRecord *nr = *it;
			
			if (nr->node->getType() == T::class_type) {
				n++;
				nl.add(nr->node);
			}
		}
		
		return n;
	}
	
	
	/**
		Retrieve a list of all current and confirmed neighbors.
		Returns NULL if there are no neighbors. The list has to be freed 
		by the caller.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	size_type retrieveNeighbors(NodeRefList& nl);
	size_type retrieveDefinedNeighbors(NodeRefList& nl); // MOS

    /**
        SW: Grab all the nodes in the node store.

		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
    */
    size_type retrieveAllNodes(NodeRefList& nl);

	/**
		Returns the number of current neighbors in the store.
	*/
	size_type numNeighbors();
	/**
		Retrieve a list of (local) applications.
		Returns NULL if there are none. The list has to be freed 
		by the caller.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	size_type retrieveLocalApplications(NodeRefList& nl); // MOS
	size_type retrieveApplications(NodeRefList& nl); // MOS
	size_type retrieveApplications(NodeRef target, NodeRefList& nl); // MOS
	/**
		Returns the number of local application nodes in the store
	*/
	size_type numLocalApplications(); // MOS
	/**
		Remove a node from the node store.

		Returns true if the node was removed, and false if the node
		was not located in the store.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
	bool remove(const NodeRef& node);
	/**
		Remove all nodes in the store of a specific type.

		Returns: The number of nodes removed, or -1 on error.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        int remove(const Node::Type_t type);

	/**
		Remove a node that have a specific interface.

		Returns: A reference to the removed node.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        NodeRef remove(const InterfaceRef& iface);
	/**
		Update a node in the node store given a "fresher" version
		of the node. The interfaces of the node given as in-argument is matched
		against the nodes in the store. If at least one interface given node matches 
		a node in the store, the stored node will be updated.
		
		DEADLOCK WARNING: the calling thread may not hold the lock on a object 
		reference or an interface reference while calling this function.
	*/
        bool update(NodeRef &inNode, NodeRefList *nl = NULL);

	bool preemptObsoleteNodeDescriptions() { return preemptObsoleteNodeDesc; } // MOS
	void setPreemptObsoleteNodeDescriptions(bool on) { preemptObsoleteNodeDesc = on; } // MOS

	bool preemptLocalObsoleteNodeDescriptions() { return preemptLocalObsoleteNodeDesc; } // MOS
	void setPreemptLocalObsoleteNodeDescriptions(bool on) { preemptLocalObsoleteNodeDesc = on; } // MOS

	void setContinuousBloomfilters(bool on) { continuousBloomfilters = on; } // MOS
	void setContinuousBloomfilterUpdateInterval(unsigned long ms) { continuousBloomfilterUpdateInterval = ms; } // MOS

	//JM SOcial node tracking
	string returnMyNodeSocialGroupName();
	string returnNodeSocialGroupName( NodeRef node);
	void setMyNodeSocialGroupName( string name);
	void insertNodeSocialGroup( NodeRef node);
        void removeNodeSocialGroup( NodeRef node);
        string returnNodeSocialGroup( NodeRef node);
        NodeRefList returnNodesPerSocialGroup(string socialGroup);
        int returnNumInSocialGroup(string socialGroup);
        List<string> returnSocialGroups();
  	void socialPrintGroups();	
#ifdef DEBUG
        void print();
#endif
        void getNodesAsMetadata(Metadata *m); // CBMEN, HL
};

#endif /* _NODESTORE_H */
