/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _CACHE_STRAT_ASYNC_H
#define _CACHE_STRAT_ASYNC_H

class CacheStrategyTask;
class CacheStrategyAsynchronous;

typedef enum {
    CACHE_STRAT_NEW_DATAOBJECT,
    CACHE_STRAT_SEND_SUCCESS,
    CACHE_STRAT_DELETED_DATAOBJECT,
    CACHE_STRAT_PERIODIC,
    CACHE_STRAT_CONFIG,
    CACHE_STRAT_PRINT_DEBUG,
    CACHE_STRAT_GET_AS_METADATA, // CBMEN, HL
    CACHE_STRAT_QUIT
} CacheStrategyTaskType_t; 

#include "ManagerModule.h"
#include "DataManager.h"
#include "Metadata.h"

class CacheStrategyTask {
private:
    const CacheStrategyTaskType_t type;
    DataObjectRef dObj;
    NodeRef node;
    Metadata *m;
public:
    CacheStrategyTask(
        const CacheStrategyTaskType_t _type,
        const DataObjectRef &_dObj = NULL,
        const NodeRef &_node = NULL) :
            type(_type),
            dObj(_dObj),
            node(_node), 
            m(NULL) {}

    // CBMEN, HL, Begin
    CacheStrategyTask(
        const CacheStrategyTaskType_t _type,
        Metadata *_m) :
            type(_type),
            dObj(NULL),
            node(NULL),
            m(_m) {}
    // CBMEN, HL, End

     CacheStrategyTask(
        const Metadata &_m) :
            type(CACHE_STRAT_CONFIG),
            dObj(NULL),
            node(NULL), 
            m(_m.copy()) {} 

    DataObjectRef &getDataObject() {
        return dObj;
    }

    NodeRef &getNode() {
        return node;
    }

    void setDataObject(DataObjectRef &_dObj) {
        dObj = _dObj;
    }

    CacheStrategyTaskType_t getType() {
        return type;
    }

    Metadata *getConfig() {
        return m;
    }

    // CBMEN, HL, Begin
    Metadata *getMetadata() {
        return m;
    }
    // CBMEN, HL, End

    ~CacheStrategyTask() {
        if (m) {
            delete m;
        }
    }
};

class CacheStrategyAsynchronous : public CacheStrategy {
private: 
    bool _isDone;
    bool isRegistered;
protected:
    HaggleKernel *kernel;
    GenericQueue<CacheStrategyTask *> *taskQ;	

    bool run(void);
    virtual void _handlePeriodic() {};
    virtual void _handleNewDataObject(DataObjectRef &dObj) {};
    virtual void _handleSendSuccess(DataObjectRef &dObj, NodeRef &node) {};
    virtual void _handleDeletedDataObject(DataObjectRef &dObj) {};
    virtual void _handlePrintDebug();
    virtual void _handleGetCacheStrategyAsMetadata(Metadata *m); // CBMEN, HL
    virtual void _onConfig(const Metadata& m) {};
public:
    CacheStrategyAsynchronous(
        DataManager *m = NULL,
        const string name = "Asynchronous cache strategy module",
        GenericQueue<CacheStrategyTask *> *taskQ 
            = new GenericQueue<CacheStrategyTask *>());

    ~CacheStrategyAsynchronous();

    void onConfig(const Metadata& m);

    bool isResponsibleForDataObject(DataObjectRef &dObj);

    virtual void printDebug();

    virtual void getCacheStrategyAsMetadata(Metadata *m); // CBMEN, HL

    virtual void firePeriodic();

    virtual void handleNewDataObject(DataObjectRef &dObj);

    virtual void handleSendSuccess(DataObjectRef &dObj, NodeRef &node);

    virtual void handleDeletedDataObject(DataObjectRef &dObj);

    GenericQueue<CacheStrategyTask *> *getTaskQueue() {
        return taskQ;
    }
    
    void cleanup();
    void unregisterWithManager();

    virtual void quitHook() {};

    virtual bool isDone() { return _isDone; }
};

#endif /* _CACHE_STRAT_ASYNC_H */
