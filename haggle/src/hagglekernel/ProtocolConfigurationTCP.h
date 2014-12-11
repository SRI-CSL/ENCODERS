/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _PROTOCOLCFGTCP_H
#define _PROTOCOLCFGTCP_H

#define PROT_TCP_PORT_NAME "port"
#define PROT_TCP_BACKLOG_NAME "backlog"

class ProtocolConfigurationTCP;

#include "ProtocolConfiguration.h"
#include "ProtocolTCP.h"

class ProtocolConfigurationTCP : public ProtocolConfiguration 
{
private:
    int port;
    int backlog;
public:
    ProtocolConfigurationTCP() : 
        ProtocolConfiguration(Protocol::TYPE_TCP),
        port(TCP_DEFAULT_PORT),
        backlog(TCP_BACKLOG_SIZE) {};

    void onConfig(const Metadata &m);

    int getPort() { return port; }

    int getBacklog() { return backlog; }

    ProtocolConfiguration *clone(ProtocolConfiguration *o_cfg = NULL);

    ~ProtocolConfigurationTCP() {};
};

#endif /* _PROTOCOLCFGTCP_H */
