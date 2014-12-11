/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */


#include "EvictStrategyLRU_K.h"

void EvictStrategyLRU_K::onConfig(const Metadata* m)
{
    const char *param = m->getParameter("name");
    if (!param) {
        HAGGLE_ERR("No name specified.\n");
        return;
    }
    name = string(param);

    param = m->getParameter("countType");
    if (!param) {
        HAGGLE_ERR("No count type specified.\n");
        return;
    }
    countType = string(param);

    if (countType != EVICT_STRAT_LRU_K_COUNT_TYPE_TIME && 
        countType != EVICT_STRAT_LRU_K_COUNT_TYPE_VIRTUAL) {
        HAGGLE_ERR("Unknown count type: %s.\n", countType.c_str());
        return;
    }

    param = m->getParameter("kValue");
    if (!param) {
        HAGGLE_ERR("No k-value specified.\n");
        return;
    }
    k = atoi(param);

    if (k <= 0) {
        HAGGLE_ERR("Bad kvalue: %d\n", k);
    }
}

void EvictStrategyLRU_K::updateInfoDataObject(DataObjectRef &dObj, unsigned int count, Timeval time) 
{  

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();
    double paramValueK;
    if (countType == EVICT_STRAT_LRU_K_COUNT_TYPE_TIME) {
	paramValueK = time.getTimeAsMilliSecondsDouble();
    } else {  //otherwise count
	paramValueK = (double) count; 
    }

   string paramHistoryName=name;
   paramHistoryName.append("_LRU_history");
   double paramValueHistory = 0.0;

   if (pad->hasScratchpadAttributeDouble(dObj, paramHistoryName)) {
        //get highest history value
        paramValueHistory = pad->getScratchpadAttributeDouble(dObj, paramHistoryName);
        if (paramValueHistory < k) { //just add a value, increment historyValue
            paramValueHistory += 1.0;
            pad->setScratchpadAttributeDouble(dObj, paramHistoryName, paramValueHistory);
            string entry;
            stringprintf(entry,"K%lu", (unsigned long long) paramValueHistory);
            pad->setScratchpadAttributeDouble(dObj, entry, paramValueK);
        } else {  //have a history of 'k' values, remove oldest, add newest
	    string entry;
            stringprintf(entry,"K%lu", (unsigned long long) paramValueHistory-k+1);
            pad->removeScratchpadAttribute(dObj, entry);
            paramValueHistory += 1.0;
            pad->setScratchpadAttributeDouble(dObj, paramHistoryName, paramValueHistory);

            stringprintf(entry,"K%lu", (unsigned long long) paramValueHistory);
            pad->setScratchpadAttributeDouble(dObj, entry, paramValueK);
        }
        //debug help only
        int start = (int) paramValueHistory -k+1;
        if (start < 1) start = 1;
        string strHistory="", strHistory2;
        string entry;
        for (int i=start; i<start+k; i++) {
            stringprintf(entry,"K%lu", (unsigned long) i);
            if (pad->hasScratchpadAttributeDouble(dObj, entry)) {
                stringprintf(strHistory2,"%f", pad->getScratchpadAttributeDouble(dObj, entry));
		strHistory.append(" : ");
		strHistory.append(strHistory2);
            }
        }
        //debug info 
        HAGGLE_DBG("%s has %d history: %s\n", dObj->getIdStr(), k, strHistory.c_str());
   } else {  //first time
        pad->setScratchpadAttributeDouble(dObj, paramHistoryName, 1.0);
        pad->setScratchpadAttributeDouble(dObj, "K1", paramValueK);
        HAGGLE_DBG("%s LRU_K = %f\n", dObj->getIdStr(), paramValueK);
   }


}

double EvictStrategyLRU_K::getEvictValueForDataObject(DataObjectRef &dObj) 
{  
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();
    string paramHistoryName=name;
    double paramValueHistory;
    paramHistoryName.append("_LRU_history");
    if (pad->hasScratchpadAttributeDouble(dObj, paramHistoryName)) {
        paramValueHistory = pad->getScratchpadAttributeDouble(dObj, paramHistoryName);
        string entry;
        signed long long kentry = paramValueHistory-k+1;
        if (kentry < 1) {
            kentry = 1;
        } 
        stringprintf(entry,"K%lu", (unsigned long long) kentry);
        double value = pad->getScratchpadAttributeDouble(dObj, entry);
        HAGGLE_DBG("%s returned value of %f\n", entry.c_str(), value);
        return value;
    } else {  
        HAGGLE_DBG("No LRU_K HISTORY!\n");
        return 0.0;
    }
}
