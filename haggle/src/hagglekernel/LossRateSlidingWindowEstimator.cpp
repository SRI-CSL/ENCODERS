/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 *   Hasnain Lakhani (HL)
 */

#include "LossRateSlidingWindowEstimator.h"

void LossRateSlidingWindowEstimator::advance(Map<string, bool> & flagMap){
    Mutex::AutoLocker l(swMutex);
    
   // HAGGLE_DBG("advance\n");

    for(HashMap<string, LossRateSlidingWindowElement>::iterator iterator = slidingWindowMap.begin(); iterator != slidingWindowMap.end(); iterator++) {
        string key = (*iterator).first;
        if(flagMap.find(key)==flagMap.end())
            (*iterator).second.advance(0);
        else
            (*iterator).second.advance(1);
    }

    for(Map<string, bool>::iterator itflag = flagMap.begin(); itflag  != flagMap.end(); itflag++){

        string key = itflag->first;
		
		//HAGGLE_DBG("flagmap checking key: %s\n", key.c_str());
        
        if(slidingWindowMap.find(key)==slidingWindowMap.end()){ //new interface, add sliding window
        	//HAGGLE_DBG("adding key to sliding window map\n");
            LossRateSlidingWindowElement newElement(initialLossRate, windowSize); 
            slidingWindowMap.insert(make_pair(key, newElement));
        }            
    }
    
}

double LossRateSlidingWindowEstimator::queryLossRate(string iface){
    Mutex::AutoLocker l(swMutex);
    HashMap<string, LossRateSlidingWindowElement>::iterator it = slidingWindowMap.find(iface);
    if(it!=slidingWindowMap.end()){
        double lossrate = (*it).second.getLossRate();
        return lossrate;
    }
    return -1;
}

bool LossRateSlidingWindowEstimator::hasLossRate(string iface){
    Mutex::AutoLocker l(swMutex);
    return (slidingWindowMap.find(iface)!=slidingWindowMap.end());
}
