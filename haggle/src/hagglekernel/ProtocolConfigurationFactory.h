/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _PROTOCOLCFGFACTORY_H
#define _PROTOCOLCFGFACTORY_H

class ProtocolConfigurationFactory;

#include "ProtocolConfiguration.h"
#include "Protocol.h"

class ProtocolConfigurationFactory
{
public:
    ProtocolConfigurationFactory() {};

    ~ProtocolConfigurationFactory() {};

    ProtocolConfiguration *configForName(string protocolName);
};

#endif
