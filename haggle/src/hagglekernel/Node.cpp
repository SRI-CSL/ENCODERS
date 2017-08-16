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
 *   Joshua Joy (JJ, jjoy)
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
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Thread.h>
#include <openssl/sha.h>
#include <base64.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include "Node.h"
#include "Interface.h"
#include "Attribute.h"
#include "DataObject.h"
#include "Event.h"
#include "Filter.h"
#include "Utility.h"
#include "Trace.h"
#include "XMLMetadata.h"

using namespace haggle;

#if defined(OS_WINDOWS_MOBILE)
#include <GetDeviceUniqueId.h>
#elif defined(OS_WINDOWS_DESKTOP)
#include <iphlpapi.h>
#elif defined(OS_UNIX)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
//#include <net/ethernet.h>
#include <arpa/inet.h>

#ifdef OS_MACOSX
#include <net/if_dl.h>
#endif

#endif /* OS_WINDOWS_MOBILE */

Node::NDAttr_t Node::nodeDescriptionAttribute = NDATTR_ID;
unsigned long Node::nodeDescriptionAttributeWeight = 1;

unsigned long Node::totNum = 0;

const char *Node::typestr[] = {
	"undefined",
	"local_device", // Local device node is really a peer
	"application",
	"peer",
	"gateway",
	NULL
};

inline bool Node::init_node(const unsigned char *_id)
{
	memset(id, 0, sizeof(Id_t));
	memset(idStr, 0, MAX_NODE_ID_STR_LEN);
	memset(proxyId, 0, sizeof(Id_t)); // MOS
	memset(proxyIdStr, 0, MAX_NODE_ID_STR_LEN); // MOS

	if (createdFromNodeDescription) {
		struct base64_decode_context b64_ctx;
		size_t decodelen;
		const char *pval;

		nodeDescriptionCreateTime = dObj->getCreateTime();
		nodeDescriptionUpdateTime = dObj->getUpdateTime();
		if(!nodeDescriptionUpdateTime.isValid()) nodeDescriptionUpdateTime = nodeDescriptionCreateTime;

		Metadata *nm = dObj->getMetadata()->getMetadata(NODE_METADATA);

		if (!nm) {
		        HAGGLE_DBG("Missing node metata\n");
			return false;
		}

		pval = nm->getParameter(NODE_METADATA_ID_PARAM);

		if (!pval) {
		        HAGGLE_DBG("Missing node id metata\n");
			return false;
		}

		decodelen = NODE_ID_LEN;
		base64_decode_ctx_init(&b64_ctx);
		
		if (!base64_decode(&b64_ctx, pval, strlen(pval), (char *)id, &decodelen)) {
		        HAGGLE_DBG("Cannot decode base64 id\n");
			return false;
		}
		
		calcIdStr();

		pval = nm->getParameter(NODE_METADATA_NAME_PARAM);

		if (pval)
			name = pval;

		pval = nm->getParameter(NODE_METADATA_THRESHOLD_PARAM);

		if (pval)
			matchThreshold = strtoul(pval, NULL, 10);

		pval = nm->getParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM);

		if (pval)
			numberOfDataObjectsPerMatch = strtoul(pval, NULL, 10);

		pval = nm->getParameter(NODE_METADATA_PROXY_ID_PARAM); // MOS

		if (pval) {
		
		  decodelen = NODE_ID_LEN;
		  base64_decode_ctx_init(&b64_ctx);
		  
		  if (!base64_decode(&b64_ctx, pval, strlen(pval), (char *)proxyId, &decodelen)) {
		    HAGGLE_DBG("Cannot decode base64 proxy id\n");
		    return false;
	          }

		  calcProxyIdStr();
		}

		/*
		Should we really override the wish of another node to receive all
		matching data objects? And in that case, why set it to our rather
		conservative default value?
		if (numberOfDataObjectsPerMatch == 0)
		numberOfDataObjectsPerMatch = NODE_DEFAULT_DATAOBJECTS_PER_MATCH;
		*/

		Metadata *bm = nm->getMetadata(BLOOMFILTER_METADATA);

		if (bm) {
			if (!setBloomfilter(Bloomfilter::fromMetadata(*bm))) {
				HAGGLE_ERR("Bad bloomfilter metadata\n");
				return false;
			}
		}

		Metadata *im = nm->getMetadata(INTERFACE_METADATA);

		// A node without interfaces should not really be valid
		if(type != Node::TYPE_APPLICATION) { // MOS - make exception for application nodes
		  if (!im) {
		        HAGGLE_ERR("Missing interface metadata\n");
			return false;
		  }
		}
		while (im) {
			InterfaceRef iface = Interface::fromMetadata(*im);

			if (iface) {
				addInterface(iface);
			} else {
				HAGGLE_ERR("Could not create interface from metadata\n");
			}
			im = nm->getNextMetadata();
		}
	}

	if (isLocalDevice()) {
		dObj->setIsThisNodeDescription(true);
		calcId();
	} else if (type == TYPE_UNDEFINED) {
		if (_id) {
			HAGGLE_DBG("Attempted to create undefined node with ID. ID ignored.\n");
		}
		// CBMEN, HL, Begin
		char tmp[15];
		sprintf(tmp, "%d", num);
		string _tmp = tmp;
		name = "[Unnamed node " + _tmp + "]";
		strncpy(idStr, "[Unknown id]", MAX_NODE_ID_STR_LEN);
		// CBMEN, HL, End
	} else if (_id) {
		memcpy(id, _id, sizeof(Id_t));
		calcIdStr();
	} 

	return true;
}

Node::Node(const Type_t _type, const string _name, Timeval _nodeDescriptionCreateTime) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_NODE),
#endif
	type(_type), num(totNum++), name(_name), nodeDescExch(false), 
	dObj(NULL), doBF(NULL), doBF2(NULL), stored(false), createdFromNodeDescription(false),
	nodeDescriptionCreateTime(_nodeDescriptionCreateTime), 
	nodeDescriptionUpdateTime(_nodeDescriptionCreateTime), 
	createTime(Timeval::now()),
	lastDataObjectQueryTime(-1, -1),
	lastRoutingUpdateTime(-1, -1),
	matchThreshold(NODE_DEFAULT_MATCH_THRESHOLD), 
	numberOfDataObjectsPerMatch(NODE_DEFAULT_DATAOBJECTS_PER_MATCH),
	localApplication(false), // MOS
	deleteStateOnDeRegister(false) // SW
{
        memset(id, 0, sizeof(Id_t)); // MOS
	memset(idStr, 0, MAX_NODE_ID_STR_LEN); // MOS
}

Node::Node(const Node& n) :
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_NODE),
#endif
	type(n.type), num(totNum++), name(n.name), 
	nodeDescExch(n.nodeDescExch), 
	dObj(NULL), interfaces(n.interfaces), 
	doBF(n.doBF ? Bloomfilter::create(*n.doBF) : NULL), // SW - use n.doBF? instead of doBF?
	doBF2(n.doBF2 ? Bloomfilter::create(*n.doBF2) : NULL), // MOS
	stored(n.stored), 
        createdFromNodeDescription(n.createdFromNodeDescription),
	nodeDescriptionCreateTime(n.nodeDescriptionCreateTime),
	nodeDescriptionUpdateTime(n.nodeDescriptionUpdateTime),
	createTime(n.createTime),
	lastDataObjectQueryTime(n.lastDataObjectQueryTime),
	lastRoutingUpdateTime(n.lastRoutingUpdateTime),
	matchThreshold(n.matchThreshold),
	numberOfDataObjectsPerMatch(n.numberOfDataObjectsPerMatch),
	localApplication(n.localApplication), // MOS
	deleteStateOnDeRegister(n.deleteStateOnDeRegister)
{
	memcpy(id, n.id, NODE_ID_LEN);
	strncpy(idStr, n.idStr, MAX_NODE_ID_STR_LEN);
	memcpy(proxyId, n.proxyId, NODE_ID_LEN); // MOS
	strncpy(proxyIdStr, n.proxyIdStr, MAX_NODE_ID_STR_LEN); // MOS

	if (n.dObj)
		dObj = n.dObj->copy();
}


Node *Node::create(const DataObjectRef& dObj)
{
	if (!dObj || !dObj->getMetadata()) {
		HAGGLE_ERR("Bad data object\n");
		return NULL;
	}
	
	const Metadata *m = dObj->getMetadata()->getMetadata(NODE_METADATA);

	if (!m)
		return NULL;

	const char *typestr = m->getParameter(NODE_METADATA_TYPE_PARAM);

	if (!typestr)
		return NULL;

	Type_t type = strToType(typestr);

	if (type == TYPE_UNDEFINED)
		return NULL;
	
	return create(type, dObj);
}

Node *Node::create(Type_t type, const DataObjectRef& dObj)
{
	Node *node = NULL;
	
	if (!dObj) {
		HAGGLE_ERR("Bad data object\n");
		return NULL;
	}

	switch (type) {
		case TYPE_LOCAL_DEVICE:
			node = new LocalDeviceNode();
			break;
		case TYPE_PEER:
			node = new PeerNode();
			break;
		case TYPE_GATEWAY:
			node = new GatewayNode();
			break;
		case TYPE_UNDEFINED:
			node = new UndefinedNode();
			break;
		case TYPE_APPLICATION:
			node = new ApplicationNode();
			break;
		default:
			break;
			
	}
	
	if (!node)
		return NULL;

	node->dObj = dObj;
	node->createdFromNodeDescription = true;

	if (!node->init_node(NULL)) {
	        HAGGLE_DBG("Node could not be initialized - created from data object %s\n", DataObject::idString(dObj).c_str());
		delete node;
		node = NULL;
	}

	return node;
}

Node *Node::create(Type_t type, const string name, Timeval _nodeDescriptionCreateTime)
{
	Node *node = NULL;
	
	switch (type) {
		case TYPE_LOCAL_DEVICE:
			node = new LocalDeviceNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_PEER:
			node = new PeerNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_GATEWAY:
			node = new GatewayNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_UNDEFINED:
			node = new UndefinedNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_APPLICATION:
			node = new ApplicationNode(name, _nodeDescriptionCreateTime);
			break;
		default:
			break;			
	}

	if (!node)
		return NULL;
	
	node->dObj = DataObject::create();

	if (!node->dObj) {
		HAGGLE_DBG("Data object could not be created\n");
		delete node;
		return NULL;
	}

	if (!node->init_node(NULL)) {
		HAGGLE_DBG("Node could not be initialized\n");
		delete node;
		node = NULL;
	}

	return node;
}

Node *Node::create_with_id(Type_t type, const Id_t id, const string name, Timeval _nodeDescriptionCreateTime)
{
	Node *node = NULL;
	
	switch (type) {
		case TYPE_LOCAL_DEVICE:
			node = new LocalDeviceNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_PEER:
			node = new PeerNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_GATEWAY:
			node = new GatewayNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_UNDEFINED:
			node = new UndefinedNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_APPLICATION:
			node = new ApplicationNode(name, _nodeDescriptionCreateTime);
			break;
		default:
			break;			
	}
	
	if (!node)
		return NULL;
	
	node->dObj = DataObject::create();

	if (!node->dObj) {
		HAGGLE_DBG("Data object could not be created\n");
		delete node;
		return NULL;
	}

	if (!node->init_node(id)) {
		HAGGLE_DBG("Node could not be initialized\n");
		delete node;
		node = NULL;
	}

	return node;
}

Node *Node::create_with_id(Type_t type, const char *_idStr, const string name, Timeval _nodeDescriptionCreateTime)
{
	Node::Id_t iD;
	unsigned int i;

	if (!_idStr) {
		HAGGLE_ERR("Bad identifier string\n");
		return NULL;
	}

	if (strlen(_idStr) != (MAX_NODE_ID_STR_LEN - 1)) {
		HAGGLE_ERR("Bad length of identifier string\n");
		return NULL;
	}
	for (i = 0; i < NODE_ID_LEN; i++) {
		iD[i] = 0;
		if (_idStr[i*2] <= '9')
			iD[i] += (_idStr[i*2] - '0') << 4;
		else if (_idStr[i*2] <= 'G')
			iD[i] += (_idStr[i*2] - 'A' + 10) << 4;
		else //if(_idStr[i*2] <= 'g')
			iD[i] += (_idStr[i*2] - 'a' + 10) << 4;
		if (_idStr[i*2 + 1] <= '9')
			iD[i] += (_idStr[i*2 + 1] - '0');
		else if (_idStr[i*2 + 1] <= 'G')
			iD[i] += (_idStr[i*2 + 1] - 'A' + 10);
		else //if (_idStr[i*2 + 1] <= 'g')
			iD[i] += (_idStr[i*2 + 1] - 'a' + 10);
	}

	Node *node = NULL;
	
	switch (type) {
		case TYPE_LOCAL_DEVICE:
			node = new LocalDeviceNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_PEER:
			node = new PeerNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_GATEWAY:
			node = new GatewayNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_APPLICATION:
			node = new ApplicationNode(name, _nodeDescriptionCreateTime);
			break;
		case TYPE_UNDEFINED:
			node = new UndefinedNode(name, _nodeDescriptionCreateTime);
			break;
		default:
			break;			
	}
	
	if (!node)
		return NULL;
	
	node->dObj = DataObject::create();

	if (!node->dObj) {
		HAGGLE_DBG("Data object could not be created\n");
		delete node;
		return NULL;
	}

	if (!node->init_node(iD)) {
		HAGGLE_DBG("Node could not be initialized\n");
		delete node;
		node = NULL;
	}

	return node;
}

Node::~Node()
{
	if (doBF)
		delete doBF;
	if (doBF2)
		delete doBF2;
}

Node::Type_t Node::getType() const
{
	return type;
}

const char *Node::typeToStr(Type_t type)
{
        return typestr[type];
}

Node::Type_t Node::strToType(const char *type)
{
	int i = 0;

	while (typestr[i]) {
		if (strcmp(typestr[i], type) == 0)
			return (Type_t)i;
		i++;
	}
	return TYPE_UNDEFINED;
}

const unsigned char *Node::getId() const
{
	return id;
}

const unsigned char *Node::getProxyId() const // MOS
{
	return proxyId;
}

void Node::setId(const Id_t _id)
{
	memcpy(id, _id, sizeof(Node::Id_t));
	calcIdStr();
}

void Node::setProxyId(const Id_t _id) // MOS
{
	memcpy(proxyId, _id, sizeof(Node::Id_t));
	calcProxyIdStr();
}

const char *Node::getIdStr() const
{
	return idStr;
}

string Node::idString(const NodeRef& node)
{
  string id(node->getIdStr());
  return id;
}

const char *Node::getProxyIdStr() const // MOS
{
	return proxyIdStr;
}

string Node::getName() const
{
	return name;
}

string Node::nameString(const NodeRef& node)
{
  string name(node->getName());
  return name;
}

void Node::setName(const string _name)
{
	name = _name;
}

Metadata *Node::toMetadata(bool withBloomfilter) const
{
	char *b64str = NULL;

        Metadata *nm = new XMLMetadata(NODE_METADATA);
        
        if (!nm)
                return NULL;
        
	base64_encode_alloc((const char *)getId(), NODE_ID_LEN, &b64str);

	if (!b64str) {
		HAGGLE_ERR("Could not convert node id to metadata\n");
		return NULL;
	}
	
	if (type == TYPE_LOCAL_DEVICE) {
		// Make sure local device node looks like a peer to other nodes.
		nm->setParameter(NODE_METADATA_TYPE_PARAM, typeToStr(TYPE_PEER));
	} else {
		nm->setParameter(NODE_METADATA_TYPE_PARAM, getTypeStr());
	}
	
	nm->setParameter(NODE_METADATA_ID_PARAM, b64str);
	
	free(b64str);
	
        nm->setParameter(NODE_METADATA_NAME_PARAM, name.c_str());
		
        nm->setParameter(NODE_METADATA_THRESHOLD_PARAM, matchThreshold);

        nm->setParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM, numberOfDataObjectsPerMatch);

	// MOS
	if (type == TYPE_APPLICATION) {
	  
	  base64_encode_alloc((const char *)getProxyId(), NODE_ID_LEN, &b64str);

	  if (!b64str) {
	    HAGGLE_ERR("Could not convert node id to metadata\n");
	    return NULL;
	  }
		
	  nm->setParameter(NODE_METADATA_PROXY_ID_PARAM, b64str);
	
	  free(b64str);
	}

	if (type != TYPE_APPLICATION) { // MOS - no need to transmit application node interfaces
	                                // because they are not relevant outside of local device
            for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		Metadata *im = (*it)->toMetadata();
		
		if (im)
			nm->addMetadata(im);
	    }
	}
	
	if (type != TYPE_APPLICATION) { // MOS - no need to transmit application node BF 
                                        // because they are aggregated their proxy node descriptions
	  if (withBloomfilter)
                nm->addMetadata(getBloomfilter()->toMetadata());
	}

	return nm;        
}

/*
   TODO: the fromMetadata() function should ideally exist to adhere to the general
   model of converting Metadata to objects, as used in other classes. However, for 
   the node, most metadata parsing is done in init_node(), where access to an 
   instanciated node object is possible. Therefore, this function is currently
   unimplemented.
 
 */
Node *Node::fromMetadata(const Metadata& m)
{
	Node *node = NULL;
#if 0
	struct base64_decode_context b64_ctx;
	size_t decodelen;
	const char *pval;
	Node::Id_t id;
	
	if (!m.isName(NODE_METADATA))
		return NULL;
	
	pval = m.getParameter(NODE_METADATA_ID_PARAM);
	
	if (pval) {
		decodelen = NODE_ID_LEN;
		base64_decode_ctx_init(&b64_ctx);
		
		if (!base64_decode(&b64_ctx, pval, strlen(pval), (char *)id, &decodelen))
			return NULL;
		
		//calcIdStr();
	}
	
	pval = m.getParameter(NODE_METADATA_NAME_PARAM);
	/*
	if (pval)
		name = pval;
	*/
	pval =m.getParameter(NODE_METADATA_THRESHOLD_PARAM);
	
	/*
	if (pval)
		matchThreshold = strtoul(pval, NULL, 10);
	*/
	pval = m.getParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM);
	
	/*
	if (pval)
		numberOfDataObjectsPerMatch = strtoul(pval, NULL, 10);
	*/
	/*
	 Should we really override the wish of another node to receive all
	 matching data objects? And in that case, why set it to our rather
	 conservative default value?
	 if (numberOfDataObjectsPerMatch == 0)
	 numberOfDataObjectsPerMatch = NODE_DEFAULT_DATAOBJECTS_PER_MATCH;
	 */
	
	const Metadata *bm = m.getMetadata(BLOOMFILTER_METADATA);
	
	if (bm) {
		/*
		if (!setBloomfilter(Bloomfilter::fromMetadata(*bm))) {
			HAGGLE_ERR("Bad bloomfilter metadata\n");
			return false;
		}
		 */
	}
	
	const Metadata *im = m.getMetadata(INTERFACE_METADATA);
	
	while (im) {
		InterfaceRef iface = Interface::fromMetadata(*im);
		
		if (iface) {
			//addInterface(iface);
		} else {
			HAGGLE_ERR("Could not create interface from metadata\n");	
		}
		im = m.getNextMetadata();
	}
#endif
	return node;
}

int Node::addAttribute(const Attribute & a)
{
	int n = dObj->addAttribute(a);
	
	if (n) {
		setNodeDescriptionCreateTime();
		setNodeDescriptionUpdateTime();
	}

	return n;
}

int Node::addAttribute(const string name, const string value, const unsigned long weight)
{
  	int n = dObj->addAttribute(name, value, weight);
	
	if (n) {
		setNodeDescriptionCreateTime();
		setNodeDescriptionUpdateTime();
	}	

	return n;
}

int Node::removeAttribute(const Attribute & a)
{
	int n = dObj->removeAttribute(a);
	
	if (n) {
		setNodeDescriptionCreateTime();
		setNodeDescriptionUpdateTime();
	}
	
	return n;
}

int Node::removeAttribute(const string name, const string value)
{
	int n = dObj->removeAttribute(name, value);
	
	if (n) {
		setNodeDescriptionCreateTime();
		setNodeDescriptionUpdateTime();
	}

	return n;
}

const Attribute *Node::getAttribute(const string name, const string value, const unsigned int n) const
{
	return dObj->getAttribute(name, value, n);
}

const Attributes *Node::getAttributes() const
{
	return dObj->getAttributes();
}

const unsigned char *Node::strIdToRaw(const char *strId)
{
	static unsigned char id[NODE_ID_LEN];
	int i = 0, j = 0;

	while (i < MAX_NODE_ID_STR_LEN && j < NODE_ID_LEN) {
		char hexByteStr[3] = { strId[i], strId[i + 1], '\0' };

		id[j++] = (unsigned char) strtol(hexByteStr, NULL, 16);
		i += 2;
	}
	return id;
}

#if defined(OS_MACOSX) && !defined(OS_MACOSX_IPHONE)
#include <CoreFoundation/CoreFoundation.h> 

#include <IOKit/IOKitLib.h> 
#include <IOKit/network/IOEthernetInterface.h> 
#include <IOKit/network/IONetworkInterface.h> 
#include <IOKit/network/IOEthernetController.h> 

static void getserial(char *serialNumber) 
{ 
        kern_return_t kernResult; 
        mach_port_t machPort; 

        kernResult = IOMasterPort( MACH_PORT_NULL, &machPort ); 

        serialNumber[0] = 0; 

        // if we got the master port 
        if ( kernResult == KERN_SUCCESS ) { 

                // create a dictionary matching IOPlatformExpertDevice 
                CFMutableDictionaryRef classesToMatch = IOServiceMatching("IOPlatformExpertDevice" ); 

                // if we are successful 
                if (classesToMatch) { 

                        // get the matching services iterator 
                        io_iterator_t iterator; 
                        kernResult = IOServiceGetMatchingServices( machPort, classesToMatch, &iterator ); 

                        // if we succeeded 
                        if ( (kernResult == KERN_SUCCESS) && iterator ) { 

                                io_object_t serviceObj; 
                                bool done = false; 
                                do { 
                                        // get the next item out of the dictionary 
                                        serviceObj = IOIteratorNext( iterator ); 

                                        // if it is not NULL 
                                        if (serviceObj) { 

                                                CFDataRef data = (CFDataRef) IORegistryEntryCreateCFProperty(serviceObj, CFSTR("serial-number" ), kCFAllocatorDefault, 0 ); 

                                                if (data != NULL) { 
                                                        CFIndex datalen = CFDataGetLength(data); 
                                                        const UInt8* rawdata = CFDataGetBytePtr(data); 
                                                        char dataBuffer[256]; 
                                                        memcpy(dataBuffer, rawdata, datalen); 
                                                        sprintf(serialNumber, "%s%s", dataBuffer+13, dataBuffer); 
                                                        CFRelease(data); 
                                                        done = true; 
                                                } 
                                        } 
                                } while (serviceObj && !done); 
                                IOObjectRelease(serviceObj); 
                        } 

                        IOObjectRelease(iterator); 
                } 
        } 
}
#endif

/**

   Calculate a unique node Id which will be consistent each time.
*/
#if defined(OS_ANDROID)
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/system_properties.h>
#endif
void Node::calcId()
{
#if defined(OS_WINDOWS_MOBILE)
#define APPLICATION_DATA "@^!Haggle!^@"

	HRESULT hr = NOERROR;
	DWORD idlen = sizeof(id);

	hr = GetDeviceUniqueID(reinterpret_cast<PBYTE>(APPLICATION_DATA), 
		strlen(APPLICATION_DATA), 
		GETDEVICEUNIQUEID_V1,
		(LPBYTE)id,
		&idlen);

#elif defined(OS_ANDROID)
	SHA_CTX ctx;
	char serialno[50] = {'\0'};
	SHA1_Init(&ctx);	

	__system_property_get("ro.serialno", serialno);

	SHA1_Update(&ctx, (unsigned char *)serialno, strlen(serialno));
	SHA1_Final((unsigned char *)id, &ctx);
	
#elif defined(OS_LINUX) || defined(OS_MACOSX_IPHONE) || defined(OS_WINDOWS_DESKTOP)
        InterfaceRefList iflist;
	SHA_CTX ctx;
	SHA1_Init(&ctx);

        if (getLocalInterfaceList(iflist, false) > 0) {
                for (InterfaceRefList::iterator it = iflist.begin(); it != iflist.end(); ++it) {
                        SHA1_Update(&ctx, (unsigned char *)(*it)->getIdentifier(), (*it)->getIdentifierLen());
                        //printf("Found local interface %s\n", (*it)->getIdentifierStr());
                }
        }

	SHA1_Final((unsigned char *)id, &ctx);

#elif defined(OS_MACOSX)
	SHA_CTX ctx;
	char serial[256];
	
	SHA1_Init(&ctx);
	getserial(serial);
	SHA1_Update(&ctx, (unsigned char *)serial, strlen(serial));
	SHA1_Final((unsigned char *)id, &ctx);
#else
#error "Unsupported OS!"
#endif

	calcIdStr();

	HAGGLE_DBG("Node type=%d, id=[%s]\n", type, idStr);
}

void Node::calcIdStr()
{
	int len = 0;

	// Generate a readable string of the Id
	for (int i = 0; i < NODE_ID_LEN; i++) {
		len += sprintf(idStr + len, "%02x", id[i] & 0xff);
	}
}

void Node::calcProxyIdStr() // MOS
{
	int len = 0;

	// Generate a readable string of the proxy Id
	for (int i = 0; i < NODE_ID_LEN; i++) {
		len += sprintf(proxyIdStr + len, "%02x", proxyId[i] & 0xff);
	}
}

#ifdef DEBUG
void Node::printInterfaces() const
{
	int n = 0;
	for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++)  {
		const InterfaceRef& iface = *it;
		iface.lock();
		const Addresses *addrs = iface->getAddresses();

		printf("%d : %s %s %s\n", n++, iface->getIdentifierStr(), iface->getTypeStr(), iface->isUp() ? "up" : "down");
		
		for (Addresses::const_iterator itt = addrs->begin(); itt != addrs->end(); itt++) {
			const Address *addr = *itt;
                       printf("\t%s\n", addr->getURI());
		}
		iface.unlock();
 	}
}
#endif

// CBMEN, HL, Begin
void Node::getInterfacesAsMetadata(Metadata *m) const {
	for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++)  {
		Metadata *dm = m->addMetadata("Interface");
		if (!dm) {
			continue;
		}

		const InterfaceRef& iface = *it;
		iface.lock();

		dm->setParameter("identifier", iface->getIdentifierStr());
		dm->setParameter("type", iface->getTypeStr());
		dm->setParameter("is_up", iface->isUp() ? "true" : "false");
		dm->setParameter("name", iface->getName());

		Metadata *dmm = dm->addMetadata("Addresses");
		if (dmm) {
			const Addresses *addrs = iface->getAddresses();
			for (Addresses::const_iterator itt = addrs->begin(); itt != addrs->end(); itt++) {
				Metadata *addr = dmm->addMetadata("Address");
				if (addr) {
					addr->setParameter("uri", (*itt)->getURI());
				}
			}
		}
		iface.unlock();
 	}
}
// CBMEN, HL, End

bool Node::addInterface(InterfaceRef inIface)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef& iface = *it;
		if (inIface == iface) {
			// Node already has interface -> replace with updated reference.
			iface = inIface;
			return false;
		}
	}

	interfaces.add(inIface);
	setNodeDescriptionCreateTime();
	setNodeDescriptionUpdateTime();
	return true;
}

// Mark an interface as up
bool Node::setInterfaceUp(const InterfaceRef inIface)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef& iface = *it;
		if (inIface == iface && !iface->isUp()) {
			iface->up();
			setNodeDescriptionCreateTime();
			setNodeDescriptionUpdateTime();
			return true;
		}
	}
	return false;
}

// Mark an interface as down.
bool Node::setInterfaceDown(const InterfaceRef inIface)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef& iface = *it;
		if (inIface == iface && iface->isUp()) {
			iface->down();
			// MOS - do not set time stamps otherwise new node descriptions
			// for peers could be unnecessarily suppressed
			// setNodeDescriptionCreateTime();
			// setNodeDescriptionUpdateTime();
			return true;
		}
	}
	return false;
}

bool Node::removeInterface(const InterfaceRef& inIface)
{
	for (InterfaceRefList::iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		InterfaceRef& iface = *it;
		if (inIface == iface) {
			interfaces.erase(it);
			// MOS - do not set time stamps otherwise new node descriptions
			// for peers could be unnecessarily suppressed
			// setNodeDescriptionCreateTime();
			// setNodeDescriptionUpdateTime();
			return true;
		}
	}
	return false;
}

const InterfaceRefList *Node::getInterfaces() const 
{
	return &interfaces;
} 

unsigned int Node::numActiveInterfaces() const
{
        unsigned int num = 0;

        for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		const InterfaceRef& iface = *it;
		if (iface->isUp()) {
			num++;
		}
	}
	return num;
}

bool Node::hasInterface(const InterfaceRef inIfaceRef) const
{
	for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		const InterfaceRef& ifaceRef = *it;
		
		if (inIfaceRef == ifaceRef) {
			return true;
		}
	}
	return false;
}

bool Node::hasProxyId(const Id_t _id) const
{
  return (memcmp(proxyId, _id, NODE_ID_LEN) == 0);
}

bool Node::isAvailable() const
{
	for (InterfaceRefList::const_iterator it = interfaces.begin(); it != interfaces.end(); it++) {
		const InterfaceRef& iface = *it;
		if (iface->isUp()) {
			return true;
		}
	}
	return false;
}

bool Node::isNeighbor() const
{
       return ((type == TYPE_PEER) || (type == TYPE_UNDEFINED) || (type == TYPE_GATEWAY)) && isAvailable();
}

// MOS - Adding NodeDescription attribute.
// We use weight 0 instead of 1 to mark as internal attribute (keeping the semantics clean).
// To support first class applications this is also done for application nodes.
// The first case is the original setting using the node id as value.
// The other cases abstract from the id to allow for shared attributes,
// which in Vanilla Hagggle are needed to propagrate node descriptions.

void Node::addNodeDescriptionAttribute(DataObjectRef& dObj) const
{
      if(Node::nodeDescriptionAttribute == Node::NDATTR_ID) {
	  if(!dObj->getAttribute(NODE_DESC_ATTR)) dObj->addAttribute(NODE_DESC_ATTR, getIdStr(), Node::nodeDescriptionAttributeWeight);
      }
      else if(Node::nodeDescriptionAttribute == Node::NDATTR_TYPE) {
          if(!dObj->getAttribute(NODE_DESC_ATTR)) dObj->addAttribute(NODE_DESC_ATTR, typeToStr(getType()), Node::nodeDescriptionAttributeWeight);
      }
      else if(Node::nodeDescriptionAttribute == Node::NDATTR_EMPTY) {
          if(!dObj->getAttribute(NODE_DESC_ATTR)) dObj->addAttribute(NODE_DESC_ATTR, "", Node::nodeDescriptionAttributeWeight);
      }
}

DataObjectRef Node::getDataObject(bool withBloomfilter) const
{
        // If the node was created from a node description (i.e., the
        // node description was received over the network), then
        // return it the node description as is.
        if (createdFromNodeDescription)
                return dObj;

        // Else generate a node description with our data object with
        // interests as a basis
        DataObjectRef ndObj = dObj->copy();
        
        // Add the node description attribute
	// ndObj->addAttribute(NODE_DESC_ATTR, getIdStr());
	addNodeDescriptionAttribute(ndObj); // MOS

	ndObj->setNodeDesc(true); // MOS - make sure that this is always set
	
        Metadata *m = ndObj->getMetadata();

        if (!m)
                return NULL;

	// MOS - Note that the Bloomfilter metadata is only generated once but not
        // updated on Bloomfilter changes (currently not needed).

        if (!m->addMetadata(toMetadata(withBloomfilter)))
                return NULL;
	
	return ndObj;
}

// MOS, SW

void Node::setMatchingThreshold(unsigned long value)
{
    matchThreshold = value;
    if (!createdFromNodeDescription) {
        return;
    }
    Metadata *nm = dObj->getMetadata()->getMetadata(NODE_METADATA);
    if (!nm) {
        HAGGLE_DBG("Missing node metadata\n");
    }
    nm->setParameter(NODE_METADATA_THRESHOLD_PARAM, getMatchingThreshold());
}

// MOS, SW

void Node::setMaxDataObjectsInMatch(unsigned long value)
{
    numberOfDataObjectsPerMatch = value;
    if (!createdFromNodeDescription) {
        return;
    }
    Metadata *nm = dObj->getMetadata()->getMetadata(NODE_METADATA);
    if (!nm) {
        HAGGLE_DBG("Missing node metadata\n");
    }
    nm->setParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM, getMaxDataObjectsInMatch());
}

Bloomfilter *Node::getBloomfilter(void)
{
	if (!doBF) {
		/* Init Bloomfilter */
		doBF = Bloomfilter::create(type == Node::TYPE_APPLICATION ? 
					   Bloomfilter::TYPE_COUNTING : 
					   Bloomfilter::TYPE_NORMAL);
	}

	return doBF;
}

Bloomfilter *Node::getBloomfilter2(void)
{
	return doBF2;
}

const Bloomfilter *Node::getBloomfilter(void) const
{
	return const_cast<Node *>(this)->getBloomfilter();
}

bool Node::setBloomfilter(const char *base64, const bool set_create_time)
{
	if (!base64 || strlen(base64) <= 0)
		return false;
	
	if (doBF) {
		if (!doBF->fromBase64(base64))
			return false;
	} else {
		Bloomfilter *tmpBF = Bloomfilter::create_from_base64(Bloomfilter::TYPE_NORMAL, base64);

		if (!tmpBF)
			return false;

		doBF = tmpBF;
	}
	/*
	 Notes about setting create time
	 ===============================
	 Here we can decide to set the create time or not every time we update the bloomfilter
	 (i.e., when we receive or send a new data object). The implication of setting the
	 create time is that the node description will by definition be 'new', and therefore
	 we will be triggered to send it to nodes that we meet as they will seem to not have
	 received this new version yet. The downside is that we may send a lot of node description
	 updates altough nothing else has changed (e.g., our interests). On the other hand, other
	 nodes will be updated about what data objects we have received (or sent), and therefore
	 it may reduce the amount of reduntant transmissions of data object we already have.
	 
	 It is not yet clear what is the best strategy here.
	 */
	      
	if (set_create_time) {
		setNodeDescriptionCreateTime();
	}

	return true;
}

bool Node::setBloomfilter(Bloomfilter *bf, const bool set_create_time)
{
	if (!bf)
		return false;

	if (doBF)
		delete doBF;
	
	doBF = bf;
	
	/* See not above about setting the create time here. */
	if (set_create_time) {
		setNodeDescriptionCreateTime();
	}

	return true;
}

bool Node::setBloomfilter(const Bloomfilter& bf, const bool set_create_time)
{
	Bloomfilter *tmpBF;

	tmpBF = bf.to_noncounting();

	if (!tmpBF)
		return false;
	
	if (doBF)
		delete doBF;

	doBF = tmpBF;

	/* See not above about setting the create time here. */
	if (set_create_time) {
		setNodeDescriptionCreateTime();
	}

	return true;
}

bool Node::setBloomfilter2(Bloomfilter *bf)
{
	if (!bf)
		return false;

	if (doBF2)
		delete doBF2;
	
	doBF2 = bf;

	return true;
}

bool Node::has(const unsigned char *blob, size_t len) const
{
  if(doBF && doBF->has(blob,len)) return true;
  if(doBF2 && doBF2->has(blob,len)) return true;
  return false;
}

bool Node::has(const DataObjectId_t& id) const
{
  if(doBF && doBF->has(id)) return true;
  if(doBF2 && doBF2->has(id)) return true;
  return false;
}

bool Node::has(const DataObjectRef &dObj) const
{
  if(doBF && doBF->has(dObj)) return true;
  if(doBF2 && doBF2->has(dObj)) return true;
  return false;
}

bool Node::hasNetworkCodingParentDataObject(const DataObjectRef &dObj) const {
  if(doBF && doBF->hasNetworkCodingParentDataObject(dObj)) return true;
  if(doBF2 && doBF2->hasNetworkCodingParentDataObject(dObj)) return true;
  return false;
}

bool Node::hasFragmentationParentDataObject(const DataObjectRef &dObj) const {
  if(doBF && doBF->hasFragmentationParentDataObject(dObj)) return true;
  if(doBF2 && doBF2->hasFragmentationParentDataObject(dObj)) return true;
  return false;
}

bool Node::hasParentDataObject(const DataObjectRef &dObj) const {
  if(doBF && doBF->hasParentDataObject(dObj)) return true;
  if(doBF2 && doBF2->hasParentDataObject(dObj)) return true;
  return false;
}

bool Node::hasThisOrParentDataObject(const DataObjectRef &dObj) const {
  if(doBF && doBF->hasThisOrParentDataObject(dObj)) return true;
  if(doBF2 && doBF2->hasThisOrParentDataObject(dObj)) return true;
  return false;
}

void Node::setNodeDescriptionCreateTime(Timeval t)
{
        if (isLocalDevice() || isLocalApplication()) { // MOS - added local application
		//HAGGLE_DBG("SETTING create time on node description\n");
		dObj->setCreateTime(t);
		nodeDescriptionCreateTime = t;
	}
}

void Node::setNodeDescriptionUpdateTime(Timeval t)
{
	if (isLocalDevice()) {
		//HAGGLE_DBG("SETTING update time on node description\n");
		dObj->setUpdateTime(t);
		nodeDescriptionUpdateTime = t;
	}
}

Timeval Node::getNodeDescriptionCreateTime() const
{
	return nodeDescriptionCreateTime;
}

Timeval Node::getNodeDescriptionUpdateTime() const
{
	return nodeDescriptionUpdateTime;
}

Timeval Node::getCreateTime() const
{
	return createTime;
}

Timeval Node::getLastDataObjectQueryTime() const
{
	return lastDataObjectQueryTime;
}

Timeval Node::getLastRoutingUpdateTime() const
{
	return lastRoutingUpdateTime;
}

void Node::setLastDataObjectQueryTime(Timeval t)
{
	lastDataObjectQueryTime = t;
}

void Node::setLastRoutingUpdateTime(Timeval t)
{
	lastDataObjectQueryTime = t;
}

bool operator==(const Node &n1, const Node &n2)
{
	if (n1.type == Node::TYPE_UNDEFINED || n2.type == Node::TYPE_UNDEFINED) {
		// Check if the interfaces overlap:
		
		for (InterfaceRefList::const_iterator it = n1.interfaces.begin(); it != n1.interfaces.end(); it++) {
			for (InterfaceRefList::const_iterator jt = n2.interfaces.begin(); jt != n2.interfaces.end(); jt++) {
				if (*it == *jt)
					return true;
			}
		}
		return false;
	} else {
		return (memcmp(n1.id, n2.id, NODE_ID_LEN) == 0);
	}
}

bool operator!=(const Node & n1, const Node & n2)
{
	return (!(n1 == n2));
}

ApplicationNode::~ApplicationNode()
{
	while (eventInterests.size()) {
		Pair<long, long> p = *eventInterests.begin();
// SW: START: fixing mem-leak here where private event was not free'd
		long type = p.first;
		removeEventInterest(type);
/*
		if (EVENT_TYPE_PRIVATE(p.second)) 
			Event::unregisterType(p.second);
		
		eventInterests.erase(p.first);
*/
// SW: END: fixing mem-leak here where private event was not free'd
	}
}

// SW: START: fixing mem-leak here where private event was not free'd
void ApplicationNode::clearEventInterests()
{
    eventInterests.clear();
}
// SW: END: fixing mem-leak here where private event was not free'd

bool ApplicationNode::setFilterEvent(long feid) 
{ 
	if (filterEventId != -1)
		return false; 
	
	filterEventId = feid;
	
	return true;
}

// CBMEN, HL, Begin

bool ApplicationNode::clearFilterEvent() {

	if (filterEventId == -1)
		return false;

	filterEventId = -1;

	return true;

}

// CBMEN, HL, End


bool ApplicationNode::addEventInterest(long type)
{
	return eventInterests.insert(make_pair(type, '\0')).second;
}

void ApplicationNode::removeEventInterest(long type)
{
	eventInterests.erase(type);
	
	if (EVENT_TYPE_PRIVATE(type)) 
		Event::unregisterType(type);
}

bool ApplicationNode::hasEventInterest(long type) const
{
	return eventInterests.find(type) != eventInterests.end();
}

