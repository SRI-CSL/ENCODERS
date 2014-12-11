/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef _SOCKETWRAPPERUDP_H
#define _SOCKETWRAPPERUDP_H

class SocketWrapperUDP;

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/Timeval.h>

#include "Protocol.h"
#include "Manager.h"
#include "SocketWrapper.h"
#include "Interface.h"

// JS ---
#if defined(OS_ANDROID)
#define PING_COMMAND_PATH "/system/bin/ping"
#define ARPHELPER_COMMAND_PATH "/system/etc/arphelper" // MOS
#else
#define PING_COMMAND_PATH "/bin/ping"
#define ARPHELPER_COMMAND_PATH "/etc/arphelper" // MOS
#endif
// --- JS

class SocketWrapperUDP : public SocketWrapper {
private:
    bool useArpHack;
    string localIfaceName;
public:
    SocketWrapperUDP( 
        HaggleKernel *_kernel,
        Manager *_manager,
        string localIfaceName = string(""),
        SOCKET _sock = INVALID_SOCKET);

    virtual ~SocketWrapperUDP();

    void setUseArpHack(bool _useArpHack) { useArpHack = _useArpHack; }

    bool getUseArpHack() { return useArpHack; }

    bool openLocalSocket(
        struct sockaddr *local_addr,
        socklen_t addrlen,
        bool registersock,
        bool nonblock);

    // SW: TODO: deprecate and remove this dead code (fake accept)
    SOCKET accept(struct sockaddr *saddr, socklen_t *addrlen);

    bool checkAndSendArp(const struct sockaddr *to);

    static bool hasArpEntry(string localIfaceName, const struct sockaddr *to);
    
    static bool sendArpRequest(const struct sockaddr *to);

    ssize_t sendTo(
        const void *buf, 
        size_t len, 
        int flags, 
        const struct sockaddr *to, 
        socklen_t tolen);

    static bool doManualArpInsertion(
        const char *arp_insertion_path,
        const char *iface_str,
        const char *ip_str,
        const char *mac_str);

    bool doSnoopMacAddress(unsigned char *o_mac);

    bool doSnoopIPAddress(sockaddr *o_ip);
};

#endif /* SOCKETWRAPPERUDP_H */
