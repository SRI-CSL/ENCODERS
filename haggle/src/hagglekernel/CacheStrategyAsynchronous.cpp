/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Hasnain Lakhani (HL)
 */

#include "CacheStrategyAsynchronous.h"

CacheStrategyAsynchronous::CacheStrategyAsynchronous(
    DataManager *m,
    const string name,
    GenericQueue<CacheStrategyTask *> *_taskQ) :
        CacheStrategy(m, name),
        _isDone(false),
        isRegistered(true),
        kernel(getManager()->getKernel()),
        taskQ(_taskQ)
{
}

void
CacheStrategyAsynchronous::cleanup()
{
    if (isRunning()) {
        stop(); 
    }
    if (taskQ) {
        taskQ->close();
        while (!taskQ->empty()) {
            CacheStrategyTask *qe = NULL;
            taskQ->retrieveTry(&qe);
            if (qe) {
                delete qe;
            }
        }
    }
    quitHook();
}
     
CacheStrategyAsynchronous::~CacheStrategyAsynchronous() 
{
    if (taskQ) {
        delete taskQ;
    }
}

void CacheStrategyAsynchronous::onConfig(const Metadata& m)
{
    CacheStrategyTask *task = new CacheStrategyTask(m);
    taskQ->insert(task);
}

bool CacheStrategyAsynchronous::isResponsibleForDataObject(DataObjectRef &dObj)
{
    return false;
}

void CacheStrategyAsynchronous::printDebug()
{
    if (getKernel()->isShuttingDown()) {
        return;
    }

    CacheStrategyTask *task =
        new CacheStrategyTask(CACHE_STRAT_PRINT_DEBUG);

    taskQ->insert(task);
}

// CBMEN, HL, Begin
void CacheStrategyAsynchronous::getCacheStrategyAsMetadata(Metadata *m)
{
    if (getKernel()->isShuttingDown()) {
        return;
    }

    CacheStrategyTask *task =
        new CacheStrategyTask(CACHE_STRAT_GET_AS_METADATA, m);

    taskQ->insert(task);
}
// CBMEN, HL, End

void CacheStrategyAsynchronous::firePeriodic()
{
    if (getKernel()->isShuttingDown()) {
        return;
    }

    CacheStrategyTask *task =
        new CacheStrategyTask(CACHE_STRAT_PERIODIC);

    taskQ->insert(task);
}

void CacheStrategyAsynchronous::handleNewDataObject(DataObjectRef &dObj)
{
    if (getKernel()->isShuttingDown()) {
        return;
    }

    if (!dObj) {
        return;
    }

    CacheStrategyTask *task =
        new CacheStrategyTask(CACHE_STRAT_NEW_DATAOBJECT, dObj);

    taskQ->insert(task);
}

void CacheStrategyAsynchronous::handleSendSuccess(DataObjectRef &dObj, NodeRef &node)
{
    if (getKernel()->isShuttingDown()) {
        return;
    }

    if (!dObj || !node) {
        return;
    }

    CacheStrategyTask *task =
        new CacheStrategyTask(CACHE_STRAT_SEND_SUCCESS, dObj, node);

    taskQ->insert(task);
}

void CacheStrategyAsynchronous::handleDeletedDataObject(DataObjectRef &dObj)
{
    if (getKernel()->isShuttingDown()) {
        return;
    }

    if (!dObj) {
        return;
    }

    CacheStrategyTask *task =
        new CacheStrategyTask(CACHE_STRAT_DELETED_DATAOBJECT, dObj);

    taskQ->insert(task);
}

bool
CacheStrategyAsynchronous::run()
{
    while (!shouldExit() && !_isDone) {
        CacheStrategyTask *task = NULL;
        switch (taskQ->retrieve(&task)) {
        case QUEUE_TIMEOUT:
        {
            HAGGLE_DBG("WARNING: timeout occurred in cache strategy task queue.\n");
            break;
        }
        case QUEUE_ELEMENT:
        {
            switch (task->getType()) {
            case CACHE_STRAT_NEW_DATAOBJECT:
            {
                _handleNewDataObject(task->getDataObject());
                break;
            }
            case CACHE_STRAT_SEND_SUCCESS:
            {
                _handleSendSuccess(task->getDataObject(), task->getNode());
                break;
            }
            case CACHE_STRAT_DELETED_DATAOBJECT:
            {
                _handleDeletedDataObject(task->getDataObject());
                break;
            }
            case CACHE_STRAT_PERIODIC:
            {
                _handlePeriodic();
                break;
            }
            case CACHE_STRAT_PRINT_DEBUG:
            {
                _handlePrintDebug();
                break;
            }
            // CBMEN, HL, Begin
            case CACHE_STRAT_GET_AS_METADATA:
            {
                _handleGetCacheStrategyAsMetadata(task->getMetadata());
                break;
            }
            // CBMEN, HL, End
            case CACHE_STRAT_CONFIG:
            {
                _onConfig(*task->getConfig());
                break;
            }
            case CACHE_STRAT_QUIT:
            {
                _isDone = true;
                break;
            }
            default:
            {
                HAGGLE_ERR("Uknown task type\n");
                break;
            }
            }
            break;
        }
        case QUEUE_WATCH_ABANDONED:
        {
            _isDone = true;
            break;
        }
        default:
        {
            HAGGLE_ERR("Unknown queue type\n");
            _isDone = true;
            break;
        }
        }

        if (task) {
            delete task;
        }
    }
    HAGGLE_DBG("Cache strategy EXITS!\n");
    return false;
}

void
CacheStrategyAsynchronous::_handlePrintDebug()
{
    printf("CacheStrategy print debug unimplemented.\n");
}

// CBMEN, HL, Begin
void 
CacheStrategyAsynchronous::_handleGetCacheStrategyAsMetadata(Metadata *m)
{
    return;
}
// CBMEN, HL, End
