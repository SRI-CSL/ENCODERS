/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 */

/* Copyright 2009 Uppsala University
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
#ifndef _FORWARDERPROPHET_H
#define _FORWARDERPROPHET_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class ForwarderProphet;

// SW: use refactored design (shared task queue)
#include "ForwarderAsynchronousInterface.h"

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

using namespace haggle;
/**
	Prophet forwarding ids are only used internally in the forwarderprophet 
	manager module, but for technical reasons it needs to be defined here.
*/
typedef unsigned long prophet_node_id_t;

// Symbolic constant for the local node.
#define this_node_id ((prophet_node_id_t) 1)

typedef Pair<double, Timeval> prophet_metric_t;
typedef Map<prophet_node_id_t, prophet_metric_t> prophet_rib_t;


#define NF_max (5) // Not sure what a good number is here...

/**
	Proof-of-concept PRoPHET routing module.
*/
// SW: use refactored design (shared task queue)
class ForwarderProphet : public ForwarderAsynchronousInterface {
#define PROPHET_NAME "Prophet"
	// A forwarding strategy class that can implement
	// the strategies proposed in the Prophet draft.
	// With this class it is possible to dynamically set the forwarding
	// strategy
	class ForwardingStrategy {
	private:
		const string name;
	protected:
		// Allow no instances of this base strategy
		ForwardingStrategy(const string& _name = "No strategy") : name(_name) {}
	public:
		virtual ~ForwardingStrategy() {}
		const string& getName() const { return name; }
		//
		virtual bool operator() (const double& P_ad, const double& P_bd, const unsigned int& NF = 0) const = 0;	
	};
	
	class GRTR : public ForwardingStrategy 
	{
	public:
		GRTR() : ForwardingStrategy("GRTR") {}
		bool operator() (const double& P_ad, const double& P_bd, const unsigned int& NF = 0) const
		{
			return P_bd > P_ad ? true : false;
		}
	};
	
	class GTMX : public ForwardingStrategy 
	{
	public:
		GTMX() : ForwardingStrategy("GTMX") {}
		bool operator() (const double& P_ad, const double& P_bd, const unsigned int& NF = 0) const
		{
			return (P_bd > P_ad && NF < NF_max);
		}
	};
private:  
	double P_encounter;
	double alpha;
	double beta;
	double gamma;
	double delta; // MOS
	double aging_time_unit;
	double aging_constant;
#define PROPHET_P_ENCOUNTER ForwarderProphet::P_encounter
#define PROPHET_ALPHA ForwarderProphet::alpha
#define PROPHET_BETA ForwarderProphet::beta
#define PROPHET_GAMMA ForwarderProphet::gamma
#define PROPHET_AGING_TIME_UNIT ForwarderProphet::aging_time_unit
#define PROPHET_AGING_CONSTANT ForwarderProphet::aging_constant

	bool sampling; // MOS

	HaggleKernel *kernel;
        
	/**
		In order to reduce the amount of memory taken up by the forwarding 
		table, this mapping between node ids and forwarding manager id numbers
		is used. This means that this forwarding module cannot handle more than
		2^32 other nodes.
	*/
	Map<string, prophet_node_id_t> nodeid_to_id_number;
	/**
		In order to reduce the amount of memory taken up by the forwarding 
		table, this mapping between forwarding manager id numbers and node ids 
		is used. This means that this forwarding module cannot handle more than
		2^32 other nodes.
	*/
	Map<prophet_node_id_t, string> id_number_to_nodeid;
	/**
		The next id number to use. Since the default value for a 
		prophet_node_id_t in a map is 0, this should never be 0, in order 
		to avoid confusion.
	*/
	prophet_node_id_t next_id_number;
	
	/**
		This is the local node's internal PRoPHET metrics.
	*/
	prophet_rib_t rib;
	Timeval rib_timestamp;
	/**
		This is a mapping of id numbers (of other nodes) to those nodes' public
		metrics.
	*/
	Map<prophet_node_id_t, prophet_rib_t> neighbor_ribs;
	
	virtual size_t getSaveState(RepositoryEntryList& rel);
	virtual bool setSaveState(RepositoryEntryRef& e);
	
	/**
	 This function returns the nonzero id number for the given node id 
	 string. It ensures that the node id is in the nodeid_to_id_number map
	 and that the returned id number is nonzero. If the node id wasn't in the
	 map to begin with, it is inserted, along with a new id number.
	 */
	prophet_node_id_t id_for_string(const string& nodeid);
	
	prophet_metric_t& age_metric(prophet_metric_t& metric, bool force = false);
	
	bool newRoutingInformation(const Metadata *m);
	
	bool addRoutingInformation(DataObjectRef& dObj, Metadata *parent);
	/**
		Does the actual work of newNeighbor.
	*/
	void _newNeighbor(const NodeRef &neighbor);

	/**
		Does the actual work of endNeighbor.
	*/
	void _endNeighbor(const NodeRef &neighbor);
	
	/**
		Does the actual work of getTargetsFor.
	*/
	void _generateTargetsFor(const NodeRef &neighbor);
	
	/**
		Does the actual work of getDelegatesFor.
	*/
	void _generateDelegatesFor(const DataObjectRef &dObj, const NodeRef &target, const NodeRefList *other_targets);

	void updateApplications(void); // MOS
	bool isApplication(string id); // MOS

	/**
		Does the actual work or printRoutingTable().
	*/
	void _printRoutingTable(void);
	void _getRoutingTableAsMetadata(Metadata *m); // CBMEN, HL
	void _onForwarderConfig(const Metadata& m);

	ForwardingStrategy *forwarding_strategy;

	void updateCurrentNeighbors();

public:
    // SW: use refactored design (shared task queue)
	ForwarderProphet(ForwarderAsynchronous* forwarderAsync,
			         ForwardingStrategy *_forwarding_strategy = new GRTR());
	~ForwarderProphet();
};

#endif
