/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ReplicationUtilityFunction.h"
#include "ReplicationUtilityFunctionFactory.h"
#include "ReplicationManagerFactory.h"
#include "math.h" // for ceil()

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

void
ReplicationUtilityFunction::onConfig(const Metadata& m)
{
    const char *param;
    param = m.getParameter("name");

    if (param) {
        name = string(param);
    }
}

// START: aggregate functions:

void 
ReplicationUtilityAggregate::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);

    const char *param;
    const Metadata *dm;

    int i = 0;
    int loaded = 0;
    int failed = 0;
    while ((dm = m.getMetadata("Factor", i++))) {
        param = dm->getParameter("name");
        if (!param) {
            HAGGLE_ERR("factor does not have name.\n");
            failed++;
            continue;
        }
        ReplicationUtilityFunction *func = ReplicationUtilityFunctionFactory::getNewUtilityFunction(getManager(), getGlobalOptimizer(), string(param));
        if (!func) {
            HAGGLE_ERR("unknown function for name: %s\n", param);
            failed++;
            continue;
        }
        const Metadata *fm = dm->getMetadata(param);
        if (fm) {
            func->onConfig(*fm);
        }
        factors.push_back(func);
        loaded++;
    }
    HAGGLE_DBG("Aggregate loaded %d functions, faild to load: %d functions.\n", loaded, failed);
}


void 
ReplicationUtilityAggregate::notifyInsertion(DataObjectRef dObj)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyInsertion(dObj);
    }
}

void 
ReplicationUtilityAggregate::notifyDelete(DataObjectRef dObj)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyDelete(dObj);
    }
}

void 
ReplicationUtilityAggregate::notifySendSuccess(DataObjectRef dObj, NodeRef node)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifySendSuccess(dObj, node);
    }
}

void 
ReplicationUtilityAggregate::notifySendFailure(DataObjectRef dObj, NodeRef node)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifySendFailure(dObj, node);
    }
}

void 
ReplicationUtilityAggregate::notifyNewContact(NodeRef node)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyNewContact(node);
    }
}

void 
ReplicationUtilityAggregate::notifyDownContact(NodeRef node)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyDownContact(node);
    }
}

void 
ReplicationUtilityAggregate::notifyUpdatedContact(NodeRef node)
{
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyUpdatedContact(node);
    }
}

ReplicationUtilityAggregate::~ReplicationUtilityAggregate() 
{
    while (!factors.empty()) {
        ReplicationUtilityFunction *func = factors.front();
        factors.pop_front();
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        delete func;
    }
}

// Aggregate sum

string 
ReplicationUtilityAggregateSum::getPrettyName()
{
    bool first = true;
    string pretty = string("min(1, max(0,");
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        double weight = getGlobalOptimizer()->getWeightForName(func->getName());
        string factor;
        if (first && weight > 0) {
            stringprintf(factor, "(%.2f*%s)", weight, func->getPrettyName().c_str());
        }
        else if (weight > 0){
            stringprintf(factor, "+(%.2f*%s)", weight, func->getPrettyName().c_str());
        }
        else {
            stringprintf(factor, "-(%.2f*%s)", -1*weight, func->getPrettyName().c_str());
        }
        pretty = pretty + factor;
        first = false;
    }
    pretty = pretty + "))";
    return pretty;
}

double 
ReplicationUtilityAggregateSum::compute(string do_id, string node_id)
{
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    if (0 == weight) {
        return 0;
    }

    double sum = 0;
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        double computed_utility = func->compute(do_id, node_id);
        sum += computed_utility;
    }

    double normalized = sum;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);

    return normalized*weight;
}

// Aggregate Min

string 
ReplicationUtilityAggregateMin::getPrettyName()
{
    bool first = true;
    string pretty = string("max(0,min(1, ");
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        double weight = getGlobalOptimizer()->getWeightForName(func->getName());
        string factor;
        if (first) {
            stringprintf(factor, "(%.2f*%s)", weight, func->getPrettyName().c_str());
        }
        else {
            stringprintf(factor, ",(%.2f*%s)", weight, func->getPrettyName().c_str());
        }
        pretty = pretty + factor;
        first = false;
    }
    pretty = pretty + "))";
    return pretty;
}

double 
ReplicationUtilityAggregateMin::compute(string do_id, string node_id)
{
string strResult="Min[d:";
strResult.append(do_id);
strResult.append(",n:");
strResult.append(node_id);
strResult.append("](");
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    if (weight == 0) {
HAGGLE_DBG("0*MIN()=0,");
        return 0;
    }
//JM DELTEME

    double min_util = 1.0;
    double value;
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        value = func->compute(do_id, node_id);
    if (!strResult.empty()) {
       strResult.append(func->getPrettyName());
       strResult.append(",");
    }
        min_util = MIN(min_util, value);
    }

    double normalized = min_util;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    strResult.append(")=");
    char buffer[128];
    sprintf(buffer, "%1.2f", weight*normalized);
    strResult.append(buffer);
    HAGGLE_DBG("%s\n", strResult.c_str());
    return weight*normalized;
}

string 
ReplicationUtilityAggregateMax::getPrettyName()
{
    bool first = true;
    string pretty = string("min(1,max(0, ");
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        double weight = getGlobalOptimizer()->getWeightForName(func->getName());
        string factor;
        if (first) {
            stringprintf(factor, "(%.2f*%s)", weight, func->getPrettyName().c_str());
        }
        else {
            stringprintf(factor, ",(%.2f*%s)", weight, func->getPrettyName().c_str());
        }
        pretty = pretty + factor;
        first = false;
    }
    pretty = pretty + "))";

    return pretty;
}

// Aggregate Max

double 
ReplicationUtilityAggregateMax::compute(string do_id, string node_id)
{
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    if (weight == 0) {
        return 0;
    }

    double max_util = 0;
    for (List<ReplicationUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        ReplicationUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        max_util = MAX(max_util, func->compute(do_id, node_id));
    }

    double normalized = max_util;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
   
    return weight*normalized;
}

// END: aggregate functions.

// random

void
ReplicationUtilityRandom::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);
}

double 
ReplicationUtilityRandom::compute(string do_id, string node_id)
{
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    if (weight == 0) {
        return 0;
    }
    const int max_r = 1000000;
    double random_num = RANDOM_INT(max_r)/(double)max_r;

    double normalized = random_num;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    return normalized * weight;
}


// NOP: useful in conjunction with global optimizer to
// dynamically disable negative utility functions.

void
ReplicationUtilityNOP::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);
}

double 
ReplicationUtilityNOP::compute(string do_id, string node_id)
{
    return getGlobalOptimizer()->getWeightForName(getName());
}

//wait 'x' miliseconds before it can be sent, opposite effect of TTLREL
//thus, before time 'x', it has a value of 0, and after time 'x', it is a value of '1'

void
ReplicationUtilityWait::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("wait_s");

    if (param) {
        expireTimeval = Timeval(string(param));
    }
}

ReplicationUtilityWait::~ReplicationUtilityWait()
{
    time_map_t::iterator it = time_map.begin();
    for (; it != time_map.end(); it++) {
        time_map.erase(it);
    }
}

void
ReplicationUtilityWait::notifyInsertion(DataObjectRef dObj) 
{
    time_map_t::iterator it = time_map.find(DataObject::idString(dObj));
    if (it != time_map.end()) {
        time_map.erase(it);
    }
    string dobj_id = DataObject::idString(dObj);
    //only do this for non node DO's
    Timeval lifetime = Timeval::now() + expireTimeval;
    time_map.insert(make_pair(dobj_id, lifetime));
    
    double fudge_s = 0.2; // wait 200 ms to ensure recompute

    // schedule a replication event after the time has expired
    getKernel()->addEvent(ReplicationManagerFactory::getReplicateAllEvent(expireTimeval.getTimeAsSecondsDouble() + fudge_s));
}

void
ReplicationUtilityWait::notifyDeletion(DataObjectRef dObj) 
{
    string dobj_id = DataObject::idString(dObj);
    time_map_t::iterator it = time_map.find(dobj_id.c_str());
    if (it != time_map.end()) {
        time_map.erase(it);
    }
}

double 
ReplicationUtilityWait::compute(string do_id, string node_id)
{
    time_map_t::iterator it = time_map.find(do_id);
    if (it == time_map.end()) {
  	//HAGGLE_DBG2("DO %s has value of %f\n", do_id.c_str(), getGlobalOptimizer()->getWeightForName(getName()));
        return getGlobalOptimizer()->getWeightForName(getName());
    } 
    Timeval expiration = (*it).second;
    if (Timeval::now() < expiration) {
	//HAGGLE_DBG2("DO %s has value of %f\n", do_id.c_str(), 0.0);
        return 0.0;
    }
	//HAGGLE_DBG2("DO %s has value of %f\n", do_id.c_str(), getGlobalOptimizer()->getWeightForName(getName()));
    return getGlobalOptimizer()->getWeightForName(getName());
}

ReplicationUtilityAttribute::~ReplicationUtilityAttribute()
{
    map_attribute_t::iterator it = attribute_map.begin();
    for (; it != attribute_map.end(); it++) {
        attribute_map.erase(it);
    }
}

void
ReplicationUtilityAttribute::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("attribute_name");

    attr_name = string(""); 

    if (param) {
        attr_name = string(param);
    } else {
        HAGGLE_ERR("Utility attribute must specify name (attribute_name)\n");
    }

    param = m.getParameter("attr_max_value");

    if (param) {
        attr_max_value = atof(param);
    }
    if (attr_max_value <= 0) {
        HAGGLE_ERR("Attr max value must be > 0\n");
        attr_max_value = 1;
    }
    HAGGLE_DBG("Loaded attribute utility function: attr name: %s, attr max value: %d\n", attr_name.c_str(), attr_max_value);
}

double 
ReplicationUtilityAttribute::compute(string do_id, string node_id)
{
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    if (weight == 0) {
	//HAGGLE_DBG2("DO %s has value of %f\n", do_id.c_str(), 0.0);
        return 0;
    }
    map_attribute_t::iterator it = attribute_map.find(do_id);
    if (it == attribute_map.end()) {
	//HAGGLE_DBG2("DO %s has value of %f\n", do_id.c_str(), getGlobalOptimizer()->getWeightForName(getName()));
        return getGlobalOptimizer()->getWeightForName(getName());
    } 
    double attrBoost = (*it).second;
    //HAGGLE_DBG2("DO %s has value of %f\n", do_id.c_str(), weight * attrBoost);
    return weight * attrBoost;
}

void
ReplicationUtilityAttribute::notifyDelete(DataObjectRef dObj)
{
   map_attribute_t::iterator it = attribute_map.find(DataObject::idString(dObj));
   if (it != attribute_map.end()) {
       attribute_map.erase(it);
   }

}

void
ReplicationUtilityAttribute::notifyInsertion(DataObjectRef dObj)
{
    map_attribute_t::iterator it = attribute_map.find(DataObject::idString(dObj));
    if (it != attribute_map.end()) {
        attribute_map.erase(it);
    }
    const Attribute *attr = dObj->getAttribute(attr_name, "*", 1);
    if (!attr) {
        return;
    }
    double doWeight = atof(attr->getValue().c_str());

    double attrBoost = doWeight/(double)attr_max_value;
    if (attrBoost > 1.0) {
       attrBoost = 1.0;
    }
    if (attrBoost < 0) {
        attrBoost = 0;
    }
    attribute_map.insert(make_pair(DataObject::idString(dObj), attrBoost));

}

/**
* onConfig supports the following parameters, from the configuration file.
*
* exclude_my_group = <bool>   Exclude all nodes in my social group for calculation purpose, if true. 
*
* max_group_count = <int>   Maximum nodes per same social group to consider, as we give a value of 1-1/(2^#), 
*    where # is the number of nodes having the content.   By limiting this to '1', you can make a discrete
*    value with a weight of 2 (0,1), or give it a higher value, to add more weight to more copies in other groups.
*    This is 'M' value defined in the formula in compute().
*
*/
void
ReplicationUtilityNeighborhoodOtherSocial::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);

    const char *param;

    param = m.getParameter("exclude_my_group");
    if (param) {
        exclude_my_group = (0 == strcmp("true", param));
    }

    param = m.getParameter("max_group_count");
    if (param) {
        max_group_count = atoi(param);
    }

}

/**
* \return a value between [0, 1], signifying the diversity of the DO in relation to other social groups.
* 
* compute() will return a value based upon how many unique social groups (N) have
* a copy of the content in question.    Each social group has an equal weight of [0, 1/N],
* which is summed and returned.    We have a minimum number of social groups, to allow us to have
* a baseline, without knowing all the groups in the network (think predefined groups, such as the military).
* This minimum number (MN), does the following:  N=# of social groups seen; if (N<MN) then N=MN;
*
* Each social group returns a value, based upon 1-(1/(2^r)), where 
* r is the number of nodes in that social group, having the specified content.
* 
* Note the maximum value of r is M, and a minimum value of N, are set up in the onConfig().
* Thus, having no copies in any social group, will return a value of 0, 
* having a single copy in the network will return 1/2 * 1/N,
* having 2 copies in the same social group will return 3/4 * 1/N, 
* having r >= M copies in the same social group will return 1 * 1/N, 
* and returning r>=M copies for N social groups will return 1.
* 
*/
double 
ReplicationUtilityNeighborhoodOtherSocial::compute(string do_id/**< string id of the data object.
*/, string node_id /**< string id of the destination node*/) //,string &strResult/**< [in,out] DO compute result string.  Leave empty for no results */)
{
    string strResult=":"; //placeholder for debugging option
    double weight = getGlobalOptimizer()->getWeightForName(getName());
    char buffer[128];
    if (!strResult.empty()) {
       sprintf(buffer, "%1.2f*", weight);
       strResult.append(buffer);
       strResult.append(getShortName());
       strResult.append("(");
    }
   
    if (weight == 0) {
        if (!strResult.empty()) {
           strResult.append("), ");
	}
        return weight;
    }

    //Should only need to use the current node passed to us.

    NodeRef node = getKernel()->getNodeStore()->retrieve(node_id, true);
    //we can only replicate to 1-hop neighbors.   If not, it has 0 replication value.
    if (!node) {
        if (!strResult.empty()) {
           strResult.append("NBR0=0),");
	}
        return 0.0; 
    }  

    if (node == getKernel()->getThisNode()) {
        if (!strResult.empty()) {
           strResult.append("SELF=0),");
	}
        return 0.0; 
    }

    if (!node->isNeighbor() && !node->isLocalApplication()) {
        if (!strResult.empty()) {
           strResult.append("LOCAL=0),");
	}
        return 0.0; 
    }
 
    //skip if same social group as ourself and option set
    string mySocialGroup = getKernel()->getNodeStore()->returnMyNodeSocialGroupName();
    string nodeSocialGroup = getKernel()->getNodeStore()->returnNodeSocialGroupName(node);
    if (exclude_my_group && !mySocialGroup.compare(nodeSocialGroup)) {
        if (!strResult.empty()) {
           strResult.append("S_S=NODE_S:=0),");
	}
        return 0.0; 
    } 


    DataObjectId_t id;
    DataObject::idStrToId(do_id, id);

    NodeRefList socialMembers= getKernel()->getNodeStore()->returnNodesPerSocialGroup(nodeSocialGroup);
    //count copies in social group 
    int numcopies = 0;
    int numMembersSocialGroup = socialMembers.size();
    for (NodeRefList::iterator it = socialMembers.begin(); it != socialMembers.end(); it++) {
        NodeRef fakeneigh = (*it);
	//seems this method doesnt have bloomfilters, need to get it from nodestore
	NodeRef neigh = getKernel()->getNodeStore()->retrieve(fakeneigh);
   	//in case neigh returns null
        if (neigh && neigh->getBloomfilter()->has(id)) {
           numcopies++;
	} 
    }

    //count how many in each group
    double diversity=1.0;

    if (numcopies == 0) {
        diversity=0.0;
    } else if (numcopies >= max_group_count) {
        diversity=1.0;
    } else {
        diversity=(1-(1.0/pow(2.0, numcopies)));
    }
 
    //Replication value = 1-diversity.   High diversity/low replication.
    //Low diversity/high replication value
    double sum = 1.0-diversity;

    if (!strResult.empty()) {
       sprintf(buffer, "[G:%d/%d]=%1.2f), ", numcopies, max_group_count, sum*weight);
       strResult.append(buffer);
    }
    return sum*weight;
}

// local utility function:
/**
* onConfig supports the following parameters, from the configuration file.
*
* protect_local = <bool>  This option, if true, will give all locally published
* content a value of 1.   If false, it is treated as other content.
*
*/
void
ReplicationUtilityLocal::onConfig(const Metadata& m)
{
    ReplicationUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("protect_local");

    localImmunity = (0 == strcmp("true", param));
    HAGGLE_DBG("Loaded Localutility function:\n");
}

/**
* \return Value of either 0 or 1. This signifies if it is locally generated content or not.
* 
* compute() will return a value based upon if a specified content is locally generated (1) or not (0).
*/
double 
ReplicationUtilityLocal::compute(string do_id /**< string id of the data object */, string node_id) //string &strResult/**< [in,out] DO compute result string.  Leave empty for no results */)
{
string strResult; //placesetting for future debugging string
    double weight = getGlobalOptimizer()->getWeightForName(getName());
    char buffer[128];
    if (!strResult.empty()) {
       sprintf(buffer, "%1.2f*", weight);
       strResult.append(buffer);
       strResult.append(getShortName());
       strResult.append("(");
    }

    if (weight == 0) {
        if (!strResult.empty()) {
           strResult.append("), ");
        }
        return 0;
    }

    local_map_t::iterator it = local_map.find(do_id);
    if (it == local_map.end() ) {
        if (!strResult.empty()) {
           strResult.append("NL=0), ");
        }
        return 0;
    } else {
        if (!strResult.empty()) {
           char buffer[128];
           sprintf(buffer,"%1.2f*", weight);
           strResult.append(buffer);
           strResult.append("L=1), ");
        }
	return weight;
    }

}

/**
* \param dObj  DataObject that has been deleted
*
* notifyDelete() will remove all internal record keeping of the 
* dataobject, as it is no longer in the system.
*/

void
ReplicationUtilityLocal::notifyDelete(DataObjectRef dObj)
{
    local_map_t::iterator it = local_map.find(DataObject::idString(dObj));
    if (it != local_map.end()) {
        local_map.erase(it);
        //local_size -= dObj->getOrigDataLen();
    }

}

/**
* \param dObj  DataObject that has been deleted
*
* notifyInsertion() will add the dataobject into its local
* record keeping, so we can have a fast lookup between the 
* DO ID, and if it is a locally generated or not.   Otherwise,
* all computes would need the actual data object, as opposed to
* their ID. 
*/

void
ReplicationUtilityLocal::notifyInsertion(DataObjectRef dObj)
{

    bool isLocal = dObj->getRemoteInterface() && dObj->getRemoteInterface()->isApplication();
    if (!isLocal) {
        return;
    }
    local_map.insert(make_pair(DataObject::idString(dObj), dObj->getOrigDataLen() ));
    //local_size += dObj->getOrigDataLen();

}
