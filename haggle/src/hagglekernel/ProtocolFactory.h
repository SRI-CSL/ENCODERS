/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _PROTOCOLFACTORY_H
#define _PROTOCOLFACTORY_H

class ProtocolFactory;

#include "Protocol.h"
#include "ManagerModule.h"
#include "ProtocolManager.h"
#include "ProtocolClassifier.h"
#include "ProtocolConfigurationFactory.h"

class ProtocolFactory {

private:
    ProtocolManager *pm;
    Metadata *config; 
    
    ProtocolConfiguration *defaultCfg;
    ProtocolClassifier *classifier;
    ProtocolConfigurationFactory *configFactory;

    void instantiateFromCfg(InterfaceRef localIface, ProtocolConfiguration *cfg);

    Protocol *instantiateSenderProtocol(
        ProtocolConfiguration *cfg, 
        const InterfaceRef& localIface,
        const InterfaceRef& peerIface);

    ProtocolConfigurationFactory *getConfigFactory();

    ProtType_t protocolNameToType(string protoName);

    List<ProtocolConfiguration *>configs; 

    typedef Map< Pair<InterfaceRef, ProtocolConfiguration *>, Protocol *> server_registry_t;
    server_registry_t registeredServers; 

protected:
    void removeConfiguration(ProtocolConfiguration *cfg);

public:
    
    ProtocolFactory(ProtocolManager *_pm);

   ~ProtocolFactory();

    void initialize(const Metadata& m);

    Protocol *getSenderProtocol(
        const InterfaceRef& localIface,
        const InterfaceRef& peerIface,
        DataObjectRef& dObj);

    void instantiateServers(InterfaceRef localIface);

    ProtocolClassifier *getClassifier();

    void setClassifier(ProtocolClassifier *_classifier);
};

#endif /* _PROTOCOLFACTORY_H */
