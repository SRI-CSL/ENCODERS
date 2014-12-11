/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _CACHE_KS_OPT_H
#define _CACHE_KS_OPT_H

#include "HaggleKernel.h"

class DataObjectUtilityMetadata;

class CacheKnapsackOptimizerResults {
private:
    bool hadError;
    Timeval duration;
public:
    CacheKnapsackOptimizerResults(
        bool _hadError) :
            hadError(_hadError) {};
    void setDuration(Timeval _duration) { duration = _duration; }
    Timeval getDuration() { return duration; }
    void setHadError(bool _hadError) { hadError = _hadError; }
    bool getHadError() { return hadError; }
};

class CacheKnapsackOptimizerTiebreaker {
public:
    void sortList(List<DataObjectUtilityMetadata*> *list);
    virtual int compare(DataObjectUtilityMetadata *m1, DataObjectUtilityMetadata *m2) = 0;
};

class CacheKnapsackOptimizerTiebreakerCreateTime : public CacheKnapsackOptimizerTiebreaker {
public:
    int compare(DataObjectUtilityMetadata *m1, DataObjectUtilityMetadata *m2);
};

class CacheKnapsackOptimizer {
private: 
    string name;
    CacheKnapsackOptimizerTiebreaker *tiebreaker;
public:
    CacheKnapsackOptimizer(
        string _name,
        CacheKnapsackOptimizerTiebreaker *_tiebreaker = NULL) :
            name(_name),
            tiebreaker(_tiebreaker) {};
    virtual ~CacheKnapsackOptimizer();
    virtual CacheKnapsackOptimizerResults solve(
        List<DataObjectUtilityMetadata *> *to_process, 
        List<DataObjectUtilityMetadata *> *to_delete,
        int bag_size, bool mem_only=false) = 0;
    virtual void onConfig(const Metadata& m) {};
    string getName() { return name; }
    CacheKnapsackOptimizerTiebreaker *getTiebreaker();
};

#define CACHE_KNAPSACK_GREEDY_NAME "CacheKnapsackOptimizerGreedy"
class CacheKnapsackOptimizerGreedy : public CacheKnapsackOptimizer {
private:
    int discrete_size;
public:
    CacheKnapsackOptimizerGreedy() :
        CacheKnapsackOptimizer(CACHE_KNAPSACK_GREEDY_NAME),
        discrete_size(1) {};
    CacheKnapsackOptimizerResults solve(
        List<DataObjectUtilityMetadata *> *to_process,
        List<DataObjectUtilityMetadata *> *to_delete,
        int bag_size, bool mem_only=false);
    void onConfig(const Metadata& m);
};

#endif /* _CACHE_STRAT_KS_OPT_H */
