/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#ifndef _EVICT_STRAT_H
#define _EVICT_STRAT_H

class EvictStrategy;

#include "DataManager.h"
#include "DataObject.h"
#include "Metadata.h"


class EvictStrategy { 
protected:
    HaggleKernel *kernel;
    string className;
    string name;
public:
    EvictStrategy(HaggleKernel *_kernel, string _className) :
        kernel(_kernel),
        className(_className) {};
    virtual ~EvictStrategy() {};
    virtual void onConfig(const Metadata *m) { return; };
    virtual void updateInfoDataObject(DataObjectRef &dObj, unsigned int count, Timeval time) { return; } ;
    virtual string getClass() { return className; }; 
    virtual string getName() { return name; } 
    virtual double getEvictValueForDataObject(DataObjectRef &dObj) { return 0.0; }
    HaggleKernel *getKernel() { return kernel; }
};

#endif /* _EVICT_STRAT_H */
