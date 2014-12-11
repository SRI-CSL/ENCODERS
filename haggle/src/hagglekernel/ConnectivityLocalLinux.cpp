/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Watch.h>
#include <libcpphaggle/List.h>

#if defined(OS_LINUX)

#ifdef ENABLE_BLUETOOTH
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include "ProtocolRFCOMM.h"
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#endif

#if defined(ENABLE_ETHERNET)
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include <errno.h>
#endif

#if defined(HAVE_DBUS)
#include <dbus/dbus.h>
#endif /* HAVE_DBUS */

#include "ConnectivityManager.h"
#include "ConnectivityLocalLinux.h"
#include "ConnectivityBluetooth.h"
#include "ConnectivityEthernet.h"
#include "Interface.h"

#if defined(ENABLE_BLUETOOTH)
Interface *hci_get_interface_from_name(const char *ifname);
#endif

#if defined(ENABLE_ETHERNET)

static const char *blacklist_device_names[] = { "vmnet", "vmaster", "rmnet", "pan", "lo", "wifi", NULL };
// static const char *blacklist_device_names[] = { "vmnet", "vmaster", "rmnet", "pan", "lo", "wifi", "usb", NULL };

static bool isBlacklistDeviceName(const char *devname)
{
	int i = 0;
        string dname = devname;

        // Assume that the last character is a single digit indicating
        // the device number. Remove it to be able to match exactly
        // against the blacklist names
        dname[dname.length()-1] = '\0';
        
	while (blacklist_device_names[i]) {
		if (strcmp(blacklist_device_names[i], dname.c_str()) == 0)
			return true;
		i++;
	}

	return false;
}

struct if_info {
	int msg_type;
	int ifindex;
	bool isUp;
	bool isWireless;
	char ifname[256];
	unsigned char mac[ETH_ALEN];
	struct in_addr ip;
	struct in_addr broadcast;
	struct sockaddr_in ipaddr;
};

#define netlink_getlink(nl) netlink_request(nl, RTM_GETLINK)
#define netlink_getneigh(nl) netlink_request(nl, RTM_GETNEIGH)
#define netlink_getaddr(nl) netlink_request(nl, RTM_GETADDR | RTM_GETLINK)

static int netlink_request(struct netlink_handle *nlh, int type);
//static int read_netlink(struct netlink_handle *nlh, struct if_info *ifinfo);

static int nl_init_handle(struct netlink_handle *nlh)
{
	int ret;
	socklen_t addrlen;

	if (!nlh)
		return -1;

	memset(nlh, 0, sizeof(struct netlink_handle));
	nlh->seq = 0;
	nlh->local.nl_family = PF_NETLINK;
	nlh->local.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;
	nlh->local.nl_pid = getpid();
	nlh->peer.nl_family = PF_NETLINK;

	nlh->sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	if (!nlh->sock) {
		CM_DBG("Could not create netlink socket");
		return -2;
	}
	addrlen = sizeof(nlh->local);

	ret = bind(nlh->sock, (struct sockaddr *) &nlh->local, addrlen);

	if (ret == -1) {
		close(nlh->sock);
		CM_DBG("Bind for RT netlink socket failed");
		return -3;
	}
	ret = getsockname(nlh->sock, (struct sockaddr *) &nlh->local, &addrlen);

	if (ret < 0) {
		close(nlh->sock);
		CM_DBG("Getsockname failed ");
		return -4;
	}

	return 0;
}

static int nl_close_handle(struct netlink_handle *nlh)
{
	if (!nlh)
		return -1;

	return close(nlh->sock);
}

static int nl_send(struct netlink_handle *nlh, struct nlmsghdr *n)
{
	int res;
	struct iovec iov;
	struct msghdr msg;
	
	memset(&iov, 0, sizeof(iov));
	iov.iov_base = n;
	iov.iov_len = n->nlmsg_len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &nlh->peer;
	msg.msg_namelen = sizeof(nlh->peer);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	n->nlmsg_seq = ++nlh->seq;
	n->nlmsg_pid = nlh->local.nl_pid;

	/* Request an acknowledgement by setting NLM_F_ACK */
	n->nlmsg_flags |= NLM_F_ACK;

	/* Send message to netlink interface. */
	res = sendmsg(nlh->sock, &msg, 0);

	if (res < 0) {
		HAGGLE_ERR("error: %s\n", 
			   strerror(errno));
		return -1;
	}
	return 0;
}

static int nl_parse_link_info(struct nlmsghdr *nlm, struct if_info *ifinfo)
{
	struct rtattr *rta = NULL;
	struct ifinfomsg *ifimsg = (struct ifinfomsg *) NLMSG_DATA(nlm);
	int attrlen = nlm->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifinfomsg));
	int n = 0;

	if (!ifimsg || !ifinfo)
		return -1;

	ifinfo->isWireless = false;
	ifinfo->ifindex = ifimsg->ifi_index;
	ifinfo->isUp = ifimsg->ifi_flags & IFF_UP ? true : false;

	for (rta = IFLA_RTA(ifimsg); RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
		if (rta->rta_type == IFLA_ADDRESS) {
			if (ifimsg->ifi_family == AF_UNSPEC) {
				if (RTA_PAYLOAD(rta) == ETH_ALEN) {
					memcpy(ifinfo->mac, (char *) RTA_DATA(rta), ETH_ALEN);
					n++;
				}
			}
		} else if (rta->rta_type == IFLA_IFNAME) {
			strcpy(ifinfo->ifname, (char *) RTA_DATA(rta));
			n++;
		} else if (rta->rta_type == IFLA_WIRELESS) {
			// wireless stuff
			ifinfo->isWireless = true;
		}
	}
	return n;
}
static int nl_parse_addr_info(struct nlmsghdr *nlm, struct if_info *ifinfo)
{
	struct rtattr *rta = NULL;
	struct ifaddrmsg *ifamsg = (struct ifaddrmsg *) NLMSG_DATA(nlm);
	int attrlen = nlm->nlmsg_len - NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	int n = 0;

	if (!ifamsg || !ifinfo)
		return -1;

	ifinfo->ifindex = ifamsg->ifa_index;
	for (rta = IFA_RTA(ifamsg); RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
		if (rta->rta_type == IFA_ADDRESS) {
			memcpy(&ifinfo->ipaddr.sin_addr, RTA_DATA(rta), RTA_PAYLOAD(rta));
			memcpy(&ifinfo->ip, RTA_DATA(rta), RTA_PAYLOAD(rta));
			ifinfo->ipaddr.sin_family = ifamsg->ifa_family;
		} if (rta->rta_type == IFA_BROADCAST) {
			memcpy(&ifinfo->broadcast, RTA_DATA(rta), RTA_PAYLOAD(rta));
		} else if (rta->rta_type == IFA_LOCAL) {
			if (RTA_PAYLOAD(rta) == ETH_ALEN) {
			}
		} else if (rta->rta_type == IFA_LABEL) {
			strcpy(ifinfo->ifname, (char *) RTA_DATA(rta));
		}
	}

	return n;
}

#if 0
static int fill_in_ipconf(struct if_info *ifinfo)
{
	struct ifreq ifr;
	struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;
	int sock;

	if (!ifinfo)
		return -1;
	
	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_ifindex = ifinfo->ifindex;
	strcpy(ifr.ifr_name, ifinfo->ifname);

	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		//HAGGLE_ERR("Could not get IP address for %s\n", ifinfo->ifname);
		close(sock);
		return -1;
	}
	memcpy(&ifinfo->ip, &sin->sin_addr, sizeof(struct in_addr));

	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) < 0) {
		//HAGGLE_ERR("Could not get IP address for %s\n", ifinfo->ifname);
		close(sock);
		return -1;
	}
	memcpy(&ifinfo->broadcast, &sin->sin_addr, sizeof(struct in_addr));

	close(sock);

	return 0;
}
#endif // 0

static int fill_in_macaddr(struct if_info *ifinfo)
{
	int fd;
	struct ifreq ifr;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifinfo->ifname, IFNAMSIZ-1);
	
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		close(fd);
		return -1;
	}
	
	close(fd);

	memcpy(ifinfo->mac, ifr.ifr_hwaddr.sa_data, 6);

	return 0;
}

ConnectivityLocalLinux::ConnectivityLocalLinux(ConnectivityManager *m) :
	ConnectivityLocal(m, "ConnectivityLocalLinux")
{
}

ConnectivityLocalLinux::~ConnectivityLocalLinux()
{
}

int ConnectivityLocalLinux::read_netlink()
{
	int len, num_msgs = 0;
	socklen_t addrlen;
	struct nlmsghdr *nlm;
	struct if_info ifinfo;
#define BUFLEN 2000
	char buf[BUFLEN];

	addrlen = sizeof(struct sockaddr_nl);

	memset(buf, 0, BUFLEN);

	len = recvfrom(nlh.sock, buf, BUFLEN, 0, (struct sockaddr *) &nlh.peer, &addrlen);

	if (len == EAGAIN) {
		CM_DBG("Netlink recv would block\n");
		return 0;
	}
	if (len < 0) {
		CM_DBG("len negative\n");
		return len;
	}
	for (nlm = (struct nlmsghdr *) buf; NLMSG_OK(nlm, (unsigned int) len); nlm = NLMSG_NEXT(nlm, len)) {
		struct nlmsgerr *nlmerr = NULL;

		memset(&ifinfo, 0, sizeof(struct if_info));

		num_msgs++;

		switch (nlm->nlmsg_type) {
		case NLMSG_ERROR:
			nlmerr = (struct nlmsgerr *) NLMSG_DATA(nlm);
			if (nlmerr->error == 0) {
				CM_DBG("NLMSG_ACK");
			} else {
				CM_DBG("NLMSG_ERROR, error=%d type=%d\n", nlmerr->error, nlmerr->msg.nlmsg_type);
			}
			break;
		case RTM_NEWLINK:
		        nl_parse_link_info(nlm, &ifinfo);

			//CM_DBG("RTM NEWLINK %s [%s] %s\n", ifinfo.ifname, eth_to_str(ifinfo.mac), ifinfo.isUp ? "UP" : "DOWN");

#if 0 // Only Bring up interfaces on NEWADDR
			/* TODO: Should find a good way to sort out unwanted interfaces. */
			if (!isBlacklistDeviceName(ifinfo.ifname)) {
				
				if (fill_in_ipconf(&ifinfo) < 0) {
					//HAGGLE_ERR("Could not get IP configuration for interface %s\n", ifinfo.ifname);
					break;
				}
				
				if (ifinfo.mac[0] == 0 &&
                                    ifinfo.mac[1] == 0 &&
                                    ifinfo.mac[2] == 0 &&
                                    ifinfo.mac[3] == 0 &&
                                    ifinfo.mac[4] == 0 &&
                                    ifinfo.mac[5] == 0)
                                        break;
                             
				CM_DBG("Interface newlink %s %s %s\n", 
				       ifinfo.ifname, eth_to_str(ifinfo.mac), 
				       ifinfo.isUp ? "up" : "down");
				
				if (!ifinfo.isUp)
					break;
		
				Addresses addrs;
				
				addrs.add(new EthernetAddress(ifinfo.mac));
				addrs.add(new IPv4Address(ifinfo.ip));
				addrs.add(new IPv4BroadcastAddress(ifinfo.broadcast));
				
				InterfaceRef iface;

				if (ifinfo.isWireless) {
					iface = Interface::create<WiFiInterface>(ifinfo.mac, ifinfo.ifname, addrs, IFFLAG_LOCAL | IFFLAG_UP);
				} else {
					iface = Interface::create<EthernetInterface>(ifinfo.mac, ifinfo.ifname, addrs, IFFLAG_LOCAL | IFFLAG_UP);
				}
				
				if (iface) {
					report_interface(iface, rootInterface, new ConnectivityInterfacePolicyAgeless());
				}
			}
#endif
			if (!ifinfo.isUp) {
				CM_DBG("Interface %s [%s] went DOWN\n", ifinfo.ifname, eth_to_str(ifinfo.mac));
				delete_interface(ifinfo.isWireless ? Interface::TYPE_WIFI : Interface::TYPE_ETHERNET, ifinfo.mac);
			}
			break;
		case RTM_DELLINK:
		{
			nl_parse_link_info(nlm, &ifinfo);
			EthernetAddress mac(ifinfo.mac);
			CM_DBG("Interface dellink %s %s\n", ifinfo.ifname, mac.getStr());
			// Delete interface here?
			
			delete_interface(ifinfo.isWireless ? Interface::TYPE_WIFI : Interface::TYPE_ETHERNET, ifinfo.mac);
		}
			break;
		case RTM_DELADDR:
			nl_parse_addr_info(nlm, &ifinfo);
			CM_DBG("Interface deladdr %s %s\n", ifinfo.ifname, ip_to_str(ifinfo.ipaddr.sin_addr));
			// Delete interface here?
			delete_interface(ifinfo.isWireless ? Interface::TYPE_WIFI : Interface::TYPE_ETHERNET, ifinfo.mac);
			break;
		case RTM_NEWADDR:
			nl_parse_addr_info(nlm, &ifinfo);
			CM_DBG("Interface newaddr %s %s\n", ifinfo.ifname, ip_to_str(ifinfo.ipaddr.sin_addr));
			
			if (!isBlacklistDeviceName(ifinfo.ifname)) {
				Addresses addrs;

				if (fill_in_macaddr(&ifinfo) != 0) {
					HAGGLE_ERR("Could not fill in mac address of interface %s\n", ifinfo.ifname);
					break;
				}
				
				if (ifinfo.mac[0] == 0 &&
                                    ifinfo.mac[1] == 0 &&
                                    ifinfo.mac[2] == 0 &&
                                    ifinfo.mac[3] == 0 &&
                                    ifinfo.mac[4] == 0 &&
                                    ifinfo.mac[5] == 0)
				     break;
				
				addrs.add(new EthernetAddress(ifinfo.mac));
				addrs.add(new IPv4Address(ifinfo.ip));
				addrs.add(new IPv4BroadcastAddress(ifinfo.broadcast));

				/*
				HAGGLE_DBG("New interface %s %s/%s [%s]\n", ifinfo.ifname, ip_to_str(ifinfo.ip), 
					   ip_to_str(ifinfo.broadcast), eth_to_str(ifinfo.mac));
				*/
				
				InterfaceRef iface;
				
				if (ifinfo.isWireless) {
					iface = Interface::create<WiFiInterface>(ifinfo.mac, ifinfo.ifname, addrs, IFFLAG_LOCAL | IFFLAG_UP);
				} else {
					iface = Interface::create<EthernetInterface>(ifinfo.mac, ifinfo.ifname, addrs, IFFLAG_LOCAL | IFFLAG_UP);
				}
				if (iface) {
					report_interface(iface, rootInterface, new ConnectivityInterfacePolicyAgeless());
				}
			}
			break;
		case NLMSG_DONE:
			CM_DBG("NLMSG_DONE\n");
			break;
		default:
			CM_DBG("Unknown netlink message\n");
			break;
		}
	}
	return num_msgs;
}

static int netlink_request(struct netlink_handle *nlh, int type)
{
	struct {
		struct nlmsghdr nh;
		struct rtgenmsg rtg;
	} req;

	if (!nlh)
		return -1;

	memset(&req, 0, sizeof(req));
    // SW: uninit error if length is incorrect:
	req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(req.rtg));
	//req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(req));
	req.nh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
	req.nh.nlmsg_type = type;
	req.rtg.rtgen_family = AF_INET;

	// Request interface information
	return nl_send(nlh, &req.nh);
}

#define	max(a,b) ((a) > (b) ? (a) : (b))
void ConnectivityLocalLinux::findLocalEthernetInterfaces()
{
	int sock, ret = 0;
#define REQ_BUF_SIZE (sizeof(struct ifreq) * 20)
	struct {
		struct ifconf ifc;
		char buf[REQ_BUF_SIZE];
	} req = { { REQ_BUF_SIZE, { req.buf}}, { 0 } };

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		CM_DBG("Could not open socket\n");
		return;		//-1;
	}
	// This call finds all active interfaces where you can open a UDP socket
	ret = ioctl(sock, SIOCGIFCONF, &req);

	if (ret < 0) {
		CM_DBG("ioctl() failed\n");
		return;		//-1;
	}

	struct ifreq *ifr = (struct ifreq *) req.buf;

	// Goes through all responses
	while (req.ifc.ifc_len) {
		unsigned char macaddr[6];

		int len = (sizeof(ifr->ifr_name) + sizeof(struct sockaddr));

		// Check that the result is an IP adress and that it is an ethernet
		//interface
		if (ifr->ifr_addr.sa_family != AF_INET || strncmp(ifr->ifr_name, "eth", 3) != 0) {
			
			req.ifc.ifc_len -= len;
			ifr = (struct ifreq *) ((char *) ifr + len);

			continue;
		}
		// Get the Ethernet address:
		struct ifreq ifbuf;
		strcpy(ifbuf.ifr_name, ifr->ifr_name);
		ioctl(sock, SIOCGIFHWADDR, &ifbuf);
		memcpy(macaddr, ifbuf.ifr_hwaddr.sa_data, 6);

		struct in_addr ip, broadcast;

                printf("Found interface %02x\n", macaddr[5]);
		memcpy(&ip, &((struct sockaddr_in *) &(ifr->ifr_addr))->sin_addr, sizeof(struct in_addr));

		ioctl(sock, SIOCGIFBRDADDR, &ifbuf);

		memcpy(&broadcast, &((struct sockaddr_in *) &(ifbuf.ifr_broadaddr))->sin_addr, sizeof(struct in_addr));
		
		Addresses addrs;
		addrs.add(new EthernetAddress(macaddr));
		addrs.add(new IPv4Address(ip));
		addrs.add(new IPv4BroadcastAddress(broadcast));
		
		// Create the interface
		InterfaceRef iface = Interface::create<EthernetInterface>(macaddr, ifr->ifr_name, 
									  addrs, IFFLAG_LOCAL | IFFLAG_UP);
 
		if (iface) 
			report_interface(iface, rootInterface, new ConnectivityInterfacePolicyAgeless());

		req.ifc.ifc_len -= len;
		ifr = (struct ifreq *) ((char *) ifr + len);
	}

	close(sock);

	return;
}
#endif // ENABLE_ETHERNET

#if defined(ENABLE_BLUETOOTH) && defined(HAVE_DBUS)

/* Main loop integration for D-Bus */
struct watch_data {
	DBusWatch *watch;
	int fd;
	//Watchable *wa;
	int watchIndex;
	void *data;
};
static List<watch_data *> dbusWatches;

void dbus_close_handle(struct dbus_handle *dbh)
{
	if (dbh && dbh->conn)
		dbus_connection_unref(dbh->conn);
}

static dbus_bool_t dbus_watch_add(DBusWatch * watch, void *data)
{
	struct watch_data *wd;

	if (!dbus_watch_get_enabled(watch))
		return TRUE;

	wd = new struct watch_data;

	if (!wd)
		return FALSE;

	wd->watch = watch;
	wd->data = data;
#if HAVE_DBUS_WATCH_GET_UNIX_FD
	wd->fd = dbus_watch_get_unix_fd(watch);
#else
	wd->fd = dbus_watch_get_fd(watch);
#endif

	dbusWatches.push_back(wd);

	dbus_watch_set_data(watch, (void *) data, NULL);

	return TRUE;
}

static void dbus_watch_remove(DBusWatch * watch, void *data)
{
	for (List<watch_data *>::iterator it = dbusWatches.begin(); it != dbusWatches.end(); it++) {
		struct watch_data *wd = *it;

		if (wd->watch == watch) {
			dbusWatches.erase(it);
			//delete wd->wa;
			delete wd;
			return;
		}
	}
}

static void dbus_watch_remove_all()
{
	while (!dbusWatches.empty()) {
		struct watch_data *wd = dbusWatches.front();

		dbusWatches.pop_front();
		delete wd;
	}
}

static void dbus_watch_toggle(DBusWatch * watch, void *data)
{
	if (dbus_watch_get_enabled(watch))
		dbus_watch_add(watch, data);
	else
		dbus_watch_remove(watch, data);
}

void append_variant(DBusMessageIter *iter, int type, void *val)
{
	DBusMessageIter value_iter;
	char var_type[2] = { type, '\0'};
	dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, var_type, &value_iter);
	dbus_message_iter_append_basic(&value_iter, type, val);
	dbus_message_iter_close_container(iter, &value_iter);
}

static int dbus_bluetooth_set_adapter_property(struct dbus_handle *dbh, const char *object, 
					       const char *c_key, void *value, int type)
{
	DBusMessage *reply, *msg;
	DBusMessageIter iter;

	dbus_error_init(&dbh->err);
	
	msg = dbus_message_new_method_call("org.bluez", object, "org.bluez.Adapter", "SetProperty");

	if (!msg) {
		HAGGLE_ERR("%Can't allocate new method call for GetProperties!\n");
		return -1;
	}
	
	dbus_message_append_args(msg, DBUS_TYPE_STRING, &c_key, DBUS_TYPE_INVALID);
	dbus_message_iter_init_append(msg, &iter);
	append_variant(&iter, type, value);
	
	reply = dbus_connection_send_with_reply_and_block(dbh->conn, 
							  msg, -1, &dbh->err);

	dbus_message_unref(msg);
	
	if (!reply) {
		if (dbus_error_is_set(&dbh->err)) {
			HAGGLE_ERR("D-Bus error: %s (%s)\n", 
				   dbh->err.name, dbh->err.message);
			
			dbus_error_free(&dbh->err);
		} else {
			HAGGLE_ERR("DBus reply is NULL\n");
		}
		return -1;
	}

	dbus_message_unref(reply);

	return 0;
}

static int dbus_bluetooth_set_adapter_property_integer(struct dbus_handle *dbh, const char *adapter, 
						       const char *c_key, unsigned int value)
{
	return dbus_bluetooth_set_adapter_property(dbh, adapter, c_key, (void *)&value, DBUS_TYPE_UINT32);
}

static int dbus_bluetooth_set_adapter_property_boolean(struct dbus_handle *dbh, const char *adapter, 
						       const char *c_key, int value)
{
	return dbus_bluetooth_set_adapter_property(dbh, adapter, c_key, (void *)&value, DBUS_TYPE_BOOLEAN);
}

static const char *dbus_get_interface_name_from_object_path(const char *path)
{
	const char *ifname = NULL;
	int i = 0;

	while (path[i] != '\0') {
		if (path[i] == '/')
			ifname = &path[i+1];
		i++;
	}
	return ifname;
}

int dbus_bluetooth_get_default_adapter(struct dbus_handle *dbh, char **adapter) 
{
	DBusMessage *msg, *reply;
	char *path = NULL;
	int retval = 0;

	msg = dbus_message_new_method_call("org.bluez", "/", "org.bluez.Manager", "DefaultAdapter");

	if (!msg)
		return -1;

	dbus_error_init(&dbh->err);

	reply = dbus_connection_send_with_reply_and_block(dbh->conn, msg, -1, &dbh->err);

	if (!reply)
		goto out;

	if (dbus_message_get_args(reply, &dbh->err,
				  DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID)) {
		retval = strlen(path);
		*adapter = (char *)malloc(retval + 1);
		if (*adapter) {
			strcpy(*adapter, path);
			retval = -1;
		}
	} else {
		if (dbus_error_is_set(&dbh->err)) {
			HAGGLE_ERR("DBus error %s %s\n", dbh->err.name, dbh->err.message);
			dbus_error_free(&dbh->err);
		}
		retval = -1;
	}
	dbus_message_unref(reply);
out:
	dbus_message_unref(msg);

	return retval;
}

static InterfaceRef dbus_bluetooth_get_interface(struct dbus_handle *dbh, const char *objectpath)
{
	DBusMessage *msg, *reply;
	DBusMessageIter iter, dict;
	InterfaceRef iface = NULL;
	const char *interface_name = NULL, *interface_mac = NULL, *device_name = NULL;

	/* 
	   Set the device name by looking for the last occurence of a '/' in the
	   object path, which looks like /org/bluez/<daemon pid>/hciX
	*/

	device_name = dbus_get_interface_name_from_object_path(objectpath);

	if (!device_name)
		return NULL;

	msg = dbus_message_new_method_call("org.bluez", objectpath, "org.bluez.Adapter", "GetProperties");
	
	if (!msg) {
		HAGGLE_ERR("%Can't allocate new method call for GetProperties!\n");
		return NULL;
	}
	
	dbus_error_init(&dbh->err);

	reply = dbus_connection_send_with_reply_and_block(dbh->conn, msg, -1, &dbh->err);
	
	dbus_message_unref(msg);
	
	if (!reply) {
		if (dbus_error_is_set(&dbh->err)) {
			HAGGLE_ERR("D-Bus error: %s (%s)\n", dbh->err.name, dbh->err.message);
			dbus_error_free(&dbh->err);
		} 
		return NULL;
	}
	
	dbus_message_iter_init(reply, &iter);

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
		goto failure;

	dbus_message_iter_recurse(&iter, &dict);

	do {
		int type = 0;
		DBusMessageIter dict_entry, prop_val;
		char *property = NULL;

		if (dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_DICT_ENTRY)
			continue;

		dbus_message_iter_recurse(&dict, &dict_entry);
				
		if (dbus_message_iter_get_arg_type(&dict_entry) != DBUS_TYPE_STRING) {
			HAGGLE_ERR("Unexpected type %d in iterator\n", dbus_message_iter_get_arg_type(&dict_entry));
			continue;
		}
		
		dbus_message_iter_get_basic(&dict_entry, &property);
				
		if (!dbus_message_iter_next(&dict_entry))
			continue;
		
		if (dbus_message_iter_get_arg_type(&dict_entry) != DBUS_TYPE_VARIANT)
			continue;

		dbus_message_iter_recurse(&dict_entry, &prop_val);
		
		type = dbus_message_iter_get_arg_type(&prop_val);

		if (strcmp("Address", property) == 0 && type == DBUS_TYPE_STRING) {
			dbus_message_iter_get_basic(&prop_val, &interface_mac);
			//HAGGLE_DBG("Mac address is %s\n", interface_mac);
		} else if (strcmp("Name", property) == 0 && type == DBUS_TYPE_STRING) {
			dbus_message_iter_get_basic(&prop_val, &interface_name);
			//HAGGLE_DBG("Mac address is %s\n", interface_name);
		}
	} while (dbus_message_iter_next(&dict));
	
	if (device_name && interface_mac && interface_name) {
		bdaddr_t *mac = strtoba(interface_mac);				
		
		BluetoothAddress addr((unsigned char *)mac);
		iface = Interface::create<BluetoothInterface>(mac, device_name, addr, IFFLAG_LOCAL | IFFLAG_UP);
		free(mac);
	}
failure:
	dbus_message_unref(reply);
	
	return iface;
}

DBusHandlerResult dbus_handler(DBusConnection *conn, DBusMessage *msg, void *data)
{
	ConnectivityLocalLinux *cl = static_cast < ConnectivityLocalLinux * >(data);
	DBusError *err = &cl->dbh.err;
	/*
	  Parsing of these messages rely on the Bluez 4 API.
	 */
	dbus_error_init(err);

	if (dbus_message_is_signal(msg, "org.bluez.Manager", "AdapterAdded")) {
		char *adapter = NULL;
		if (dbus_message_get_args(msg, err,
					  DBUS_TYPE_OBJECT_PATH, &adapter, DBUS_TYPE_INVALID)) {
			dbus_bluetooth_set_adapter_property_boolean(&cl->dbh, adapter, "Discoverable", true);
			dbus_bluetooth_set_adapter_property_integer(&cl->dbh, adapter, "DiscoverableTimeout", 0);
		
			InterfaceRef iface = dbus_bluetooth_get_interface(&cl->dbh, adapter);
			
			if (iface) {				
				cl->report_interface(iface, cl->rootInterface, new ConnectivityInterfacePolicyAgeless());
			}
		} else if (dbus_error_is_set(err)) {
			HAGGLE_ERR("D-Bus error: %s (%s)\n", err->name, err->message);
			dbus_error_free(err);
		}
	} else if (dbus_message_is_signal(msg, "org.bluez.Manager", "AdapterRemoved")) {
		char *adapter = NULL;
		if (dbus_message_get_args(msg, err,
					  DBUS_TYPE_OBJECT_PATH, &adapter, DBUS_TYPE_INVALID)) {
			cl->delete_interface(dbus_get_interface_name_from_object_path(adapter));
		} else if (dbus_error_is_set(err)) {
			HAGGLE_ERR("D-Bus error: %s (%s)\n", err->name, err->message);
			dbus_error_free(err);
		}
	} else if (dbus_message_is_signal(msg, "org.bluez.Adapter", "PropertyChanged")) {
		DBusMessageIter iter;
                const char *property = NULL;
		const char *adapter = NULL;	
		const char *ifname = NULL;
		
		adapter = dbus_message_get_path(msg);
		ifname = dbus_get_interface_name_from_object_path(adapter);
		
		if (!ifname || !adapter)
			return DBUS_HANDLER_RESULT_HANDLED;

		//CM_DBG("D-Bus: PropertyChanged for %s ifname %s\n", adapter, ifname);

                dbus_message_iter_init(msg, &iter);

		/* Check for change in Discoverable and DiscoverableTimeout */
		do {
			int type;
			DBusMessageIter prop_val;

			if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
				break;
			
			dbus_message_iter_get_basic(&iter, &property);
			
			if (!dbus_message_iter_next(&iter))
				break;
			
			if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT)
				break;
			
			dbus_message_iter_recurse(&iter, &prop_val);
			
			type = dbus_message_iter_get_arg_type(&prop_val);
			
			if (strcmp("Discoverable", property) == 0 && type == DBUS_TYPE_BOOLEAN) {
				int discoverable = 0;
				dbus_message_iter_get_basic(&prop_val, &discoverable);
				HAGGLE_DBG("Discoverable on interface %s is set to %s\n", 
					   ifname, discoverable ? "true" : "false");
				if (discoverable == 0) {
					/* Force discoverability */
					HAGGLE_DBG("Forcing discoverable mode on interface %s\n", ifname);
					dbus_bluetooth_set_adapter_property_boolean(&cl->dbh, adapter, "Discoverable", true);
				}
			} else if (strcmp("DiscoverableTimeout", property) == 0 && type == DBUS_TYPE_UINT32) {
				uint32_t timeout = 0;
				dbus_message_iter_get_basic(&prop_val, &timeout);
				HAGGLE_DBG("New Discoverable timeout for interface %s is %u\n", 
					   ifname, timeout);

				if (timeout != 0) {
					/* Force timeout to infinity */
					HAGGLE_DBG("Forcing infinite discoverabilty timeout on interface %s\n", ifname);
					dbus_bluetooth_set_adapter_property_integer(&cl->dbh, adapter, "DiscoverableTimeout", 0);
				}
			}	
		} while (0);
		
	} 
	return DBUS_HANDLER_RESULT_HANDLED;
}

int dbus_hci_adapter_removed_watch(struct dbus_handle *dbh, void *data)
{
	int ret = -1;

	if (!dbus_connection_add_filter(dbh->conn, dbus_handler, (void *) data, NULL))
		return -1;

	dbus_error_init(&dbh->err);

	dbus_bus_add_match(dbh->conn, "type='signal',interface='org.bluez.Manager',member='AdapterRemoved'", &dbh->err);

	if (dbus_error_is_set(&dbh->err))
		dbus_error_free(&dbh->err);
	else
		ret = 0;

	return ret;
}


int dbus_hci_property_changed_watch(struct dbus_handle *dbh, void *data)
{
	int ret = -1;
	
	if (!dbus_connection_add_filter(dbh->conn, dbus_handler, (void *) data, NULL))
		return -1;

	dbus_error_init(&dbh->err);
	
	dbus_bus_add_match(dbh->conn, "type='signal',interface='org.bluez.Adapter',member='PropertyChanged'", &dbh->err);
	dbus_bus_add_match(dbh->conn, "type='signal',interface='org.bluez.Manager',member='AdapterAdded'", &dbh->err);
	dbus_bus_add_match(dbh->conn, "type='signal',interface='org.bluez.Manager',member='AdapterRemoved'", &dbh->err);

	if (dbus_error_is_set(&dbh->err))
		dbus_error_free(&dbh->err);
	else
		ret = 0;

	return ret;
}

static int dbus_init_handle(struct dbus_handle *dbh)
{
	if (!dbh)
		return -1;

	dbus_error_init(&dbh->err);
	
#if defined(OS_ANDROID)
	dbus_threads_init_default();
#endif
	dbh->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbh->err);
	
	if (dbus_error_is_set(&dbh->err)) {
		HAGGLE_ERR("D-Bus Connection Error (%s)\n", dbh->err.message);
		dbus_error_free(&dbh->err);
		return -1;
	}

	dbus_connection_set_exit_on_disconnect(dbh->conn, FALSE);

	dbus_connection_flush(dbh->conn);

	return 0;
}

#endif

#if defined(ENABLE_BLUETOOTH)

#if defined(OS_ANDROID)
// Code taken from hciconfig

static void print_dev_hdr(struct hci_dev_info *di)
{
	static int hdr = -1;
	char addr[18];

	if (hdr == di->dev_id)
		return;
	hdr = di->dev_id;

	ba2str(&di->bdaddr, addr);

	printf("%s:\tType: %s\n", di->name, hci_dtypetostr(di->type) );
	printf("\tBD Address: %s ACL MTU: %d:%d SCO MTU: %d:%d\n",
		addr, di->acl_mtu, di->acl_pkts,
		di->sco_mtu, di->sco_pkts);
}

static int bluetooth_page_timeout(int ctl, int hdev, const char *opt)
{
	struct hci_request rq;
	int s;

	if ((s = hci_open_dev(hdev)) < 0) {
		fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
			hdev, strerror(errno), errno);
		return -1;
	}

	memset(&rq, 0, sizeof(rq));

	if (opt) {
		unsigned int timeout;
		write_page_timeout_cp cp;

		if (sscanf(opt,"%5u", &timeout) != 1) {
			printf("Invalid argument format\n");
			return -1;
		}

		rq.ogf = OGF_HOST_CTL;
		rq.ocf = OCF_WRITE_PAGE_TIMEOUT;
		rq.cparam = &cp;
		rq.clen = WRITE_PAGE_TIMEOUT_CP_SIZE;

		cp.timeout = htobs((uint16_t) timeout);

		if (timeout < 0x01 || timeout > 0xFFFF)
			printf("Warning: page timeout out of range!\n");

		if (hci_send_req(s, &rq, 2000) < 0) {
			fprintf(stderr, "Can't set page timeout on hci%d: %s (%d)\n",
				hdev, strerror(errno), errno);
			return -1;
		}
	} else {
		uint16_t timeout;
		read_page_timeout_rp rp;
		
		rq.ogf = OGF_HOST_CTL;
		rq.ocf = OCF_READ_PAGE_TIMEOUT;
		rq.rparam = &rp;
		rq.rlen = READ_PAGE_TIMEOUT_RP_SIZE;
		
		if (hci_send_req(s, &rq, 1000) < 0) {
			fprintf(stderr, "Can't read page timeout on hci%d: %s (%d)\n",
				hdev, strerror(errno), errno);
			return -1;
		}
		if (rp.status) {
			printf("Read page timeout on hci%d returned status %d\n",
			       hdev, rp.status);
			return -1;
		}
		//print_dev_hdr(&di);
		
		timeout = btohs(rp.timeout);
		printf("\tPage timeout: %u slots (%.2f ms)\n",
		       timeout, (float)timeout * 0.625);
	}
	return 0;
}

static int bluetooth_page_parms(int ctl, int hdev, const char *opt)
{
	struct hci_request rq;
	int s;

	if ((s = hci_open_dev(hdev)) < 0) {
		fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
		return -1;
	}

	memset(&rq, 0, sizeof(rq));

	if (opt) {
		unsigned int window, interval;
		write_page_activity_cp cp;

		if (sscanf(opt,"%4u:%4u", &window, &interval) != 2) {
			printf("Invalid argument format\n");
			return -1;
		}

		rq.ogf = OGF_HOST_CTL;
		rq.ocf = OCF_WRITE_PAGE_ACTIVITY;
		rq.cparam = &cp;
		rq.clen = WRITE_PAGE_ACTIVITY_CP_SIZE;

		cp.window = htobs((uint16_t) window);
		cp.interval = htobs((uint16_t) interval);

		if (window < 0x12 || window > 0x1000)
			printf("Warning: page window out of range!\n");

		if (interval < 0x12 || interval > 0x1000)
			printf("Warning: page interval out of range!\n");

		if (hci_send_req(s, &rq, 2000) < 0) {
			fprintf(stderr, "Can't set page parameters name on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
			return -1;
		}
	} else {
		uint16_t window, interval;
		read_page_activity_rp rp;

		rq.ogf = OGF_HOST_CTL;
		rq.ocf = OCF_READ_PAGE_ACTIVITY;
		rq.rparam = &rp;
		rq.rlen = READ_PAGE_ACTIVITY_RP_SIZE;

		if (hci_send_req(s, &rq, 1000) < 0) {
			fprintf(stderr, "Can't read page parameters on hci%d: %s (%d)\n",
						hdev, strerror(errno), errno);
			return -1;
		}
		if (rp.status) {
			printf("Read page parameters on hci%d returned status %d\n",
							hdev, rp.status);
			return -1;
		}
		//print_dev_hdr(&di);

		window   = btohs(rp.window);
		interval = btohs(rp.interval);
		printf("\tPage interval: %u slots (%.2f ms), window: %u slots (%.2f ms)\n",
				interval, (float)interval * 0.625, window, (float)window * 0.625);
	}
	return 0;
}

static int bluetooth_set_scan(int ctl, int hdev, const char *opt)
{
	struct hci_dev_req dr;

	dr.dev_id = hdev;
	dr.dev_opt = SCAN_DISABLED;
	
	if (!strcmp(opt, "iscan"))
		dr.dev_opt = SCAN_INQUIRY;
	else if (!strcmp(opt, "pscan"))
		dr.dev_opt = SCAN_PAGE;
	else if (!strcmp(opt, "piscan"))
		dr.dev_opt = SCAN_PAGE | SCAN_INQUIRY;
	
	if (ioctl(ctl, HCISETSCAN, (unsigned long) &dr) < 0) {
		fprintf(stderr, "Can't set scan mode on hci%d: %s (%d)\n",
                        hdev, strerror(errno), errno);
		return -1;
	}
	/*
	bluetooth_page_timeout(ctl, hdev, NULL);
	sleep(1);
	bluetooth_page_parms(ctl, hdev, NULL);
	*/
        return 0;
}
#endif // OS_ANDROID

Interface *hci_get_interface_from_name(const char *ifname)
{
	struct hci_dev_info di;
	char name[249];
	char macaddr[BT_ALEN];
	int dd, i;

	dd = hci_open_dev(0);

	if (dd < 0) {
		HAGGLE_ERR("Can't open device : %s (%d)\n", strerror(errno), errno);
		return NULL;
	}

	memset(&di, 0, sizeof(struct hci_dev_info));

        strcpy(di.name, ifname);

	if (ioctl(dd, HCIGETDEVINFO, (void *) &di) < 0) {
		CM_DBG("HCIGETDEVINFO failed for %s\n", ifname);
		return NULL;
	}

	/* The interface name, e.g., 'hci0' */
	baswap((bdaddr_t *) & macaddr, &di.bdaddr);

	/* Read local "hostname" */
	if (hci_read_local_name(dd, sizeof(name), name, 1000) < 0) {
		HAGGLE_ERR("Can't read local name on %s: %s (%d)\n", ifname, strerror(errno), errno);
		CM_DBG("Could not read adapter name for device %s\n", ifname);
		hci_close_dev(dd);
		return NULL;
	}
	hci_close_dev(dd);

	/* Process name */
	for (i = 0; i < 248 && name[i]; i++) {
		if ((unsigned char) name[i] < 32 || name[i] == 127)
			name[i] = '.';
	}

	name[248] = '\0';
	BluetoothAddress addr((unsigned char *)macaddr);
	
	return Interface::create<BluetoothInterface>(macaddr, ifname, addr, IFFLAG_LOCAL | IFFLAG_UP);
}

#if !defined(HAVE_DBUS)
static int hci_init_handle(struct hci_handle *hcih)
{
	if (!hcih)
		return -1;

	memset(hcih, 0, sizeof(struct hci_handle));

	/* Create HCI socket */
	hcih->sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);

	if (hcih->sock < 0) {
		HAGGLE_ERR("could not open HCI socket: %s\n",
			   strerror(errno));
		return -1;
	}

	/* Setup filter */
	hci_filter_clear(&hcih->flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &hcih->flt);

	hci_filter_set_event(EVT_STACK_INTERNAL, &hcih->flt);

	if (setsockopt(hcih->sock, SOL_HCI, HCI_FILTER, &hcih->flt, sizeof(hcih->flt)) < 0) {
		HAGGLE_ERR("Could not set HCI_FILTER: %s\n",
			   strerror(errno));
		close(hcih->sock);
		return -1;
	}

	/* Bind socket to the HCI device */
	hcih->addr.hci_family = AF_BLUETOOTH;
	hcih->addr.hci_dev = HCI_DEV_NONE;

	if (bind(hcih->sock, (struct sockaddr *) &hcih->addr, 
		 sizeof(struct sockaddr_hci)) < 0) {
		HAGGLE_ERR("Could not bind HCI sock: %s\n",
			   strerror(errno));
		close(hcih->sock);
		return -1;
	}

	HAGGLE_DBG("Opened HCI socket\n");

	return 0;
}

void hci_close_handle(struct hci_handle *hcih)
{
	if (hcih->sock > 0)
		close(hcih->sock);
}
#endif

int ConnectivityLocalLinux::read_hci()
{
	char buf[HCI_MAX_FRAME_SIZE];
	hci_event_hdr *hdr = (hci_event_hdr *) & buf[1];
	char *body = buf + HCI_EVENT_HDR_SIZE + 1;
	int len;
	u_int8_t type;

	len = read(hcih.sock, buf, HCI_MAX_FRAME_SIZE);

	type = buf[0];

	if (type != HCI_EVENT_PKT) {
		CM_DBG("Not HCI_EVENT_PKT type\n");
		return -1;
	}
	// STACK_INTERNAL events require root permissions
	if (hdr->evt == EVT_STACK_INTERNAL) {
		evt_stack_internal *si = (evt_stack_internal *) body;
		evt_si_device *sd = (evt_si_device *) & si->data;
                struct hci_dev_info di;
                Interface *iface;
                int ret;
                
                ret = hci_devinfo(sd->dev_id, &di);

                // TODO: Should check return value
                
		switch (sd->event) {
		case HCI_DEV_REG:
			HAGGLE_DBG("HCI dev %d registered\n", sd->dev_id);
			break;

		case HCI_DEV_UNREG:
			HAGGLE_DBG("HCI dev %d unregistered\n", sd->dev_id);
			break;

		case HCI_DEV_UP:
			HAGGLE_DBG("HCI dev %d up\n", sd->dev_id);
				
			iface = hci_get_interface_from_name(di.name);
				
			if (iface) {
#if defined(OS_ANDROID)
				set_piscan_mode = true;
				dev_id = sd->dev_id;
				// Force discoverable mode for Android device
				HAGGLE_DBG("Forcing piscan mode (discoverable)\n");
				/* 
				 Sleep a couple of seconds to let
				 the interface configure itself
				 before we try to set the piscan
				 mode. 
				 */
				sleep(3);
				if (bluetooth_set_scan(hcih.sock, sd->dev_id, "piscan") == -1) {
					fprintf(stderr, "Could not force discoverable mode for Bluetooth device %s\n", di.name);
				}
#endif
				report_interface(iface, rootInterface, new ConnectivityInterfacePolicyAgeless());
				delete iface;
			}
                                                
			break;

		case HCI_DEV_DOWN:
#if defined(OS_ANDROID)
			set_piscan_mode = false;
#endif
			HAGGLE_DBG("HCI dev %d down\n", sd->dev_id);
                        CM_DBG("Bluetooth interface %s down\n", di.name);
                        delete_interface(string(di.name));
			break;
		default:
			HAGGLE_DBG("HCI unrecognized event\n");
			break;
		}
	} else {
		CM_DBG("Unknown HCI event\n");
	}
	return 0;
}

#if defined(HAVE_DBUS)

void ConnectivityLocalLinux::findLocalBluetoothInterfaces()
{
	DBusMessage *reply, *msg;
	char **adapters = NULL;
	int len = 0;

	if (!dbh.conn) {
		CM_DBG("Invalid d-bus handle. Cannot detect Bluetooth interfaces\n");
		return;
	}

	dbus_error_init(&dbh.err);
	
	// TODO: ListAdapters is deprecated, should use GetProperties instead
	msg = dbus_message_new_method_call("org.bluez", "/", "org.bluez.Manager", "ListAdapters");

	if (!msg) {
		HAGGLE_ERR("%Can't allocate new method call for GetProperties!\n");
		return;
	}
	
	reply = dbus_connection_send_with_reply_and_block(dbh.conn, msg, -1, &dbh.err);

	dbus_message_unref(msg);
	
	if (!reply) {
		if (dbus_error_is_set(&dbh.err)) {
			HAGGLE_ERR("D-Bus error: %s (%s)\n", dbh.err.name, dbh.err.message);
			dbus_error_free(&dbh.err);
		}
		return;
	}
	
	if (dbus_message_get_args(reply, &dbh.err,
				  DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, 
				  &adapters, &len, DBUS_TYPE_INVALID)) {
		int i;
		
		for (i = 0; i < len; i++) {
			dbus_bluetooth_set_adapter_property_boolean(&dbh, adapters[i], 
								    "Discoverable", true);
			dbus_bluetooth_set_adapter_property_integer(&dbh, adapters[i], 
								    "DiscoverableTimeout", 0);
			
			InterfaceRef iface = dbus_bluetooth_get_interface(&dbh, adapters[i]);
			
			if (iface) {				
				report_interface(iface, rootInterface, 
						 new ConnectivityInterfacePolicyAgeless());
			}
		}
		dbus_free_string_array(adapters);
	} else {
		if (dbus_error_is_set(&dbh.err)) {
			HAGGLE_ERR("D-Bus error: %s (%s)\n", dbh.err.name, dbh.err.message);
			dbus_error_free(&dbh.err);
		} 
	}

	dbus_message_unref(reply);
	
	return;
}
#else
void ConnectivityLocalLinux::findLocalBluetoothInterfaces()
{
	int i, ret = 0;
	struct {
		struct hci_dev_list_req dl;
		struct hci_dev_req dr[HCI_MAX_DEV];
	} req;

	memset(&req, 0, sizeof(req));

	req.dl.dev_num = HCI_MAX_DEV;

	ret = ioctl(hcih.sock, HCIGETDEVLIST, (void *) &req);

	if (ret < 0) {
		CM_DBG("HCIGETDEVLIST failed\n");
		return;		// ret;
	}

	for (i = 0; i < req.dl.dev_num; i++) {
		struct hci_dev_info di;
		char devname[9];
		char name[249];
		char macaddr[BT_ALEN];
		int dd, hdev;

		memset(&di, 0, sizeof(struct hci_dev_info));

		di.dev_id = req.dr[i].dev_id;
		hdev = di.dev_id;

		ret = ioctl(hcih.sock, HCIGETDEVINFO, (void *) &di);

		if (ret < 0) {
			CM_DBG("HCIGETDEVINFO failed for dev_id=%d\n", req.dr[i].dev_id);
			return;	// ret;
		}
		/* The interface name, e.g., 'hci0' */
		strncpy(devname, di.name, 9);
		baswap((bdaddr_t *) & macaddr, &di.bdaddr);

		dd = hci_open_dev(hdev);

		if (dd < 0) {
			HAGGLE_ERR("Can't open device hci%d: %s (%d)\n", hdev, strerror(errno), errno);
			continue;
		}

		/* Read local "hostname" */
		if (hci_read_local_name(dd, sizeof(name), name, 1000) < 0) {
			HAGGLE_ERR("Can't read local name on %s: %s (%d)\n", devname, strerror(errno), errno);
			CM_DBG("Could not read adapter name for device %s\n", devname);
			hci_close_dev(dd);
			continue;
		}
		hci_close_dev(dd);

		/* Process name */
		for (i = 0; i < 248 && name[i]; i++) {
			if ((unsigned char) name[i] < 32 || name[i] == 127)
				name[i] = '.';
		}

		name[248] = '\0';
#if defined(OS_ANDROID)
                HAGGLE_DBG("Forcing piscan mode (discoverable)\n");

		dev_id = di.dev_id;
                set_piscan_mode = true;
		
                // Force discoverable mode for Android device
                if (bluetooth_set_scan(hcih.sock, hdev, "piscan") == -1) {
                        fprintf(stderr, "Could not force discoverable mode for Bluetooth device %s\n", devname);
                }
#endif
		BluetoothAddress addy((unsigned char *) macaddr);
		
		InterfaceRef iface = Interface::create<BluetoothInterface>(macaddr, 
									   name, 
									   addy, 
									   IFFLAG_LOCAL|IFFLAG_UP);
		
		if (iface) {
			report_interface(iface, rootInterface, 
					 new ConnectivityInterfacePolicyAgeless());
		}
	}
	return;
}
#endif // HAVE_DBUS

#if defined(OS_ANDROID)
static ConnectivityLocalLinux *cl = NULL;

void Java_org_haggle_kernel_BluetoothConnectivity_onBluetoothTurnedOn(JNIEnv *env, jobject obj,
								      jstring addr, jstring name)
{
        const char *mac = env->GetStringUTFChars(addr, NULL); 
        
        if (!mac)
                return;

	HAGGLE_DBG("Bluetooth adapter %s turned on\n", mac);

	if (cl)
		cl->findLocalBluetoothInterfaces();

        env->ReleaseStringUTFChars(addr, mac);
}

void Java_org_haggle_kernel_BluetoothConnectivity_onBluetoothTurnedOff(JNIEnv *env, jobject obj,
								       jstring addr, jstring name)
{
        const char *name_str = env->GetStringUTFChars(name, NULL); 
        
	if (!name_str) {
		HAGGLE_ERR("Could not get adapter name\n");
		return;
	}

	HAGGLE_DBG("Bluetooth adapter %s turned off\n", name_str);
	
	if (cl)
		cl->delete_interface(name_str);

        env->ReleaseStringUTFChars(name, name_str);
}

#endif

#endif // ENABLE_BLUETOOTH

void ConnectivityLocalLinux::hookCleanup()
{
#if defined(ENABLE_BLUETOOTH)
#if defined(HAVE_DBUS)
	dbus_watch_remove_all();
	dbus_close_handle(&dbh);
#else
	hci_close_handle(&hcih);
#endif // HAVE_DBUS

#endif // ENABLE_BLUETOOTH

#if defined(ENABLE_ETHERNET)
	nl_close_handle(&nlh);
#endif
}

bool ConnectivityLocalLinux::run()
{
	int ret;
	ConnectivityLocalLinux *cl;
#if defined(ENABLE_ETHERNET)

	ret = nl_init_handle(&nlh);

	if (ret < 0) {
		CM_DBG("Could not open netlink socket\n");
	}

	netlink_getaddr(&nlh);

#endif
#if defined(ENABLE_BLUETOOTH)

#if defined(HAVE_DBUS)
	ret = dbus_init_handle(&dbh);

	if (ret < 0) {
		CM_DBG("Could not open D-Bus connection\n");
	} else {
		
                if (dbus_hci_property_changed_watch(&dbh, this) < 0) {
			HAGGLE_ERR("Failed add dbus watch\n");
		}
                
		if (dbus_hci_adapter_removed_watch(&dbh, this) < 0) {
			HAGGLE_ERR("Failed add dbus watch\n");
		}

		dbus_connection_set_watch_functions(dbh.conn, dbus_watch_add, dbus_watch_remove, 
						    dbus_watch_toggle, (void *) this, NULL);
		dbus_connection_add_filter(dbh.conn, dbus_handler, this, NULL);
	}
#else
#if defined(OS_ANDROID)	
#define DISCOVERABLE_RESET_INTERVAL 5000
	Timeval wait_start = Timeval::now();;
	long to_wait = DISCOVERABLE_RESET_INTERVAL;
#endif

	cl = this;

	// Some events on this socket require root permissions. These
	// include adapter up/down events in read_hci()
	ret = hci_init_handle(&hcih);

	if (ret < 0) {
		CM_DBG("Could not open HCI socket\n");
	}
#endif // HAVE_DBUS
	
	/* Find any local Bluetooth interfaces. */
	findLocalBluetoothInterfaces();

#endif // ENABLE_BLUETOOTH

	while (!shouldExit()) {
		Watch w;

		w.reset();
		
#if defined(ENABLE_BLUETOOTH)
#if defined(HAVE_DBUS)
		for (List<watch_data *>::iterator it = dbusWatches.begin(); it != dbusWatches.end(); it++) {
			(*it)->watchIndex = w.add((*it)->fd);
                }
#else
		int hciIndex = w.add(hcih.sock);
#endif
#endif // ENABLE_BLUETOOTH

#if defined(ENABLE_ETHERNET)
		int nlhIndex = w.add(nlh.sock);
#endif

#if defined(OS_ANDROID) && defined(ENABLE_BLUETOOTH) && !defined(HAVE_DBUS)
                if (set_piscan_mode) {
			Timeval now = Timeval::now();
			long waited = (now - wait_start).getTimeAsMilliSeconds();
			
			to_wait = DISCOVERABLE_RESET_INTERVAL - waited;

			if (to_wait < 0)
				to_wait = 1;

			ret = w.waitTimeout(to_wait);
		} else
#endif
			ret = w.wait();
                
		if (ret == Watch::FAILED) {
			// Some error
			break;
		}
		if (ret == Watch::ABANDONED) {
			// We should exit
			break;
		}
		if (ret == Watch::TIMEOUT) {
#if defined(OS_ANDROID) && defined(ENABLE_BLUETOOTH) && !defined(HAVE_DBUS)
			/* 
			   The Android bluetooth daemon automatically
			 switches off Bluetooth discoverable mode
			 after 2 minutes, therefore we reset
			 discoverable mode at frequent intervals. We
			 could set an infinate discoverable timeout,
			 but that is only available in the dbus
			 Bluetooth API.
			*/
			
			//HAGGLE_DBG("Resetting Bluetooth piscan mode on interface id %d\n", dev_id);

			if (set_piscan_mode && bluetooth_set_scan(hcih.sock, dev_id, "piscan") == -1) {
				fprintf(stderr, "Could not force discoverable mode for Bluetooth device %d\n", dev_id);
			}

			wait_start = Timeval::now();
#endif
			continue;
		}
#if defined(ENABLE_ETHERNET)
		if (w.isSet(nlhIndex)) {
			read_netlink();
		}
#endif

#if defined(ENABLE_BLUETOOTH)
#if defined(HAVE_DBUS)
		bool doDispatch = false;
		for (List<watch_data *>::iterator it = dbusWatches.begin(); it != dbusWatches.end(); it++) {
			if (w.isSet((*it)->watchIndex)) {
				if (dbus_watch_get_flags((*it)->watch) & DBUS_WATCH_READABLE) {
					dbus_watch_handle((*it)->watch, DBUS_WATCH_READABLE);
					doDispatch = true;
				}
			}
		}
	
		if (doDispatch) {
			while (dbus_connection_dispatch(dbh.conn) == DBUS_DISPATCH_DATA_REMAINS) { }
		}
#else
		if (w.isSet(hciIndex)) {
			read_hci();
		}
#endif // HAVE_DBUS

#endif // ENABLE_BLUETOOTH
	}

#if defined(OS_ANDROID)
	cl = NULL;
#endif
	return false;
}

#endif
