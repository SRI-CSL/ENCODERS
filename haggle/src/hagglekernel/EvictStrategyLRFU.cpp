/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "EvictStrategyLRFU.h"

void EvictStrategyLRFU::onConfig(const Metadata* m)
{
    const char *param; 
    param = m->getParameter("name");
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

    if (countType != EVICT_STRAT_LRFU_COUNT_TYPE_TIME && 
        countType != EVICT_STRAT_LRFU_COUNT_TYPE_VIRTUAL) {
        HAGGLE_ERR("Unknown count type: %s.\n", countType.c_str());
        return;
    }

    param = m->getParameter("pValue");
    if (!param) {
        HAGGLE_ERR("No p-value specified.\n");
        return;
    }
    pValue = atof(param);

    if (pValue <= 0) {
        HAGGLE_ERR("Bad pvalue: %.5f\n", pValue);
        return;
    }

    param = m->getParameter("lambda");
    if (!param) {
        HAGGLE_ERR("No lambda specified.\n");
        return;
    }
    lambda = atof(param);

    if (lambda <= 0 || lambda > 1) {
        HAGGLE_ERR("Bad lambda: %.5f\n", lambda);
        return;
    }
}

void EvictStrategyLRFU::updateInfoDataObject(DataObjectRef &dObj, unsigned int count, Timeval time) 
{

    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();
    //create scratchpad keys
    string paramNameF0=name;
    paramNameF0.append("_LRU_F0");
    double c_now = 0.0;
    double c_old = 0.0;
    double lastTime = 0.0;
    double paramValueF0 = 0.0;
    double paramValueLastk = 0.0;
    string paramName_c_now=name;
    paramName_c_now.append("_c_now");
    string paramNameLastk=name;
    paramNameLastk.append("_LRU_last_k");
    
    if (countType == EVICT_STRAT_LRFU_COUNT_TYPE_TIME) {
	paramValueLastk = time.getTimeAsMilliSecondsDouble();
    } else {  //otherwise count
	paramValueLastk = (double) count; 
    }

    bool has_attr = pad->hasScratchpadAttributeDouble(dObj, paramNameF0);
    if (has_attr) {
        paramValueF0 = pad->getScratchpadAttributeDouble(dObj, paramNameF0);
	c_now = pad->getScratchpadAttributeDouble(dObj, paramName_c_now);
	c_old = c_now;
       	lastTime = pad->getScratchpadAttributeDouble(dObj, paramNameLastk);
        c_now *= fx_calc(paramValueLastk-lastTime);
        c_now += paramValueF0; 
       	HAGGLE_DBG("%s f(x) = %f + G(%f - %f)*%f = %f\n", dObj->getIdStr() ,paramValueF0 , (float) count, lastTime, c_old,  c_now);

    } else {
	c_now = 1.0; //fx_calc(paramValueLastk);
       	pad->setScratchpadAttributeDouble(dObj, paramNameF0, c_now);
       	HAGGLE_DBG("%s f(0) = g(%f) = %f\n", dObj->getIdStr(), paramValueLastk, c_now);
    }

   //set current values
   pad->setScratchpadAttributeDouble(dObj, paramName_c_now, c_now);
   pad->setScratchpadAttributeDouble(dObj, paramNameLastk, paramValueLastk);
   

}

double EvictStrategyLRFU::getEvictValueForDataObject(DataObjectRef &dObj) 
{
    ScratchpadManager *pad = getKernel()->getDataStore()->getScratchpadManager();
   string paramName_c_now = name;
   paramName_c_now.append("_c_now");
   if (pad->hasScratchpadAttributeDouble(dObj, paramName_c_now)) {
      return pad->getScratchpadAttributeDouble(dObj, paramName_c_now);
   } else {
      return -1.0;
   }
}

double
EvictStrategyLRFU::fx_calc(double value) 
{
  double retVal = pow((1.0/pValue), (lambda * value));
  //return 1.0-retVal; //this makes earlier values small, later values larger
  return retVal; //this makes earlier values small, later values larger

}
