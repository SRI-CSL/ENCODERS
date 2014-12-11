/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _PROTOCOLCFG_UDP_GENERIC
#define _PROTOCOLCFG_UDP_GENERIC

class ProtocolConfigurationUDPGeneric;
class ProtocolConfigurationUDPUnicast;
class ProtocolConfigurationUDPBroadcast;

#include "ProtocolConfiguration.h"

#include "ProtocolUDPUnicastProxy.h"
#include "ProtocolUDPBroadcastProxy.h"
#include "SocketWrapperUDP.h"

#define PROT_GEN_UDP_PORT_A_NAME "control_port_a"
#define PROT_GEN_UDP_PORT_B_NAME "control_port_b"
#define PROT_GEN_UDP_PORT_C_NAME "no_control_port"
#define PROT_GEN_UDP_ARP_HACK_NAME "use_arp_hack"
#define PROT_GEN_UDP_USE_MAN_ARP_NAME "use_arp_manual_insertion"
#define PROT_GEN_UDP_USE_CONTROL_PROTO_NAME "use_control_protocol"
#define PROT_GEN_UDP_ARP_INSERT_PATH_NAME "arp_manual_insertion_path"

class ProtocolConfigurationUDPGeneric : public ProtocolConfiguration 
{
protected:
    int controlPortA;
    int controlPortB;
    int nonControlPortC;
    
    bool useArpHack;
    bool manualArpInsertion;
    bool useControlProtocol;

    string arpInsertionPathString;
public:
    ProtocolConfigurationUDPGeneric(
        ProtType_t type,
        int portA,
        int portB,
        int portC) : 
        ProtocolConfiguration(type),
        controlPortA(portA),
        controlPortB(portB),
        nonControlPortC(portC),
        useArpHack(false),
        manualArpInsertion(true),
        useControlProtocol(false),
        arpInsertionPathString(ARPHELPER_COMMAND_PATH) { };

    void onConfig(const Metadata &m);

    int getControlPortA() { return controlPortA; }
    int getControlPortB() { return controlPortB; }
    int getNonControlPortC() { return nonControlPortC; }

    bool getUseArpHack() { return useArpHack; }
    bool getUseManualArpInsertion() { return manualArpInsertion; }
    bool getUseControlProtocol() {  return useControlProtocol; }

    string getArpInsertionPathString() { return arpInsertionPathString; }

    virtual ~ProtocolConfigurationUDPGeneric() { }
};

class ProtocolConfigurationUDPUnicast : public ProtocolConfigurationUDPGeneric
{
public:
    ProtocolConfigurationUDPUnicast() :
        ProtocolConfigurationUDPGeneric(
            Protocol::TYPE_UDP_UNICAST,
            UDP_UCAST_PROXY_PORT_A,
            UDP_UCAST_PROXY_PORT_B,
            UDP_UCAST_PROXY_PORT_C) {};

    ProtocolConfiguration *clone(ProtocolConfiguration *o_cfg = NULL);

    ~ProtocolConfigurationUDPUnicast() {};
};

class ProtocolConfigurationUDPBroadcast : public ProtocolConfigurationUDPGeneric
{
public:
    ProtocolConfigurationUDPBroadcast() :
        ProtocolConfigurationUDPGeneric(
            Protocol::TYPE_UDP_BROADCAST,
            UDP_BCAST_PROXY_PORT_A,
            UDP_BCAST_PROXY_PORT_B,
            UDP_BCAST_PROXY_PORT_C) {};

    ProtocolConfiguration *clone(ProtocolConfiguration *o_cfg = NULL);

    ~ProtocolConfigurationUDPBroadcast() {};
};

#endif /* _PROTOCOLCFG_UDP_GENERIC */
