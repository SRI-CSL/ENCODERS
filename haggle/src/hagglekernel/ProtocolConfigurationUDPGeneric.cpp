/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolConfigurationUDPGeneric.h"

void ProtocolConfigurationUDPGeneric::onConfig(const Metadata &m)
{
    ProtocolConfiguration::onConfig(m);
    
    int param;

    if ((param = parseInt(m, PROT_GEN_UDP_PORT_A_NAME)) > 0) {
        controlPortA = param;
    }

    if ((param = parseInt(m, PROT_GEN_UDP_PORT_B_NAME)) > 0) {
        controlPortB = param;
    }

    if ((param = parseInt(m, PROT_GEN_UDP_PORT_C_NAME)) > 0) {
        nonControlPortC = param;
    }

    bool bparam; 

    if (parseBool(m, PROT_GEN_UDP_ARP_HACK_NAME, &bparam)) {
        useArpHack = bparam;
    }

    if (parseBool(m, PROT_GEN_UDP_USE_MAN_ARP_NAME, &bparam)) {
        manualArpInsertion = bparam;
    }

    if (parseBool(m, PROT_GEN_UDP_USE_CONTROL_PROTO_NAME, &bparam)) {
        useControlProtocol = bparam;
    }

    const char *sparam = m.getParameter(PROT_GEN_UDP_ARP_INSERT_PATH_NAME);
    if (sparam) {
        arpInsertionPathString = string(sparam);
    }
}

ProtocolConfiguration *ProtocolConfigurationUDPUnicast::clone(
    ProtocolConfiguration *o_cfg)
{
    if (!o_cfg) {
        o_cfg = new ProtocolConfigurationUDPUnicast(); 
    }

    if (o_cfg->getType() != type) {
        HAGGLE_ERR("Mismatched types.\n");
        return NULL;
    }

    ProtocolConfiguration::clone(o_cfg);

    ProtocolConfigurationUDPUnicast *o_ucfg = static_cast<ProtocolConfigurationUDPUnicast *>(o_cfg);

    o_ucfg->controlPortA = controlPortA;
    o_ucfg->controlPortB = controlPortB;
    o_ucfg->nonControlPortC = nonControlPortC;
    
    o_ucfg->useArpHack = useArpHack;
    o_ucfg->manualArpInsertion = manualArpInsertion;
    o_ucfg->useControlProtocol = useControlProtocol;
    
    o_ucfg->arpInsertionPathString = arpInsertionPathString;

    return o_cfg;
}

ProtocolConfiguration *ProtocolConfigurationUDPBroadcast::clone(
    ProtocolConfiguration *o_cfg)
{
    if (!o_cfg) {
        o_cfg = new ProtocolConfigurationUDPBroadcast(); 
    }

    if (o_cfg->getType() != type) {
        HAGGLE_ERR("Mismatched types.\n");
        return NULL;
    }

    ProtocolConfiguration::clone(o_cfg);

    ProtocolConfigurationUDPBroadcast *o_bcfg = static_cast<ProtocolConfigurationUDPBroadcast *>(o_cfg);

    o_bcfg->controlPortA = controlPortA;
    o_bcfg->controlPortB = controlPortB;
    o_bcfg->nonControlPortC = nonControlPortC;
    
    o_bcfg->useArpHack = useArpHack;
    o_bcfg->manualArpInsertion = manualArpInsertion;
    o_bcfg->useControlProtocol = useControlProtocol;
    
    o_bcfg->arpInsertionPathString = arpInsertionPathString;

    return o_cfg;
}
