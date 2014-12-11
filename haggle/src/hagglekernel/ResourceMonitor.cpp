#include "ResourceMonitor.h"

#if defined(OS_ANDROID)
#include "ResourceMonitorAndroid.h"
#elif defined(OS_LINUX)
#include "ResourceMonitorLinux.h"
#elif defined(OS_MACOSX)
#include "ResourceMonitorMacOSX.h"
#elif defined(OS_WINDOWS_DESKTOP)
#include "ResourceMonitorWindowsXP.h"
#elif defined(OS_WINDOWS_MOBILE)
#include "ResourceMonitorWindowsMobile.h"
#else
#error "Bad OS - Not supported by ResourceMonitor.h"
#endif

const char *ResourceMonitor::power_mode_str[] = {
	"AC",
	"USB",
	"BATTERY",
	"UNKNOWN",
};

ResourceMonitor::ResourceMonitor(ResourceManager *m, const string name) :
	ManagerModule<ResourceManager>(m, name)
{
}

ResourceMonitor::~ResourceMonitor()
{
}

ResourceMonitor *ResourceMonitor::create(ResourceManager *m)
{
#if defined(OS_ANDROID)
	return new ResourceMonitorAndroid(m);
#elif defined(OS_LINUX)
	return new ResourceMonitorLinux(m);
#elif defined(OS_MACOSX)
	return new ResourceMonitorMacOSX(m);
#elif defined(OS_WINDOWS_DESKTOP)
	return new ResourceMonitorWindowsXP(m);
#elif defined(OS_WINDOWS_MOBILE)
	return new ResourceMonitorWindowsMobile(m);
#else
	return NULL;
#endif
}
