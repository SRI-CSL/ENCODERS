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


#include <libcpphaggle/PlatformDetect.h>

#if defined(ENABLE_MEDIA)
#if defined(OS_MACOSX)

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>

#include "CoreFoundation/CoreFoundation.h"
#include "CoreFoundation/CFString.h"
#include "CoreServices/CoreServices.h"

#include "ConnectivityMedia.h"

#define deviceName "roh-mb"

ConnectivityMedia::ConnectivityMedia(ConnectivityManager * m, const InterfaceRef& iface) : 
	Connectivity(m, iface, "Media connectivity"),
{
}

ConnectivityMedia::~ConnectivityMedia()
{
}

int selectThisNode(struct dirent *file) {
	// select function for scandir
	// returns 1 if directory with name [HAGGLE_FOLDER_PREFIX]-[deviceName]
	string thisDeviceName = HAGGLE_FOLDER_PREFIX;
	thisDeviceName.append("-");
	thisDeviceName.append(deviceName);
	
	if (!(file->d_type == DT_DIR)) 
		return 0;
	
	if (!strstr(file->d_name, thisDeviceName.c_str())) 
		return 0;
	
	HAGGLE_DBG("%s\n", file->d_name);
	
	return 1;
}

int selectNotThisNode(struct dirent *file) {
	// select function for scandir
	// returns 1 if directory not with name [HAGGLE_FOLDER_PREFIX]-[deviceName]
	string thisDeviceName = HAGGLE_FOLDER_PREFIX;
	thisDeviceName.append("-");
	thisDeviceName.append(deviceName);
	
	if (!(file->d_type == DT_DIR))
		return 0;
	
	if (!strstr(file->d_name, HAGGLE_FOLDER_PREFIX)) 
		return 0;

	if (strstr(file->d_name, thisDeviceName.c_str()))
		return 0;
	
	return 1;
}

void ConnectivityMedia::findRemoteInterfaces(char *path)
{
	static struct dirent **current;
	static struct statfs stat;
	static int numCurrent = 0;

	if (statfs(path, &stat))
		return;

	numCurrent = scandir(path, &current, NULL, NULL);

	CM_DBG("%s (size:%ldMB; available:%ldMB; %s%s%s)\n", path, stat.f_bsize * stat.f_blocks / (1 << 20), stat.f_bsize * stat.f_bavail / (1 << 20), (stat.f_flags & MNT_RDONLY ? "MNT_RDONLY " : ""),
	       (stat.f_flags & MNT_AUTOMOUNTED ? "MNT_AUTOMOUNTED " : ""), (stat.f_flags & MNT_JOURNALED ? "MNT_JOURNALED " : "")
	    );

	// only writable and not automatically mounted volumes. todo: check for ejectable?
	if ((stat.f_flags & MNT_RDONLY) ||
	    (stat.f_flags & MNT_AUTOMOUNTED)) {
		return;
	} 

	struct dirent **files = NULL;
	int numFiles = 0;
	string interfacePath = path;
	int fileno = 0;
	
	// search for "local interface", i.e., a directory that we created on the media earlies
	// (see comment in selectThisNode)
	numFiles = scandir(path, &files, &selectThisNode, NULL);

	if (numFiles == 1 && files[0] && files[0]->d_name) {
		
		interfacePath.append("/");
		interfacePath.append(files[0]->d_name);
		// This code is leaking memory, so replaced

//		interfacePath = (char*)malloc(strlen(path) + strlen(files[0]->d_name) + 2);
//		sprintf(interfacePath, "%s/%s", path, files[0]->d_name);
		fileno = files[0]->d_fileno;
		CM_DBG("localInterface %s (%d)\n", interfacePath.c_str(), files[0]->d_fileno);
	} else {
		// no "local interface" directory on device,
		// is there a haggle directory at all?
		if (!scandir(path, &files, &selectNotThisNode, NULL)) 
			return;
	}
	//InterfaceRef localIface(
//			IFACE_TYPE_MEDIA,
//			(char*)&fileno,
//			NULL,		  
//			-1,
//			interfacePath.c_str(),
//			IFACE_FLAG_LOCAL);
//	add_interface(&localIface, iface, new ConnectivityInterfacePolicyAgeless());
	
	// Search for "remote interfaces", i.e., specific directories on the mounted media.
	numFiles = scandir(path, &files, &selectNotThisNode, NULL);
	
	for (int i = 0; i < numFiles; i++) {
		interfacePath.clear();
		interfacePath = path;
		interfacePath.append("/");
		interfacePath.append(files[i]->d_name);
		
		CM_DBG("remoteInterface %s (%d)\n", interfacePath.c_str(), files[i]->d_fileno);
		
		//InterfaceRef remoteIface(
//				IFACE_TYPE_MEDIA,
//				(char*)&files[i]->d_fileno,
//				NULL,		  
//				-1,
//				interfacePath.c_str());
//		iface->up();
//		
//		add_interface(&remoteIface, &localIface, new ConnectivityInterfacePolicyAgeless());
//		
		free(files[i]);
	}
	if (files)
		free(files);
}

void ConnectivityMedia::hookStopOrCancel()
{
}

void ConnectivityMedia::hookCleanup()
{
	CM_DBG("Cleaning up %s\n", getName());
}

void ConnectivityMedia::run()
{

}


#endif
#endif
