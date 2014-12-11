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
#include "Utility.h"

#include <stdio.h>
#include <stdlib.h>

#include <libcpphaggle/String.h>
#include <utils.h>
#include "Address.h"
#include "Debug.h"
#include "Trace.h"

using namespace haggle;

#if defined(OS_UNIX)
// Needed for stat()
#include <sys/stat.h>
// Needed for getpwuid()
#include <pwd.h>
// SW: Needed for getlogin()
#include <unistd.h>
#include <libgen.h>
#endif

#if defined(OS_MACOSX)
#include <ctype.h>
#endif

#if defined(OS_ANDROID)
#define DEFAULT_STORAGE_PATH "/data/haggle"
#elif defined(OS_MACOSX_IPHONE)
#define DEFAULT_STORAGE_PATH_PREFIX "/var/mobile"
#define DEFAULT_STORAGE_PATH_SUFFIX "/.Haggle"
#elif defined(OS_LINUX)
#define DEFAULT_STORAGE_PATH_PREFIX "/home/"
#define DEFAULT_STORAGE_PATH_SUFFIX "/.Haggle"
#elif defined(OS_MACOSX)
#define DEFAULT_STORAGE_PATH_PREFIX "/Users/"
#define DEFAULT_STORAGE_PATH_SUFFIX "/Library/Application Support/Haggle"
#elif defined(OS_WINDOWS)
#include <shlobj.h>
#define DEFAULT_STORAGE_PATH_PREFIX ""
#define DEFAULT_STORAGE_PATH_SUFFIX "\\Haggle"
#else
#error "Unsupported Platform"
#endif

// SW: used in path buffer for haggle.db path
#ifndef MAX_PATH
#define MAX_PATH (1024)
#endif 

// This pointer will be filled in the first time HAGGLE_DEFAULT_STORAGE_PATH is used
// SW: place buffer in bss to avoid memory leak
const char *hdsp;
char hdspBuffer[MAX_PATH+1];

// SW: refactoring macro into a function which places path on bss versus
//     heap, to avoid memory leak
const char *get_default_storage_path() {
    if (NULL == hdsp) {
        char *t = fill_in_default_path();
        if ((t == NULL) || (strlen(t) > MAX_PATH)) {
            return NULL; 
        }
        strcpy(hdspBuffer, t); 
        free(t);
        hdsp = hdspBuffer; 
    } 
    return hdsp; 
}

#if !defined(OS_ANDROID) && !defined(OS_MACOSX_IPHONE)
static char *fill_prefix_and_suffix(const char *fillpath)
{
	char *path;

        if (!fillpath)
                return NULL;

        path = (char *)malloc(strlen(DEFAULT_STORAGE_PATH_PREFIX) + 
                                strlen(fillpath) + strlen(DEFAULT_STORAGE_PATH_SUFFIX) + 1);
        
	if (path != NULL)
		sprintf(path, "%s%s%s" , DEFAULT_STORAGE_PATH_PREFIX, fillpath, DEFAULT_STORAGE_PATH_SUFFIX);
        
        return path;
}
#endif

#if defined(OS_ANDROID)

static char *fill_in_path(const char *fillpath)
{
	char *path;

	if (mkdir(fillpath, 0755) == -1) {
		switch (errno) {
		case EEXIST:
			/* Everything OK */
			break;
		default:
			return NULL;
		}
	}

        path = (char*)malloc(strlen(fillpath) + 1);
        
        if (!path)
                return NULL;

	strcpy(path, fillpath);

        return path;
}

// SW: refactoring ddsp to be on bss instead of heap, to avoid mem leak.
const char *ddsp;
char ddspBuffer[MAX_PATH+1];

// SW: use function instead of macro with memleak
const char *get_default_datastore_path() {
    if (NULL == ddsp) {
        char *t = fill_in_default_datastore_path();
        if (strlen(t) > MAX_PATH) {
            return NULL; 
        }
        strcpy(ddspBuffer, t); 
        free(t);
        ddsp = ddspBuffer; 
    } 
    return ddsp; 
}

// SW: allow android to replace default data store path, in case default
//     is invalid.
void set_default_datastore_path(const char *str) {
    if (NULL == str) {
        return;
    }
    if (strlen(str) > MAX_PATH) {
        return;
    }
    strcpy(ddspBuffer, str);
    ddsp = ddspBuffer;
}

char *fill_in_default_datastore_path(void)
{
	char *path = fill_in_path(DEFAULT_STORAGE_PATH);

	if (path) {
		HAGGLE_DBG("data store path is %s\n", path);
	}

	return path;
}

char *fill_in_default_path(void)
{
	char *path;

	path = fill_in_path("/sdcard/haggle");

	if (path)
		return path;

	path = fill_in_default_datastore_path();


	if (path) {
		HAGGLE_DBG("data storage path is %s\n", path);
	} else {
		fprintf(stderr, "could not create data storage path\n");
	}

	return path;
}

#elif defined(OS_MACOSX_IPHONE)
char *fill_in_default_path()
{
        char *path, *home;
        
        home = getenv("HOME");

        if (!home) {
                home = DEFAULT_STORAGE_PATH_PREFIX;
        }

        path = (char*)malloc(strlen(home) + strlen(DEFAULT_STORAGE_PATH_SUFFIX) + 1);
        
        if (!path)
                return NULL;

        sprintf(path, "%s%s", home, DEFAULT_STORAGE_PATH_SUFFIX);

        return path;
}
#elif defined(OS_UNIX)
// SW: refactored this function to have simpler logic and to get the
//     login from the environment variable first, and use thread safe
//     getpwuid and getlogin upon failure. Note that getpwuid and
//     getlogin have a memory leak.
char *fill_in_default_path()
{
    char *login = NULL;
	
    size_t sz = 128;
    char buf[sz];
    int ret;

#if defined(OS_UNIX)
    // use getenv to avoid race conditions and memory leaks
    // found in libc getpwuid and getlogin_r
    if (NULL == login) {
        login = getenv("USER");
    }
#endif

    if (NULL == login) {
        struct passwd *pwd = NULL;
        struct passwd old_pw;
        // NOTE: there is a memory leak within this function, but 
        // there isn't really much we can do, without switching to
        // a different libc...
        ret = getpwuid_r(getuid(), &old_pw, buf, sz, &pwd);
    
        if ((0 == ret) && (NULL != pwd->pw_name)) {
            login = pwd->pw_name;
        }
    }

    if (NULL == login) {
        // NOTE: there is a memory leak within this function, but 
        // there isn't really much we can do, without switching to
        // a different libc...
        ret = getlogin_r(buf, sz);
        login = buf;
        if (0 != ret) {
            login = NULL;
        }
    }

    if (NULL == login) {
        printf("Unable to get user name!\n");
        return NULL;
    }

    // TODO: we may not want to check for root here, but instead
    // in main.cpp

    if (0 == strcmp(login, "root")) {
        printf("Haggle should not be run as root!\n");
        return NULL;
    }

    return fill_prefix_and_suffix(login);
}
#elif defined(OS_WINDOWS_MOBILE)
static bool has_haggle_folder(LPCWSTR path)
{
	WCHAR my_path[MAX_PATH+1];
	long i, len;
	WIN32_FILE_ATTRIBUTE_DATA data;
	
	len = MAX_PATH;
	for (i = 0; i < MAX_PATH && len == MAX_PATH; i++) {
		my_path[i] = path[i];
		if (my_path[i] == 0)
			len = i;
	}
	if (len == MAX_PATH)
		return false;
	i = -1;
	do {
		i++;
		my_path[len+i] = DEFAULT_STORAGE_PATH_SUFFIX[i];
	} while (DEFAULT_STORAGE_PATH_SUFFIX[i] != 0 && i < 15);

	if (GetFileAttributesEx(my_path, GetFileExInfoStandard, &data)) {
		return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	} 
	return false;
}

#include <projects.h>
#pragma comment(lib,"note_prj.lib")
char *fill_in_default_path()
{
	HANDLE	find_handle;
	WIN32_FIND_DATA	find_data;
	char path[MAX_PATH+1];
	WCHAR best_path[MAX_PATH+1];
	bool best_path_has_haggle_folder;
	ULARGE_INTEGER best_avail, best_size;
	long i;
	
	// Start with the application data folder, as a fallback:
	if (!SHGetSpecialFolderPath(NULL, best_path, CSIDL_APPDATA, FALSE)) {
		best_path[0] = 0;
		best_avail.QuadPart = 0;
		best_size.QuadPart = 0;
		best_path_has_haggle_folder = false;
	} else {
		GetDiskFreeSpaceEx(best_path, &best_avail, &best_size, NULL);
		best_path_has_haggle_folder = has_haggle_folder(best_path);
	}
	fprintf(stdout, "Found data path: \"%ls\" (size: %I64d/%I64d, haggle folder: %s)\n", 
		best_path, best_avail, best_size,
		best_path_has_haggle_folder ?" Yes" : "No");

	find_handle = FindFirstFlashCard(&find_data);
	if (find_handle != INVALID_HANDLE_VALUE) {
		do {
			// Ignore the root directory (this has been checked for above)
			if (find_data.cFileName[0] != 0) {
				ULARGE_INTEGER	avail, size, free;
				bool haggle_folder;
				
				GetDiskFreeSpaceEx(find_data.cFileName, &avail, &size, &free);
				haggle_folder = has_haggle_folder(find_data.cFileName);
				fprintf(stdout, "Found data card path: \"%ls\" (size: %I64d/%I64d, haggle folder: %s)\n", 
					find_data.cFileName, avail, size,
					haggle_folder ? "Yes" : "No");
				// is this a better choice than the previous one?
				// FIXME: should there be any case when a memory card is not used?
				if (true) {
					// Yes.
					
					// Save this as the path to use:
					for (i = 0; i < MAX_PATH; i++)
						best_path[i] = find_data.cFileName[i];

					best_avail = avail;
					best_size = size;
					best_path_has_haggle_folder = haggle_folder;
				}
			}
		} while (FindNextFlashCard(find_handle, &find_data));

		FindClose(find_handle);
	}
	// Convert the path to normal characters.
	for (i = 0; i < MAX_PATH; i++)
		path[i] = (char) best_path[i];

	char *full_path = fill_prefix_and_suffix(path);

	if (!full_path) {
		HAGGLE_ERR("Could not create default path\n");
		return NULL;
	}

	if (!create_path(full_path)) {
		HAGGLE_ERR("Could not create default  path \'%s\'\n", full_path);
	}

	return full_path;
}

const char *ddsp;

char *fill_in_default_datastore_path()
{
	char path[MAX_PATH+1];
	WCHAR best_path[MAX_PATH+1];
	ULARGE_INTEGER best_avail, best_size;
	long i;
	
	// Start with the application data folder, as a fallback:
	if (!SHGetSpecialFolderPath(NULL, best_path, CSIDL_APPDATA, FALSE)) {
		best_path[0] = 0;
	} else {
		GetDiskFreeSpaceEx(best_path, &best_avail, &best_size, NULL);
	}
	
	// Convert the path to normal characters.
	for (i = 0; i < MAX_PATH; i++)
		path[i] = (char) best_path[i];

	char *full_path = fill_prefix_and_suffix(path);

	if (!full_path) {
		HAGGLE_ERR("Could not create default data store path\n");
		return NULL;
	}

	if (!create_path(full_path)) {
		HAGGLE_ERR("Could not create default data store path \'%s\'\n", full_path);
	}

	return full_path;
}

#elif defined(OS_WINDOWS_XP) || defined(OS_WINDOWS_2000)	
char *fill_in_default_path()
{
	char login[MAX_PATH];
	
	if (SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, login) != S_OK) {
		fprintf(stderr, "Unable to get folder path!\n");
		return NULL;
	}

        return fill_prefix_and_suffix(login);
}
#elif defined(OS_WINDOWS_VISTA)
char *fill_in_default_path()
{
	PWSTR login1;
	char login[MAX_PATH];
	long i;
	
	if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &login1) != S_OK) {
		fprintf(stderr, "Unable to get user application data folder!\n");
		return NULL;
	}
	
	for(i = 0; login1[i] != 0; i++)
		login[i] = (char) login1[i];
	login[i] = 0;
	CoTaskMemFree(login1);

        return fill_prefix_and_suffix(login);
}
#endif

bool create_path(const char *p)
{
	// Make sure we don't try to create the string using a NULL pointer:
	if (p == NULL)
		return false;
	
	// Make a string out of the pointer:
	string path = p;
	
	// Check if the path is ""
	if (path.empty())
		return false;
	
	// prefix - the path to the directory directly above the desired directory:
	string prefix = path.substr(0, path.find_last_of(PLATFORM_PATH_DELIMITER));
	
#if defined(OS_WINDOWS)
	wchar_t *wpath = strtowstr_alloc(path.c_str());
	
	if (!wpath)
		return false;

	// Try to create the desired directory:
	if (CreateDirectory(wpath, NULL) != 0) {
		// No? What went wrong?
		switch (GetLastError()) {
			// The directory already existed:
			case ERROR_ALREADY_EXISTS:
				// Done:
				free(wpath);
				return true;
			break;
			
			// Part of the path above the desired directory didn't exist:
			case ERROR_PATH_NOT_FOUND:
				// Try to create that path:
				if (!create_path(prefix.c_str())) {
					// That failed? Then fail.
					free(wpath);
					return false;
				}
				
				// Now the path above the desired directory exists. Create the 
				// desired directory
				if (CreateDirectory(wpath, NULL) != 0) {
					// Failed? Then fail.
					free(wpath);
					return false;
				}
			break;
		}
	}
	free(wpath);
#else 
	struct stat sbuf;
	
	// Check if the directory already exists:
	if (stat(path.c_str(), &sbuf) == 0) {
		// It does. Is it a directory?
		if ((sbuf.st_mode & S_IFDIR) == S_IFDIR) {
			// Yes. Succeed:
			return true;
		} else {
			// No. Fail:
			return false;
		}
	}
	
	// Check to see if the directory above it exists
	if (stat(prefix.c_str(), &sbuf) == 0) {
		switch (errno) {
			// Access violation - unable to write here:
			case EACCES:
			// Would create a loop in the directory tree:
			case ELOOP:
			// Path name too long:
			case ENAMETOOLONG:
			// One of the prior path member was not a directory:
			case ENOTDIR:
				// unrecoverable error:
				return false;
			break;
			
			default:
				// Recoverable error:
				
				// Try to create the directory directly above the desired 
				// directory
				if (!create_path(prefix.c_str())) {
					// Not possible? Fail.
					return false;
				}
			break;
		}
	}
	// The directory above this one now exists - create the desired directory:
	if (mkdir(path.c_str(), 
		// This means: 755.
		S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
		// mkdir failed. Fail:
		return false;
	}
#endif
	// Success:
	return true;
}

#if defined(OS_ANDROID) && defined(ENABLE_TI_WIFI)

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

#include <cutils/properties.h>

#include "TI_IPC_Api.h"
#include "TI_AdapterApiC.h"
/*
  This is a bit of a hack for the android platform. It seems that the
  TI wifi interface cannot be read by the normal ioctl() functions
  (at least not to discover the name and mac). Therefore we have special
  code that uses the driver API directly.
 */
int getLocalInterfaceList(InterfaceRefList& iflist, const bool onlyUp) 
{
        char wifi_iface_name[PROPERTY_VALUE_MAX];
        OS_802_11_MAC_ADDRESS hwaddr;
        unsigned char mac[6];
        tiINT32 res;
        unsigned int flags;
        struct in_addr addr;
	struct in_addr baddr;
        struct ifreq ifr;   
        int ret, s;
        Addresses addrs;
        
        // Read the WiFi interface name from the Android property manager
        ret = property_get("wifi.interface", wifi_iface_name, "sta");
        
        if (ret < 1)
                return -1;

        // Get a handle to the adapter throught the TI API
        TI_HANDLE h_adapter = TI_AdapterInit((tiCHAR *)wifi_iface_name);

        if (h_adapter == NULL)
                return -1;

        memset(&hwaddr, 0, sizeof(OS_802_11_MAC_ADDRESS));
        
        // Read the mac from the adapter
        res = TI_GetCurrentAddress(h_adapter, &hwaddr);
        
        if (res != TI_RESULT_OK) {
                // Deinit handle
                TI_AdapterDeinit(h_adapter);
                return -1;
        }

        memcpy(mac, &hwaddr, 6);

        addrs.add(new EthernetAddress(mac));

        // We are done with the adapter handle
        TI_AdapterDeinit(h_adapter);

        s = socket(AF_INET, SOCK_DGRAM, 0);
        
        if (s < 0) {
            HAGGLE_ERR("socket() failed: %s\n", strerror(errno));
            return -1;
        }

        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, wifi_iface_name, IFNAMSIZ);
        ifr.ifr_name[IFNAMSIZ - 1] = 0;

        /*
          Getting the mac address via ioctl() does not seem to work
        for the TI wifi interface, but we got the mac from the driver
        API instead

        ret = ioctl(s, SIOCGIFHWADDR, &ifr);

        if (ret < 0) {
                CM_DBG("Could not get mac address of interface %s\n", name);
                close(s);
                return NULL;
        }
        */

        if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
                flags = 0;
        } else {
                flags = ifr.ifr_flags;
        }
        
        if (onlyUp && !(flags & IFF_UP)) {
                close(s);
                return 0;
        }

        if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		close(s);
		return 0;
        } 
	addr.s_addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
        
        if (ioctl(s, SIOCGIFBRDADDR, &ifr) < 0) {
		close(s);
		return 0;
        }
	
	baddr.s_addr = ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr;
	addrs.add(new IPv4Address(addr));
	addrs.add(new IPv4BroadcastAddress(baddr));
        
        close(s);

	InterfaceRef iface = Interface::create<EthernetInterface>(mac, wifi_iface_name, addrs, 
								  IFFLAG_LOCAL | ((flags & IFF_UP) ? IFFLAG_UP : 0));

	if (iface)
		iflist.push_back(iface);

        return 1;
}

#elif defined(OS_LINUX)
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>

int getLocalInterfaceList(InterfaceRefList& iflist, const bool onlyUp) 
{
        char buf[1024];
	int sock;
        struct ifconf ifc;
	struct ifreq *ifr;
	int n, i, num = 0;
        
        sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("Could not calculate node id\n");
		return -1;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
        
	if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
	        close(sock);
		HAGGLE_ERR("SIOCGIFCONF failed\n");
		return -1;
	}
        
        ifr = ifc.ifc_req;
	n = ifc.ifc_len / sizeof(struct ifreq);
        
	for (i = 0; i < n; i++) {
                Addresses addrs;
		struct ifreq *item = &ifr[i];

                // if (ioctl(sock, SIOCGIFBRDADDR, item) < 0)
                //         continue;         
                
		//printf(", MAC %s\\n", ether_ntoa((struct ether_addr *)item->ifr_hwaddr.sa_data));
                if (onlyUp && !(item->ifr_flags & IFF_UP)) 
                        continue;

                addrs.add(new IPv4Address(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));
		addrs.add(new IPv4BroadcastAddress(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr));

		if (ioctl(sock, SIOCGIFHWADDR, item) < 0)
                        continue;
               
                addrs.add(new EthernetAddress((unsigned char *)item->ifr_hwaddr.sa_data));
		
		InterfaceRef iface = Interface::create<EthernetInterface>(item->ifr_hwaddr.sa_data, item->ifr_name, IFFLAG_LOCAL | ((item->ifr_flags & IFF_UP) ? IFFLAG_UP : 0));

		if (iface) {
			iface->addAddresses(addrs);
			iflist.push_back(iface);
			num++;
		}
        }
        
        close(sock);
              
        return num;
}
#elif defined(OS_MACOSX)

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/types.h>
#define	max(a,b) ((a) > (b) ? (a) : (b))

int getLocalInterfaceList(InterfaceRefList& iflist, const bool onlyUp) 
{
	int sock, num = 0, ret = -1;
#define REQ_BUF_SIZE (sizeof(struct ifreq) * 20)
	struct {
		struct ifconf ifc;
		char buf[REQ_BUF_SIZE];
	} req = { { REQ_BUF_SIZE, { req.buf}}, { 0 } };

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock == INVALID_SOCKET) {
		HAGGLE_ERR("Could not open socket\n");
		return -1;
	}

	ret = ioctl(sock, SIOCGIFCONF, &req);

	if (ret < 0) {
	        close(sock);
		HAGGLE_ERR("ioctl() failed\n");
		return -1;
	}

	struct ifreq *ifr = (struct ifreq *) req.buf;
	int len = 0;
	
	for (; req.ifc.ifc_len != 0; ifr = (struct ifreq *) ((char *) ifr + len), req.ifc.ifc_len -= len) {
		Addresses addrs;
		unsigned char macaddr[6];
			
		len = (sizeof(ifr->ifr_name) + max(sizeof(struct sockaddr),
						       ifr->ifr_addr.sa_len));

		if (ifr->ifr_addr.sa_family != AF_LINK	// || strncmp(ifr->ifr_name, "en", 2) != 0
		    ) {
			continue;
		}
		struct sockaddr_dl *ifaddr = (struct sockaddr_dl *) &ifr->ifr_addr;

		// Type 6 seems to be Ethernet
		if (ifaddr->sdl_type != 6) {
			continue;
		}

		memcpy(macaddr, LLADDR(ifaddr), 6);
		
		addrs.add(new EthernetAddress(macaddr));

		ifr->ifr_addr.sa_family = AF_INET;

		if (ioctl(sock, SIOCGIFADDR, ifr) != -1) {
			addrs.add(new IPv4Address(((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr));
		}
		if (ioctl(sock, SIOCGIFBRDADDR, ifr) != -1) {
			addrs.add(new IPv4BroadcastAddress(((struct sockaddr_in *) &ifr->ifr_broadaddr)->sin_addr));
		}
#if defined(ENABLE_IPv6)
		ifr->ifr_addr.sa_family = AF_INET6;
		
		if (ioctl(sock, SIOCGIFADDR, ifr) != -1) {
			addrs.add(new IPv6Address(((struct sockaddr_in6 *) &ifr->ifr_addr)->sin6_addr));
		}
		
		if (ioctl(sock, SIOCGIFBRDADDR, ifr) != -1) {
			addrs.add(new IPv6BroadcastAddress(((struct sockaddr_in6 *) &ifr->ifr_broadaddr)->sin6_addr));
		}
#endif
		if (ioctl(sock, SIOCGIFFLAGS, ifr) == -1) {
                        continue;
                }

                if (onlyUp && !(ifr->ifr_flags & IFF_UP)) 
                        continue;

		if (addrs.size() <= 1) {
			// No IPv4 or IPv6 addresses on interface --> ignore it
			continue;
		}
		// FIXME: separate 802.3 (wired) from 802.11 (wireless) ethernet
		iflist.push_back(InterfaceRef(Interface::create(Interface::TYPE_ETHERNET, macaddr, 
								ifr->ifr_name, addrs, IFFLAG_UP | IFFLAG_LOCAL)));
			
                num++;
	}

	close(sock);

	return num;
}
#elif defined(OS_WINDOWS)

#include <iphlpapi.h>

int getLocalInterfaceList(InterfaceRefList& iflist, const bool onlyUp) 
{
	IP_ADAPTER_ADDRESSES *ipAA, *ipAA_head;
	DWORD flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_PREFIX;
	ULONG bufSize;
        int num = 0;

	// Figure out required buffer size
	DWORD iReturn = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &bufSize);

	if (iReturn != ERROR_BUFFER_OVERFLOW) {
		return -1;
	}

	// Allocate the required buffer
	ipAA_head = (IP_ADAPTER_ADDRESSES *)malloc(bufSize);

	if (!ipAA_head) {
		HAGGLE_ERR("Could not allocate IP_ADAPTER_ADDRESSES buffer\n");
		return -1;
	}

	// Now, get the information for real
	iReturn = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, ipAA_head, &bufSize);

	switch (iReturn) {
		case ERROR_SUCCESS:
			break;
		case ERROR_NO_DATA:
		case ERROR_BUFFER_OVERFLOW:
		case ERROR_INVALID_PARAMETER:
		default:
			HAGGLE_ERR("ERROR: %s\n", STRERROR(ERRNO));
			free(ipAA_head);
			return -1;
	}
	
	// Loop through all the adapters and their addresses
	for (ipAA = ipAA_head; ipAA; ipAA = ipAA->Next) {
		
		// Ignore interfaces that are not Ethernet or 802.11
		if (ipAA->IfType != IF_TYPE_IEEE80211 &&
			ipAA->IfType != IF_TYPE_FASTETHER &&
			ipAA->IfType != IF_TYPE_GIGABITETHERNET &&
			ipAA->IfType != IF_TYPE_ETHERNET_CSMACD) {
		
			//HAGGLE_DBG("Skipping interface of wrong type = %d\n", ipAA->IfType);
			continue;
		}

		// Ignore non operational interfaces
		if (onlyUp && ipAA->OperStatus != IfOperStatusUp) {
			//HAGGLE_DBG("Skipping non operational interface status=%d\n", ipAA->OperStatus);
			continue;
		}

		if (ipAA->Mtu > 1500) {
			// Some weird MTU. Some discovered "Ethernet" interfaces have a really high MTU.
			// These are probably virtual serial line interfaces enabled when synching the phone 
			// over, e.g., a USB cable.
			continue;
		}
		// Ok, this interface seems to be interesting
		unsigned char macaddr[ETH_MAC_LEN];
		memcpy(macaddr, ipAA->PhysicalAddress, ETH_MAC_LEN); 
		EthernetAddress addr(macaddr);

		EthernetInterface *ethIface = new EthernetInterface(macaddr, 
								    ipAA->AdapterName, &addr, 
								    IFFLAG_LOCAL | ((ipAA->OperStatus == IfOperStatusUp) ? IFFLAG_UP : 0));
		
		if (!ethIface)
			continue;
		
		/*	
		HAGGLE_DBG("LOCAL Interface type=%d index=%d name=%s mtu=%d mac=%s\n", 
			ipAA->IfType, ipAA->IfIndex, ipAA->AdapterName, ipAA->Mtu, mac.getAddrStr());
		*/	
		
		IP_ADAPTER_UNICAST_ADDRESS *ipAUA;
		IP_ADAPTER_PREFIX *ipAP;

		for (ipAUA = ipAA->FirstUnicastAddress, ipAP = ipAA->FirstPrefix; ipAUA && ipAP; ipAUA = ipAUA->Next, ipAP = ipAP->Next) {
			if (ipAUA->Address.lpSockaddr->sa_family == AF_INET) {
				struct in_addr bc;
				struct sockaddr_in *saddr_v4 = (struct sockaddr_in *)ipAUA->Address.lpSockaddr;
				DWORD mask = 0xFFFFFFFF;

				memcpy(&bc, &saddr_v4->sin_addr, sizeof(bc));

				// Create mask
				mask >>= (32 - ipAP->PrefixLength);
				
				// Create broadcast address
				bc.S_un.S_addr = saddr_v4->sin_addr.S_un.S_addr | ~mask;

				IPv4Address ipv4(*saddr_v4);
				ethIface->addAddress(ipv4);

				IPv4BroadcastAddress ipv4bc(bc);
				ethIface->addAddress(ipv4bc);

				/*
				HAGGLE_DBG("IPv4 ADDRESS for interface[%s]: IP=%s PrefixLen=%d Broadcast=%s\n", 
					mac.getAddrStr(), ip_to_str(saddr_v4->sin_addr), 
					ipAP->PrefixLength, ip_to_str(bc));
				*/

#if defined(ENABLE_IPv6)
			} else if (ipAUA->Address.lpSockaddr->sa_family == AF_INET6) {
				// TODO: Handle IPv6 prefix and broadcast address
				struct sockaddr_in6 *saddr_v6 = (struct sockaddr_in6 *)ipAUA->Address.lpSockaddr;
				IPv6Address ipv6(*saddr_v6);
				ethIface->addAddress(ipv6);
#endif
			}
		}
                iflist.push_back(ethIface);
                num++;
	}

	free(ipAA_head);

        return num;
}

#else

// For those platforms with no implementation of this function

int getLocalInterfaceList(InterfaceRefList& iflist, const bool onlyUp) 
{
        return -1;
}

#endif

#if defined(OS_WINDOWS_MOBILE)
char *strdup(const char *src)
{
	char *retval;

	retval = (char *) malloc(strlen(src)+1);
	if(retval)
		strcpy(retval, src);
	return retval;
}
#endif

