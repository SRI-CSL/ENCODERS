/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Jong-Seok Choi (JS)
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

// JS ---
#if defined(OS_ANDROID)
#define ARP_PATH "/system/xbin/arp"
#else
#define ARP_PATH "/usr/sbin/arp"
#endif
// --- JS

//
// This is a helper program to set arp table entries, used in the
// the haggle ethernet connectivity.
//
// It should be setuid as root:
//
// sudo su
// chown root:root arphelper
// chmod 4755 arphelper

static int argc = 0;
static char **argv = 0;

char *arg(int i)
{
    return argc > i ? argv[i] : NULL;
}

int main(int argc_, char *argv_[])
{
    argc = argc_;
    if (4 != argc) {
        fprintf(stderr, "bad args: %d\n", argc);
        // expect interface ip_address mac_address
        return 1; 
    }
    argv = argv_;
    char *iface = arg(1);
    char *ip_addr = arg(2);
    char *mac_addr = arg(3);

    if (0 != setuid(0)) {
        fprintf(stderr, "setuid failed\n");
        return 1;
    }

    char arpcmd[BUFSIZ];
    if (0 >= sprintf(arpcmd, "%s -i %s -s %s %s", ARP_PATH, iface, ip_addr, mac_addr)) {
        fprintf(stderr, "sprintf failed\n");
        return 1;
    }

    if (-1 == system(arpcmd)) {
        return 1;
    }

    return 0;
}
