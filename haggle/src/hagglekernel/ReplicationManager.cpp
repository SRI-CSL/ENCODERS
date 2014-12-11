/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ReplicationManager.h"

#include "Event.h"
#include "ReplicationManagerFactory.h"

bool
ReplicationManager::init_derived()
{
#define __CLASS__ ReplicationManager

    int ret;
    ret = setEventHandler(EVENT_TYPE_NODE_UPDATED, onNodeUpdated);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_NEW, onNodeContactNew);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }


    ret = setEventHandler(EVENT_TYPE_NODE_CONTACT_END, onNodeContactEnd);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND, onSend);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onSendSuccessful);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, onSendFailure);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }


    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_DELETED, onDeletedDataObject);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_NEW, onInsertedDataObject);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_REPL_MANAGER, onReplicationManagerEvent);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    HAGGLE_DBG("Starting data helper...\n");
    return true;
}

void
ReplicationManager::onDeletedDataObject(Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring send data object fail results\n");
        return;
    }

    if (!e || !e->hasData()) {
        return;
    }

    DataObjectRefList dObjs = e->getDataObjectList();
    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        getModule()->notifyDeleteDataObject((*it));
    }
}

void
ReplicationManager::onInsertedDataObject(Event *e) 
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring send data object fail results\n");
        return;
    }

    if (!e || !e->hasData()) {
        return;
    }

    DataObjectRef& dObj = e->getDataObject();
    getModule()->notifyInsertDataObject(dObj);
}

void
ReplicationManager::onNodeContactEnd(Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring new node contact\n");
        return;
    }
	
    if (!e) {
        return;
    }

    NodeRef node = e->getNode();

    if (!node) {
        HAGGLE_ERR("Node was missing on contact end\n");
        return;
    }

    getModule()->notifyDownContact(node);
}

void
ReplicationManager::onNodeUpdated(Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring node contact end\n");
        return;
    }
	
    if (!e) {
        return;
    }

    NodeRef node = e->getNode();

    if (node && node->getType() != Node::TYPE_UNDEFINED) {
        getModule()->notifyUpdatedContact(node);
    }
}

void
ReplicationManager::onNodeContactNew(Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring node contact end\n");
        return;
    }
	
    if (!e) {
        return;
    }

    NodeRef node = e->getNode();

    if (node && node->getType() != Node::TYPE_UNDEFINED) {
        getModule()->notifyNewContact(node);
    }
}

void
ReplicationManager::onSendSuccessful(Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring send data object results\n");
        return;
    }

    if (!e) {
        return;
    }

    NodeRef neigh = e->getNode();

    if (!neigh) {
        return;
    }

    DataObjectRef &dObj = e->getDataObject();
    getModule()->notifySendSuccess(dObj, neigh, e->getFlags());   
}

void
ReplicationManager::onSend(Event *e)
{
 if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring send data object results\n");
        return;
    }

    if (!e) {
        return;
    }

    NodeRefList nl = e->getNodeList();

    DataObjectRef &dObj = e->getDataObject();

    getModule()->notifySendDataObject(dObj, nl);

}

void
ReplicationManager::onSendFailure(Event *e)
{
    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring send data object fail results\n");
        return;
    }

    if (!e) {
        return;
    }

    NodeRef neigh = e->getNode();

    if (!neigh) {
        return;
    }

    DataObjectRef &dObj = e->getDataObject();

    getModule()->notifySendFailure(dObj, neigh);
}

void
ReplicationManager::onConfig(Metadata *m)
{
    if (!m) {
	HAGGLE_DBG("No configuration for replication manager\n");
        return;
    }
    string param = m->getParameter("name");
    HAGGLE_DBG("Loading ReplicationManager with parameter: %s\n", param.c_str());
    setModule(ReplicationManagerFactory::getNewReplicationManagerModule(this, param));
    const Metadata *dm = m->getMetadata(param);
    if (!dm) {
	HAGGLE_DBG("No valid replication manager module found!\n");
        return;
    }
    getModule()->onConfig(*dm);
    getModule()->start();
}

void 
ReplicationManager::onReplicationManagerEvent(Event *e) {

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring send data object fail results\n");
        return;
    }

    if (!e) {
        HAGGLE_ERR("No event\n");
        return;
    }
    if (e->getType() != EVENT_TYPE_REPL_MANAGER) {
        HAGGLE_ERR("Not rate manager event\n");
        return;
    }
    if (!e->getData()) {
        HAGGLE_ERR("Missing rate data\n");
        return;
    }
    getModule()->handleReplicationManagerEvent(e);
}

void 
ReplicationManager::onPrepareShutdown()
{
    getModule()->quit();
    getModule()->join();
    signalIsReadyForShutdown();
}

void 
ReplicationManager::setModule(ReplicationManagerModule *m)
{
    if (!m) {
        return;
    }

    if (module) {
        module->shutdown();
        module->join();
        delete module;
    }

    module = m;
}


ReplicationManagerModule *
ReplicationManager::getModule()
{
    if (!module) {
        module = new ReplicationManagerModule(this);
    }
    return module;
}

void
ReplicationManager::onShutdown()
{
    HAGGLE_DBG("Shutting down network rate manager\n");
    this->unregisterWithKernel();
}

ReplicationManager::~ReplicationManager() 
{
    if (module) {
        delete module;
    }
}
