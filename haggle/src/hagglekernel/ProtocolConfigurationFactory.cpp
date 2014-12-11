/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolConfigurationFactory.h"
#include "ProtocolConfigurationTCP.h"
#include "ProtocolConfigurationUDPGeneric.h"

ProtocolConfiguration *
ProtocolConfigurationFactory::configForName(
        string protoName)
{

    ProtocolConfiguration *cfg = NULL;

    if (protoName == "ProtocolTCP") {
        cfg = new ProtocolConfigurationTCP();
    }

    if (protoName == "ProtocolUDPBroadcast") {
        cfg = new ProtocolConfigurationUDPBroadcast();
    }

    if (protoName == "ProtocolUDPUnicast") {
        cfg = new ProtocolConfigurationUDPUnicast();
    }

    return cfg;
}
