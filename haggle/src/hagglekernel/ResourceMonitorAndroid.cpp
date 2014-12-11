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

#include "ResourceMonitorAndroid.h"

#include <libcpphaggle/Watch.h>
#include <haggleutils.h>

#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <linux/netlink.h>

ResourceMonitorAndroid::ResourceMonitorAndroid(ResourceManager *m) : 
	ResourceMonitor(m, "ResourceMonitorAndroid")
{
}

ResourceMonitorAndroid::~ResourceMonitorAndroid()
{
}

static ssize_t readfile(const char *path, char *buf, size_t len)
{
        FILE *fp;
        size_t nitems;

        fp = fopen(path, "r");

        if (!fp)
                return -1;

        nitems = fread(buf, 1, len, fp);

        fclose(fp);

        if (nitems == 0)
                return -1;

        return nitems;
}

typedef enum uevent_type {
	UEVENT_AC = 0,
	UEVENT_USB,
	UEVENT_BATTERY,
	UEVENT_BATTERY_STATUS,
	UEVENT_BATTERY_HEALTH,
	UEVENT_BATTERY_CAPACITY,
	UEVENT_UNKNOWN,
} uevent_type_t;

static const char *uevent_str[] = {
	"power_supply/ac",
	"power_supply/usb",
	"power_supply/battery",
	"power_supply/battery/status",
	"power_supply/battery/health",
	"power_supply/battery/capacity",
	NULL
};

static uevent_type_t getEvent(const char *event)
{
	size_t event_len = strlen(event);

	int i = 0;

	if (strncmp("change@", event, 7) != 0)
		return UEVENT_UNKNOWN;

	while (uevent_str[i] != NULL) {
		size_t const_len = strlen(uevent_str[i]);
		
		if (const_len < event_len && 
		    strcmp(event + (event_len - const_len), uevent_str[i]) == 0) {
			return (uevent_type_t)i;
		}
		i++;
	}
        return UEVENT_UNKNOWN;
}

static const char *getSystemPath(const char *event)
{
	if (strncmp("change@", event, 7) != 0)
		return "";
	
	return (event + 7);
}

#define AC_ONLINE "/sys/class/power_supply/ac/online"
#define USB_ONLINE "/sys/class/power_supply/usb/online"

static const char *power_state_paths[] = {
	AC_ONLINE,
	USB_ONLINE,
	NULL
};

ResourceMonitor::PowerMode_t ResourceMonitorAndroid::getPowerMode() const
{
	char buffer[10];
	int i = 0;

	while (power_state_paths[i]) {
		ssize_t len = readfile(power_state_paths[i], buffer, sizeof(buffer));
		
		if (len > 0) {
			long on = strtol(buffer, NULL, 10);
			
			if (on == 1)
				return (PowerMode_t)i;
		}
		i++;
	}
	// Default to battery
	return POWER_MODE_BATTERY;
}

#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/battery/capacity"

long ResourceMonitorAndroid::getBatteryPercent() const 
{
	char buffer[10];
	long capacity = -1;
	
	ssize_t len = readfile(BATTERY_CAPACITY_PATH, buffer, sizeof(buffer));
	
	if (len > 0) {
		capacity = strtol(buffer, NULL, 10);
	}

	return (unsigned char)capacity;	
}

unsigned int ResourceMonitorAndroid::getBatteryLifeTime() const
{
	// Unknown: return 1 hour
	return 1*60*60;
}

unsigned long ResourceMonitorAndroid::getAvaliablePhysicalMemory() const 
{
	// Unknown: return 1 GB
	return 1*1024*1024*1024;
}

unsigned long ResourceMonitorAndroid::getAvaliableVirtualMemory() const 
{
	// Unknown: return 1 GB
	return 1*1024*1024*1024;
}

/* Returns -1 on failure, 0 on success */
int ResourceMonitorAndroid::uevent_init()
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

        if (bind(uevent_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                close(uevent_fd);
                return -1;
        }

        return (uevent_fd > 0) ? 0 : -1;
}

void ResourceMonitorAndroid::uevent_close()
{
        if (uevent_fd > 0)
                close(uevent_fd);
}

void ResourceMonitorAndroid::uevent_read()
{
        char buffer[1024];
        int buffer_length = sizeof(buffer);
        uevent_type_t event_type;
	long battery_capacity = 100;

        ssize_t len = recv(uevent_fd, buffer, buffer_length, 0);
        
        if (len <= 0) {
                HAGGLE_ERR("Could not read uevent\n");
                return;
        }
        
        //HAGGLE_DBG("UEvent: %s\n", buffer);

	event_type = getEvent(buffer);

	switch (event_type) {
	case UEVENT_AC:
	case UEVENT_USB:
		HAGGLE_DBG("Power state: %s\n", power_mode_str[getPowerMode()]);
		break;
	case UEVENT_BATTERY:
	case UEVENT_BATTERY_CAPACITY:
		battery_capacity = getBatteryPercent();

		HAGGLE_DBG("Battery capacity is %ld\n", battery_capacity);
		
		if (battery_capacity > 0 && 
		    battery_capacity < 10 && 
		    getPowerMode() == POWER_MODE_BATTERY) {
			HAGGLE_DBG("Shutting down due to low power\n");
			getManager()->getKernel()->shutdown();
		}
		break;
	case UEVENT_BATTERY_STATUS:
		HAGGLE_DBG("Battery status\n");
		break;
	case UEVENT_BATTERY_HEALTH:
		HAGGLE_DBG("Battery health\n");
		break;
	case UEVENT_UNKNOWN:
		break;
	}
}

bool ResourceMonitorAndroid::run()
{
	Watch w;

	HAGGLE_DBG("Running resource monitor\n");

        if (uevent_init() == -1) {
                HAGGLE_ERR("Could not open uevent socket\n");
                return false;
        }

	while (!shouldExit()) {
		int ret;

		w.reset();
                
                int ueventIndex = w.add(uevent_fd);

		ret = w.wait();
                
		if (ret == Watch::ABANDONED) {
			break;
		} else if (ret == Watch::FAILED) {
			HAGGLE_ERR("Wait on objects failed\n");
			break;
		}

                if (w.isSet(ueventIndex)) {
                        uevent_read();
                }
	}
	return false;
}

void ResourceMonitorAndroid::cleanup()
{
        uevent_close();
}
