/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#include "ReplicationKnapsackOptimizer.h"

#include <libcpphaggle/Heap.h>

class ReplicationKnapsackGreedyHeapItem;

class ReplicationKnapsackGreedyHeapItem : public HeapItem {
protected:
    double marginal_util;
    ReplicationDataObjectUtilityMetadataRef do_metadata;
public: 
    ReplicationKnapsackGreedyHeapItem(
        double _marginal_util,
        ReplicationDataObjectUtilityMetadataRef _do_metadata) :
            marginal_util(_marginal_util),
            do_metadata(_do_metadata) {};

    bool compare_less(const HeapItem& i) const {
        // organize highest marginaul utility first
        return (marginal_util > static_cast<const ReplicationKnapsackGreedyHeapItem&>(i).marginal_util);
    }

    bool compare_greater(const HeapItem& i) const {
        // organize highest marginaul utility first
        return (marginal_util < static_cast<const ReplicationKnapsackGreedyHeapItem&>(i).marginal_util);
    }
    
    ReplicationDataObjectUtilityMetadataRef getMetadata() { return do_metadata; }
};

ReplicationKnapsackOptimizerTiebreaker *
ReplicationKnapsackOptimizer::getTiebreaker() {
    if (!tiebreaker) {
        // SW NOTE: we default to the create time tie breaker,
        // fututure versions will allow this to be specified in config.xml
        tiebreaker = new ReplicationKnapsackOptimizerTiebreakerCreateTime();
    }
    return tiebreaker;
}

void
ReplicationKnapsackOptimizerGreedy::onConfig(const Metadata& m)
{
    const char *param;
  
    param = m.getParameter("discrete_size");
    if (param) {
        discrete_size = atoi(param);
    }

    if (discrete_size <= 0) {
        HAGGLE_ERR("Discrete size must be > 0\n");
    }

    HAGGLE_DBG("Loaded greedy optimizer with discrete size of: %d\n", discrete_size);
}

ReplicationKnapsackOptimizerResults
ReplicationKnapsackOptimizerGreedy::solve( 
        List<ReplicationDataObjectUtilityMetadataRef > *to_process, 
        List<ReplicationDataObjectUtilityMetadataRef > *to_include, 
        List<ReplicationDataObjectUtilityMetadataRef > *to_exclude, 
        int bag_size)
{
    Timeval then = Timeval::now();
    ReplicationKnapsackOptimizerResults results = ReplicationKnapsackOptimizerResults(false);

    if (!to_process) {
        HAGGLE_ERR("NULL process list.\n");
        results.setHadError(true);
        return results;
    }

    if (!to_exclude) {
        HAGGLE_ERR("NULL delete list.\n");
        results.setHadError(true);
        return results;
    }

    double max_cost = 1;
    double util_bump = 0;

    // avoid possible precision errors, we first calculate
    // the maximum cost to compute util_bump
    for (List<ReplicationDataObjectUtilityMetadataRef >::iterator it = to_process->begin();
         it != to_process->end(); it++) {
        ReplicationDataObjectUtilityMetadataRef do_info = (*it);
        if (!do_info) {
            HAGGLE_ERR("NULL DO in list\n");
            results.setHadError(true);
            continue;
        }
        double current_cost = (double)do_info->getCost();
        max_cost = max_cost > current_cost ? max_cost : current_cost;
    }
    util_bump = max_cost;

    Heap heap;

    int total_free = bag_size;

    for (List<ReplicationDataObjectUtilityMetadataRef >::iterator it = to_process->begin();
         it != to_process->end(); it++) {
        ReplicationDataObjectUtilityMetadataRef do_info = (*it);
        if (!do_info) {
            continue;
        }
        // NOTE: we also include the age, to avoid ties and default to
        // evicting by creation date.
        int discrete_cost = (int) (discrete_size) * ((int) do_info->getCost() / (int) discrete_size);
        discrete_cost = discrete_cost <= 0 ? 1 : discrete_cost;
        double marginal_utility = util_bump * (do_info->getUtility() + 1)  / (double) discrete_cost;
	//HAGGLE_DBG2("DO:%s = %f (%f * %f / %d(%d * %d / %d))", do_info->getId().c_str(), marginal_utility, util_bump, do_info->getUtility() + 1, discrete_cost, discrete_size, do_info->getCost(), discrete_size);
        ReplicationKnapsackGreedyHeapItem *heap_item 
            = new ReplicationKnapsackGreedyHeapItem(marginal_utility, do_info);
        heap.insert(heap_item);
    }

    while (!heap.empty()) {
        ReplicationKnapsackGreedyHeapItem *do_heap_item = 
            static_cast<ReplicationKnapsackGreedyHeapItem *>(heap.extractFirst());
        if (!do_heap_item) {
            HAGGLE_ERR("NULL heap item\n");
            results.setHadError(true);
            continue;
        }
        ReplicationDataObjectUtilityMetadataRef do_meta = do_heap_item->getMetadata();
        delete do_heap_item;
        if (!do_meta) {
            HAGGLE_ERR("NULL DO in heap\n");
            results.setHadError(true);
            continue;
        }

        int discrete_cost = (int) (discrete_size) * ((int) do_meta->getCost() / (int) discrete_size);
        discrete_cost = discrete_cost <= 0 ? 1 : discrete_cost;
        const double marginal_utility = util_bump * (do_meta->getUtility() + 1)  / (double) discrete_cost;

        //HAGGLE_ERR("marginal util: %.2f\n", marginal_utility);

        List<ReplicationDataObjectUtilityMetadataRef > ties;
        ties.push_front(do_meta);

        while (!heap.empty()) {
            ReplicationKnapsackGreedyHeapItem *do_heap_item2 = 
                static_cast<ReplicationKnapsackGreedyHeapItem *>(heap.front());
            if (!do_heap_item2) {
                HAGGLE_ERR("NULL heap item (inner while).\n"); 
                results.setHadError(true);
                break;
            }
            ReplicationDataObjectUtilityMetadataRef do_meta2 = do_heap_item2->getMetadata();
            if (!do_meta) {
                HAGGLE_ERR("NULL DO in heap (inner while).\n");
                results.setHadError(true);
                break;
            }

            int discrete_cost2 = (int) (discrete_size) * ((int) do_meta2->getCost() / (int) discrete_size);
            discrete_cost2 = discrete_cost2 <= 0 ? 1 : discrete_cost2;
            const double marginal_utility2 = util_bump * (do_meta2->getUtility() + 1)  / discrete_cost2;

            if (marginal_utility != marginal_utility2) {
                // no tie!
                break;
            }

            // tie occured 

            heap.extractFirst();
            delete do_heap_item2;
            ties.push_front(do_meta2);
        }

        getTiebreaker()->sortList(&ties);

        if (ties.size() > 1) {
            HAGGLE_DBG("Breaking %d ties.\n", ties.size());
        }

        for (List<ReplicationDataObjectUtilityMetadataRef >::iterator it = ties.begin();
             it != ties.end(); it++) {

            ReplicationDataObjectUtilityMetadataRef do_meta3 = (*it);
 
            if (!do_meta3) {
                HAGGLE_ERR("NULL meta data!\n");
                continue;
            }

            const int cost3 = do_meta3->getCost();

            if ((total_free - cost3) > 0) {
                total_free -= cost3;
                to_include->push_back(do_meta3);
            }
            else {
                to_exclude->push_back(do_meta3);
            }
        }
    }

    Timeval now = Timeval::now();
    results.setDuration(now - then); 
    return results;
}

ReplicationKnapsackOptimizer::~ReplicationKnapsackOptimizer() 
{
    if (tiebreaker) {
        delete tiebreaker;
    }
}

class ReplicationKnapsackOptimizerTiebreakerHeapItem : public HeapItem {
private: 
    ReplicationKnapsackOptimizerTiebreaker *tiebreaker;
    ReplicationDataObjectUtilityMetadataRef do_metadata;
public:
    ReplicationKnapsackOptimizerTiebreakerHeapItem(
        ReplicationKnapsackOptimizerTiebreaker* _tiebreaker,
        ReplicationDataObjectUtilityMetadataRef _do_metadata) :
            tiebreaker(_tiebreaker),
            do_metadata(_do_metadata) {};

    bool compare_less(const HeapItem& i) const {
        ReplicationKnapsackOptimizerTiebreakerHeapItem other_heap_item = static_cast<const ReplicationKnapsackOptimizerTiebreakerHeapItem&>(i);
        int cmp = tiebreaker->compare(do_metadata, other_heap_item.getMetadata());
        // other is bigger, we are less
        return cmp == 1;
    }

    bool compare_greater(const HeapItem& i) const {
        ReplicationKnapsackOptimizerTiebreakerHeapItem other_heap_item = static_cast<const ReplicationKnapsackOptimizerTiebreakerHeapItem&>(i);
        int cmp = tiebreaker->compare(do_metadata, other_heap_item.getMetadata());
        // other is smaller, we are greater
        return cmp == -1;
    }

    ReplicationDataObjectUtilityMetadataRef getMetadata() { return do_metadata; }
};


// returns a sorted list, where the first entry
// should be inserted in the cache before the last entry
void
ReplicationKnapsackOptimizerTiebreaker::sortList(
    List<ReplicationDataObjectUtilityMetadataRef> *list)
{
    if (!list) {
        HAGGLE_ERR("NULL list.\n");
        return; 
    }

    // heap sort

    Heap heap;

    for (List<ReplicationDataObjectUtilityMetadataRef >::iterator it = list->begin();
         it != list->end(); it++) {

        ReplicationDataObjectUtilityMetadataRef do_info = (*it);

        if (!do_info) {
            HAGGLE_ERR("NULL do_info in the sort list\n");
            continue;
        }

        ReplicationKnapsackOptimizerTiebreakerHeapItem *heap_item =
            new ReplicationKnapsackOptimizerTiebreakerHeapItem(this, do_info);
        
        heap.insert(heap_item);
    }

    list->clear();

    while (!heap.empty()) {
        ReplicationKnapsackOptimizerTiebreakerHeapItem *do_heap_item = 
            static_cast<ReplicationKnapsackOptimizerTiebreakerHeapItem *>(heap.extractFirst());
        if (!do_heap_item) {
            HAGGLE_ERR("NULL heap item\n");
            continue;
        }
        ReplicationDataObjectUtilityMetadataRef do_meta = do_heap_item->getMetadata();
        delete do_heap_item;
        if (!do_meta) {
            HAGGLE_ERR("NULL DO in heap\n");
            continue;
        }
        list->push_back(do_meta);
    }
}

int 
ReplicationKnapsackOptimizerTiebreakerCreateTime::compare(
    ReplicationDataObjectUtilityMetadataRef m1, 
    ReplicationDataObjectUtilityMetadataRef m2)
{
    if (!m1 && !m2) {
        HAGGLE_ERR("NULL arguments\n");
        return 0;
    }

    if (!m1) {
        HAGGLE_ERR("NULL m1 argument.\n");
        return 1;
    }
    
    if (!m2) {
        HAGGLE_ERR("NULL m2 argument.\n");
        return -1;
    }

    if (m1->getCreateTime() == m2->getCreateTime()) {
        return 0;
    }

    if (m1->getCreateTime() < m2->getCreateTime()) {
        return -1;
    }

    return 1;
}
