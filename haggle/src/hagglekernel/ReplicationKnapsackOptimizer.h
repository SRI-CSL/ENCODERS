/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _REPL_KS_OPT_H
#define _REPL_KS_OPT_H

#include "HaggleKernel.h"
#include "ReplicationDataObjectUtilityMetadata.h"

class ReplicationDataObjectUtilityMetadata;

/**
 * The `ReplicationKnapsackOptimizerResults` class is used by the knapsack
 * optimizer to pass state to the caller upon performing knapsack optimization.
 */
class ReplicationKnapsackOptimizerResults {

private:

    bool hadError; /**< Flag to indicate whether or not an error occured. */

    Timeval duration; /**< The duration it took to solve the knapsack. */

public:

    /**
     * Construct a results object after solving the optimization problem.
     */
    ReplicationKnapsackOptimizerResults(
        bool _hadError /**< Flag if an error occured */) :
            hadError(_hadError) {};

    /**
     * Set the how long it took to solve the optimization problem.
     */
    void setDuration(Timeval _duration /**< Duration to solve the optimization problem. */) { duration = _duration; }

    /**
     * Get the duration to solve the optimization problem.
     * @return The solving duration.
     */
    Timeval getDuration() { return duration; }

    /**
     * Flag to set if an error occurred.
     */
    void setHadError(bool _hadError /**< Set to `true` if an error occured, `false` otherwise. */) { hadError = _hadError; }

    /**
     * Return whether an error occurred.
     * @ `true` if an error occured, `false` otherwise.
     */
    bool getHadError() { return hadError; }
};

/**
 * An abstract class to break ties when two objects have identical costs and utilities.
 */
class ReplicationKnapsackOptimizerTiebreaker {

public:

    /**
     * This will sort a list of data objects that tied, where the first element
     * has greatest priority.
     */
    void sortList(List<ReplicationDataObjectUtilityMetadataRef> *list /**< The list to be sorted, and reorganized by breaking the ties. */);

    /**
     * Compare two utilities to break the tie.
     * @return 0 if there is another tie, -1 if m1 < m2, and 1 otherwise.
     */
    virtual int compare(ReplicationDataObjectUtilityMetadataRef m1 /**< The first utility to be compared. */, ReplicationDataObjectUtilityMetadataRef m2 /**< The second utility to be compared. */) = 0;
};

/**
 * Class to break ties using the create time. 
 */
class ReplicationKnapsackOptimizerTiebreakerCreateTime : public ReplicationKnapsackOptimizerTiebreaker {

public:

    /**
     * Compare two utilities by create time.
     * @return -1 if m1 create time is < m2, 0 if equal, 1 otherwise.
     */
    int compare(ReplicationDataObjectUtilityMetadataRef m1 /**< The first utility to be compared. */, ReplicationDataObjectUtilityMetadataRef m2 /**< The second utility to be compared. */);
};

/**
 * The ReplicationKnapsackOptimizer base class defines the interface for
 * knapsack optimizers to determine the optimal data object to replicate to
 * a node. The problem is expressed as a 0-1 knapsack optimization problem.
 */
class ReplicationKnapsackOptimizer {

private: 

    string name; /**< The name of the knapsack optimizer. */

    ReplicationKnapsackOptimizerTiebreaker *tiebreaker; /**< The tiebreaker class used to break ties. */

public:

    /**
     * Instantiate a new knapsack optimizer.
     */
    ReplicationKnapsackOptimizer(
        string _name /**< The name of the optimizer. */,
        ReplicationKnapsackOptimizerTiebreaker *_tiebreaker = NULL /**< The tiebreaker class. */) :
            name(_name),
            tiebreaker(_tiebreaker) {};

    /**
     * Deconstruct the knapsack optimizer and free its resources.
     */ 
    virtual ~ReplicationKnapsackOptimizer();

    /**
     * Solve the knapsack optimizer problem by specifiying the utilities
     * and costs, and bag size.
     * @return The optimization results data object.
     */
    virtual ReplicationKnapsackOptimizerResults solve(
        List<ReplicationDataObjectUtilityMetadataRef > *to_process /**< The list of utilities that are elligible to go in the knapsack. */,
        List<ReplicationDataObjectUtilityMetadataRef > *to_include /**< The utilities that the optimizer says to include. */,
        List<ReplicationDataObjectUtilityMetadataRef > *to_exclude /**< The utilities that the optimizer says to exlcude. */,
        int bag_size) = 0 /**< The optimization bag size. */;

    /**
     * Configure the knapsack optimizer based on the config.xml.
     */
    virtual void onConfig(const Metadata& m /**< The metadata from the configuration. */) {};

    /**
     * Get the name of the knapsack optimizer.
     * @return The name of the knapsack optimizer.
     */
    string getName() { return name; }

    /**
     * Get the class used to break ties during optimization.
     */
    ReplicationKnapsackOptimizerTiebreaker *getTiebreaker();
};

#define REPL_KNAPSACK_GREEDY_NAME "ReplicationKnapsackOptimizerGreedy" /**< The name of the greedy optimizer. */

/**
 * The greedy optimizer uses the marginal cost hueristic to solve the 
 * optimization problem. The maginal cost is the utility / cost, and data
 * objects are picked greatest marginal cost first.
 */
class ReplicationKnapsackOptimizerGreedy : public ReplicationKnapsackOptimizer {

private:

    int discrete_size; /**< Normalize the data object size by this amount when computing cost. */

public:

    /**
     * Construct a new greedy knapsack optimizer.
     */
    ReplicationKnapsackOptimizerGreedy() :
        ReplicationKnapsackOptimizer(REPL_KNAPSACK_GREEDY_NAME),
        discrete_size(1) {};

    /**
     * Solve the knapsack problem by selecting the greatest marginal utility
     * first until the bag size is full. 
     */
    ReplicationKnapsackOptimizerResults solve(
        List<ReplicationDataObjectUtilityMetadataRef > *to_process /**< The data objects that are eligible for insertion into the bag. */,
        List<ReplicationDataObjectUtilityMetadataRef > *to_include /**< [out] The data objects to include in the bag, after the optimization. */,
        List<ReplicationDataObjectUtilityMetadataRef > *to_exclude /**< [out] The data objects to exclude from the bag, after the optimization. */,
        int bag_size /**< The size of the bag. */);

    /**
     * Configure the knapsack optimizer by specifying the config metadata.
     */
    void onConfig(const Metadata& m /**< The metadata used to configure the knapsack optimizer. */);
};

#endif /* _REPL_STRAT_KS_OPT_H */
