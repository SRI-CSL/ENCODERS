/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ReplicationManagerAsynchronous.h"

void
ReplicationManagerAsynchronous::cleanup()
{
    if (isRunning()) {
        stop(); 
    }
    if (taskQ) {
        taskQ->close();
        while (!taskQ->empty()) {
            ReplicationManagerTask *qe = NULL;
            taskQ->retrieveTry(&qe);
            if (qe) {
                delete qe;
            }
        }
    }

    onExit();
}

ReplicationManagerAsynchronous::~ReplicationManagerAsynchronous() 
{
    if (taskQ) {
        delete taskQ;
    }
}

void
ReplicationManagerAsynchronous::onConfig(const Metadata& m)
{
    Mutex::AutoLocker l(mutex);

    ReplicationManagerTask *task = new ReplicationManagerTask(m);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyNewContact(NodeRef n)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_NEW_CONTACT, n);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyUpdatedContact(NodeRef n)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_UPDATED_CONTACT, n);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyDownContact(NodeRef n)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_DOWN_CONTACT, n);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyNewNodeDescription(NodeRef n)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_NEW_NODE_DESCRIPTION, n);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifySendDataObject(DataObjectRef dObj, NodeRefList nodeList)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(nodeList, dObj);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifySendSuccess(DataObjectRef dObj, NodeRef n, unsigned long flags)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_SEND_SUCCESS, dObj, n, flags);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifySendFailure(DataObjectRef dObj, NodeRef n)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_SEND_FAILURE, dObj, n);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyInsertDataObject(DataObjectRef dObj)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_INSERT_DATAOBJECT, dObj);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyDeleteDataObject(DataObjectRef dObj)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_DELETE_DATAOBJECT, dObj);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifySendStats(NodeRef node, DataObjectRef dObj, long send_delay, long send_bytes)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }
    ReplicationManagerTask *task = new ReplicationManagerTask(node, dObj, send_delay, send_bytes);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::notifyNodeStats(NodeRef node, Timeval duration)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }
    ReplicationManagerTask *task = new ReplicationManagerTask(node, duration);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::sendNodeDescription(DataObjectRef dObj, NodeRefList n)
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }
    ReplicationManagerTask *task = new ReplicationManagerTask(dObj, n);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::replicateAll()
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }
    ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_REPLICATE_ALL);
    taskQ->insert(task);
}

void
ReplicationManagerAsynchronous::firePeriodic()
{
    Mutex::AutoLocker l(mutex);

    if (getManager()->getKernel()->isShuttingDown()) {
        return;
    }

    ReplicationManagerTask *task = new ReplicationManagerTask();
    taskQ->insert(task);
}

void 
ReplicationManagerAsynchronous::quit()
{
    if (isRunning()) {
        ReplicationManagerTask *task = new ReplicationManagerTask(REPL_MANAGER_QUIT);
        taskQ->insert(task);
        taskQ->close();
        join();
    }
}

bool
ReplicationManagerAsynchronous::run()
{
    while (!shouldExit() && !_isDone) {
        ReplicationManagerTask *task = NULL;
        switch (taskQ->retrieve(&task)) {
        case QUEUE_TIMEOUT:
        {
            HAGGLE_DBG("WARNING: timeout occurred in cache strategy task queue.\n");
            break;
        }
        case QUEUE_ELEMENT:
        {
            switch (task->getType()) {
            case REPL_MANAGER_CONFIG:
            {
                _onConfig(*task->getConfig());
                break;
            }
            case REPL_MANAGER_QUIT:
            {
                _isDone = true;
                break;
            }
            case REPL_MANAGER_NEW_CONTACT:
            {
                _notifyNewContact(task->getNode());
                break;
            }
            case REPL_MANAGER_UPDATED_CONTACT:
            {
                _notifyUpdatedContact(task->getNode());
                break;
            }
            case REPL_MANAGER_DOWN_CONTACT:
            {
                _notifyDownContact(task->getNode());
                break;
            }
            case REPL_MANAGER_NEW_NODE_DESCRIPTION:
            {
                _notifyNewNodeDescription(task->getNode());
                break;
            }
            case REPL_MANAGER_SEND:
            {
                _notifySendDataObject(task->getDataObject(), task->getNodeList());
                break;
            }
            case REPL_MANAGER_SEND_SUCCESS:
            {
                _notifySendSuccess(task->getDataObject(), task->getNode(), task->getFlags());
                break;
            }
            case REPL_MANAGER_SEND_FAILURE:
            {
                _notifySendFailure(task->getDataObject(), task->getNode());
                break;
            }
            case REPL_MANAGER_INSERT_DATAOBJECT:
            {
                _notifyInsertDataObject(task->getDataObject());
                break;
            }
            case REPL_MANAGER_DELETE_DATAOBJECT:
            {
                _notifyDeleteDataObject(task->getDataObject());
                break;
            }
            case REPL_MANAGER_SEND_STATS:
            {
                _notifySendStats(task->getNode(), task->getDataObject(), task->getSendDelay(), task->getSendBytes());
                break;
            }
            case REPL_MANAGER_NODE_STATS:
            {
                _notifyNodeStats(task->getNode(), task->getDuration());
                break;
            }
            case REPL_MANAGER_SEND_NODE_DESCRIPTION:
            {
                _sendNodeDescription(task->getDataObject(), task->getNodeList());
                break;
            }
            case REPL_MANAGER_PERIODIC:
            {
                _handlePeriodic();
                break;
            }
            case REPL_MANAGER_REPLICATE_ALL:
            {
                _replicateAll();
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
    HAGGLE_DBG("Replication manager EXITS!\n");
    return false;
}
