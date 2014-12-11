/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ProtocolConfigurationTCP.h"

void ProtocolConfigurationTCP::onConfig(const Metadata &m)
{
    ProtocolConfiguration::onConfig(m);

    int param;

    if ((param = parseInt(m, PROT_TCP_PORT_NAME)) > 0) {
        port = param;
    }

    if ((param = parseInt(m, PROT_TCP_BACKLOG_NAME)) > 0) {
        backlog = param;
    }
}

ProtocolConfiguration *ProtocolConfigurationTCP::clone(
    ProtocolConfiguration *o_cfg)
{
    if (NULL == o_cfg) {
        o_cfg = new ProtocolConfigurationTCP();
    }
    
    if (o_cfg->getType() != type) {
        HAGGLE_ERR("Mismatched types.\n");
        return NULL;
    }

    ProtocolConfiguration::clone(o_cfg);

    ProtocolConfigurationTCP *tcpCfg;
    tcpCfg = static_cast<ProtocolConfigurationTCP *>(o_cfg);
    tcpCfg->port = port;
    tcpCfg->backlog = backlog;

    return tcpCfg;
}
