/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Minyoung Kim (MK)
 *   Hasnain Lakhani (HL)
 *   Jihwa Lee (JL)
 */

#include "CacheUtilityFunction.h"
#include "CacheUtilityFunctionFactory.h"
#include "CacheReplacementTotalOrder.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
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
CacheUtilityFunction::onConfig(const Metadata& m)
{
    const char *param;
    param = m.getParameter("name");

    if (param) {
        name = string(param);
    }
}

// START: aggregate functions:

void 
CacheUtilityAggregate::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

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
        CacheUtilityFunction *func = CacheUtilityFunctionFactory::getNewUtilityFunction(getDataManager(), getGlobalOptimizer(), string(param));
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
CacheUtilityAggregate::notifyInsertion(DataObjectRef dObj)
{
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyInsertion(dObj);
    }
}

void 
CacheUtilityAggregate::notifyDelete(DataObjectRef dObj)
{
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifyDelete(dObj);
    }
}

void 
CacheUtilityAggregate::notifySendSuccess(DataObjectRef dObj, NodeRef node)
{
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        func->notifySendSuccess(dObj, node);
    }
}

CacheUtilityAggregate::~CacheUtilityAggregate() 
{
    while (!factors.empty()) {
        CacheUtilityFunction *func = factors.front();
        factors.pop_front();
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        delete func;
    }
}

string 
CacheUtilityAggregateSum::getPrettyName()
{
    bool first = true;
    string pretty = string("min(1, max(0,");
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
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
CacheUtilityAggregateSum::compute(string do_id, string &strResult)
{
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    char buffer[128];
    if (!strResult.empty()) {
       sprintf(buffer, "%1.2f*", weight);
       strResult.append(buffer);
       strResult.append(getShortName());
       strResult.append("(");
    }
    if (0 == weight) {
        if (!strResult.empty()) {
           strResult.append("), ");
	}
        return 0;
    }

    double sum = 0;
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        double computed_utility = func->compute(do_id,strResult );
        sum += computed_utility;
    }

    double normalized = sum;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    if (!strResult.empty()) {
       sprintf(buffer,":=%1.2f", normalized);
       strResult.append(buffer);
       strResult.append("), ");
    }

    return normalized*weight;
}

string 
CacheUtilityAggregateMin::getPrettyName()
{
    bool first = true;
    string pretty = string("max(0,min(1, ");
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
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
CacheUtilityAggregateMin::compute(string do_id, string &strResult)
{
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

    double min_util = 1;
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        min_util = MIN(min_util, func->compute(do_id, strResult));
    }

    double normalized = min_util;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    if (!strResult.empty()) {
       sprintf(buffer,":=%1.2f", normalized);
       strResult.append(buffer);
       strResult.append("), ");
    }

    return weight*normalized;
}

string 
CacheUtilityAggregateMax::getPrettyName()
{
    bool first = true;
    string pretty = string("min(1,max(0, ");
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
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
CacheUtilityAggregateMax::compute(string do_id, string &strResult)
{
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

    double max_util = 0;
    for (List<CacheUtilityFunction *>::const_iterator it = factors.begin(); 
         it != factors.end(); it++) {
        CacheUtilityFunction *func = (*it);
        if (!func) {
            HAGGLE_ERR("NULL function.\n");
            continue;
        }
        max_util = MAX(max_util, func->compute(do_id, strResult));
    }
    double normalized = max_util;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    if (!strResult.empty()) {
       sprintf(buffer,":=%1.2f", normalized);
       strResult.append(buffer);
       strResult.append("), ");
    }
    return weight*normalized;
}

// END: aggregate functions.

// popularity

CacheUtilityPopularity::~CacheUtilityPopularity()
{
    if (evictManager) {
        delete evictManager;
    }
}

void 
CacheUtilityPopularity::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    if (evictManager) {
        HAGGLE_ERR("Evict manager is already configured!\n");
        return;
    } 

    const Metadata *dm = m.getMetadata("EvictStrategyManager");

    if (!dm) {
        HAGGLE_ERR("Could not get config for evict manager\n");
        return;
    }

    evictManager = new EvictStrategyManager(getKernel());
    evictManager->onConfig(*dm);

    HAGGLE_DBG("Initialized evict manager.\n");
}

void 
CacheUtilityPopularity::notifyDelete(DataObjectRef dObj)
{
    if (!evictManager) {
        HAGGLE_ERR("No evict manager specified\n");
        return;
    }

    pop_map_t::iterator it = pop_map.find(dObj->getIdStr());
    if (it != pop_map.end()) {
        pop_map.erase(it);
    }
}

void 
CacheUtilityPopularity::notifyInsertion(DataObjectRef dObj)
{
    if (!evictManager) {
        HAGGLE_ERR("No evict manager specified\n");
        return;
    }

    evictManager->updateDataObject(dObj);
    
    pop_map_t::iterator it = pop_map.find(dObj->getIdStr());
    if (it != pop_map.end()) {
        pop_map.erase(it);
    }

    double evict_value = evictManager->getEvictValueForDataObject(dObj);
    
    if (evict_value < 0) {
        HAGGLE_ERR("Problems computing evict value\n");
        return;
    }
    
    max_evict_value = MAX(evict_value, max_evict_value);
    min_evict_value = MIN(evict_value, min_evict_value);

    pop_map.insert(make_pair(dObj->getIdStr(), evict_value));
}

void 
CacheUtilityPopularity::notifySendSuccess(DataObjectRef dObj, NodeRef node)
{
    if (!evictManager) {
        HAGGLE_ERR("No evict manager specified\n");
        return;
    }

    evictManager->updateDataObject(dObj);

    pop_map_t::iterator it = pop_map.find(dObj->getIdStr());
    if (it != pop_map.end()) {
        pop_map.erase(it);
    }

    double evict_value = evictManager->getEvictValueForDataObject(dObj);

    max_evict_value = MAX(evict_value, max_evict_value);
    min_evict_value = MIN(evict_value, min_evict_value);

    pop_map.insert(make_pair(dObj->getIdStr(), evict_value));
}

double 
CacheUtilityPopularity::compute(string do_id, string &strResult)
{
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

   if (!evictManager) {
        if (!strResult.empty()) {
           strResult.append("em=0),");
	}
        return 0;
    }

    pop_map_t::iterator it = pop_map.find(do_id);
    if (it == pop_map.end()) {
        HAGGLE_ERR("Data Object not in evict cache\n");
        if (!strResult.empty()) {
           strResult.append("EC=0),");
	}
        return 0;
    }

    double evict_value = (*it).second;
    bool firsttime = false; 
    // JM: if this is the first value, make it min.
    if (min_evict_value < 0.0) {
        min_evict_value = evict_value;
        firsttime = true;
    }

    double normalize_factor = max_evict_value - min_evict_value;
    double normalized_evict_value = evict_value;
//HAGGLE_DBG("evict max %f, min %f, evictValue: %f\n", max_evict_value, min_evict_value, evict_value);
    if (normalize_factor > 0.0) {
        normalized_evict_value = (evict_value-min_evict_value) / normalize_factor;
        //since we are working with FP, we may be 'slightly' negative
        if (normalized_evict_value < 0.0) {
            normalized_evict_value = 0.0;
        }
    }
    else {
        if (!firsttime) {
          HAGGLE_DBG("Normalize factor is  0!: min: %.2f, max: %.2f, factor: %.2f\n", min_evict_value, max_evict_value, normalize_factor); 
        }
        normalized_evict_value = 1.0;
    }


    //HAGGLE_DBG("%s computed normalized evict value: %.2f, orig: %.2f, factor: %.2f, weight: %.2f\n", do_id.c_str(), normalized_evict_value, evict_value, normalize_factor, weight);

    if (normalized_evict_value > 1) {
        HAGGLE_ERR("Warning: lru value is greater than 1: %.2f!\n", normalized_evict_value);
        normalized_evict_value = 1;
    }

    normalized_evict_value = MAX(normalized_evict_value, 0);
    normalized_evict_value = MIN(normalized_evict_value, 1);
    if (!strResult.empty()) {
       sprintf(buffer,"(%.2f-%.2f)/%.2f=%1.2f", evict_value, min_evict_value, normalize_factor, normalized_evict_value);
       strResult.append(buffer);
       strResult.append("), ");
    }
    return weight*normalized_evict_value;
}

// neighborhood

void
CacheUtilityNeighborhood::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("discrete_probablistic");
    if (param) {
        discrete_probablistic = (0 == strcmp("true", param));
    }

    param = m.getParameter("neighbor_fudge");
    if (param) {
        neighbor_fudge = atof(param);
    }
}

double 
CacheUtilityNeighborhood::compute(string do_id, string &strResult)
{
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
    
    NodeRefList nl;
    int num_nbrs = getKernel()->getNodeStore()->retrieveNeighbors(nl);
    if (0 == num_nbrs) {
        if (!strResult.empty()) {
           strResult.append("NN:1), ");
	}
        return weight;
    }

    int num_replicas = 0;
    for (NodeRefList::iterator it = nl.begin(); it != nl.end(); it++) {
        NodeRef& neigh = *it;

        DataObjectId_t id;
        DataObject::idStrToId(do_id, id);

        if (neigh->getBloomfilter()->has(id)) {
            num_replicas++;
        }
    }
    //JM: added 1 to include self
    double normalized = 1 - ((num_replicas+1)/((double)num_nbrs+1 + neighbor_fudge));

    if (discrete_probablistic) {
        const int max_r = 1000000;
        double random_num = RANDOM_INT(max_r)/(double)max_r;
        if (random_num < normalized) {
            normalized = 1;
        } else {
            normalized = 0;
        }
    }

    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    if (!strResult.empty()) {
       sprintf(buffer,"1-(%d/(%d+%d))[%s]=%1.2f", num_replicas+1, num_nbrs+1, neighbor_fudge, discrete_probablistic ? "D" : "", normalized);
       strResult.append(buffer);
       strResult.append("), ");
    }

    return weight*normalized;
}

/**
* onConfig supports the following parameters, from the configuration file.
*
* exp_num_neighbor_group = <int>  This sets the minimum expected groups for compute().
*   If the number of groups seen by the node is less than this value, then it will assume the groups
*   exist, but are unseen, and are included in value calculations.  This is shown as the value MN in compute().
*
* default_value = <float>   Default value for DO's that are not affected by this utility function.
*
* only_physical_neighbors = <bool>   Only consider 1-hop neighboor nodes for compute() calculation, if true. 
*   Otherwise, consider all known unexpired nodes.
* 
* less_is_more = <bool>  The result from compute() is avg(value), which tends to be higher when more copies seen.
*   If true, compute() inverts its return value to '1-value'. 
* 
* exclude_my_group = <bool>  This excludes all nodes in my social group for calculation purpose, if true. 
*
* max_group_count = <int>  This sets the maximum number of members per group to credit for having the content.   
*   The return value of compute() is based upon how many neighbor groups have a copy of the content. The exact formula is:
*     r = the number of members of a given group that have the content
*     M = the maximum number of members per group to credit for having the content
*   return = avg(value) over all neighboring social groups, where
*     value = 1, if r >= M, otherwise
*     value = 1 - (1 / (2^r))
*/
void
CacheUtilityNeighborhoodOtherSocial::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    const char *param;

    param = m.getParameter("exp_num_neighbor_group");
    if (param) {
        exp_neighbor_groups = atoi(param);
    }

    param = m.getParameter("default_value");
    if (param) {
        default_value = atof(param);
    }

    param = m.getParameter("only_physical_neighbors");
    if (param) {
        only_phy_neighbors = (0 == strcmp("true", param));
    }

    param = m.getParameter("less_is_more");
    if (param) {
        lessismore = (0 == strcmp("true", param));
    }

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
*
* For example, imagine 4 social groups (a,b,c,d) where group b has 1 copy of the
* content, and group c has 2 copies of the content. Then we have values:

*  A: 1 - (1 / 2^0) = 1 - (1 / 1) = 0
*  B: 1 - (1 / 2^1) = 1 - (1 / 2) = 1/2
*  C: 1 - (1 / 2^2) = 1 - (1 / 4) = 3/4
*  D: 1 - (1 / 2^0) = 1 - (1 / 1) = 0
*
* The average of 0, 1/2, 3/4, and 0 is 5/16, so the value is 5/16
*
* If 3 of the groups (a, b, c) had a single copy, the value would be
*
*  A: 1/2
*  B: 1/2
*  C: 1/2
*  D: 0
*
* The average of 1/2, 1/2, 1/2, and 0 is 6/16. Thus, diversity amongst social
* groups gives a higher value than a single group having many copies.
*
* By setting M to 1, value will either be 1 or 0, for each social group.
* In the above example (0,1,2,0) with an M of 1, gives (0+1+1+0)/4 = 1/2.
* The (1,1,1,0) example gives (1+1+1+0)/4 = 3/4 (still higher for same amount of content).
* 
*/
double 
CacheUtilityNeighborhoodOtherSocial::compute(string do_id/**< string id of the data object.
*/, string &strResult/**< [in,out] DO compute result string.  Leave empty for no results */)
{
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

    //get num groups and max numbers to use
    NodeRefList nodes;
    int num_nbrs;
    if (only_phy_neighbors) {
        num_nbrs = getKernel()->getNodeStore()->retrieveNeighbors(nodes);
    }
    else {
        num_nbrs = getKernel()->getNodeStore()->retrieveAllNodes(nodes);
    }
    if (0 == num_nbrs) {
        if (!strResult.empty()) {
           strResult.append("NBR0=0),");
	}
        return weight;
    }
    DataObjectId_t id;
    DataObject::idStrToId(do_id, id);

    //count copies in other groups
    Map < string, int > groupCount; 
    string nodeGroupName;
    string myGroupName = getKernel()->getNodeStore()->returnMyNodeSocialGroupName();
    int numGroups = 0, temp;
    for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
        NodeRef neigh = (*it);

        nodeGroupName = getKernel()->getNodeStore()->returnNodeSocialGroup(neigh);
        //dont count our group if option is set
        if (exclude_my_group && (nodeGroupName == myGroupName)) {
            continue;
        }

	Map < string, int >::iterator itt = groupCount.find(nodeGroupName);
        if (itt == groupCount.end()) {
           numGroups++;
           if (neigh->getBloomfilter()->has(id)) {
             groupCount.insert(make_pair(nodeGroupName, 1));
           } else {
             groupCount.insert(make_pair(nodeGroupName, 0));
           } 
        } else {
             if (neigh->getBloomfilter()->has(id)) {
               temp=(*itt).second;
	       groupCount.erase(itt);
               groupCount.insert(make_pair(nodeGroupName, ++temp));
             } //else {
               //temp=(*itt).second;
             //} 
        }          

    }

    //sum each, with each group 1/n total value.
    if (numGroups < exp_neighbor_groups) {
        numGroups = exp_neighbor_groups;
    }
    //count how many in each group
    double sum=0.0, sum1=0.0;
    int num;
    for(Map<string,int>::iterator it=groupCount.begin(); it != groupCount.end(); it++) {
       num=(*it).second;

       if (num == 0) {
          sum1 = 0.0;
       } else if (num >= max_group_count) {
          sum1 = (1.0/numGroups);
       } else { //no value added
          sum1 = (1.0/numGroups)*(1-(1.0/pow(2.0, num)));
       }
       sum += sum1;
    }
   
    if (lessismore) {
        sum = 1.0-sum;
    } 

    if (!strResult.empty()) {
       sprintf(buffer, "[G:%d/%d]=%1.2f), ", numGroups, max_group_count, sum);
       strResult.append(buffer);
    }
    return sum*weight;
}


// social neighborhood 

void
CacheUtilityNeighborhoodSocial::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("discrete_probablistic");
    if (param) {
        discrete_probablistic = (0 == strcmp("true", param));
    }

    param = m.getParameter("neighbor_fudge");
    if (param) {
        neighbor_fudge = atoi(param);
    }

    // JM, default value is typically, '0' or '1'
    // This is the value returned when there is NO MATCH
    // Thus, if you want a 'miss' to be a '0' or '1', set the value here.
    // Hits are always '1' (modified by weight)
    // I use '0', as misses should be zero (careful of min/max functions, fine for sum)
    param = m.getParameter("default_value");
    if (param) {
        default_value = atof(param);
    }
    // /JM

    param = m.getParameter("only_physical_neighbors");
    if (param) {
        only_phy_neighbors = (0 == strcmp("true", param));
    }

    param = m.getParameter("social_identifier");
    if (!param) {
        HAGGLE_ERR("Expects a social identifier\n");
        param=""; //should read our Node
    }
    social_identifier = string(param);
    //JM printout values
}

double 
CacheUtilityNeighborhoodSocial::compute(string do_id, string &strResult)
{
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

    if (string("") == social_identifier) {
        if (!strResult.empty()) {
           strResult.append("NSI=,");
           sprintf(buffer,"%1.2f", default_value);
           strResult.append(buffer);
           strResult.append("), ");
       }

       return weight*default_value; //JM misses are default value
    }

    const Attribute *my_attr = getKernel()->getThisNode()->getAttribute(social_identifier);
    if (!my_attr) {
        if (!strResult.empty()) {
           strResult.append("NSA=,");
           sprintf(buffer,"%1.2f", default_value);
           strResult.append(buffer);
           strResult.append("), ");
       }

        return weight*default_value; //JM no identifier, return default value
    } 

    //JM check if we have a match here, otherwise, return default
    social_neighbor_map_t::iterator it = soc_nbr_map.find(do_id);
    if (it == soc_nbr_map.end() ) {
        if (!strResult.empty()) {
           strResult.append("EOL=,");
           sprintf(buffer,"%1.2f", default_value);
           strResult.append(buffer);
           strResult.append("), ");
       }

        return weight*default_value;
    }
    
    NodeRefList nl;
    int num_nbrs;
    if (only_phy_neighbors) {
        num_nbrs = getKernel()->getNodeStore()->retrieveNeighbors(nl);
    }
    else {
        num_nbrs = getKernel()->getNodeStore()->retrieveAllNodes(nl);
    }
    if (0 == num_nbrs) {
        if (!strResult.empty()) {
           strResult.append("NBR0=0),");
	}
        return weight;
    }

    //Match, calculate # neighbors
    int num_relevant_nbrs = 0;
    int num_replicas = 0;

    for (NodeRefList::iterator it = nl.begin(); it != nl.end(); it++) {
        NodeRef& neigh = *it;

        DataObjectId_t id;
        DataObject::idStrToId(do_id, id);

        const Attribute *nbr_attr = neigh->getAttribute(social_identifier);
        if (!nbr_attr) {
            continue;
        }

        if (nbr_attr->getValue() != my_attr->getValue()) {
            continue;
        }

        num_relevant_nbrs++;

        if (neigh->getBloomfilter()->has(id)) {
            num_replicas++;
        }
    }

    double normalized = 1 - (num_replicas/((double)num_relevant_nbrs + neighbor_fudge));

    if (discrete_probablistic) {
        const int max_r = 1000000;
        double random_num = RANDOM_INT(max_r)/(double)max_r;
        if (random_num < normalized) {
            normalized = 1;
        } else {
            normalized = 0;
        }
    }

    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);

    if (!strResult.empty()) {
       sprintf(buffer,"1-(%d/(%d+%d))[%s]=%1.2f", num_replicas, num_nbrs, neighbor_fudge, discrete_probablistic ? "D" : "", normalized);
       strResult.append(buffer);
       strResult.append("), ");
    }
    //HAGGLE_DBG("DO: %s Social NBR utility: %f (%d replicas / %d neighbors)\n", do_id.c_str(), normalized, num_replicas, num_relevant_nbrs);
    return weight*normalized;
}

//JM
void 
CacheUtilityNeighborhoodSocial::notifyDelete(DataObjectRef dObj)
{
    social_neighbor_map_t::iterator it = soc_nbr_map.find(dObj->getIdStr());
    if (it != soc_nbr_map.end()) {
        soc_nbr_map.erase(it);
        //local_size -= dObj->getOrigDataLen();
    }
}

void 
CacheUtilityNeighborhoodSocial::notifyInsertion(DataObjectRef dObj)
{
    const Attribute *my_attr=getKernel()->getThisNode()->getAttribute(social_identifier);
    if (!my_attr) {
        return;
    }
    // our node has social neighborhood, do we match?
    //Neighborhood social matches subscribed attributes to attributes in the DO
    if (dObj->hasAttribute(*my_attr)) {
        soc_nbr_map.insert(make_pair(dObj->getIdStr(), 1 ));
        //local_size += dObj->getOrigDataLen();
    }
}

// random

void
CacheUtilityRandom::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);
}

double 
CacheUtilityRandom::compute(string do_id, string &strResult)
{
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
    const int max_r = 1000000;
    double random_num = RANDOM_INT(max_r)/(double)max_r;

    double normalized = random_num;
    normalized = MAX(normalized, 0);
    normalized = MIN(normalized, 1);
    if (!strResult.empty()) {
       sprintf(buffer,"%1.2f", normalized);
       strResult.append(buffer);
       strResult.append("), ");
    }
    return normalized * weight;
}

/*
This is the original version, to prevent nc to auto-reconstruct the file
it does not work, as the NC has its own cache outside of DataManager, so we cant
affect it.   Ideally we want this code, instead of our hack, only when the file is constructed, do we delete NC coded fragments. */
// SW: START SECURE NC:
CacheUtilitySecureCoding::CacheUtilitySecureCoding(DataManager *_dataManager, CacheGlobalOptimizer *_globalOptimizer) :
        CacheUtilityFunction(_dataManager, _globalOptimizer, CACHE_UTIL_SECURE_CODING_NAME),
        ncUtil(NULL),
        max_ratio(1.5),
	time_to_delete(0.0)
{
    ncUtil = new NetworkCodingDataObjectUtility();
}

CacheUtilitySecureCoding::~CacheUtilitySecureCoding()
{
    if (ncUtil) {
        delete ncUtil;
    }
}

double
CacheUtilitySecureCoding::compute(string do_id, string &strResult)
{
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


    //assumes if not a match, return '1'
    //check if reconstructed parent
    bool enough_time_passed = false;
    bool do_is_parent=false;
    parent_info_t parent_info;
    parent_id_map_t::iterator it = parent_id_map.find(do_id);
    if (it == parent_id_map.end()) {   //not an NC encoded DO
        //check if reconstuted parent
      	block_count_map_t::iterator itt = block_count.find(do_id);
        if (itt != block_count.end()) {   //nc reconstituted file
	     //if enough time has passed ...
             enough_time_passed = (Timeval::now()-(*itt).second.last_update) > time_to_delete;
	     do_is_parent = true;
             Timeval nowis=Timeval::now()-(*itt).second.last_update;
             int timepass=(int) (nowis.getTimeAsSecondsDouble());
             HAGGLE_DBG(" Time %d > %d to remove parent %s ", timepass, (int) time_to_delete, do_id.c_str());
        } else {   //not an NC encoded DO, nor a reconstituted file
            if (!strResult.empty()) {
              strResult.append("NNC:1), ");
	     }
             return weight;
	}
    } else {     //NC encoded DO
    	parent_info = (*it).second;
      	block_count_map_t::iterator ittt = block_count.find(parent_info.parent_id);
        if (ittt == block_count.end()) {  //unknown error
		HAGGLE_DBG("Unable to find parent info!");
                if (!strResult.empty()) {
                    strResult.append("EOL:1), ");
	        }
		return weight;
        }
        //enough time passed?
        enough_time_passed = (Timeval::now()-(*ittt).second.last_update) > time_to_delete;
    }
   
    //return 0.0/weight ONLY if enough time from last update has happened
    if (enough_time_passed ) { 
	if (do_is_parent) {
            if (!strResult.empty()) {
              strResult.append("TTL:0), ");
	     }
	     return 0.0;
	}
    	if (parent_info.originalToBeDeleted) {
            if (!strResult.empty()) {
              strResult.append("DOrig:0), ");
	     }
             return 0.0;
    	} else {
            if (!strResult.empty()) {
              strResult.append("NDOrig:1), ");
	     }
             return weight;
    	}
    } else {
       if (!strResult.empty()) {
          strResult.append("TS:1), ");
       }
       return weight;
    } 	 
}
void
CacheUtilitySecureCoding::notifyInsertion(DataObjectRef dObj)
{
    if (!ncUtil->isNetworkCodedDataObject(dObj)) {
        return;
    }

    string parent_id = ncUtil->getOriginalDataObjectId(dObj);
    block_count_map_t::iterator it = block_count.find(parent_id);
    int count = 0;
    int delete_count = 0;
    if (it != block_count.end()) {
        count = (*it).second.num_blocks;
        delete_count = (*it).second.delete_blocks;
        block_count.erase(it);
    }
    count++;

    BlockCountEntry_t e;
    e.num_blocks = count;
    e.delete_blocks = delete_count;
    e.last_update = dObj->getReceiveTime();
    double temp = ncUtil->getOriginalDataObjectLength(dObj) / ncUtil->getBlockSize();
    e.total_blocks = (int)ceil(temp);

    //JM:  if we are over ratio, mark a random block to be deleted
    double ratio = (e.num_blocks-e.delete_blocks) / ((double) e.total_blocks);
    if (ratio > max_ratio) {
        e.delete_blocks++; 
        //find a random block to delete, notice this will exclude the current block
        //since we purged the current block, lets mark a random block, or first block
	//to be deleted
	//a 3rd index might be more efficient, but for now ...
        unsigned long rand_indx=(long) (e.num_blocks-e.delete_blocks+1)*(rand()/ (double) RAND_MAX);  //get teh rand_indx value
        unsigned long rand_cnt=0;
        string last_id_match;
        for (Map<string, parent_info_t>::iterator it =
             parent_id_map.begin(); it != parent_id_map.end(); it++) {
                if (it->second.parent_id == parent_id) {
                    //check if not already deleted
		    //so, we are looking for the rand_indx non deleted entry, to mark it for
		    //deletion, example, if we have 10 entries, but 2 are deleted,
		    //we want a random value from 1 to (10-2).   We roll a 3.
		    //Assume the 2 pre deleted DO's are at locations 1 and 3.
		    // X, 1, X, 2, 3   <== we want to mark 3 to be deleted
                    if (!it->second.originalToBeDeleted) {
			rand_cnt++;
		    	last_id_match = it->first;
                    	if (rand_indx == rand_cnt) {
				break;
		    	}
		    }
                }
        }
        //check for errors
        if (rand_cnt == 0) {

		HAGGLE_DBG("ERROR! No match for %s in search!\n",parent_id.c_str());
 	}

        parent_id_map_t::iterator low_value = parent_id_map.find(last_id_match);
    	if (low_value != parent_id_map.end()) {
                parent_info_t newValue;
                newValue.parent_id = (*low_value).second.parent_id;
        	newValue.originalToBeDeleted = true;
        	//(*low_value).second.originalToBeDeleted = true;
		parent_id_map.erase(low_value);
		parent_id_map.insert(make_pair(last_id_match, newValue));
    	} else {
             HAGGLE_DBG("lost %s DO!", last_id_match.c_str());
	}      
    }

    // we need to store this since compute() takes a string and we don't
    // want to cache all of the data objects
    string orig_id = dObj->getIdStr();
    parent_info_t parent_info;
    parent_info.parent_id = parent_id;
    parent_info.originalToBeDeleted = false;
    parent_id_map_t::iterator itt = parent_id_map.find(orig_id);
    if (itt == parent_id_map.end()) {
        parent_id_map.insert(make_pair(orig_id, parent_info));
    }

    block_count.insert(make_pair(parent_id, e));
}

void
CacheUtilitySecureCoding::notifyDelete(DataObjectRef dObj)
{
    if (!ncUtil->isNetworkCodedDataObject(dObj)) {
        return;
    }

    string parent_id = ncUtil->getOriginalDataObjectId(dObj);
    block_count_map_t::iterator it = block_count.find(parent_id);
    BlockCountEntry_t e;
    if (it != block_count.end()) {
        e = (*it).second;
    }
    else {
        HAGGLE_ERR("More deletes than inserts for network coded block.\n");
    }

    block_count.erase(it);
    if (e.delete_blocks > 0) {
       e.delete_blocks--;
    }
    //JM: In theory, we could recheck ratio, after this delete, and unmark blocks already selected for deletion, 
    //in case they were deleted by another utility function, but that is a lot of work for questionable benefits.
    e.num_blocks--;

    if (e.num_blocks > 0) {
        block_count.insert(make_pair(parent_id, e));
    }

    string orig_id = dObj->getIdStr();
    parent_id_map_t::iterator itt = parent_id_map.find(orig_id);
    if (itt != parent_id_map.end()) {
        parent_id_map.erase(itt);
    }
    else {
        HAGGLE_ERR("parent id entry missing\n");
    }
}

void
CacheUtilitySecureCoding::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("max_block_ratio");
    if (!param) {
        HAGGLE_ERR("Missing max_block_ratio parameter\n");
        return;
    }

   
    max_ratio = atof(param);

    param = m.getParameter("rel_ttl_since_last_block");
    if (!param) {
        return;
    }
   
    time_to_delete = atof(param);
    //HAGGLE_DBG("max block ratio is %f and REL_TTL is %f\n", max_ratio, time_to_delete);
}

// SW: END SECURE NC.


// new DO time immunity
// In case of a LOT of DO's sent over short period of time, give those
// DO's within the window immunity/time to be transmitted
void
CacheUtilityImmunityNewByTime::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);
    const char *param;
    param = m.getParameter("TimeWindowInMS");
    if (!param) {
        return;
    }

    double timewindow = atof(param);
    if (timewindow < 0.0) {
        return;
    }

    time_window_in_ms = timewindow;

    param = m.getParameter("linear_declining");
    if (!param) {
        return;
    }
   
    linear_declining = (0 == strcmp("true", param));
}

double 
CacheUtilityImmunityNewByTime::compute(string do_id, string &strResult)
{
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

    receivetime_map_t::iterator it = time_map.find(do_id);
    if (it == time_map.end()) {
        //HAGGLE_DBG("DO: %s timeimmunity utility: 0.0\n", do_id.c_str());
        if (!strResult.empty()) {
           strResult.append("NF:0), ");
	}
        return 0.0;
    }
    //are we within range
    Timeval insertTime = (*it).second;
    Timeval lifelimit = Timeval::now()-Timeval(time_window_in_ms/1000.0);
    if (insertTime >= lifelimit) {
       //HAGGLE_DBG("DO: %s timeimmunity utility: %f..\n", do_id.c_str(), weight);
       if (linear_declining) {
        if (!strResult.empty()) {
           sprintf(buffer,"(%.2f-%.2f)/%.2fms=%1.2f", insertTime.getTimeAsSecondsDouble(), lifelimit.getTimeAsSecondsDouble(), time_window_in_ms,(insertTime.getTimeAsMilliSecondsDouble() - lifelimit.getTimeAsMilliSecondsDouble())/time_window_in_ms );
           strResult.append(buffer);
           strResult.append("), ");
	}
           return weight*(insertTime.getTimeAsMilliSecondsDouble() - lifelimit.getTimeAsMilliSecondsDouble())/time_window_in_ms;
        } else {
           if (!strResult.empty()) {
              strResult.append("TM:1), ");
	   }
           return weight;
        }
    } else {
       //it exists, but not within time range, delete it
       time_map.erase(it);
        if (!strResult.empty()) {
           strResult.append("TE:0), ");
	}
       return 0.0;
    }
}

void 
CacheUtilityImmunityNewByTime::notifyDelete(DataObjectRef dObj)
{
    receivetime_map_t::iterator it = time_map.find(dObj->getIdStr());
    if (it != time_map.end()) {
        time_map.erase(it);
    }
}

void 
CacheUtilityImmunityNewByTime::notifyInsertion(DataObjectRef dObj)
{
    time_map.insert(make_pair(dObj->getIdStr(), dObj->getReceiveTime()));
}

// NOP: useful in conjunction with global optimizer to
// dynamically disable negative utility functions.

void
CacheUtilityNOP::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);
}

double 
CacheUtilityNOP::compute(string do_id, string &strResult)
{
    char buffer[128];
    double weight = getGlobalOptimizer()->getWeightForName(getName());

    if (!strResult.empty()) {
       sprintf(buffer, "%1.2f*", weight);
       strResult.append(buffer);
       strResult.append(getShortName());
       strResult.append("(=1), ");
    }

    return weight;
}

// Replacement: 

double
CacheUtilityReplacement::compute(string do_id, string &strResult)
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();
    double weight = getGlobalOptimizer()->getWeightForName(getName());
    char buffer[128];
    if (!strResult.empty()) {
       sprintf(buffer, "%1.2f*", weight);
       strResult.append(buffer);
       strResult.append(getShortName());
       strResult.append("(");
    }

    if (pad->hasScratchpadAttributeDouble(do_id, getName() + ":delete_flag")) {
        double delete_flag = pad->getScratchpadAttributeDouble(do_id, getName() + ":delete_flag");
        if (delete_flag) {
            if (!strResult.empty()) {
               strResult.append("DEL:0),");
            }
            return 0;
        }
    }
    if (!strResult.empty()) {
       strResult.append("NM=1),");
    }

    return weight;
}

void
CacheUtilityReplacement::notifyDelete(DataObjectRef dObj)
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":delete_flag")) {
        pad->removeScratchpadAttribute(dObj, getName() + ":delete_flag");
    }
}

void
CacheUtilityReplacement::notifyInsertion(DataObjectRef dObj)
{
    if (!isResponsibleForDataObject(dObj)) {
        return;
    }

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    // SW: NOTE: This is carry over from CacheStrategyReplacementPurger
    // TODO: unify these two approaches to reduce code duplication,
    // although the intention is for utility based caching to
    // subsume CacheStrategyReplacementPurger
    DataObjectRefList subsumed = DataObjectRefList();
    DataObjectRefList equiv = DataObjectRefList();
    DataObjectRefList nonsubsumed = DataObjectRefList();
    bool isDatabaseTimeout = false;

    getOrganizedDataObjectsByNewDataObject(
        dObj,
        &subsumed,
        &equiv,
        &nonsubsumed,
        isDatabaseTimeout);

    bool stored = false;

    if ((subsumed.size() > 0 || equiv.size() > 0) || 
        (subsumed.size() == 0 && equiv.size() == 0 && nonsubsumed.size() == 0)) {

        for (DataObjectRefList::iterator it = subsumed.begin(); 
            it != subsumed.end(); it++) {
            // mark them for deletion
            pad->setScratchpadAttributeDouble(*it, getName() + ":delete_flag", 1);
        }
    
        stored = true;
    }

    if (!stored) {
        // mark for delete:
        pad->setScratchpadAttributeDouble(dObj, getName() + ":delete_flag", 1);
    }
}

// Replacement Priority:


// SW: START REPLACEMENT PRIORITY HELPERS:
// SW: TODO: this is directly copied from CacheReplacementPriority,
// and should be integrated with this class to avoid code 
// duplication. We copy it for now, due to time constraints.
static void
sortedReplacementListInsert(
    List<Pair<CacheUtilityReplacement *, int> >& list, 
    CacheUtilityReplacement *replacement, 
    int priority)
{
    List<Pair<CacheUtilityReplacement *, int> >::iterator it = list.begin();

    for (; it != list.end(); it++) {
        if (priority > (*it).second) {
            break;
        }
    }
    list.insert(it, make_pair(replacement, priority));
}

static DataObjectRefList 
intersectDataObjectList(DataObjectRefList a, DataObjectRefList b)
{
    // naive O(n^2) intersection, not relying on dObj order
    DataObjectRefList intersection = DataObjectRefList();
    for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
        for (DataObjectRefList::iterator itB = b.begin(); itB != b.end(); itB++) {
            if (*itB == *itA) {
                intersection.add(*itA);
            }
        }
    }
    return intersection;
}

static DataObjectRefList 
unionDataObjectList(DataObjectRefList a, DataObjectRefList b)
{
    // naive O(n^2) union, not relying on dObj order
    DataObjectRefList setUnion = DataObjectRefList();
    for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
        setUnion.add(*itA);
    }

    for (DataObjectRefList::iterator itB = b.begin(); itB != b.end(); itB++) {
        bool inA = false;
        for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
            if (*itB == *itA) {
                inA = true;
            }
        }

        if (!inA) {
            setUnion.add(*itB);
        }
    }

    return setUnion;
}

static bool
anyRelevant(
    DataObjectRef dObj, 
    List<CacheUtilityReplacement *> *relevantReplacements) 
{
    List<CacheUtilityReplacement* >::iterator it = 
            relevantReplacements->begin();

    for (; it != relevantReplacements->end(); it++) {
        CacheUtilityReplacement *replacement = *it;
        if (replacement->isResponsibleForDataObject(dObj)) {
            return true;
        }
    }

    return false;
}

static bool
allRelevant(
    DataObjectRef dObj, 
    List<CacheUtilityReplacement *> *relevantReplacements) 
{
    List<CacheUtilityReplacement* >::iterator it = 
            relevantReplacements->begin();

    bool hasItem = false;
    for (; it != relevantReplacements->end(); it++) {
        hasItem = true;
        CacheUtilityReplacement *replacement = *it;
        if (!(replacement->isResponsibleForDataObject(dObj))) {
            return false;
        }
    }

    return hasItem && true;
}

static DataObjectRefList 
filterDataObjectListByReplacementList(
    DataObjectRefList a, 
    List<CacheUtilityReplacement *> *relevantReplacements,
    List<CacheUtilityReplacement *> *irrelevantReplacements)
{
    DataObjectRefList filterdObjs = DataObjectRefList();
    for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
        DataObjectRef dObj = *itA; 

        if (allRelevant(dObj, relevantReplacements) 
            && !anyRelevant(dObj, irrelevantReplacements)) {
            filterdObjs.add(dObj);
        }
    }

    return filterdObjs;
}

static List<CacheUtilityReplacement *> *
filterRelevantReplacementListByDataObject(
    DataObjectRef dObj,  
    List<Pair<CacheUtilityReplacement *, int> > *replacements) 
{
    List<CacheUtilityReplacement* > *filteredReplacements = new List<CacheUtilityReplacement* >();

    List<Pair<CacheUtilityReplacement*, int> >::iterator itA = replacements->begin();
    for (; itA != replacements->end(); itA++) {
        CacheUtilityReplacement *replacement = (*itA).first;
        if (replacement->isResponsibleForDataObject(dObj)) {
            filteredReplacements->push_back(replacement);
        }
    }

    return filteredReplacements;
}

static List<CacheUtilityReplacement *> *
filterIrrelevantReplacementListByDataObject(
    DataObjectRef dObj,  
    List<Pair<CacheUtilityReplacement *, int> > *replacements) 
{
    List<CacheUtilityReplacement* > *filteredReplacements = new List<CacheUtilityReplacement* >();

    List<Pair<CacheUtilityReplacement*, int> >::iterator itA = replacements->begin();
    for (; itA != replacements->end(); itA++) {
        CacheUtilityReplacement *replacement = (*itA).first;
        if (!(replacement->isResponsibleForDataObject(dObj))) {
            filteredReplacements->push_back(replacement);
        }
    }

    return filteredReplacements;
}

// SW: END REPLACEMENT PRIORITY HELPERS.

void
CacheUtilityReplacementPriority::onConfig(const Metadata& m)
{
    int i = 0;
    const char *param = NULL;
    const Metadata *replacementConfig;
    CacheUtilityReplacement *replacement;

   while (NULL != (replacementConfig = m.getMetadata("CacheUtilityReplacement", i++))) {
        param = replacementConfig->getParameter(CACHE_UTIL_REPLACEMENT_PRIORITY_NAME_FIELD_NAME);
        if (!param) {
            HAGGLE_ERR("No replacement class specified\n");
            return;
        }
                        
        replacement = (CacheUtilityReplacement *)CacheUtilityFunctionFactory::getNewUtilityFunction(getDataManager(), getGlobalOptimizer(), string(param));

        if (!replacement) {
            HAGGLE_ERR("Could not instantiate replacement\n");
            return;
        }

        const Metadata *options = replacementConfig->getMetadata(param);
        if (!options) {
            delete replacement;
            HAGGLE_ERR("Could not get configuration options for replacement\n");
            return;
        }
        
        replacement->onConfig(*options);

        param = replacementConfig->getParameter(CACHE_UTIL_REPLACEMENT_PRIORITY_FIELD_NAME);
        if (!param) {
            HAGGLE_ERR("No replacement class specified\n");
            return;
        }

        int priority = 0;
        char *endptr = NULL;
        int paramInt = (int) strtol (param, &endptr, 10);
        if (endptr && endptr != param && paramInt > 0) {
            priority = paramInt;
        }

        if (0 >= priority) { 
            delete replacement;
            HAGGLE_ERR("Could not assign priority to replacement\n");
            return;
        }

        sortedReplacementListInsert(sorted_replacement_list, replacement, priority);
    }
}

void
CacheUtilityReplacementPriority::getOrganizedDataObjectsByNewDataObject(
    DataObjectRef &dObj,
    DataObjectRefList *o_subsumed,
    DataObjectRefList *o_equiv,
    DataObjectRefList *o_nonsubsumed,
    bool &isDatabaseTimeout)
{
    if (NULL == o_subsumed || NULL == o_equiv || NULL == o_nonsubsumed) {
        HAGGLE_ERR("Null args\n");
        return;
    }

    bool firstIteration = false;
    DataObjectRefList equivdObjs = DataObjectRefList();
    DataObjectRefList subsumeddObjs = DataObjectRefList();
    DataObjectRefList nonSubsumeddObjs = DataObjectRefList();

    List<CacheUtilityReplacement *> *relevantReplacements = 
        filterRelevantReplacementListByDataObject(
            dObj, 
            &sorted_replacement_list);

    List<CacheUtilityReplacement *> *irrelevantReplacements = 
        filterIrrelevantReplacementListByDataObject(
            dObj, 
            &sorted_replacement_list);

    List<CacheUtilityReplacement *>::iterator it = relevantReplacements->begin();

    for (; it != relevantReplacements->end(); it++) {
        CacheUtilityReplacement *replacement = *it;

        DataObjectRefList currentSubsumeddObjs;
        DataObjectRefList currentEquivdObjs;
        DataObjectRefList currentNonSubsumeddObjs;

        replacement->getOrganizedDataObjectsByNewDataObject(
            dObj, 
            &currentSubsumeddObjs,
            &currentEquivdObjs,
            &currentNonSubsumeddObjs,
            isDatabaseTimeout);

        currentSubsumeddObjs = filterDataObjectListByReplacementList(
            currentSubsumeddObjs,
            relevantReplacements,
            irrelevantReplacements);

        currentEquivdObjs = filterDataObjectListByReplacementList(
            currentEquivdObjs,
            relevantReplacements,
            irrelevantReplacements);

        currentNonSubsumeddObjs = filterDataObjectListByReplacementList(
            currentNonSubsumeddObjs,
            relevantReplacements,
            irrelevantReplacements);

        if (!firstIteration) {
            firstIteration = true;
            equivdObjs = currentEquivdObjs;
            subsumeddObjs = currentSubsumeddObjs;
            nonSubsumeddObjs = currentNonSubsumeddObjs;
            continue;
        }

        subsumeddObjs = unionDataObjectList(
            subsumeddObjs, 
            intersectDataObjectList(equivdObjs, currentSubsumeddObjs));

        nonSubsumeddObjs = unionDataObjectList(
            nonSubsumeddObjs, 
            intersectDataObjectList(equivdObjs, currentNonSubsumeddObjs));

        equivdObjs = intersectDataObjectList(
            equivdObjs, 
            currentEquivdObjs);
    }

    delete relevantReplacements;
    delete irrelevantReplacements;

    (*o_subsumed) = subsumeddObjs;
    (*o_equiv) = equivdObjs;
    (*o_nonsubsumed) = nonSubsumeddObjs;
}

bool
CacheUtilityReplacementPriority::isResponsibleForDataObject(DataObjectRef dObj)
{
    List<Pair<CacheUtilityReplacement *, int> >::iterator it = sorted_replacement_list.begin();

    for (; it != sorted_replacement_list.end(); it++) {
        CacheUtilityReplacement *replacement = (*it).first;
        if (replacement->isResponsibleForDataObject(dObj)) {
            return true;
        }
    }

    return false;
}

CacheUtilityReplacementPriority::~CacheUtilityReplacementPriority()
{
    List<Pair<CacheUtilityReplacement *, int> >::iterator it = sorted_replacement_list.begin();

    for (; it != sorted_replacement_list.end(); it++) {
        delete (*it).first;
    }
}

string
CacheUtilityReplacementPriority::getPrettyName()
{
    List<Pair<CacheUtilityReplacement *, int> >::iterator it = sorted_replacement_list.begin();

    string pretty = ("priority(");
    
    bool first = true;
    for (; it != sorted_replacement_list.end(); it++) {
        CacheUtilityReplacement *replacement = (*it).first;
        int priority = (*it).second;
        //pretty = pretty + (!first ? string(",") : string("")) +  string("(") + replacement->getPrettyName() + string(", ") + string(priority) + string(")");
        if (first) {
            pretty = pretty + string("(") + replacement->getPrettyName() + string(", ") + string('0' + priority) + string(")");
        } 
        else {
            pretty = pretty + string(",(") + replacement->getPrettyName() + string(", ") + string('0' + priority) + string(")");
        }
        first = false;
    }

    pretty = pretty + string(")");
    return pretty;
}

// Total Order Replacement:

bool
CacheUtilityReplacementTotalOrder::isResponsibleForDataObject(DataObjectRef dObj)
{
    return cacheReplacement->isResponsibleForDataObject(dObj);
}



void
CacheUtilityReplacementTotalOrder::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    string metricField;
    string idField;
    string tagField;
    string tagFieldValue;

    const char *param;

    param = m.getParameter(CACHE_UTIL_REPLACEMENT_METRIC);
    if (!param) {
        HAGGLE_ERR("No metric specified\n");
        return;
    }

    metricField = string(param);

    param = m.getParameter(CACHE_UTIL_REPLACEMENT_ID);
    if (!param) {
        HAGGLE_ERR("No id specified\n");
        return;
    }

    idField = string(param);

    param = m.getParameter(CACHE_UTIL_REPLACEMENT_TAG);
    if (!param) {
        HAGGLE_ERR("No tag specified\n");
        return;
    }

    tagField = string(param);

    param = m.getParameter(CACHE_UTIL_REPLACEMENT_TAG_VALUE);
    if (!param) {
        HAGGLE_ERR("No id specified\n");
        return;
    }

    tagFieldValue = string(param);

    cacheReplacement = new CacheReplacementTotalOrder(getDataManager(), metricField, idField, tagField, tagFieldValue);
    if (!cacheReplacement) {
        HAGGLE_ERR("Could not instantiate cache replacement.\n");
        return;
    }

    HAGGLE_DBG("Loaded total order replacement utility function with metric: %s, id: %s, tag: %s:%s\n",
        metricField.c_str(), idField.c_str(), tagField.c_str(), tagFieldValue.c_str());
}

void
CacheUtilityReplacementTotalOrder::getOrganizedDataObjectsByNewDataObject(
        DataObjectRef &dObj,
        DataObjectRefList *o_subsumed,
        DataObjectRefList *o_equiv,
        DataObjectRefList *o_nonsubsumed,
        bool &isDatabaseTimeout)
{
    return cacheReplacement->getOrganizedDataObjectsByNewDataObject(dObj, o_subsumed, o_equiv, o_nonsubsumed, isDatabaseTimeout);
}

string
CacheUtilityReplacementTotalOrder::getPrettyName()
{
    return string("TOrepl(d)");
}

CacheUtilityReplacementTotalOrder::~CacheUtilityReplacementTotalOrder()
{
    if (cacheReplacement) {
        delete cacheReplacement;
    }
}

// Purgers: allows backwards compatibility with existing 
// ReplacementPurger architecture

bool
CacheUtilityTimePurger::isResponsibleForDataObject(
    DataObjectRef& dObj)
{
    if (!dObj) {
        return false;
    }

    const Attribute *attr;
    attr = dObj->getAttribute(tagField, tagFieldValue, 1);
    
    if (!attr) {
        return false;
    }

    attr = dObj->getAttribute(metricField, "*", 1);

    if (!attr) {
        return false;
    }

    return true; 
}

double
CacheUtilityTimePurger::compute(string do_id, string &strResult)
{
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

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();
    double now = 0.0;
    double evict_date = 0.0;
    double base_value=1;
    double ttl = pad->getScratchpadAttributeDouble(do_id, getName()+":TTL");
    if (pad->hasScratchpadAttributeDouble(do_id, getName())) {
        now = Timeval::now().getTimeAsSecondsDouble();
        base_value = pad->getScratchpadAttributeDouble(do_id, getName()+":base");
        evict_date = pad->getScratchpadAttributeDouble(do_id, getName());
        if (now > evict_date) {
          if (!strResult.empty()) {
            strResult.append("TE=0),");
          }
          return 0;
        }
    } else {
        if (!strResult.empty()) {
           strResult.append("NSP=1),");
        }
        return weight;
    }

    if (linear_declining) {
        if (!strResult.empty()) {
          strResult.append("[L]");
           sprintf(buffer,"(%.2f-%.2f)/%.2f=%1.2f", evict_date, now, ttl, (evict_date - now)/ttl);
           strResult.append(buffer);
           strResult.append("), ");
        }
        return weight*(evict_date - now)/ttl;
    } else {
        if (!strResult.empty()) {
         strResult.append("[NL]=1),");
        }
        return weight;
    }

}

void
CacheUtilityTimePurger::notifyDelete(DataObjectRef dObj)
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    if (pad->hasScratchpadAttributeDouble(dObj, getName())) {
        pad->removeScratchpadAttribute(dObj, getName());
    }
}

void
CacheUtilityTimePurger::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);
    const char *param;

    param = m.getParameter(CACHE_UTIL_PURGER_METRIC);
    if (!param) {
        HAGGLE_ERR("No metric specified.\n");
        return;
    }

    metricField = string(param); 

    param = m.getParameter(CACHE_UTIL_PURGER_TAG);
    if (!param) {
        HAGGLE_ERR("No tag specified.\n");
        return;
    }

    tagField = string(param);

    param = m.getParameter(CACHE_UTIL_PURGER_TAG_VALUE);
    if (!param) {
        HAGGLE_ERR("No tag value specified.\n");
        return;
    }

    tagFieldValue = string(param);

    param = m.getParameter(CACHE_UTIL_PURGER_MIN_DB_TIME_S);
    if (param) {
        minDBtimeS = atof(param);
    } else {
        minDBtimeS = 0;
    }

    param = m.getParameter("linear_declining");
    if (!param) {
        return;
    }
   
    linear_declining = (0 == strcmp("true", param));

    HAGGLE_DBG("Loaded purger utility function %s with metric: %s, tag=%s:%s, min db time: %f\n", 
        getName().c_str(), metricField.c_str(), tagField.c_str(), tagFieldValue.c_str(), minDBtimeS);
}

void
CacheUtilityTimePurger::notifyInsertion(DataObjectRef dObj)
{
    if (!isResponsibleForDataObject(dObj)) {
        return;
    }

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    double now = Timeval::now().getTimeAsSecondsDouble();
    double then = (dObj->getReceiveOrCreateTime()).getTimeAsSecondsDouble();
    double new_evict_date = getEvictDate(dObj);

    if (new_evict_date < now) {
        new_evict_date = now;
    }

    if ((new_evict_date - then) < minDBtimeS) {
        new_evict_date = then + minDBtimeS;
    }


    if (pad->hasScratchpadAttributeDouble(dObj, getName())) {
        double evict_date = pad->getScratchpadAttributeDouble(dObj, getName());
        if (new_evict_date > evict_date) {
            return;
        }
    }

    pad->setScratchpadAttributeDouble(dObj, getName(), new_evict_date);
    pad->setScratchpadAttributeDouble(dObj, getName()+":base", then);
    const Attribute *attr = dObj->getAttribute(metricField, "*", 1);
    if (!attr) {
        HAGGLE_ERR("Somehow missing metric field!\n");
        return;
    }
    double ttl = atof(attr->getValue().c_str());
    pad->setScratchpadAttributeDouble(dObj, getName()+":TTL", ttl );
}

double
CacheUtilityPurgerRelTTL::getEvictDate(DataObjectRef& dObj)
{
    double now = Timeval::now().getTimeAsSecondsDouble();
    if (!isResponsibleForDataObject(dObj)) {
        HAGGLE_ERR("Trying to get evict date for data object we're not responsible for.\n");
        return now;
    }

    const Attribute *attr = dObj->getAttribute(metricField, "*", 1);
    if (!attr) {
        HAGGLE_ERR("Somehow missing metric field!\n");
        return now;
    }

    double ttl = atof(attr->getValue().c_str());
    double then = (dObj->getReceiveOrCreateTime()).getTimeAsSecondsDouble();

    return ttl + then;
}

string
CacheUtilityPurgerRelTTL::getPrettyName()
{
    return string("relttl(d)");
}

double
CacheUtilityPurgerAbsTTL::getEvictDate(DataObjectRef& dObj)
{
    double now = Timeval::now().getTimeAsSecondsDouble();
    if (!isResponsibleForDataObject(dObj)) {
        HAGGLE_ERR("Trying to get evict date for data object we're not responsible for.\n");
        return now;
    }

    const Attribute *attr = dObj->getAttribute(metricField, "*", 1);
    if (!attr) {
        HAGGLE_ERR("Somehow missing metric field!\n");
        return now;
    }

    double absttl = atof(attr->getValue().c_str());

    return absttl;
}

string
CacheUtilityPurgerAbsTTL::getPrettyName()
{
    return string("absttl(d)");
}

// Attribute utility function:

void
CacheUtilityAttribute::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

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
        attr_max_value = atoi(param);
    }
    if (attr_max_value <= 0) {
        HAGGLE_ERR("Attr max value must be > 0\n");
        attr_max_value = 1;
    }
    HAGGLE_DBG("Loaded attribute utility function: attr name: %s, attr max value: %d\n", attr_name.c_str(), attr_max_value);
}

double 
CacheUtilityAttribute::compute(string do_id, string &strResult)
{
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

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    double attrBoost = 0;
    if (pad->hasScratchpadAttributeDouble(do_id, getName() + ":attrValue")) {
        attrBoost = pad->getScratchpadAttributeDouble(do_id, getName() + ":attrValue");
    }
    attrBoost = attrBoost/(double)attr_max_value;
    if (attrBoost > 1.0) {
       attrBoost = 1.0;
    }
    if (attrBoost < 0) {
        attrBoost = 0;
    }
    if (!strResult.empty()) {
           sprintf(buffer,"%1.2f", attrBoost);
           strResult.append(buffer);
           strResult.append("), ");

    }
    return weight * attrBoost;
}

void
CacheUtilityAttribute::notifyDelete(DataObjectRef dObj)
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":attrValue")) {
        pad->removeScratchpadAttribute(dObj, getName() + ":attrValue");
        //local_size -= dObj->getOrigDataLen();
    }
}

void
CacheUtilityAttribute::notifyInsertion(DataObjectRef dObj)
{
    const Attribute *attr = dObj->getAttribute(attr_name, "*", 1);
    if (!attr) {
        return;
    }
    double doWeight = atof(attr->getValue().c_str());
    
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":attrValue")) {
        return;
    }

    pad->setScratchpadAttributeDouble(dObj, getName() + ":attrValue", doWeight);
    //local_size += dObj->getOrigDataLen();

}

// hop count utility function:

void
CacheUtilityHopCount::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("attribute_name");

    hc_attr_name = string(""); 

    if (param) {
        hc_attr_name = string(param);
    } else {
        HAGGLE_ERR("Utility hop count must specify name (attribute_name)\n");
    }

    param = m.getParameter("attribute_max_name");

    if (param) {
        hc_attr_max_name = string(param);
    }

    param = m.getParameter("attribute_min_name");

    if (param) {
        hc_attr_min_name = string(param);
    }

    param = m.getParameter("geometric_series_constant");

    if (param) {
    	geometric_series_constant = atof(param);
    }

    // geometric_series_constant of 0 disables summation
    if (geometric_series_constant < 0 || geometric_series_constant >= 1) {
        HAGGLE_ERR("Geometric series constant c must be: 0 <= c < 1\n");
    }

    HAGGLE_DBG("Loaded hop count utility function: attr name: %s, attr max name: %s, attr min name: %s, geometric series constant: %.2f\n", hc_attr_name.c_str(), hc_attr_max_name.c_str(), hc_attr_min_name.c_str(), geometric_series_constant);
}

double 
CacheUtilityHopCount::compute(string do_id, string &strResult)
{
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

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    int max_hop_count = 9999;
    if (pad->hasScratchpadAttributeDouble(do_id, getName() + ":max_hop_count")) {
        max_hop_count = pad->getScratchpadAttributeDouble(do_id, getName() + ":max_hop_count");
    } else {
        //HAGGLE_ERR("max hop count missing from scratchpad\n");
        // case where there is no hop-count information
        if (!strResult.empty()) {
            strResult.append("SP-E=0),");
        }
        return 0;
    }

    int min_hop_count = 0;
    if (pad->hasScratchpadAttributeDouble(do_id, getName() + ":min_hop_count")) {
        min_hop_count = pad->getScratchpadAttributeDouble(do_id, getName() + ":min_hop_count");
    } else {
        HAGGLE_ERR("min hop count missing from scratchpad\n");
        if (!strResult.empty()) {
           strResult.append("SP-M=0),");
        }
        return 0;
    }

    int hop_count = 0;
    if (pad->hasScratchpadAttributeDouble(do_id, getName() + ":hop_count")) {
        hop_count = pad->getScratchpadAttributeDouble(do_id, getName() + ":hop_count");
    }
    else {
        HAGGLE_ERR("Missing hop count\n");
        if (!strResult.empty()) {
           strResult.append("SP-MC=0),");
        }
        return 0;
    }

    if (hop_count > max_hop_count) {
        if (!strResult.empty()) {
           strResult.append("XHP=0),");
        }
        return 0;
    }

    if (hop_count < min_hop_count) {
        if (!strResult.empty()) {
           strResult.append("MHP=0),");
        }
	return 0;
    }

    // if geometric series is enabled, we give more
    // weight to data objects that are farther away
    // from the publisher
    if (geometric_series_constant > 0) {
        double utility = (1 - pow(geometric_series_constant, hop_count+1)) / (1 - (double) geometric_series_constant);
        const double converge = (double)1/(1 - (double) geometric_series_constant);
        if (!strResult.empty()) {
            strResult.append("), ");
            sprintf(buffer,"[G]%.2f/%.2f)=%1.2f),", utility,converge, utility/converge );
        }
        utility /= converge;
        return utility*weight;
    }

    if (!strResult.empty()) {
       sprintf(buffer,"%d<%d<%d=1)", min_hop_count, hop_count, max_hop_count);
       strResult.append(buffer);
    }
    return weight;
}

void
CacheUtilityHopCount::notifyDelete(DataObjectRef dObj)
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":max_hop_count")) {
        pad->removeScratchpadAttribute(dObj, getName() + ":max_hop_count");
    }

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":min_hop_count")) {
        pad->removeScratchpadAttribute(dObj, getName() + ":min_hop_count");
    }

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":hop_count")) {
        pad->removeScratchpadAttribute(dObj, getName() + ":hop_count");
    }
}

void
CacheUtilityHopCount::notifyInsertion(DataObjectRef dObj)
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();

    if (pad->hasScratchpadAttributeDouble(dObj, getName() + ":max_hop_count")) {
        // NOTE: for now, we use the TTL of the first DO that arrive,
        // and ignore subsequent
        return;
    }

    const Attribute *attr = dObj->getAttribute(hc_attr_name, "*", 1);
    if (!attr) {
        // not eligible for hop count ttl
        return;
    }

    string metadata_name = attr->getValue();

    if (metadata_name == "") {
        HAGGLE_ERR("No metadata name specified\n");
        return;
    }

    int max_ttl = 9999;
    const Attribute *max_attr = dObj->getAttribute(hc_attr_max_name, "*", 1);
    if (max_attr) {
        max_ttl = atoi(max_attr->getValue().c_str());
    }

    int min_ttl = 0;
    const Attribute *min_attr = dObj->getAttribute(hc_attr_min_name, "*", 1);
    if (min_attr) {
        min_ttl = atoi(min_attr->getValue().c_str());
    }

    // SW: make sure to lock the data object when
    // when modifying the metadata
    dObj.lock();


    Metadata *metadata = dObj->getMetadata();
    if (!metadata) {
        dObj.unlock();
        return;
    }

    unsigned long count = 0;
    
    Metadata *md = metadata->getMetadata(metadata_name);
    if (!md) {
        // SW: TODO: NOTE: this might not be the best way to check if it's from
        // a local application, but it works for now...
        bool isLocal = dObj->getRemoteInterface() && dObj->getRemoteInterface()->isApplication();

        if (!isLocal) {
            // we received it from a neighbor, but it was
            // missing the hop count. This can happen if
            // the neighbor is the publisher and the neighbor
            // only manages remote files
            count = 1; 
        }

        md = metadata->addMetadata(metadata_name);
    }

    const char *param = md->getParameter("hop_count");

    if (param) {
        char *endptr = NULL;
        count = strtoul(param, &endptr, 10);
        if (!endptr || endptr == param) {
            count = 0;
        }
    }

    char countstr[30];
    snprintf(countstr, 29, "%lu", count+1);
    // NOTE: we set the count to what it will be on the
    // receiver, since we do not intercept upon forwarding
    md->setParameter("hop_count", countstr);

    dObj.unlock();

    pad->setScratchpadAttributeDouble(dObj, getName() + ":max_hop_count", max_ttl);
    pad->setScratchpadAttributeDouble(dObj, getName() + ":min_hop_count", min_ttl);
    pad->setScratchpadAttributeDouble(dObj, getName() + ":hop_count", count);
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
CacheUtilityLocal::onConfig(const Metadata& m)
{
    CacheUtilityFunction::onConfig(m);

    const char *param;
    param = m.getParameter("protect_local");

    localImmunity = (0 == strcmp("true", param));
    HAGGLE_DBG("Loaded Localutility function: ");
}

/**
* \return Value of either 0 or 1. This signifies if it is locally generated content or not.
* 
* compute() will return a value based upon if a specified content is locally generated (1) or not (0).
*/
double 
CacheUtilityLocal::compute(string do_id /**< string id of the data object */, string &strResult/**< [in,out] DO compute result string.  Leave empty for no results */)
{
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
CacheUtilityLocal::notifyDelete(DataObjectRef dObj)
{
    local_map_t::iterator it = local_map.find(dObj->getIdStr());
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
CacheUtilityLocal::notifyInsertion(DataObjectRef dObj)
{
    bool isLocal = dObj->getRemoteInterface() && dObj->getRemoteInterface()->isApplication();

    if (!isLocal) {
        return;
    }
    local_map.insert(make_pair(dObj->getIdStr(), dObj->getOrigDataLen() ));
    //local_size += dObj->getOrigDataLen();
}
