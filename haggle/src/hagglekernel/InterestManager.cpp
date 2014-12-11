/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include <libcpphaggle/Platform.h>
#include "InterestManager.h"
#include "XMLMetadata.h"

#define PROXY_DOBJ_ID "__proxy_dobj_id"

InterestManager::InterestManager(HaggleKernel *_haggle) :
    Manager("InterestManager", _haggle),
    enabled(false), 
    debug(false),
    consumeInterest(true),
    periodicInterestRefreshEventType(-1),
    periodicInterestRefreshEvent(NULL),
    interestRefreshJitterMs(0),
    interestRefreshPeriodMs(0),
    absoluteTTLMs(0),
    TTL(0),
    prevNodes(0),
    interestDataObjectsCreated(0),
    interestNodesCreated(0),
    interestDataObjectsReceived(0),
    interestNodesReceived(0)
{
}

InterestManager::~InterestManager()
{
    if (periodicInterestRefreshEventType) {
        if (periodicInterestRefreshEvent->isScheduled()) {
            periodicInterestRefreshEvent->setAutoDelete(true);
        }
        else {
            delete periodicInterestRefreshEvent;
        }
    }
}

bool
InterestManager::init_derived()
{
#define __CLASS__ InterestManager
    int ret;

    registerEventTypeForFilter(interestEType, "Interest", onReceiveInterestDataObject, FILTER_INTERESTS);

    ret = setEventHandler(EVENT_TYPE_APP_NODE_CHANGED, onInterestUpdate);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_ROUTE_TO_APP, onForwardDataObject);

    if (ret < 0) {
        HAGGLE_ERR("Could not forward data object\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_DELETED, onDeletedDataObject);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event\n");
        return false;
    }

    periodicInterestRefreshEventType = registerEventType("Periodic Interest Refresh Event\n", onPeriodicInterestRefresh);

    if (periodicInterestRefreshEventType < 0) {
        HAGGLE_ERR("Could not register periodic interest refresh event type\n");
        return false;
    }

    periodicInterestRefreshEvent = new Event(periodicInterestRefreshEventType);

    if (!periodicInterestRefreshEvent) {
        HAGGLE_ERR("Could not register periodic interest refresh\n");
        return false;
    }

    periodicInterestRefreshEvent->setAutoDelete(false);

    return true;
}

/* 
 * Create a new interest data object to be propagated, encoding a list of
 * local application node descriptions.
 */
DataObjectRef
InterestManager::createInterestDataObjectFromApplications(
    NodeRefList &localApplications)
{
    DataObjectRef dObj = DataObject::create();
    Metadata *m = dObj->getMetadata();
    if (!m) {
        HAGGLE_ERR("Could not get metadata\n");
        return NULL;
    }
    Timeval now = Timeval::now();

    if ((prevNodes == 0) && (localApplications.size() <= 0)) {
        HAGGLE_DBG("No local applications to push\n");
        return NULL;
    }

    dObj->addAttribute("Interests", "Aggregate");

    TTL++; // used for total order replacement 
    
    string ttl_str;
    stringprintf(ttl_str, "%d", TTL);
    dObj->addAttribute("SeqNo", ttl_str);

    {
        char *b64str = NULL;
        base64_encode_alloc((const char *)kernel->getThisNode()->getId(), NODE_ID_LEN, &b64str);
        if (!b64str) {
            HAGGLE_ERR("Could not convert node id to metadata\n");
            return NULL;
        }
        dObj->addAttribute("Origin", b64str);
        free(b64str);
    }

    // set the absolute TTL for abs replacement 
    dObj->addAttribute("ATTL_MS", (now + Timeval(0, 1000*absoluteTTLMs)).getAsString());

    int numNodes = 0;

    // encode the application node descriptions
    for (NodeRefList::iterator it = localApplications.begin(); it != localApplications.end(); it++) {
        NodeRef app = (*it);
        if (!app) {
            HAGGLE_ERR("Could not get app node\n");
            continue;
        }
        // we couldn't just copy the xml due to a parsing bug, so we deal w/
        // metadata objects directly
        app.lock();

        string id = Node::idString(app);
        string an = app->getName();
        int maxMatches = app->getMaxDataObjectsInMatch();
        int matchThreshold = app->getMatchingThreshold();
        Timeval createTime = app->getNodeDescriptionCreateTime();
        Metadata *fakeNode = new XMLMetadata("VNode");

        {
            char *b64str = NULL;
            base64_encode_alloc((const char *)app->getId(), NODE_ID_LEN, &b64str);
            if (!b64str) {
                HAGGLE_ERR("Could not convert app node id to metadata\n");
                delete fakeNode;
                return NULL;
            }
            fakeNode->setParameter("id", b64str);
            free(b64str);
        }

        fakeNode->setParameter("an", an);
        fakeNode->setParameter("mm", maxMatches);
        fakeNode->setParameter("mt", matchThreshold);
        string ct_str;
        stringprintf(ct_str, "%ld", (long) createTime.getTimeAsMilliSeconds());
        fakeNode->setParameter("ct", ct_str.c_str());

        const Attributes *attrs = app->getAttributes();
        bool hasAttr = false;
        // add each interest for each local application
        for (Attributes::const_iterator it = attrs->begin(); 
             it != attrs->end(); it++) {
            const Attribute& a = (*it).second;
            Metadata *am = new XMLMetadata("a");
            am->setParameter("n", a.getName() + "=" + a.getValue());
            am->setParameter("w", a.getWeightAsString());
            fakeNode->addMetadata(am);
            hasAttr = true;
        }

        app.unlock();

        if (!hasAttr) {
            continue;
        }

        m->addMetadata(fakeNode);
        numNodes++;

        interestNodesCreated++;
    }

    // keep track of the number of nodes we just sent out
    // to enable removal of subscriptions, but suppress
    // redundant updates
    if (numNodes == 0 && prevNodes == 0) {
        return NULL;
    }

    prevNodes = numNodes;

    interestDataObjectsCreated++;

    return dObj;
}

/*
 * Helper to create a data object from metadata
 */
static
DataObjectRef
createDataObjectFromMetadata(Metadata *m, InterfaceRef localIface = NULL, InterfaceRef remoteIface = NULL)
{
    if (!m) {
        HAGGLE_ERR("Missing metadata");
        return NULL;
    }

    unsigned char *raw;
    size_t raw_len;

    m->getRawAlloc(&raw, &raw_len);
    if (!raw) {
        HAGGLE_ERR("Could not allocate data\n");
        return NULL;
    }

    DataObjectRef dobj = DataObject::create(raw, raw_len, localIface, remoteIface);
    const char *param = m->getParameter(DATAOBJECT_CREATE_TIME_PARAM);
    if (!param) {
        HAGGLE_ERR("Create time param missing\n");
        return NULL;    
    }
    Timeval createTime = Timeval(string(param));
    dobj->setCreateTime(createTime);
    dobj->setUpdateTime(createTime);
    return dobj;
}

/*
 * Helper function crack open a data object, and parse the encoded node
 * description data objects inside the data object's content.
 */
DataObjectRefList
InterestManager::parseReceiveDataObjects(
    DataObjectRef &dObj)
{
    DataObjectRefList parsed;
    
    if (!dObj) {
        HAGGLE_ERR("NULL data object\n");
        return parsed;
    }

    Metadata *dm = dObj->getMetadata();
    if (!dm) {
        HAGGLE_ERR("Could not get metadata\n");
        return parsed;
    }

    int i = 0;

    InterfaceRef remoteIface = dObj->getRemoteInterface();
    InterfaceRef localIface = dObj->getLocalInterface();

    const Attribute *attr = dObj->getAttribute("Origin");
    if (!attr) {
        HAGGLE_ERR("Missing origin ID\n");
        return parsed;
    }

    string parentId = attr->getValue();

    Metadata *m = NULL;

    interestDataObjectsReceived++;

    while ((m = dm->getMetadata("VNode", i++))) {
        const char *param;
        param = m->getParameter("id");
        if (!param) {
            HAGGLE_ERR("Missing id\n");
            continue;
        }
        string id = string(param);

        param = m->getParameter("an");
        if (!param) {
            HAGGLE_ERR("Missing app name\n");
            continue;
        }
        string appname = string(param);

        param = m->getParameter("mm");
        if (!param) {
            HAGGLE_ERR("Missing max matches\n");
            continue;
        }
        
        int maxMatches = 0;
        {
            char *endptr = NULL;
            unsigned long tmp = strtoul(param, &endptr, 10);
            if (!endptr || endptr == param) {
                HAGGLE_ERR("Bad max matcesh\n"); 
                continue;
            }
            maxMatches = tmp;
        }

        param = m->getParameter("mt");
        if (!param) {
            HAGGLE_ERR("Missing match threshold\n");
            continue;
        }
       
        int matchThreshold = 0;
        {
            char *endptr = NULL;
            unsigned long tmp = strtoul(param, &endptr, 10);
            if (!endptr || endptr == param) {
               HAGGLE_ERR("Bad match threshold\n"); 
                continue;
            }
            matchThreshold = tmp;
        }

        param = m->getParameter("ct");
        if (!param) {
            HAGGLE_ERR("Missing creation time\n");
            continue;
        }

        Timeval ct;
        {
            char *endptr = NULL;
            unsigned long tmp = strtoul(param, &endptr, 10);
            if (!endptr || endptr == param) {
                HAGGLE_ERR("Bad timestamp\n"); 
                continue;
            }
            ct = Timeval(0, 1000*tmp);
        }

        int i = 0;
        bool error = false;

        Attributes attrs;

        Metadata *am = NULL;
        while ((am = m->getMetadata("a", i++))) {
            param = am->getParameter("n");
            if (!param) {
                HAGGLE_ERR("Missing attribute\n");
                error = true; 
                break;
            }
            string keyval = string(param);
            string key = keyval.substr(0, keyval.find("="));
            string val = keyval.substr(keyval.find("=")+1);

            int weight = 0;
            param = am->getParameter("w");
            {
                char *endptr = NULL;
                unsigned long tmp = strtoul(param, &endptr, 10);
                if (!endptr || endptr == param) {
                    HAGGLE_ERR("Bad weight\n"); 
                    error = true;
                    break;
                }
                weight = tmp;
            }

            Attribute attr = Attribute(key, val, weight);
            attrs.insert(make_pair(key, attr));
        }

        {
            // add an empty proxy dobj attribute to enable deletions, used
            // during onDeletedDataObject
            Attribute attr = Attribute(PROXY_DOBJ_ID, DataObject::idString(dObj), 0);
            attrs.insert(make_pair(PROXY_DOBJ_ID, attr));
        }

        if (error) {
            HAGGLE_ERR("Problems with attributes\n");
            continue;
        }

        Metadata *dm = createAppNodeMetadata(ct, appname, id, maxMatches, matchThreshold, parentId, attrs);
        if (!dm) {
            HAGGLE_ERR("Could create application node metadata\n");
            continue;
        }

        interestNodesReceived++;

        DataObjectRef ndobj = createDataObjectFromMetadata(dm, localIface, remoteIface);
        if (ndobj) {
            parsed.push_back(ndobj);
        }
        else {
            HAGGLE_ERR("Could not create data object from metadata\n");
        }
    }
    return parsed;
}

/*
 * Open the interest data object and extract the encoded local application
 * node descriptions and raise the proper events.
 */
void 
InterestManager::notifyReceiveDataObjects(
    DataObjectRef &dObj)
{
    DataObjectRefList dobjs = parseReceiveDataObjects(dObj);
    for (DataObjectRefList::iterator it = dobjs.begin(); it != dobjs.end(); it++) {
        DataObjectRef dobj = (*it);
        // SW: NOTE: there may be a security violation here where a rogue node can insert fake node descriptions
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, dobj));
    }
}

void
InterestManager::onPrepareShutdown()
{
    unregisterEventTypeForFilter(interestEType);

    signalIsReadyForShutdown();
}

/*
 * Load the configuration settings. 
 */
void
InterestManager::onConfig(
    Metadata *m)
{
    const char *param = m->getParameter("enable");

    if (param)  {
        if (strcmp(param, "true") == 0) {
            enabled = true;
        }
        else if (strcmp(param, "false") == 0) {
            enabled = false;
        }
        else {
            HAGGLE_ERR("Enabled param must be true or false\n");
            return;
        }
    }
    
    if (!enabled) {
        return;
    }

    param = m->getParameter("debug");

    if (param)  {
        if (strcmp(param, "true") == 0) {
            debug = true;
        }
        else if (strcmp(param, "false") == 0) {
            debug = false;
        }
        else {
            HAGGLE_ERR("debug param must be true or false\n");
            return;
        }
    }

    param = m->getParameter("consume_interest");

    if (param)  {
        if (strcmp(param, "true") == 0) {
            consumeInterest = true;
        }
        else if (strcmp(param, "false") == 0) {
            consumeInterest = false;
        }
        else {
            HAGGLE_ERR("consume_interest param must be true or false\n");
            return;
        }
    }

    if ((param = m->getParameter("interest_refresh_period_ms"))) {
        char *endptr = NULL;
        unsigned long tmp = strtoul(param, &endptr, 10);
        if (endptr && endptr != param) {
            interestRefreshPeriodMs = tmp;
        }
    }

    if ((param = m->getParameter("interest_refresh_jitter_ms"))) {
        char *endptr = NULL;
        unsigned long tmp = strtoul(param, &endptr, 10);
        if (endptr && endptr != param) {
            interestRefreshJitterMs = tmp;
        }
    }

    if ((interestRefreshPeriodMs > 0) && (interestRefreshJitterMs > 0)) {
        // fire off the first event 
        if (!periodicInterestRefreshEvent->isScheduled()) {
            kernel->addEvent(periodicInterestRefreshEvent);
        }
    }

    if ((param = m->getParameter("absolute_ttl_ms"))) {
        char *endptr = NULL;
        unsigned long tmp = strtoul(param, &endptr, 10);
        if (endptr && endptr != param) {
            absoluteTTLMs = tmp;
        }
    }

    param = m->getParameter("run_self_tests");
    bool runSelfTests = false;
    if (param)  {
        if (strcmp(param, "true") == 0) {
            runSelfTests = true;
        }
        else if (strcmp(param, "false") == 0) {
            runSelfTests = false;
        }
        else {
            HAGGLE_ERR("run_self_tests param must be true or false\n");
            return;
        }
    }


    if (runSelfTests) {
        if (selfTests()) {
            HAGGLE_STAT("Summary Statistics - %s - Unit tests PASSED\n", getName());
        }
        else {
            HAGGLE_STAT("Summary Statistics - %s - Unit tests FAILED\n", getName());
        }
    }

    kernel->setFirstClassApplications(true);

    HAGGLE_DBG("Loaded interest manager, run self tests: %s, enabled: %s, debug: %s, interest refresh ms: %d, interest refresh jitter ms: %d, absolute ttl ms: %d, consume interests?: %s\n", runSelfTests ? "yes" : "no", enabled ? "yes" : "no", debug ? "yes" : "no", interestRefreshPeriodMs, interestRefreshJitterMs, absoluteTTLMs, consumeInterest ? "yes" : "no");
}

/*
 * Handle new data objects and notify new local applications if possible.
 */
void
InterestManager::onReceiveInterestDataObject(
    Event *e) 
{
    if (!enabled) {
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring new interest\n");
        return;
    }

    if (!e || !e->hasData()) {
        HAGGLE_ERR("Missing data\n");
        return;
    }

    DataObjectRefList& dObjs = e->getDataObjectList();
    
    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        DataObjectRef dObj = *it;
        if (dObj->isDuplicate()) {
            HAGGLE_DBG("Ignoring duplicate data object (technically this should not happen)\n");
            continue;
        }
        if (!dObj->getRemoteInterface()) {
            HAGGLE_DBG("Data object was created locally, ignoring delete.\n");
            continue;
        }

        notifyReceiveDataObjects(dObj);
    }
}

/*
 * Create a new interest data object if a local application's interest changes.
 */
void
InterestManager::onInterestUpdate(
    Event *e)
{
    if (!enabled) {
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring app update\n");
        return;
    }

    NodeRefList localApplications;
    kernel->getNodeStore()->retrieveLocalApplications(localApplications);
    
    pushObj = createInterestDataObjectFromApplications(localApplications);

    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, pushObj));
}

/*
 * Enable forwarded data objects to "consume" the interest and
 * remove it from the intermediate nodes.
 */
void
InterestManager::onForwardDataObject(
    Event *e)
{
    if (!enabled) {
        return;
    }

    if (!consumeInterest) {
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring forward\n");
        return;
    }

    DataObjectRef dObj = e->getDataObject();
    if (!dObj) {
        HAGGLE_ERR("Missing data object\n");
        return;
    }

    if (dObj->isNodeDescription()) {
        return;
    }

    NodeRef& target = e->getNode();
    if (!target) {
        HAGGLE_ERR("Missing node\n");
        return;
    }

    if (target->getType() != Node::TYPE_APPLICATION) {
        HAGGLE_DBG("Ignoring non-application node match\n");
        return;
    }

    if (target->isLocalApplication()) {
        HAGGLE_DBG("Ignoring local applications\n");
        return;
    }

    if (target->getMaxDataObjectsInMatch() != 1) {
        // ignore requests that have multiple matches
        return;
    }

    NodeRefList apps;
    kernel->getNodeStore()->retrieveApplications(apps);
    bool found = false;
    for (NodeRefList::iterator it = apps.begin(); it != apps.end(); it++) {
        NodeRef app = (*it);
        if (app != target) {
            continue;
        }
        found = true;
        kernel->getNodeStore()->remove(app);
        DataObjectRef node_do = app->getDataObject();
        if (!node_do) {
            continue;
        }
        kernel->getDataStore()->deleteDataObject(node_do);
        kernel->getDataStore()->deleteNode(app);
    }

    if (!found) {
        HAGGLE_ERR("Forward to node that is missing from node store?\n");
    }
}

/*
 * Remove the application nodes and data objects associated with
 * an interest data object.
 */
void
InterestManager::onDeletedDataObject(
    Event *e)
{
    if (!enabled) {
        return;
    }

    if (getState() > MANAGER_STATE_RUNNING) {
        HAGGLE_DBG("In shutdown, ignoring app update\n");
        return;
    }

    if (!e || !e->hasData()) {
        // NOTE: it looks like sometimes on delete is called w/o a dobj?
        HAGGLE_DBG2("Missing data\n");
        return;
    }

    DataObjectRefList dObjs = e->getDataObjectList();
    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {
        DataObjectRef dObj = (*it);
        if (!dObj->getRemoteInterface()) {
            HAGGLE_DBG("Data object was created locally, ignoring delete.\n");
            continue;
        }
        string delete_id = DataObject::idString(dObj);

        NodeRefList apps;
        kernel->getNodeStore()->retrieveApplications(apps);
        for (NodeRefList::iterator it = apps.begin(); it != apps.end(); it++) {
            NodeRef appNode = (*it);
            if (!appNode) {
                HAGGLE_ERR("Missing node\n");
                continue;
            }
            // extract the dobj proxy id and delete the dobj if matches the
            // dobj to be deleted
            const Attribute *do_attr = appNode->getAttribute(PROXY_DOBJ_ID); 
            if (!do_attr) {
                HAGGLE_DBG("Could not get proxy id\n");
                continue;
            }
            if (do_attr->getValue() != delete_id) {
                continue;
            }

            kernel->getNodeStore()->remove(appNode);
            DataObjectRef node_do = appNode->getDataObject();
            if (!node_do) {
                continue;
            }
            kernel->getDataStore()->deleteDataObject(node_do);
        }
    }
}

/*
 * Periodically refresh interests.
 */
void
InterestManager::onPeriodicInterestRefresh(
    Event *e)
{
    if (!enabled) {
        return;
    }
      
    NodeRefList localApplications;
    kernel->getNodeStore()->retrieveLocalApplications(localApplications);
    pushObj = createInterestDataObjectFromApplications(localApplications);

    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, pushObj));

    double jitter = rand() % interestRefreshJitterMs;
    // timeout uses seconds
    double nextTimeout = (interestRefreshPeriodMs + jitter) / (double) 1000;

    if (!periodicInterestRefreshEvent->isScheduled()
        && (nextTimeout > 0)) {
        // schedule next refresh event 
        periodicInterestRefreshEvent->setTimeout(nextTimeout);
        kernel->addEvent(periodicInterestRefreshEvent);
    }
}

/*
 * Print out stats during shutdown for easy debugging & logging.
 */
void
InterestManager::onShutdown()
{
     HAGGLE_STAT("Summary Statistics - %s - Interest Data Objects Created: %ld\n", getName(), interestDataObjectsCreated);
     HAGGLE_STAT("Summary Statistics - %s - Interest Nodes Created: %ld\n", getName(), interestNodesCreated);
     HAGGLE_STAT("Summary Statistics - %s - Interest Data Objects Received: %ld\n", getName(), interestDataObjectsReceived);
     HAGGLE_STAT("Summary Statistics - %s - Interest Nodes Received: %ld\n", getName(), interestNodesReceived);

    if (debug) {
        NodeRefList nodes;
        kernel->getNodeStore()->retrieveAllNodes(nodes);
        for (NodeRefList::iterator it = nodes.begin(); it != nodes.end(); it++) {
            NodeRef n = (*it);
            string name_str = n->getName();
            HAGGLE_STAT("Summary Statistics - %s - Node Entry: %s\n", getName(), name_str.c_str());
        }
    }

    unregisterWithKernel();
}

/*
 * Helper function for unit tests
 */
static bool nodeCompare(NodeRef a, NodeRef b) 
{
    if (!a || !b) {
        HAGGLE_ERR("Nodes are NULL\n");
        return false;
    }

    if (Node::idString(a) != Node::idString(b)) {
        HAGGLE_ERR("ID mismatch\n");
        return false;
    }

    if (Node::nameString(a) != Node::nameString(b)) {
        HAGGLE_ERR("Name mismatch\n");
        return false;
    }

    if (a->getType() != b->getType()) {
        HAGGLE_ERR("Type mistmatch\n");
        return false;
    }

    if (a->getMaxDataObjectsInMatch() != b->getMaxDataObjectsInMatch()) {
        HAGGLE_ERR("Max data objects in match mismatch\n");
        return false;
    }

    if (a->getMatchingThreshold() != b->getMatchingThreshold()) {
        HAGGLE_ERR("Matching threshold mismatch\n");
        return false;
    }

    if (a->getNodeDescriptionCreateTime() != b->getNodeDescriptionCreateTime()) {
        HAGGLE_ERR("Create time mismatch %ld != %ld\n", a->getNodeDescriptionCreateTime().getTimeAsMilliSeconds(), b->getNodeDescriptionCreateTime().getTimeAsMilliSeconds());
        return false;
    }

    if (a->getNodeDescriptionUpdateTime() != b->getNodeDescriptionUpdateTime()) {
        HAGGLE_ERR("Update time mismatch %ld != %ld\n", a->getNodeDescriptionUpdateTime().getTimeAsMilliSeconds(), b->getNodeDescriptionUpdateTime().getTimeAsMilliSeconds());
        return false;
    }

    const Attributes *a_attrs = a->getAttributes();
    if (!a_attrs) {
        HAGGLE_ERR("Could not get attributes for node a\n");
        return false;
    }

    const Attributes *b_attrs = b->getAttributes();
    if (!b_attrs) {
        HAGGLE_ERR("Could not get attributes for node b\n");
        return false;
    }

    Attributes::const_iterator itt = b_attrs->begin();
    for (Attributes::const_iterator it = a_attrs->begin(); it != a_attrs->end() && itt != b_attrs->end(); it++, itt++) {
        Attribute attr_a = (*it).second;
        Attribute attr_b = (*itt).second;
        if (attr_a.getName() == string(PROXY_DOBJ_ID)) {
            continue; 
        }
        if (attr_b.getName() == string(PROXY_DOBJ_ID)) {
            continue;
        }
    }

    return true;
}

Metadata *
InterestManager::createAppNodeMetadata(
    Timeval ct,
    string appname,
    string id,
    int maxMatches,
    int matchThreshold,
    string parentId,
    Attributes attrs)
{
    Metadata *m = new XMLMetadata("Haggle");
    if (!m) {
        HAGGLE_ERR("Could not allocate xml metadata\n");
        return NULL;
    }
    m->setParameter(DATAOBJECT_PERSISTENT_PARAM, "yes");
    m->setParameter(DATAOBJECT_CREATE_TIME_PARAM, ct.getAsString());
    m->setParameter(DATAOBJECT_UPDATE_TIME_PARAM, ct.getAsString());

    Metadata *nm = new XMLMetadata(NODE_METADATA);
    if (!nm) {
        HAGGLE_ERR("Could not allocate node metadata\n");
        delete m;
        return NULL;
    }
    nm->setParameter("type", "application");
    nm->setParameter(NODE_METADATA_NAME_PARAM, appname);
    nm->setParameter(NODE_METADATA_ID_PARAM, id);
    nm->setParameter(NODE_METADATA_MAX_DATAOBJECTS_PARAM, maxMatches);
    nm->setParameter(NODE_METADATA_THRESHOLD_PARAM, matchThreshold);
    nm->setParameter(NODE_METADATA_PROXY_ID_PARAM, parentId);

    m->addMetadata(nm);

    for (Attributes::iterator it = attrs.begin(); it != attrs.end(); it++) {
        Attribute a = (*it).second;
        Metadata *nattr = new XMLMetadata(DATAOBJECT_ATTRIBUTE_NAME);
        if (!nattr) {
            HAGGLE_ERR("Could not allocate attribute metadata\n");
            delete m;
            delete nm;
            return NULL;
        }
        nattr->setParameter(DATAOBJECT_ATTRIBUTE_NAME_PARAM, a.getName());
        nattr->setParameter(DATAOBJECT_ATTRIBUTE_WEIGHT_PARAM, a.getWeight());
        nattr->setContent(a.getValue());
        m->addMetadata(nattr);
    }

    return m;
}

bool
InterestManager::selfTest1()
{
    NodeRefList appsBefore;
    {
        Timeval ct = Timeval(0, 1000*(long) Timeval::now().getTimeAsMilliSeconds());
        string appname = "app1";
        string id = "123";
        int maxMatches = 1;
        int matchThreshold = 100;
        string parentId = "100";
        Attributes attrs;

        Attribute attr = Attribute("key", "val", 0);
        attrs.add(attr);

        Metadata *dm = createAppNodeMetadata(ct, appname, id, maxMatches, matchThreshold, parentId, attrs);
        if (!dm) {
            HAGGLE_ERR("Could not create app node metadata\n");
            return false;
        }

        DataObjectRef dobj = createDataObjectFromMetadata(dm);
        if (!dobj) {
            HAGGLE_ERR("Could not create data object from metadata\n");
            return false;
        }

        NodeRef appNode = Node::create(dobj);
        if (!appNode) {
            HAGGLE_ERR("Could not create app node\n");
            return false;
        }

        appsBefore.push_back(appNode);
    }

    DataObjectRef dobj = createInterestDataObjectFromApplications(appsBefore);
    if (!dobj) {
        HAGGLE_ERR("Could not create interest data object\n");
        return false;
    }
    DataObjectRefList appsDOAfter = parseReceiveDataObjects(dobj);

    NodeRefList appsAfter;
    for (DataObjectRefList::iterator it = appsDOAfter.begin(); it != appsDOAfter.end(); it++) {
        NodeRef newNode = Node::create(*it);
        if (!newNode) {
            HAGGLE_ERR("Could not create node from dobj\n");
            return false;
        }
        appsAfter.push_back(newNode);
    }

    if (appsBefore.size() != appsAfter.size()) {
        HAGGLE_ERR("Sizes don't match\n");
        return false;
    }

    NodeRefList::iterator itt = appsAfter.begin();
    for (NodeRefList::iterator it = appsBefore.begin(); (it != appsBefore.end()) && (itt != appsAfter.end()); it++, itt++) {
        if (!nodeCompare(*itt, *it)) {
            HAGGLE_ERR("Nodes are not equivalent\n");
            return false;
        }
    }
    
    return true;
}

bool
InterestManager::selfTests()
{
    bool passed = true;
    if (!selfTest1()) {
        passed = false;
    }
    return passed;
}
