/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 *   James Mathewson (JM, JLM)
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
#ifndef _NODE_H
#define _NODE_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class Node;

#include <libcpphaggle/Reference.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Thread.h>
#include <openssl/sha.h>

using namespace haggle;

#include "Attribute.h"
#include "Interface.h"
#include "Metadata.h"
#include "DataObject.h"
#include "Bloomfilter.h"
#include "Debug.h"


/* Some attribute strings in a node's metadata */
#define NODE_DESC_ATTR "NodeDescription"

#define NODE_METADATA "Node"
#define NODE_METADATA_TYPE_PARAM "type"
#define NODE_METADATA_ID_PARAM "id"
#define NODE_METADATA_NAME_PARAM "name"
#define NODE_METADATA_PROXY_ID_PARAM "proxy_id" // MOS
#define NODE_METADATA_THRESHOLD_PARAM "resolution_threshold"
#define NODE_METADATA_MAX_DATAOBJECTS_PARAM "resolution_limit"

#define NODE_DEFAULT_DATAOBJECTS_PER_MATCH 10
#define NODE_DEFAULT_MATCH_THRESHOLD 0 // MOS - original default was 10

typedef Reference<Node> NodeRef;

/** */
#ifdef DEBUG_LEAKS
class Node: public LeakMonitor
#else
class Node
#endif
{
public:
	// Different types of nodes. Determines among other things how to deal
	// with the ID
	typedef enum {
		TYPE_UNDEFINED = 0, // An uninitialized state of the node
		TYPE_LOCAL_DEVICE,
		TYPE_APPLICATION,
		TYPE_PEER,
		TYPE_GATEWAY,
		_NUM_NODE_TYPES
	} Type_t;

	// Level of abstraction for node description attribute.
	// This can influence the sharing of node descriptions.
	typedef enum {
		NDATTR_ID = 0, // full node id (default)
		NDATTR_TYPE,
		NDATTR_EMPTY,
		NDATTR_NONE,
		_NUM_NDATTR_TYPES
	} NDAttr_t;
private:
	static NDAttr_t nodeDescriptionAttribute;
	static unsigned long nodeDescriptionAttributeWeight;
public:
	static void setNodeDescriptionAttribute(NDAttr_t ndAttribute) { Node::nodeDescriptionAttribute = ndAttribute; }
	static NDAttr_t getNodeDescriptionAttribute() { return Node::nodeDescriptionAttribute; }
	static void setNodeDescriptionAttributeWeight(unsigned long  ndAttributeWeight) { Node::nodeDescriptionAttributeWeight = ndAttributeWeight; }
	static unsigned long  getNodeDescriptionAttributeWeight() { return Node::nodeDescriptionAttributeWeight; }

#define NODE_ID_LEN SHA_DIGEST_LENGTH
#define MAX_NODE_ID_STR_LEN (2*NODE_ID_LEN+1) // +1 for null termination
	typedef unsigned char Id_t[NODE_ID_LEN];
protected:	
	/**
		The type of the node.
	*/
        Type_t type;
	/**
		A unique node ID, which is a SHA1 hash.
	*/
	Id_t id;

	/**
		The node ID in string format.
	*/
        char idStr[MAX_NODE_ID_STR_LEN];

	/**
		Corresponding fields for the node id of the local device
		if the node is an application node.
	*/
	Id_t proxyId; // MOS
        char proxyIdStr[MAX_NODE_ID_STR_LEN]; // MOS

	/**
		Static count to keep track of the total number of node objects
		created in the system. It is incremented each time a new node 
		object is created.
	*/
        static unsigned long totNum;

	/**
		Static member that contains string representations of node types.
	*/
	static const char *typestr[];
	/**
		An internal count of he created node objects. Each newly created
		node object will have a diffent number.
	*/
        unsigned long num;
	/**
		A descriptive name of the node. In most cases its hostname.
	*/
	string name; 

	/**
		A boolean that 
	*/
        bool nodeDescExch;

	/**
		The data object associated with this node. It is the most recent 
		node description that we have received from the node in the
		format sent on the network. If the node was created from 
		a received node description, then that data object will be stored here,
		otherwise, this will be an entirely new data object.
	*/
        DataObjectRef dObj;

	/**
		A list of interfaces that are known to be associated with the
		node. These may be marked 'up' or 'down'. A node with at least
		one interface marked 'up' is considered active. It is otherwise 
		considered inactive.
	*/
        InterfaceRefList interfaces;

	/**
		A bloomfilter with the data objects that the node has already
		received. The bloomfilter is updated every time we receive a new
		node description, or whenever we send a data object to the node.
		
		Why a pointer? Otherwise we'd get a circular dependency during 
		compilation.
	*/
	Bloomfilter *doBF;
	Bloomfilter *doBF2;

	/**
		A utility function to calculate the node ID based on the information
		in the node object.
		(Currently only makes sense for nodes of type Node::TYPE_THIS_NODE)
	*/
        void calcId();
	/**
		A utility function to calculate the node ID in a string (char *) format.
	*/
        void calcIdStr();
        void calcProxyIdStr(); // MOS
	/**
		A function used to initialize the node. Used by the constructors.
	*/
	/**
		Boolean that indicates whether this node is stored in the node store
		or not.
	*/
	bool stored;
        bool createdFromNodeDescription;
	Timeval nodeDescriptionCreateTime;
	Timeval nodeDescriptionUpdateTime;
	/* Time when this node object was created */
	Timeval createTime; 
	/* TIme when last node query was made for this node */
	Timeval lastDataObjectQueryTime;
	Timeval lastRoutingUpdateTime;
	inline bool init_node(const Node::Id_t _id);
	unsigned long matchThreshold;
	unsigned long numberOfDataObjectsPerMatch;
	bool localApplication; // MOS - need to distinguish local application nodes

        Node(Type_t _type, const string name = "Unnamed node", 
	     Timeval _nodeDescriptionCreateTime = -1);
        Node(const Node &n); // Copy constructor

// SW: START: delete application state
    bool deleteStateOnDeRegister;
// SW: END: delete application state
public:
	static Node *create(const DataObjectRef& dObj);
	static Node *create(Type_t type, const DataObjectRef& dObj);
	static Node *create(Type_t type = TYPE_UNDEFINED, 
			    const string name = "Unnamed node", 
			    Timeval nodeDescriptionCreateTime = -1);
	static Node *create_with_id(Type_t type, const Node::Id_t id, 
				    const string name = "Unnamed node", 
				    Timeval nodeDescriptionCreateTime = -1);
	static Node *create_with_id(Type_t type, const char *idStr, 
				    const string name = "Unnamed node", 
				    Timeval nodeDescriptionCreateTime = -1);
	virtual ~Node() = 0;
	
	static const unsigned char *strIdToRaw(const char *strId);
	static const char *typeToStr(Type_t type);
	static Type_t strToType(const char *type);
        Type_t getType() const;
	const char *getTypeStr() const { return typestr[type]; }
        const unsigned char *getId() const;
	void setId(const Id_t _id);
        const char *getIdStr() const;
	static string idString(const NodeRef& node); // MOS
	unsigned long getNum() const { return num; }
	bool isStored() const { return stored; }
	void setStored(bool _stored = true) { stored = _stored; }
	string getName() const;
	static string nameString(const NodeRef& node); // MOS
	void setName(const string _name);
        const unsigned char *getProxyId() const; // MOS
	void setProxyId(const Id_t _proxy_id); // MOS
        const char *getProxyIdStr() const; // MOS

        // Create a metadata object from this node
        Metadata *toMetadata(bool withBloomfilter = true) const;
	// Create a node from metadata
	static Node *fromMetadata(const Metadata& m);

	// Functions to access and manipulate the node's interfaces
	/**
	The given interface is the property of the caller.
	*/
	bool addInterface(InterfaceRef iface);
	/**
	The given interface is the property of the caller.
	*/
	bool removeInterface(const InterfaceRef& iface);
	/**
	The returned interface list is the property of the node.
	*/
	const InterfaceRefList *getInterfaces() const;
	/**
	The given interface is the property of the caller.
	*/
	bool hasInterface(const InterfaceRef iface) const;
	/**
	Check if this node has the given node id as the proxy id.
	*/
	bool hasProxyId(const Id_t _id) const; // MOS
        /**
           Returns the number of interfaces marked as up.
        */
        unsigned int numActiveInterfaces() const;
	/**
	The given interface is the property of the caller.
	*/
	bool setInterfaceDown(const InterfaceRef iface);
	/**
	The given interface is the property of the caller.
	*/
	bool setInterfaceUp(const InterfaceRef iface);

#ifdef DEBUG
	void printInterfaces() const;
#endif
	void getInterfacesAsMetadata(Metadata *m) const; // CBMEN, HL
	// Node status functions
	bool hasExchangedNodeDescription() const {
		return nodeDescExch;
        }
        void setExchangedNodeDescription(bool yes) {
                nodeDescExch = yes;
        }
	/**
		Returns true iff this node is available (has any interface marked
		as up).
	*/
        bool isAvailable() const;
	/**
		Returns true iff this node is considered a neighbor node.
	*/
        bool isNeighbor() const;
	/**
		Returns true iff this node is considered a local (not remote) application node.
	*/
        bool isLocalApplication() const { return localApplication; } // MOS
	void setLocalApplication() { localApplication = true; }; // MOS
	/**
		Returns true iff the node represents the local device.
	 */
	virtual bool isLocalDevice() const { return false; }

	void addNodeDescriptionAttribute(DataObjectRef& dObj) const; // MOS

        DataObjectRef getDataObject(bool withBloomfilter = true) const;
		
	unsigned long getMatchingThreshold() const { return matchThreshold; } // MOS
	unsigned long getMaxDataObjectsInMatch() const { return numberOfDataObjectsPerMatch; } // MOS

	void setMatchingThreshold(unsigned long value); // MOS
	void setMaxDataObjectsInMatch(unsigned long value); // MOS

        // Wrappers for adding, removing and updating attributes in
        // the node description associated with this node
        int addAttribute(const Attribute &a);
        int addAttribute(const string name, const string value, const unsigned long weight = 1);
        int removeAttribute(const Attribute &a);
        int removeAttribute(const string name, const string value = "*");
        const Attribute *getAttribute(const string name, const string value = "*", const unsigned int n = 0) const;
        const Attributes *getAttributes() const;

        // Bloomfilter functions
	const Bloomfilter *getBloomfilter() const;
	Bloomfilter *getBloomfilter();
	Bloomfilter *getBloomfilter2();
	bool setBloomfilter(const char *base64, const bool set_create_time = false);
	bool setBloomfilter(Bloomfilter *bf, const bool set_create_time = false);
	bool setBloomfilter(const Bloomfilter& bf, const bool set_create_time = false);
	bool setBloomfilter2(Bloomfilter *bf);

	/**
		Returns true iff the data object is in the bloomfilter.
	*/	
	bool has(const unsigned char *blob, size_t len) const;
	bool has(const DataObjectId_t& id) const;
	bool has(const DataObjectRef &dObj) const;

	/**
	 * JJOY checks if the original or parent dataobject for network coding
	 * is in bloom filter
	 */
	bool hasThisOrParentDataObject(const DataObjectRef &dObj) const;
	bool hasParentDataObject(const DataObjectRef &dObj) const;
	bool hasNetworkCodingParentDataObject(const DataObjectRef &dObj) const;
	bool hasFragmentationParentDataObject(const DataObjectRef &dObj) const;
	
	/**
		Sets the create time of this node. This should only be done (and will 
		only happen for) a node description marked as the thisNode node 
		description.
		
		This function should be called whenever the node description changes
		significantly (such as when the bloomfilter is changed, or the interests
		of the node is changed).
	*/
	void setNodeDescriptionCreateTime(Timeval t = Timeval::now());
	void setNodeDescriptionUpdateTime(Timeval t = Timeval::now()); // MOS
	/**
		Returns the create time of the node description.
	*/
	Timeval getNodeDescriptionCreateTime() const;
	Timeval getNodeDescriptionUpdateTime() const; // MOS
	Timeval getCreateTime() const;
	Timeval getLastDataObjectQueryTime() const;
	Timeval getLastRoutingUpdateTime() const;
	void setLastDataObjectQueryTime(Timeval t);
	void setLastRoutingUpdateTime(Timeval t);
        // Operators
        // friend bool operator<(const Node &n1, const Node &n2);
        friend bool operator==(const Node &n1, const Node &n2);
        friend bool operator!=(const Node &n1, const Node &n2);

// SW: START: delete application state
    bool getDeleteStateOnDeRegister() { return deleteStateOnDeRegister; }
    void setDeleteStateOnDeRegister(bool d) { deleteStateOnDeRegister = d; }
// SW: END: delete application state

};

typedef ReferenceList<Node> NodeRefList;

class UndefinedNode : public Node
{
	friend class Node;
	UndefinedNode(const string name = "undefined node", Timeval _nodeDescriptionCreateTime = -1) : 
		Node(class_type, name, _nodeDescriptionCreateTime) {}
public:
	static const Type_t class_type = TYPE_UNDEFINED;
	~UndefinedNode() {}
};

typedef Reference<UndefinedNode> UndefinedNodeRef;
typedef ReferenceList<UndefinedNode> UndefinedNodeRefList;

class PeerNode : public Node
{
	friend class Node;
protected:
	PeerNode(Type_t type, const string name = "peer node", Timeval _nodeDescriptionCreateTime = -1) : 
		Node(type, name, _nodeDescriptionCreateTime) {}
	PeerNode(const string name = "peer node", Timeval _nodeDescriptionCreateTime = -1) : 
		Node(class_type, name, _nodeDescriptionCreateTime) {}
public:
	static const Type_t class_type = TYPE_PEER;
	~PeerNode() {}
};

typedef Reference<PeerNode> PeerNodeRef;
typedef ReferenceList<PeerNode> PeerNodeRefList;

class LocalDeviceNode : public PeerNode
{
	friend class Node;
	friend class PeerNode;
	LocalDeviceNode(const string name = "local device node", Timeval _nodeDescriptionCreateTime = -1) : 
		PeerNode(class_type, name, _nodeDescriptionCreateTime) {}
public:
	static const Type_t class_type = TYPE_LOCAL_DEVICE;
	~LocalDeviceNode() {}
	bool isLocalDevice() const { return true; }
};

class GatewayNode : public Node
{
	friend class Node;
	GatewayNode(const string name = "gateway node", Timeval _nodeDescriptionCreateTime = -1) : 
		Node(class_type, name, _nodeDescriptionCreateTime) {}
public:
	static const Type_t class_type = TYPE_GATEWAY;
	~GatewayNode() {}
};

typedef Reference<GatewayNode> GatewayNodeRef;
typedef ReferenceList<GatewayNode> GatewayNodeRefList;

class ApplicationNode : public Node
{
	friend class Node;
	long filterEventId, eventId;

	/**
	 This is a set of private events that correspond to filters that
	 are registered by some manager on behalf of the node.
	 
	 For example, if the node is an application, these event types are 
	 the public events and private filter events that the application 
	 is interested in.
	 */
	Map<long, char> eventInterests;
	
	ApplicationNode(const string name = "application node", Timeval _nodeDescriptionCreateTime = -1) : 
		Node(class_type, name, _nodeDescriptionCreateTime), filterEventId(-1), eventId(-1)
        {}
public:
	static const Type_t class_type = TYPE_APPLICATION;
        ApplicationNode(const ApplicationNode& n) : Node(n), filterEventId(n.filterEventId), 
               eventId(n.eventId), eventInterests(n.eventInterests) {}
	
	~ApplicationNode();
	ApplicationNode *copy() const { return new ApplicationNode(*this); }

	/**
	
		Add event interest to this application node.
	 */
	bool addEventInterest(long type);
	/**
		Remove event interest from this application node.
	 */
	void removeEventInterest(long type);
	/**
		Check whether application node is interested in a particular event.
	 */
	bool hasEventInterest(long type) const;

// SW: START: fixing mem-leak here where private event was not free'd
	void clearEventInterests();
// SW: END: fixing mem-leak here where private event was not free'd

	bool setFilterEvent(long feid);
	long getFilterEvent() const { return filterEventId; }

// CBMEN, HL, Begin
	bool clearFilterEvent();
// CBMEN, HL, End
	
};

typedef Reference<ApplicationNode> ApplicationNodeRef;
typedef ReferenceList<ApplicationNode> ApplicationNodeRefList;

#endif /* _NODE_H */
