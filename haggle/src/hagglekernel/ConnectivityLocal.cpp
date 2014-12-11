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
#include <libcpphaggle/List.h>

#include "ConnectivityLocal.h"
#include "ConnectivityManager.h"

#if defined(ENABLE_BLUETOOTH)
#include "ConnectivityBluetooth.h"
#endif
#if defined(ENABLE_ETHERNET)
#include "ConnectivityEthernet.h"
#endif
#if defined(ENABLE_MEDIA)
#include "ConnectivityMedia.h"
#endif

#if defined(OS_ANDROID) && defined(ENABLE_TI_WIFI)
#include "ConnectivityLocalAndroid.h"
#elif defined(OS_LINUX)
#include "ConnectivityLocalLinux.h"
#elif defined(OS_MACOSX)
#include "ConnectivityLocalMacOSX.h"
#elif defined(OS_WINDOWS)
#if defined(OS_WINDOWS_MOBILE)
#include "ConnectivityLocalWindowsMobile.h"
#else
#include "ConnectivityLocalWindowsXP.h"
#endif
#else
#error "Bad OS - Not supported by ConnectivityLocal.h"
#endif

ConnectivityLocal::ConnectivityLocal(ConnectivityManager *m, const string& name) : 
	Connectivity(m, NULL, name)
{
}

ConnectivityLocal::~ConnectivityLocal()
{
}

ConnectivityLocal *ConnectivityLocal::create(ConnectivityManager *m)
{
#if defined(OS_ANDROID) && defined(ENABLE_TI_WIFI)
       return new ConnectivityLocalAndroid(m);
#elif defined(OS_LINUX)
       return new ConnectivityLocalLinux(m);
#elif defined(OS_MACOSX)
       return new ConnectivityLocalMacOSX(m);
#elif defined(OS_WINDOWS_DESKTOP)
       return new ConnectivityLocalWindowsXP(m);
#elif defined(OS_WINDOWS_MOBILE)
       return new ConnectivityLocalWindowsMobile(m);
#else
       return NULL;
#endif
}

