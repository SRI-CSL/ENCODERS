/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 *   Hasnain Lakhani (HL)
 */

#ifndef _LOSSRATESLIDINGWINDOWESTIMATOR_H
#define _LOSSRATESLIDINGWINDOWESTIMATOR_H

#include <libcpphaggle/List.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/Timeval.h>

using namespace haggle;

#include "Manager.h"
#include "Trace.h"
//#include <map>
#include "LossRateSlidingWindowElement.h"

class LossRateSlidingWindowEstimator;

class LossRateSlidingWindowEstimator
{

protected:
    HashMap<string, LossRateSlidingWindowElement> slidingWindowMap;
    Mutex swMutex;//protects sliding window map
    double initialLossRate;
    double windowSize;

public:
    LossRateSlidingWindowEstimator(double _initialLossRate, double _windowSize): initialLossRate(_initialLossRate), windowSize(_windowSize) {

    }
    LossRateSlidingWindowEstimator(LossRateSlidingWindowEstimator const & o):slidingWindowMap(o.slidingWindowMap), swMutex(o.swMutex), initialLossRate(o.initialLossRate), windowSize(o.windowSize) {
        
    }
    ~LossRateSlidingWindowEstimator(){

    }
    LossRateSlidingWindowEstimator& operator=(LossRateSlidingWindowEstimator const & o){
        slidingWindowMap = o.slidingWindowMap;
        swMutex = o.swMutex;
        windowSize = o.windowSize;
        initialLossRate = o.initialLossRate;
        return *this;
    }
    
    void advance(Map<string, bool> & flagMap);
    double queryLossRate(string iface);
    bool hasLossRate(string iface);

};

#endif /* _LOSSRATESLIDINGWINDOWESTIMATOR */
