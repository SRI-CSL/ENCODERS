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
#ifndef _CONNECTIVITYLOCALWINDOWSXP_H_
#define _CONNECTIVITYLOCALWINDOWSXP_H_

#include <libcpphaggle/Platform.h>
#include "ConnectivityLocal.h"
#include "Interface.h"

#if defined(ENABLE_BLUETOOTH)
#endif
/**
	Local connectivity module

	This module scans the local hardware/software to find
	bluetooth/ethernet/etc. interfaces that are accessible, and tells the
	connectivity manager about them when they are detected.
*/
class ConnectivityLocalWindowsXP : public ConnectivityLocal
{
	friend class ConnectivityLocal;
private:
#if defined(ENABLE_BLUETOOTH)
        void findLocalBluetoothInterfaces();
#endif
#if defined(ENABLE_ETHERNET)
        void findLocalEthernetInterfaces();
#endif
        bool run();
        void hookCleanup();
        ConnectivityLocalWindowsXP(ConnectivityManager *m);
public:
        ~ConnectivityLocalWindowsXP();
};

#endif
