/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Jong-Seok Choi (JS)
 */

#include "SocketWrapperUDP.h"
#include "ProtocolUDPGeneric.h"

#include<sys/ioctl.h>
#include<net/if_arp.h>

#include "ProtocolUDPGeneric.h"

SocketWrapperUDP::SocketWrapperUDP(
    HaggleKernel *_kernel,
    Manager *_manager,
    string localIfaceName, 
    SOCKET _sock) :
        SocketWrapper(_kernel, _manager, _sock),
        useArpHack(false),
        localIfaceName(localIfaceName)
{
}

SocketWrapperUDP::~SocketWrapperUDP()
{
}

bool SocketWrapperUDP::openLocalSocket(
    struct sockaddr *local_addr,
    socklen_t addrlen,
    bool registersock,
    bool nonblock)
{
    int optval = 1;

    if (isConnected()) {
        HAGGLE_ERR("Already connected\n");
        return false;
    }

    if (!openSocket(
            AF_INET, 
            SOCK_DGRAM, 
            IPPROTO_UDP, 
            registersock, 
            nonblock)) {
        HAGGLE_ERR("Could not create UDP socket\n");
        return false;
    }

    if (!multiplyRcvBufferSize(2)) {
        HAGGLE_ERR("Could not increase receive buffer size.\n");
    }

    /* MOS - keep send buffer size small - possibly even decrease
    if (!multiplySndBufferSize(2)) {
        HAGGLE_ERR("Could not increase send buffer size.\n");
    }
    */

    if (!setSocketOption(SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
        closeSocket();
        HAGGLE_ERR("setsockopt SO_REUSEADDR failed\n");
        return false;
    }

    if (!setSocketOption(SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval))) {
        closeSocket();
        HAGGLE_ERR("setsockopt SO_BROADCAST failed\n");
        return false;
    }

    if (!bind(local_addr, addrlen)) {
        closeSocket();
        HAGGLE_ERR("Could not bind UDP socket\n");
        return false;
    }
  
    return true;
}


SOCKET SocketWrapperUDP::accept(struct sockaddr *saddr, socklen_t *addrlen) 
{
    if ((NULL == saddr) || (NULL == addrlen)) {
        HAGGLE_ERR("Invalid params to accept\n");
        return SOCKET_ERROR;
    }

    char b;
    ssize_t rc = recvFrom(&b, 1, MSG_PEEK, saddr, addrlen);

    if (1 != rc) {
        HAGGLE_ERR("Error: could not peek on UDP socket\n");
        return SOCKET_ERROR;
    }

    return dupeSOCKET();
}

bool 
SocketWrapperUDP::hasArpEntry(string localIfaceName, const struct sockaddr *to)
{
    if (localIfaceName == "") {
        HAGGLE_ERR("No local interface name set.\n");
        return false;
    }

    if (NULL == to) {
        HAGGLE_ERR("NULL sockaddr\n");
        return false;
    }

    int arpSock = socket(AF_INET, SOCK_DGRAM, 0);

    if (INVALID_SOCKET == arpSock) {
        HAGGLE_ERR("ARP socket is invalid\n");
        return false;
    }


    bool entryExists = false;
#if !defined(SIOCGARP)
/*
 * SW: NOTE: WARNING!! this is a BAD way to check the arp table,
 * since it is not atomic and it may change after reading each newline.
 * We need to find a better way to do this on OSX which does not have SIOCGARP. 
 */
    FILE *f = NULL;
    char buf[256]; 

    f = fopen("/proc/net/arp", "r");

    if (!f)
      return false;
        
    while (fgets(buf, 256, f)) {            
      HAGGLE_DBG("reading /proc/net/arp: %s\n",buf);
      char ip_str[20];
      char mac_str[20];
      char dev_str[20];
      char dummy[20];
      struct in_addr ip = { 0 };

      memset(ip_str, '\0', 20);
      memset(mac_str, '\0', 20);
      memset(dummy, '\0', 20);

      int sret = sscanf(buf, "%s %s %s %s %s %s", ip_str, dummy, dummy, mac_str, dummy, dev_str);
                
      if (sret > 0) {
	inet_aton(ip_str, &ip);
                        
	if (memcmp(&ip, &((struct sockaddr_in *)to)->sin_addr.s_addr, 4) == 0) {
	  struct ether_addr eth;
	  if (ether_aton_r(mac_str, &eth)) {
	    HAGGLE_DBG("ARP Entry found in /proc/net/arp\n");
        entryExists = true;
	    break;
	  }
	}
      }
    }
        
    fclose(f);
#else
    struct arpreq areq;
    struct sockaddr_in *sin;
    
    memset(&areq, 0, sizeof(areq));
    sin = (struct sockaddr_in *) &areq.arp_pa;
    sin->sin_family = AF_INET;

    sin->sin_addr = ((struct sockaddr_in *)to)->sin_addr;
    sin = (struct sockaddr_in *) &areq.arp_ha;
    sin->sin_family = ARPHRD_ETHER;
    strncpy(areq.arp_dev, localIfaceName.c_str(), 15);


    if (0 <= ioctl(arpSock, SIOCGARP, (caddr_t) &areq)) {
        entryExists = true;
    }
    else
      {
        HAGGLE_DBG("No ARP Entry found\n");
      }

    CLOSE_SOCKET(arpSock);
#endif
    return entryExists;
}

// SW: NOTE: This is to deal with a UDP bug where the ARP request
// is interfering with the UDP send. If we have a cache miss 
// then the packet does not go through!.
bool 
SocketWrapperUDP::sendArpRequest(const struct sockaddr *to)
{
    if (NULL == to) {
        HAGGLE_ERR("NULL sockaddr\n");
        return false;
    }
    // FIXME: SW: temporary HACK to trigger a ping (and thus an ARP request)
    // we're still looking for a better way to do this without relying 
    // on the system() call
    
    // MOS - To avoid early purging and repeated pings you may increase 
    // /proc/sys/net/ipv4/neigh/default/gc_stale_time (default is 60s)

    HAGGLE_DBG("Sending ping to trigger ARP request\n");

    char buf[60];
	// JS ---
    sprintf(buf, "%s -c 1 %s", PING_COMMAND_PATH, inet_ntoa(((struct sockaddr_in *)to)->sin_addr));
	// --- JS
    int ret = system(buf);

    if (0 != ret) {
        HAGGLE_ERR("Could not send packet to trigger ARP request\n");
        return false;
    }

    usleep(50000);

    return true;
}

bool 
SocketWrapperUDP::checkAndSendArp(const struct sockaddr *to) 
{
    bool hasEntry = hasArpEntry(localIfaceName, to);
    if (!hasEntry) {
        SocketWrapperUDP::sendArpRequest(to);
    }
    hasEntry = hasArpEntry(localIfaceName, to);
    if (!hasEntry) {
        HAGGLE_ERR("Could not get ARP entry\n");
    }

    return hasEntry;
}

ssize_t 
SocketWrapperUDP::sendTo(
    const void *buf, 
    size_t len, 
    int flags, 
    const struct sockaddr *to, 
    socklen_t tolen) 
{
    bool willSucceed = true; 

    if (useArpHack) {
        willSucceed = checkAndSendArp(to);
    }

    ssize_t ret = SocketWrapper::sendTo(buf, len, flags, to, tolen);

    if (!willSucceed) {
        HAGGLE_ERR("No ARP entry\n");
        // SW: don't trigger a fatal error, trigger a retry
        errno = EAGAIN;
        return 0;
    }

    return ret;
}

bool
SocketWrapperUDP::doManualArpInsertion(
    const char *arp_insertion_path,
    const char *iface_str,
    const char *ip_str,
    const char *mac_str)
{
    if (NULL == iface_str || NULL == ip_str|| NULL == mac_str|| NULL == arp_insertion_path) {
        HAGGLE_ERR("Null args\n");
        return false;
    }

    char cmd[BUFSIZ];
    if (0 >= sprintf(cmd, "%s %s %s %s", 
            arp_insertion_path, 
            iface_str, 
            ip_str,
            mac_str)) {
        HAGGLE_ERR("Could not construct string\n");
        return false;
    }

    int r = system(cmd);
    if (0 != r) {
      HAGGLE_ERR("Could not call arphelper for manual arp insertion - exit code: %d, error: %s (%d), cmd: %s - Please make sure arphelper is setuid root (chown root, chmod +s) and executable (chmod +x)\n", r, strerror(errno), errno, cmd);
        return false;
    }

    HAGGLE_DBG("Sucessfully executed arphelper for manual arp insertion.\n");
    return true;
}

#define SOCKADDR_SIZE sizeof(struct sockaddr_in)
#define MAC_SNOOP_STR "%02x:%02x:%02x:%02x:%02x:%02x"

bool
SocketWrapperUDP::doSnoopMacAddress(unsigned char *o_mac)
{
    if (NULL == o_mac) {
        HAGGLE_ERR("Invalid params\n");
        return false;
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *peer_addr = (struct sockaddr *)buf;
    socklen_t peer_addr_len = sizeof(*peer_addr);

    char bf[BUFSIZ];
    ssize_t rc = recvFrom(&bf, BUFSIZ, MSG_PEEK, peer_addr, &peer_addr_len);

    if ((ssize_t) sizeof(ProtocolUDPGeneric::udpmsg_t) >= rc) {
        HAGGLE_ERR("Could not peek on socket\n");
        return false;
    }

    // MOS
    if (rc <= sizeof(ProtocolUDPGeneric::udpmsg_t) + 1) {
        HAGGLE_ERR("Cannot snoop message - header missing\n");
        return false;
    }

    bf[rc-1] = '\0'; // MOS

    // SW: we need to skip over the header!
    char* b = &bf[sizeof(ProtocolUDPGeneric::udpmsg_t)];

    unsigned int mc[6];

    char *mac_loc = strstr(b, "\" type=\"ethernet\"/>");
    if (!mac_loc) {
        HAGGLE_ERR("Could not find MAC address in data (no ethernet)\n");
        return false;
    }

    mac_loc[0] = '\0';
    mac_loc = mac_loc - 17;

    if (mac_loc <= b) {
        HAGGLE_ERR("Could not find MAC address in data (out of bounds)\n");
        return false;
    }

    if (6 != sscanf(mac_loc, MAC_SNOOP_STR, &mc[0], &mc[1], &mc[2], &mc[3], &mc[4], &mc[5])) {
        HAGGLE_ERR("Could not find MAC address in data (bad format)\n");
        return false;
    }

    o_mac[0] = (unsigned char) mc[0];
    o_mac[1] = (unsigned char) mc[1];
    o_mac[2] = (unsigned char) mc[2];
    o_mac[3] = (unsigned char) mc[3];
    o_mac[4] = (unsigned char) mc[4];
    o_mac[5] = (unsigned char) mc[5];

    HAGGLE_DBG("Snooped mac: %s\n", mac_loc);

    return true;
}

bool
SocketWrapperUDP::doSnoopIPAddress(sockaddr *o_ip)
{
    if (NULL == o_ip) {
        HAGGLE_ERR("Null output arg\n");
        return false;
    }

    char buf[SOCKADDR_SIZE];
    bzero(buf, SOCKADDR_SIZE);
    struct sockaddr *peer_addr = (struct sockaddr *)buf;
    socklen_t peer_addr_len = sizeof(*peer_addr);

    char bf[BUFSIZ];
    ssize_t rc = recvFrom(&bf, BUFSIZ, MSG_PEEK, peer_addr, &peer_addr_len);

    if ((ssize_t) sizeof(ProtocolUDPGeneric::udpmsg_t) >= rc) {
        HAGGLE_ERR("Could not peek on socket\n");
        return false;
    }

    // MOS
    if (rc <= sizeof(ProtocolUDPGeneric::udpmsg_t) + 1) {
        HAGGLE_ERR("Cannot snoop message - header missing\n");
        return false;
    }

    bf[rc-1] = '\0'; // MOS

    // SW: we need to skip over the header!
    char* b = &bf[sizeof(ProtocolUDPGeneric::udpmsg_t)];

    // make sure o_ip is initialized properly
    if (NULL == memcpy(o_ip, peer_addr, peer_addr_len)) {
        HAGGLE_ERR("Could not initilaize o_ip properly\n");
        return false;
    }

    char *ip_loc = strstr(b, "\" type=\"ipv4\"/>");
    if (!ip_loc) {
        HAGGLE_ERR("Could not find IP address in data (no ipv4)\n");
        return false;
    }

    ip_loc[0] = '\0';

    while ((ip_loc > b) && ((ip_loc)[0] != '\"')) {
        ip_loc--;
    }

    if (ip_loc[0] != '\"')  {
        HAGGLE_ERR("Could not find IP address in data (out of bounds: %c)\n", ip_loc[0]);
        return false;
    }

    ip_loc++;
    
    if (0 == inet_aton(ip_loc, &((sockaddr_in *)o_ip)->sin_addr)) {
        HAGGLE_ERR("Could not parse IP address\n");
        return false;
    }

    HAGGLE_DBG("Snooped IP: %s\n", ip_loc);

    return true;
}
