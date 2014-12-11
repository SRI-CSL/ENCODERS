/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include "ProtocolFactory.h"

#include "ProtocolUDP.h"
#include "ProtocolTCP.h"
#include "ProtocolUDPUnicast.h"
#include "ProtocolUDPBroadcast.h"

#include "ProtocolClassifierFactory.h"
#include "ProtocolConfigurationFactory.h"
#include "ProtocolConfigurationTCP.h"
#include "ProtocolClassifierAllMatch.h"

ProtocolFactory::ProtocolFactory(ProtocolManager *_pm) :
    pm(_pm),
    config(NULL),
    defaultCfg(new ProtocolConfigurationTCP()),
    classifier(NULL),
    configFactory(NULL)
{
    configs.push_back(defaultCfg);
}

ProtocolClassifier *
ProtocolFactory::getClassifier()
{
    if (!classifier) {
        classifier = new ProtocolClassifier();
        classifier->setDefaultProtocolConfiguration(defaultCfg);
    }
    return classifier;
}

void
ProtocolFactory::setClassifier(ProtocolClassifier *_classifier)
{
    if (!_classifier) {
        HAGGLE_ERR("NULL classifier\n");
        return;
    }

    if (classifier) {
        delete classifier;
    }

    classifier = _classifier;
}

void ProtocolFactory::removeConfiguration(ProtocolConfiguration *cfg)
{
    for (List<ProtocolConfiguration *>::iterator it = configs.begin();
         it != configs.end(); it++) {
        if ((*it) == cfg) {
            configs.erase(it);
        }
    }

    for (server_registry_t::iterator itt = registeredServers.begin();
        itt != registeredServers.end(); itt++) {
        Pair<InterfaceRef, ProtocolConfiguration *> pair = (*itt).first; 
        if (pair.second == cfg) {
            registeredServers.erase(itt);
        }
    }
}

void ProtocolFactory::initialize(
    const Metadata& m)
{
    if (NULL != config) {
        HAGGLE_ERR("onConfig called multiple times\n");
    }

    config = m.copy();

    const Metadata *protocolConfig;
    
    const Metadata* classifierConfig = m.getMetadata("ProtocolClassifier");
    if (classifierConfig) {
        string classifierName = string(classifierConfig->getParameter("name"));
        ProtocolClassifier *newClassifier = 
            ProtocolClassifierFactory::getClassifierForName(
                                                pm,
                                                classifierName);
        if (newClassifier) {
            setClassifier(newClassifier);
            getClassifier()->onConfig(*classifierConfig);
            // NOTE: we cannot clone defaultCfg here since hash will change
            getClassifier()->setDefaultProtocolConfiguration(defaultCfg);
            HAGGLE_DBG("Loaded classifier: %s\n", classifierName.c_str());
        }
    }

    if (!getClassifier()) {
        HAGGLE_DBG("No protocol classifier specified\n");
        return;
    }

    const char *param = NULL;

    int i = 0;
    ProtocolConfiguration *lastCfg = NULL;
    while (NULL != (protocolConfig = m.getMetadata("Protocol", i++))) {
        param = protocolConfig->getParameter("name");
        if (NULL == param) {
            HAGGLE_ERR("No protocol name specified\n");
            return;
        }

        string protocolName = string(param);

        param = protocolConfig->getParameter("inputTag");
        if (NULL == param) {
            HAGGLE_ERR("No input tag specified\n");
            return;
        }

        string protocolTag = string(param);

        const Metadata* configMetadata = protocolConfig->getMetadata(protocolName);
        if (NULL == configMetadata) {
            HAGGLE_ERR("No protocol configuration set: %s\n", protocolName.c_str());
            return;
        }

        ProtocolConfiguration *cfg = getConfigFactory()->configForName(protocolName);

        if (NULL == cfg) {
            HAGGLE_ERR("Invalid configuration.\n");
            return;
        }
        
        lastCfg = cfg;

        cfg->onConfig(*configMetadata);

        configs.push_back(cfg);

        getClassifier()->registerConfigurationForClass(cfg, protocolTag);

        HAGGLE_DBG("Loaded protocol: %s, tag: %s\n", protocolName.c_str(), protocolTag.c_str());
    }

    if (lastCfg) {
        removeConfiguration(defaultCfg);
        getClassifier()->setDefaultProtocolConfiguration(lastCfg);
        if (defaultCfg) {
            delete defaultCfg;
        }
        defaultCfg = lastCfg;
    }

    HAGGLE_DBG("Loaded: %d protocols\n", i-1);
}

ProtocolFactory::~ProtocolFactory() 
{
    if (config) {
        delete config;
    }

    if (classifier) {
        delete classifier;
    }

    if (configFactory) {
        delete configFactory;
    }

    while (!configs.empty()) {
        ProtocolConfiguration *cfg = configs.front();
        configs.pop_front();
        delete cfg;
    }
}

void 
ProtocolFactory::instantiateFromCfg(
    InterfaceRef localIface,
    ProtocolConfiguration *cfg)
{
    if (NULL == cfg) {
        HAGGLE_ERR("Null config\n");
        return;
    }

    Protocol *p = NULL;

    if (registeredServers[make_pair(localIface, cfg)]) {
        HAGGLE_ERR("Server already initialized for iface/cfg pair.\n");
        return;
    }

    if (Protocol::TYPE_TCP == cfg->getType()) {
        p = new ProtocolTCPServer(localIface, pm);
    }

    if (Protocol::TYPE_UDP_UNICAST == cfg->getType()) {
        p = new ProtocolUDPUnicastProxyServer(localIface, pm);
    }

    if (Protocol::TYPE_UDP_BROADCAST == cfg->getType()) {
        p = new ProtocolUDPBroadcastProxyServer(localIface, pm);
    }

    if (NULL == p) {
        HAGGLE_ERR("Could not instantiate server, unknown protocol\n");
        return;
    }

    p->setConfiguration(cfg);

    if (!p->init()) {
        HAGGLE_ERR("Could not initialize or register protocol: %s\n", p->getName());
        delete p;
        return;
    }

    registeredServers[make_pair(localIface, cfg)] = p;

    p->registerWithManager(); // MOS - do not use registerProtocol directly
}

void
ProtocolFactory::instantiateServers(InterfaceRef localIface)
{
    int i = 0;
    for (List<ProtocolConfiguration *>::iterator it = configs.begin();
         it != configs.end(); it++) {
        instantiateFromCfg(localIface, *it);
        i++;
    }

    HAGGLE_DBG("Instantiated %d servers\n", i);
}

Protocol *ProtocolFactory::getSenderProtocol(
        const InterfaceRef& localIface,
        const InterfaceRef& peerIface,
        DataObjectRef& dObj)
{

    ProtocolConfiguration *cfg = getClassifier()->getProtocolConfigurationForDataObject(dObj);

    if (NULL == cfg) {
        HAGGLE_DBG("Could not get configuration for data object\n");
        return NULL;
    }

    Protocol *p = instantiateSenderProtocol(cfg, localIface, peerIface);
    if (NULL == p) {
        HAGGLE_ERR("Could not get sender protocol: %s\n", Protocol::protTypeToStr(cfg->getType()).c_str());
        return NULL;
    }

    return p;
}

Protocol *ProtocolFactory::instantiateSenderProtocol(
        ProtocolConfiguration *cfg, 
        const InterfaceRef& localIface,
        const InterfaceRef& peerIface)
{
    Protocol *p = NULL; 
    
    Protocol *server;

    if (!(server = registeredServers[make_pair(localIface, cfg)])) {
        HAGGLE_ERR("No server registered for iface/cfg pair.\n");
        return NULL;
    }

    if (Protocol::TYPE_TCP == cfg->getType()) {
        ProtocolTCPServer *tcpServer = 
            static_cast<ProtocolTCPServer*>(server);
        p = tcpServer->getSenderProtocol(peerIface);
    }

    if (Protocol::TYPE_UDP_UNICAST == cfg->getType()) { 
        ProtocolUDPUnicastProxyServer *proxyServer = 
            static_cast<ProtocolUDPUnicastProxyServer*>(server);
        p = proxyServer->getSenderProtocol(peerIface);
    }

    if (Protocol::TYPE_UDP_BROADCAST == cfg->getType()) { 
        ProtocolUDPBroadcastProxyServer *proxyServer = 
            static_cast<ProtocolUDPBroadcastProxyServer*>(server);
        p = proxyServer->getSenderProtocol(peerIface);
    }

    return p;
}

ProtocolConfigurationFactory *
ProtocolFactory::getConfigFactory() 
{
    if (NULL == configFactory) {
        configFactory = new ProtocolConfigurationFactory();
    }
    return configFactory;
}
