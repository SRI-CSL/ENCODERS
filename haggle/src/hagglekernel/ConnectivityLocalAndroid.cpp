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

#if defined(OS_ANDROID)

#include "ConnectivityManager.h"
#include "ConnectivityLocalAndroid.h"
#include "ConnectivityBluetooth.h"
#include "ConnectivityEthernet.h"
#include "Interface.h"
#include "Utility.h"

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

// Android specific stuff
//#include <hardware_legacy/wifi.h>
#if defined(ENABLE_WPA_EVENTS)
#include <libwpa_client/wpa_ctrl.h>
#endif

#if defined(ENABLE_BLUETOOTH)
Interface *hci_get_interface_from_name(const char *ifname);
#endif

#if defined(ENABLE_ETHERNET)

static const char *blacklist_device_names[] = { "vmnet", "vmaster", "pan", "lo", "wifi", NULL };

static bool isBlacklistDeviceName(const char *devname)
{
	int i = 0;
        string dname = devname;

	while (blacklist_device_names[i]) {
		if (strncmp(blacklist_device_names[i], 
                            dname.c_str(), 
                            strlen(blacklist_device_names[i])) == 0)
			return true;
		i++;
	}

	return false;
}

static Interface *android_get_net_interface_info(const char *name, const unsigned char *mac)
{
        //unsigned char mac[6];
        unsigned int flags;
        struct in_addr addr;
	struct in_addr baddr;
        struct ifreq ifr;   
        int ret, s;
        Addresses addrs;
        
        s = socket(AF_INET, SOCK_DGRAM, 0);
        
        if (s < 0) {
            CM_DBG("socket() failed: %s\n", strerror(errno));
            return NULL;
        }

        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, name, IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = 0;

        /*
        ret = ioctl(s, SIOCGIFHWADDR, &ifr);

        if(ret < 0) {
                CM_DBG("Could not get mac address of interface %s\n", name);
                close(s);
                return NULL;
        }
        */

        addrs.add(new EthernetAddress(mac));

        if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
                addr.s_addr = 0;
        } else {
                addr.s_addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
        }

        if (ioctl(s, SIOCGIFBRDADDR, &ifr) < 0) {
                baddr.s_addr = 0;
        } else {
                baddr.s_addr = ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr;
                addrs.add(new IPv4Address(addr));
                addrs.add(new IPv4BroadcastAddress(baddr));
        }

        if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
                flags = 0;
        } else {
                flags = ifr.ifr_flags;
        }
        
        close(s);

        if (flags & IFF_UP)
                return Interface::create<EthernetInterface>(mac, name, addrs, IFFLAG_LOCAL | IFFLAG_UP);
       
        return Interface::create<EthernetInterface>(mac, name, addrs, IFFLAG_LOCAL);
}


ConnectivityLocalAndroid::ConnectivityLocalAndroid(ConnectivityManager *m) :
	ConnectivityLocal(m, "ConnectivityLocalAndroid")
{
}

ConnectivityLocalAndroid::~ConnectivityLocalAndroid()
{
}

#define	max(a,b) ((a) > (b) ? (a) : (b))
void ConnectivityLocalAndroid::findLocalEthernetInterfaces()
{
        if (!tih.iface) {
                tih.iface = android_get_net_interface_info(TI_WIFI_DEV_NAME, tih.own_addr);
                
                if (tih.iface) {
                        if (tih.iface->isUp())
                                report_interface(tih.iface, NULL, new ConnectivityInterfacePolicyAgeless());
                        else  {
                                delete tih.iface;
                                tih.iface = NULL;
                        }
                }
        }
}
#endif

#if defined(ENABLE_BLUETOOTH)

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
	BluetoothAddress addr((unsigned char *) macaddr);
	
	return Interface::create<BluetoothInterface>(macaddr, ifname, addr, IFFLAG_LOCAL | IFFLAG_UP);
}

static int hci_init_handle(struct hci_handle *hcih)
{
	if (!hcih)
		return -1;

	memset(hcih, 0, sizeof(struct hci_handle));

	/* Create HCI socket */
	hcih->sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);

	if (hcih->sock < 0) {
		HAGGLE_ERR("could not open HCI socket\n");
		return -1;
	}

	/* Setup filter */
	hci_filter_clear(&hcih->flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &hcih->flt);

	hci_filter_set_event(EVT_STACK_INTERNAL, &hcih->flt);

	if (setsockopt(hcih->sock, SOL_HCI, HCI_FILTER, &hcih->flt, sizeof(hcih->flt)) < 0) {
		return -1;
	}

	/* Bind socket to the HCI device */
	hcih->addr.hci_family = AF_BLUETOOTH;
	hcih->addr.hci_dev = HCI_DEV_NONE;

	if (bind(hcih->sock, (struct sockaddr *) &hcih->addr, sizeof(struct sockaddr_hci)) < 0) {
		return -1;
	}

	return 0;
}

void hci_close_handle(struct hci_handle *hcih)
{
	close(hcih->sock);
}

// Code taken from hciconfig
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
        return 0;
}

int ConnectivityLocalAndroid::read_hci()
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
                                set_piscan_mode = true;
                                dev_id = sd->dev_id;
                                // Force discoverable mode for Android device
                                HAGGLE_DBG("Forcing piscan mode (discoverable)\n");
                                /* Sleep a couple of seconds to let
                                 * the interface configure itself
                                 * before we try to set the piscan
                                 * mode. */
                                sleep(3);
                                if (bluetooth_set_scan(hcih.sock, sd->dev_id, "piscan") == -1) {
                                        fprintf(stderr, "Could not force discoverable mode for Bluetooth device %s\n", di.name);
                                }
                                report_interface(iface, NULL, new ConnectivityInterfacePolicyAgeless());
                                delete iface;
                        }
			break;

		case HCI_DEV_DOWN:
			HAGGLE_DBG("HCI dev %d down\n", sd->dev_id);
                        set_piscan_mode = false;
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

// Finds local bluetooth interfaces:
void ConnectivityLocalAndroid::findLocalBluetoothInterfaces()
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
		
                HAGGLE_DBG("Forcing piscan mode (discoverable)\n");

                // Force discoverable mode for Android device
                if (bluetooth_set_scan(hcih.sock, hdev, "piscan") == -1) {
                        fprintf(stderr, "Could not force discoverable mode for Bluetooth device %s\n", devname);
                }
                
		BluetoothAddress addr((unsigned char *) macaddr);
		
		InterfaceRef iface = Interface::create<BluetoothInterface>(macaddr, devname, addr, IFFLAG_LOCAL | IFFLAG_UP);
                                        
		if (iface) {
			report_interface(iface, NULL, new ConnectivityInterfacePolicyAgeless());
		}

                set_piscan_mode = true;
                dev_id = hdev;


	}
	return;
}
#endif

#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>


/* Returns -1 on failure, 0 on success */
int ConnectivityLocalAndroid::uevent_init()
{
        struct sockaddr_nl addr;
        int sz = 64*1024;
        int s;

        memset(&addr, 0, sizeof(addr));
        addr.nl_family = AF_NETLINK;
        addr.nl_pid = getpid();
        addr.nl_groups = 0xffffffff;

        uevent_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    
        if (uevent_fd < 0)
                return -1;

        setsockopt(uevent_fd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

        if (bind(uevent_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
                close(uevent_fd);
                return -1;
        }

        return (uevent_fd > 0) ? 0 : -1;
}

void ConnectivityLocalAndroid::uevent_close()
{
        if (uevent_fd > 0)
                close(uevent_fd);
}

#define UEVENT_NET_DEVICE_REMOVE "remove@/devices/virtual/net"
#define UEVENT_NET_DEVICE_ADD "add@/devices/virtual/net"

void ConnectivityLocalAndroid::read_uevent()
{
        char buffer[1024];
        int buffer_length = sizeof(buffer);
        
        int count = recv(uevent_fd, buffer, buffer_length, 0);
        
        if (count <= 0) {
                CM_DBG("Could not read uevent\n");
                return;
        }
        
        CM_DBG("UEvent: %s\n", buffer);

        if (strncmp(UEVENT_NET_DEVICE_ADD, buffer, strlen(UEVENT_NET_DEVICE_ADD)) == 0) {
                char *ifname = buffer + strlen(UEVENT_NET_DEVICE_ADD) + 1;
                CM_DBG("Device %s was added\n", ifname);

                ti_wifi_handle_init();
        } else if (strncmp(UEVENT_NET_DEVICE_REMOVE, buffer, strlen(UEVENT_NET_DEVICE_REMOVE)) == 0) {
                char *ifname = buffer + strlen(UEVENT_NET_DEVICE_REMOVE) + 1;
                CM_DBG("Device %s was removed\n", ifname);
                
                if (tih.iface) {
                        delete_interface(tih.iface->getName());
                }
                        
                ti_wifi_handle_close();
        } 
}
#if defined(ENABLE_TI_WIFI)

void ConnectivityLocalAndroid::ti_wifi_event_handle(IPC_EV_DATA *pData)
{
        int res, msg_size;

        msg_size = (int)(pData->uBufferSize + offsetof(IPC_EV_DATA, uBuffer));

        switch (pData->EvParams.uEventType) {
                case IPC_EVENT_ASSOCIATED:
                        CM_DBG("TI Event 'associated'\n");
                        break;
                case IPC_EVENT_DISASSOCIATED:
                        CM_DBG("TI Event 'disassociated'\n");
                        if (tih.iface) {
                                delete_interface(tih.iface->getName());
                                delete tih.iface;
                                tih.iface = NULL;
                        }
                        break;
                case IPC_EVENT_SCAN_COMPLETE:
                        //CM_DBG("TI Event 'scan complete'\n");
                        break;
                case IPC_EVENT_AUTH_SUCC:
                        CM_DBG("TI Event 'authentication success'\n");
                        if (!tih.iface) {
                                // Ugly, ugly hack to make sure the interface is up:
                                // We sleep for some time to let the DHCP daemon configure the
                                // interface. 
                                // TODO: We should try to figure out a way to get some event
                                // for when the interface is properly configured.

                                for (int i = 0; i < 3; i++) {
                                        sleep(5);

                                        tih.iface = android_get_net_interface_info(TI_WIFI_DEV_NAME, tih.own_addr);
                                        if (tih.iface->getAddress<IPv4Address>() == NULL) {
                                                CM_DBG("Interface %s has no IPv4 address yet... trying later\n", tih.iface->getName());
                                                 delete tih.iface;
                                                 tih.iface = NULL;
                                                 continue;
                                        }
                                        if (tih.iface) {
                                                if (tih.iface->isUp())
                                                        report_interface(tih.iface, NULL, new ConnectivityInterfacePolicyAgeless());
                                                else {
                                                        CM_DBG("TI interface not up after association\n");
                                                        delete tih.iface;
                                                        tih.iface = NULL;
                                                }
                                                break;
                                        }
                                }
                        }
                        break;
                case IPC_EVENT_CCKM_START:
                        CM_DBG("TI Event 'CCKM start'\n");
                        break;
                default:
                        //CM_DBG("TI Event type=%d\n", pData->EvParams.uEventType);
                        break;
        }
}

void ti_wifi_event_receive(IPC_EV_DATA *pData)
{
    ConnectivityLocalAndroid *cl;
    cl = (ConnectivityLocalAndroid *)pData->EvParams.hUserParam;
    cl->ti_wifi_event_handle(pData);
}

int ConnectivityLocalAndroid::ti_wifi_try_get_local_mac(unsigned char *mac)
{
        OS_802_11_MAC_ADDRESS hwaddr;
        tiINT32 res;

        if (!tih.h_adapter)
                return -1;

        memset(&hwaddr, 0, sizeof(OS_802_11_MAC_ADDRESS));
        
        res = TI_GetCurrentAddress(tih.h_adapter, &hwaddr);
        
        if (res != TI_RESULT_OK)
                return -1;

        memcpy(mac, &hwaddr, 6);
        
        CM_DBG("mac of tiwlan0 is %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac[0],
               mac[1],
               mac[2],
               mac[3],
               mac[4],
               mac[5]);

        return 0;
}

int ConnectivityLocalAndroid::ti_wifi_handle_init()
{
        IPC_EVENT_PARAMS pEvent;
        int i;
        
        memset(&tih, 0, sizeof(struct ti_wifi_handle));

        tih.h_adapter = TI_AdapterInit((tiCHAR *)wifi_iface_name);

        if (tih.h_adapter == NULL)
                return -1;

        if (ti_wifi_try_get_local_mac(tih.own_addr) < 0)
                return -1;

        memset(tih.h_events, 0, sizeof(ULONG) * IPC_EVENT_MAX);

        for (i = IPC_EVENT_ASSOCIATED; i < IPC_EVENT_MAX; i++) {
                pEvent.uEventType = i;
                pEvent.uDeliveryType = DELIVERY_PUSH;
                pEvent.hUserParam  = (TI_HANDLE)this;
                pEvent.pfEventCallback = (TI_EVENT_CALLBACK)ti_wifi_event_receive;
                
                TI_RegisterEvent(tih.h_adapter, &pEvent);
                tih.h_events[i] = pEvent.uEventID;
        }

        return 0;
}

void ConnectivityLocalAndroid::ti_wifi_handle_close()
{
        if (tih.h_adapter) {
                IPC_EVENT_PARAMS pEvent;
                int i;
                
                memset(&pEvent, 0, sizeof(pEvent));

                for (i = IPC_EVENT_ASSOCIATED; i < IPC_EVENT_MAX; i++) {
                        pEvent.uEventType = i;
                        pEvent.uEventID = tih.h_events[i];
                        TI_UnRegisterEvent(tih.h_adapter, &pEvent);
                }
                
                TI_AdapterDeinit(tih.h_adapter);

        }
        if (tih.iface) {
                delete tih.iface;
                tih.iface = NULL;
        }
}
#endif


#if defined(ENABLE_WPA_EVENTS)
void ConnectivityLocalAndroid::read_wpa_event()
{
        int res;
        char buffer[256];
        size_t nread = sizeof(buffer) - 1;
        
        if (!wpah || wpa_ctrl_pending(wpah) != 1)
                return;

        res = wpa_ctrl_recv(wpah, buffer, &nread);

        if (res < 0) {
                CM_DBG("Could not read wpa event\n");
                return;
        }
        
        // Make sure the message is null terminated
        buffer[nread] = '\0';

        CM_DBG("wpa_event: %s\n", buffer);
}

int ConnectivityLocalAndroid::wpa_handle_init()
{ 

        char ifname[256];
        char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
#define IFACE_DIR "/data/system/wpa_supplicant"

        /* Make sure supplicant is running */
        if (!property_get("init.svc.wpa_supplicant", supp_status, NULL)
            || strcmp(supp_status, "running") != 0) {
                CM_DBG("Supplicant not running, cannot connect\n");
                return -1;
        }
        

        if (access(IFACE_DIR, F_OK) == 0) {
                snprintf(ifname, sizeof(ifname), "%s/%s", IFACE_DIR, wifi_iface_name);
        } else {
                strlcpy(ifname, wifi_iface_name, sizeof(ifname));
        }

        wpah = wpa_ctrl_open(ifname);
    
        if (!wpah)
                return -1;
        
        if (wpa_ctrl_attach(wpah) != 0) {
                wpa_ctrl_close(wpah);
                wpah = NULL;
                return -1;
        }
        return 0;
}

void ConnectivityLocalAndroid::wpa_handle_close()
{
        if (!wpah)
                return;

         wpa_ctrl_close(wpah);
         wpah = NULL;
}
#endif /* ENABLE_WPA_EVENTS */

void ConnectivityLocalAndroid::hookCleanup()
{
#if defined(ENABLE_BLUETOOTH)
	hci_close_handle(&hcih);
#endif

#if defined(ENABLE_ETHERNET)
        uevent_close();
#if defined(ENABLE_TI_WIFI)
        ti_wifi_handle_close();
#endif
#endif

#if defined(ENABLE_WPA_EVENTS)
        wpa_handle_close();
#endif
}

bool ConnectivityLocalAndroid::run()
{
	int ret;

        // Read the WiFi interface name from the Android property manager
        property_get("wifi.interface", wifi_iface_name, "sta");

#if defined(ENABLE_ETHERNET)
#if defined(ENABLE_TI_WIFI)
        if (ti_wifi_handle_init() < 0) {
                CM_DBG("Could not get TI WiFi handle\n");
        }
        
#endif
        findLocalEthernetInterfaces();

#endif

#if defined(ENABLE_WPA_EVENTS)
        if (wpa_handle_init() < 0) {
                CM_DBG("Could not get supplicant handle\n");
        }
#endif
#if defined(ENABLE_BLUETOOTH)
#define DISCOVERABLE_RESET_INTERVAL 60000
	Timeval wait_start = Timeval::now();;
	long to_wait = DISCOVERABLE_RESET_INTERVAL;
        set_piscan_mode = false;

	// Some events on this socket require root permissions. These
	// include adapter up/down events in read_hci()
	ret = hci_init_handle(&hcih);

	if (ret < 0) {
		CM_DBG("Could not open HCI socket\n");
                return false;
	}

	findLocalBluetoothInterfaces();
#endif
        ret = uevent_init();

        if (ret == -1){
		CM_DBG("Could not open uevent\n");
                return false;
	}

	while (!shouldExit()) {
		Watch w;

		w.reset();
		
#if defined(ENABLE_BLUETOOTH)
		int hciIndex = w.add(hcih.sock);
#endif

                int ueventIndex = w.add(uevent_fd);


#if defined(ENABLE_WPA_EVENTS)
                int wpaIndex = -1;
                
                if (wpah)
                        wpaIndex = w.add(wpa_ctrl_get_fd(wpah));
#endif

#if defined(ENABLE_BLUETOOTH)

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
		// This should not happen since we do not have a timeout set
		if (ret == Watch::TIMEOUT) {
                        /* Android automatically switches off
                         * Bluetooth discoverable mode after 2
                         * minutes, therefore we reset it here every
                         * minute until we find a better solution. */
                        HAGGLE_DBG("Resetting Bluetooth piscan mode on interface id %d\n", dev_id);
                        if (set_piscan_mode && bluetooth_set_scan(hcih.sock, dev_id, "piscan") == -1) {
                                fprintf(stderr, "Could not force discoverable mode for Bluetooth device %d\n", dev_id);
                        }
			wait_start = Timeval::now();
                        continue;
                }

#if defined(ENABLE_BLUETOOTH)
		if (w.isSet(hciIndex)) {
			read_hci();
		}
#endif
                if (w.isSet(ueventIndex)) {
                        read_uevent();
                }
                
#if defined(ENABLE_WPA_EVENTS)
                if (wpah && w.isSet(wpaIndex)) {
                        read_wpa_event();
                }
#endif
	}
	return false;
}

#endif /* OS_ANDROID */
