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
#ifndef _CONNECTIVITYLOCALLINUX_H_
#define _CONNECTIVITYLOCALLINUX_H_

#include <libcpphaggle/Platform.h>
#include "ConnectivityLocal.h"
#include "Interface.h"

#if defined(HAVE_DBUS)
#include <dbus/dbus.h>
struct dbus_handle {
        DBusError err;
        DBusConnection* conn;
};
#endif

#if defined(ENABLE_ETHERNET)
#include <asm/types.h>
#include <linux/netlink.h>
struct netlink_handle {
        int sock;
        int seq;
        struct sockaddr_nl local;
        struct sockaddr_nl peer;
};
#endif

#if defined(ENABLE_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

struct hci_handle {
        int sock;
        struct sockaddr_hci addr;
        struct hci_filter flt;
};
#endif

#if defined(OS_ANDROID)
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL Java_org_haggle_kernel_BluetoothConnectivity_onBluetoothTurnedOn(JNIEnv *env, jobject obj,
											jstring addr, jstring name);

JNIEXPORT void JNICALL Java_org_haggle_kernel_BluetoothConnectivity_onBluetoothTurnedOff(JNIEnv *env, jobject obj,
											 jstring addr, jstring name);
#ifdef __cplusplus
}
#endif

#endif /* OS_ANDROID */

/**
	Local connectivity module

	This module scans the local hardware/software to find
	bluetooth/ethernet/etc. interfaces that are accessible, and tells the
	connectivity manager about them when they are detected.
*/
class ConnectivityLocalLinux : public ConnectivityLocal
{
	friend class ConnectivityLocal;
private:
#if defined(HAVE_DBUS)
        struct dbus_handle dbh;
        friend DBusHandlerResult dbus_handler(DBusConnection *conn, DBusMessage *msg, void *data);
#endif
#if defined(ENABLE_BLUETOOTH)
        struct hci_handle hcih;
#if defined(OS_ANDROID)
	friend void Java_org_haggle_kernel_BluetoothConnectivity_onBluetoothTurnedOn(JNIEnv *env, jobject obj,
										     jstring addr, jstring name);
	friend void Java_org_haggle_kernel_BluetoothConnectivity_onBluetoothTurnedOff(JNIEnv *env, jobject obj,
										      jstring addr, jstring name);
        bool set_piscan_mode;
        int dev_id;
#endif	
        int read_hci();
        void findLocalBluetoothInterfaces();
#endif
#if defined(ENABLE_ETHERNET)
        struct netlink_handle nlh;
        int read_netlink();
        void findLocalEthernetInterfaces();
#endif
        bool run();
        void hookCleanup();
        ConnectivityLocalLinux(ConnectivityManager *m);
        ~ConnectivityLocalLinux();
};

#endif
