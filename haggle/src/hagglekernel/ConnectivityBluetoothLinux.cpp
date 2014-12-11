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

#if defined(OS_LINUX) && defined(ENABLE_BLUETOOTH)

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <sys/types.h>
#include <sys/socket.h>

// For netlink
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <linux/if.h>
// #include <linux/if_addr.h>
// #include <linux/if_link.h>
#include <linux/if_ether.h>
#include <errno.h>

//  ICMP
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include "ProtocolRFCOMM.h"
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define MAX_BT_RESPONSES 255

#include "ConnectivityBluetooth.h"

#include "ProtocolRFCOMM.h"
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <haggleutils.h>

bdaddr_t bdaddr_any  = {{0, 0, 0, 0, 0, 0}};
bdaddr_t bdaddr_local = {{0, 0, 0, 0xff, 0xff, 0xff}};

//Start haggle service:
/* Parts of this function are based on code from Bluez http://www.bluez.org/, 
which are licensed under the GPL. */
static int add_service(sdp_session_t *session, uint32_t *handle)
{
	int ret = 0;
	unsigned char service_uuid_int[] = HAGGLE_BLUETOOTH_SDP_UUID;
	uint8_t rfcomm_channel = RFCOMM_DEFAULT_CHANNEL;
	const char *service_name = "Haggle";
	const char *service_dsc = "A community oriented communication framework";
	const char *service_prov = "haggleproject.org";

	uuid_t root_uuid; 
	uuid_t rfcomm_uuid, l2cap_uuid, svc_uuid;
	sdp_list_t *root_list;
	sdp_list_t *rfcomm_list = 0, *l2cap_list = 0, 
		*proto_list = 0, *access_proto_list = 0, 
		*service_list = 0;
	sdp_data_t *channel = 0;
	sdp_record_t *rec;
	// connect to the local SDP server, register the service
	// record, and disconnect

	if (!session) {
		HAGGLE_DBG("Bad local SDP session\n");
		return -1;
	}
	rec = sdp_record_alloc();

	// set the general service ID
	sdp_uuid128_create(&svc_uuid, &service_uuid_int);
	service_list = sdp_list_append(0, &svc_uuid);
	sdp_set_service_classes(rec, service_list);
	sdp_set_service_id(rec, svc_uuid);

	// make the service record publicly browsable
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP); 
	root_list = sdp_list_append(0, &root_uuid); 
	sdp_set_browse_groups(rec, root_list);
	
	// set l2cap information
	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	l2cap_list = sdp_list_append(0, &l2cap_uuid);
	proto_list = sdp_list_append(0, l2cap_list);

	// set rfcomm information
	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	rfcomm_list = sdp_list_append(0, &rfcomm_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &rfcomm_channel);
	sdp_list_append(rfcomm_list, channel);
	sdp_list_append(proto_list, rfcomm_list);

	// attach protocol information to service record
	access_proto_list = sdp_list_append(0, proto_list);
	sdp_set_access_protos(rec, access_proto_list);

	// set the name, provider, and description
	sdp_set_info_attr(rec, service_name, service_prov, service_dsc);

	ret = sdp_record_register(session, rec, 0);
	
	if (ret < 0) {
		HAGGLE_DBG("Service registration failed\n");
	} else {
		*handle = rec->handle;
	}

	// cleanup
	sdp_data_free(channel);
	sdp_list_free(l2cap_list, 0);	
	sdp_list_free(rfcomm_list, 0);
	sdp_list_free(root_list, 0);
	sdp_list_free(proto_list, 0);
	sdp_list_free(access_proto_list, 0);
	sdp_list_free(service_list, 0);

	sdp_record_free(rec);

	return ret;
}

static int del_service(sdp_session_t *session, uint32_t handle)
{
	sdp_record_t *rec;
	
	CM_DBG("Deleting Service Record.\n");
	
	if (!session) {
		CM_DBG("Bad local SDP session!\n");
		return -1;
	}
	
	rec = sdp_record_alloc();
	
	if (rec == NULL) {
		return -1;
	}
	
	rec->handle = handle;

	if (sdp_device_record_unregister(session, &bdaddr_local, rec) != 0) {
		/* 
		 If Bluetooth is shut off, the sdp daemon will not be
		 running and it is therefore common that this function
		 will fail in that case. This is fine since the record
		 is removed when the daemon shuts down. We only have
		 to free our record handle here then....
		 */
		sdp_record_free(rec);
		return -1;
	}

	CM_DBG("Service Record deleted.\n");

	return 0;
}

#if defined(BLUETOOTH_SERVICE_SCAN_DEBUG)
/* 
The code for printing services here is taken from 
http://www.autistici.org/eazy/source/bluescan.c (eazy@ondaquadra.org).
Credits are due.
*/

#define MAXLEN 2056

void print_profile(void *data, void *u){
	sdp_profile_desc_t *d = (sdp_profile_desc_t *)data;
	char uuid[MAX_LEN_UUID_STR+1];
	char profile_uuid[MAX_LEN_PROFILEDESCRIPTOR_UUID_STR+1];
	char *buf = (char *)u;
	
	memset(uuid, 0, MAX_LEN_UUID_STR+1);
	memset(profile_uuid, 0, MAX_LEN_PROFILEDESCRIPTOR_UUID_STR+1);
	
	sdp_uuid2strn(&d->uuid, uuid, MAX_LEN_UUID_STR);
	sdp_profile_uuid2strn(&d->uuid, profile_uuid, 
			      MAX_LEN_PROFILEDESCRIPTOR_UUID_STR);

	snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
		 "\t%s (0x%s)\n", profile_uuid, uuid);

	if(d->version){
		snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
			 "\tVersion: 0x%04x\n", d->version);
	}
}

void print_lang(void *data, void *u){
	sdp_lang_attr_t *lang = (sdp_lang_attr_t *)data;
	char *buf = (char *)u;

	snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
		 "\tcode_ISO639: 0x%02x\n", lang->code_ISO639);
	snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
		 "\tencoding:    0x%02x\n", lang->encoding);
	snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
		 "\tbase_offset: 0x%02x\n", lang->base_offset);
}

void print_proto_desc(void *data, void *u){
	int proto = 0, i = 0;
	char uuid[MAX_LEN_UUID_STR+1];
	char proto_uuid[MAX_LEN_PROTOCOL_UUID_STR+1];
	char *buf = (char *)u;
	sdp_data_t *d;
	
	if(!data)
		return;
	
	memset(uuid, 0, MAX_LEN_UUID_STR+1);
	memset(proto_uuid, 0, MAX_LEN_PROTOCOL_UUID_STR+1);
	
	for(d = (sdp_data_t *)data; d != NULL; d = d->next, i++){
		switch(d->dtd){
		case SDP_UUID16:
		case SDP_UUID32:
		case SDP_UUID128:
			proto = sdp_uuid_to_proto(&d->val.uuid);
			sdp_uuid2strn(&d->val.uuid, uuid, MAX_LEN_UUID_STR);
			sdp_proto_uuid2strn(&d->val.uuid, 
					    proto_uuid, 
					    MAX_LEN_PROTOCOL_UUID_STR);
			snprintf(buf+strlen(buf), 
				 MAXLEN-strlen(buf)-1, 
				 "\t%s (0x%s)\n", 
				 proto_uuid, uuid);
			break;
		case SDP_UINT8:
			if(proto == RFCOMM_UUID)
				snprintf(buf+strlen(buf), 
					 MAXLEN-strlen(buf)-1, 
					 "\tChannel: %d\n", 
					 d->val.uint8);
			else
				snprintf(buf+strlen(buf), 
					 MAXLEN-strlen(buf)-1, 
					 "\tuint8: 0x%x\n", 
					 d->val.uint8);
			break;
		case SDP_UINT16:
			if (proto == L2CAP_UUID) {
				if (i == 1)
					snprintf(buf+strlen(buf), 
						 MAXLEN-strlen(buf)-1, 
						 "\tPSM: %d\n", 
						 d->val.uint16);
				else
					snprintf(buf+strlen(buf), 
						 MAXLEN-strlen(buf)-1, 
						 "\tVersion: 0x%04x\n", 
						 d->val.uint16);
				
			} else if (proto == BNEP_UUID)
				if (i == 1)
					snprintf(buf+strlen(buf), 
						 MAXLEN-strlen(buf)-1, 
						 "\tVersion: 0x%04x\n", 
						 d->val.uint16);
				else
					snprintf(buf+strlen(buf), 
						 MAXLEN-strlen(buf)-1, 
						 "\tuint16: 0x%x\n", 
						 d->val.uint16);
			else
				snprintf(buf+strlen(buf), 
					 MAXLEN-strlen(buf)-1, 
					 "\tuint16: 0x%x\n", 
					 d->val.uint16);
			break;
			/*
			  case SDP_SEQ16:
			  break;
			  case SDP_SEQ8:
			  break;
			*/
		default:
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "\tUnknow: 0x%x\n", d->dtd);
			break;
		}
	}
}
void print_proto(void *data, void *u){
	sdp_list_foreach((sdp_list_t *)data, print_proto_desc, u);
}

void print_class(void *data, void *u){
	char uuid[MAX_LEN_UUID_STR+1];
	char svclass_uuid[MAX_LEN_SERVICECLASS_UUID_STR+1];
	char *buf = (char *)u;
	
	if(!data)
		return;
	
	memset(uuid, 0, MAX_LEN_UUID_STR+1);
	memset(svclass_uuid, 0, MAX_LEN_SERVICECLASS_UUID_STR+1);
	
	sdp_uuid2strn((uuid_t *)data, uuid, MAX_LEN_UUID_STR);
	sdp_svclass_uuid2strn((uuid_t *)data, svclass_uuid, 
			      MAX_LEN_SERVICECLASS_UUID_STR);
	
	snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
		 "\t%s (0x%s)\n", svclass_uuid, uuid);
}

#endif // BLUETOOTH_SERVICE_SCAN_DEBUG

static int do_search(sdp_session_t *session, uuid_t *uuid)
{
	sdp_list_t *response_list = NULL, *attrid_list, *search_list, *r;
	uint32_t range = 0x0000ffff;
	int err = 0;
	int result = 0;
#if defined(BLUETOOTH_SERVICE_SCAN_DEBUG)
	char buf[MAXLEN];
	uuid_t group;
	sdp_uuid16_create(&group, PUBLIC_BROWSE_GROUP);
	search_list = sdp_list_append(0, &group);
	memset(buf, 0, MAXLEN);
#else
	search_list = sdp_list_append(0, uuid);
#endif
	attrid_list = sdp_list_append(0, &range);

	HAGGLE_DBG("Searching for services\n");

	// perform the search
	err = sdp_service_search_attr_req(session, search_list, 
					  SDP_ATTR_REQ_RANGE, 
					  attrid_list, &response_list);

	if (err) {
		result = -1;
		HAGGLE_ERR("Service search request failed\n");
		goto cleanup;
	}	

	// go through each of the service records
	for (r = response_list; r; r = r->next) {
		sdp_record_t *rec = (sdp_record_t*) r->data;
		sdp_list_t *list = NULL;
#if defined(BLUETOOTH_SERVICE_SCAN_DEBUG)
		sdp_data_t *data;
		
		if ((data = sdp_data_get(rec, SDP_ATTR_SVCNAME_PRIMARY)) != NULL){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Name: %s\n", data->val.str);
		}
		if ((data = sdp_data_get(rec, SDP_ATTR_SVCDESC_PRIMARY)) != NULL){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Description: %s\n", data->val.str);
		}
		if ((data = sdp_data_get(rec, SDP_ATTR_PROVNAME_PRIMARY)) != NULL){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Provider: %s\n", data->val.str);
		}
		if (sdp_get_service_classes(rec, &list) == 0){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Classes:\n");
			sdp_list_foreach(list, print_class, buf);
			sdp_list_free(list, free);
		}
		if (sdp_get_access_protos(rec, &list) == 0){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Protocol:\n");
			sdp_list_foreach(list, print_proto, buf);
			sdp_list_free(list, (sdp_free_func_t)sdp_data_free);
		}
		if (sdp_get_lang_attr(rec, &list) == 0){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Language:\n");
			sdp_list_foreach(list, print_lang, buf);
			sdp_list_free(list, free);
		}
		if (sdp_get_profile_descs(rec, &list) == 0){
			snprintf(buf+strlen(buf), MAXLEN-strlen(buf)-1, 
				 "Service Profile:\n");
			sdp_list_foreach(list, print_profile, buf);
			sdp_list_free(list, free);
		}
#else
		// get a list of the protocol sequences
		if (sdp_get_access_protos(rec, &list) == 0) {
			sdp_list_t *p = list;
			int port;

			if ((port = sdp_get_proto_port(p, RFCOMM_UUID)) != 0) {
				result = port;
			} else if ((port = 
				    sdp_get_proto_port(p, L2CAP_UUID)) != 0) {
			} else {
			}

			for(; p; p = p->next) {
				sdp_list_free((sdp_list_t*)p->data, 0);
			}
			sdp_list_free(list, 0);
		} 
#endif
		sdp_record_free(rec);
	}
#if defined(BLUETOOTH_SERVICE_SCAN_DEBUG)
	printf("Bluetooth result:\n%s\n", buf);
#endif
cleanup:
	sdp_list_free(response_list, 0);
	sdp_list_free(search_list, 0);
	sdp_list_free(attrid_list, 0);

	return result;
}

static int find_haggle_service(bdaddr_t bdaddr)
{
	const unsigned char svc_uuid_int[] = HAGGLE_BLUETOOTH_SDP_UUID;
	uuid_t svc_uuid;
	char str[20];
	sdp_session_t *sess = NULL; // This session is for the remote sdp server
	int found = 0;

	sess = sdp_connect(&bdaddr_any, &bdaddr, SDP_RETRY_IF_BUSY);

	ba2str(&bdaddr, str);

	if (!sess) {
		HAGGLE_ERR("Failed to connect to SDP server on %s: %s\n", 
			   str, strerror(errno));
		return -1;
	}

	sdp_uuid128_create(&svc_uuid, &svc_uuid_int);

	found = do_search(sess, &svc_uuid);
	
	sdp_close(sess);

	return found;
}

#include <poll.h>

void bluetoothDiscovery(ConnectivityBluetooth *conn)
{
	struct {
		struct hci_inquiry_req ir;
		inquiry_info ii[MAX_BT_RESPONSES];
	} req;
        int sock, ret, i;
	const BluetoothAddress *addr;
        InterfaceStatus_t status;
	struct pollfd fds;
	struct sockaddr_hci hcia;
	BluetoothInterfaceRefList btList;
	
	addr = conn->rootInterface->getAddress<BluetoothAddress>();

	if (!addr) {
		HAGGLE_ERR("No valid Bluetooth address\n");
		return;
	}

        HAGGLE_DBG("Inquiry on interface %s\n", 
		   conn->rootInterface->getName());
	
	memset(&req, 0, sizeof(req));

	ret = hci_get_route(NULL);

	if (ret < 0) {
		HAGGLE_ERR("Could not get device id\n");
		return;
	}
	
	req.ir.dev_id = ret;

	sock = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);

	if (sock < 0) {
		HAGGLE_DBG("Could not open Bluetooth socket\n");
		return;
	}

	req.ir.num_rsp = MAX_BT_RESPONSES;
	req.ir.length  = 8;
	req.ir.flags   = IREQ_CACHE_FLUSH;
	/* Not sure what this means, taken from hci_inquiry() in bluez */
	req.ir.lap[0] = 0x33;
	req.ir.lap[1] = 0x8b;
	req.ir.lap[2] = 0x9e;

	fds.fd = sock;
	fds.events = POLLOUT | POLLERR;
	fds.revents = 0;

	ret = poll(&fds, 1, 0);

	if (ret == -1) {
		close(sock);
		HAGGLE_ERR("poll error : %s\n", 
			   strerror(errno));
		return;
	} else if (ret > 0) {
		if (fds.revents & POLLOUT) {
			/* Everything fine */
		}
		if (fds.revents & POLLERR) {
			HAGGLE_ERR("Error on Bluetooth socket\n");
			close(sock);
			return;
		}
	} else {
		HAGGLE_ERR("Bluetooth socket not writable!\n");
		close(sock);
		return;
	}

	ret = ioctl(sock, HCIINQUIRY, &req);

	if (ret < 0) {
		HAGGLE_ERR("Inquiry failed %s\n",
			   strerror(errno));
		close(sock);
		return;
	}

	CM_DBG("Inquiry returned %d devices\n", req.ir.num_rsp);

	/* 
	   Bind socket to device, so that hci_read_remote_name does
	   not fail.
	*/
	memset(&hcia, 0, sizeof(hcia));
	hcia.hci_family = AF_BLUETOOTH;
	hcia.hci_dev = req.ir.dev_id;
	
	if (bind(sock, (struct sockaddr *)&hcia, sizeof(hcia)) < 0) {
		HAGGLE_ERR("could not bind bluetooth socket\n");
		close(sock);
		return;
	}

	for (i = 0; i < req.ir.num_rsp; i++) {
                bool report_interface = false;
		unsigned char macaddr[BT_ALEN];
		int channel = 0, timeout = 3000;
                char remote_name[256];

		memset(remote_name, 0, 256);              
                strcpy(remote_name, "unknown");
                
                if (conn->readRemoteName) {
			ret = hci_read_remote_name(sock, 
						   &req.ii[i].bdaddr, 
						   255, 
						   remote_name, 
						   timeout);
			
			if (ret < 0) {
				HAGGLE_ERR("name lookup: %s\n", strerror(errno));
			}
                }
                
		baswap((bdaddr_t *) &macaddr, &req.ii[i].bdaddr);

		BluetoothAddress addr(macaddr);
		
                status = conn->is_known_interface(Interface::TYPE_BLUETOOTH, 
						  macaddr);
        
                if (status == INTERFACE_STATUS_HAGGLE) {
                        report_interface = true;
                } else if (status == INTERFACE_STATUS_UNKNOWN) {
                        switch (ConnectivityBluetoothBase::classifyAddress(Interface::TYPE_BLUETOOTH, macaddr)) {
                        case BLUETOOTH_ADDRESS_IS_UNKNOWN:
                                channel = find_haggle_service(req.ii[i].bdaddr);
                                if (channel > 0) {
                                        report_interface = true;
                                        conn->report_known_interface(Interface::TYPE_BLUETOOTH, macaddr, true);
                                } else if (channel == 0) {
					/*
					  Reporting an interface as
					  known "non-Haggle" can cause
					  problems if a Haggle device
					  is detected before Haggle
					  has been started on that
					  device. Therefore, disable
					  the caching of negative
					  results.
					  
					  The downside of disabling
					  caching is that the SDP scan
					  can be overwhelmed when
					  there are many devices.
					 */
                                        //conn->report_known_interface(Interface::TYPE_BLUETOOTH, macaddr, false);
                                }
                                break;
                        case BLUETOOTH_ADDRESS_IS_HAGGLE_NODE:
                                report_interface = true;
                                conn->report_known_interface(Interface::TYPE_BLUETOOTH, macaddr, true);
                                break;
                        case BLUETOOTH_ADDRESS_IS_NOT_HAGGLE_NODE:
                                conn->report_known_interface(Interface::TYPE_BLUETOOTH, macaddr, false);
                                break;
                        }
                } 
        
		if (report_interface) {
			btList.push_back(new BluetoothInterface(macaddr, 
								remote_name, 
								&addr, 
								IFFLAG_UP));
			
			CM_DBG("Found Haggle device [%s - %s]\n", 
			       addr.getStr(), remote_name);
		} else {
			CM_DBG("Device [%s - %s] is not a Haggle device\n", 
			       addr.getStr(), remote_name);
		}
	}
	
	close(sock);

	CM_DBG("Bluetooth inquiry done! Num discovered=%d\n", 
	       btList.size());

	/* 
	   Now report interfaces
	*/
	for (BluetoothInterfaceRefList::iterator it = btList.begin();
	     it != btList.end(); it++) {
		InterfaceRef iface = *it;
		conn->report_interface(iface, conn->rootInterface, 
				       new ConnectivityInterfacePolicyTTL(2));
	}
	return;
}

void ConnectivityBluetooth::hookCleanup()
{
	HAGGLE_DBG("Removing SDP service\n");

	if (session) {
		del_service(session, service);
		sdp_close(session);
	}
}

void ConnectivityBluetooth::cancelDiscovery(void)
{
	hookStopOrCancel();
	cancel();
}

bool ConnectivityBluetooth::run()
{
        CM_DBG("Bluetooth connectivity detector started for %s\n", rootInterface->getIdentifierStr());
	
	/* 
	 When the Bluetooth interface is brought up, for example on Android, it takes
	 a while for the SDP service daemon to start. Therefore, we sleep here a while so
	 that we can successfully register.
	 */
	cancelableSleep(5000);

	session = sdp_connect(0, &bdaddr_local, SDP_RETRY_IF_BUSY);

	if (!session) {
		HAGGLE_ERR("Could not connect to local SDP daemon\n");
		return false;
	}

	if (add_service(session, &service) < 0) {
		CM_DBG("Could not add SDP service\n");
		return false;
	} 

	CM_DBG("SDP service handle is %u\n", service);
	
	cancelableSleep(5000);
	
	while (!shouldExit()) {
		
		bluetoothDiscovery(this);
		
		age_interfaces(rootInterface);
		
		cancelableSleep(TIME_TO_WAIT_MSECS);			
	}

	return false;
}
#endif

