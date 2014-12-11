#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

. $ANDROID_TESTRUNNER/adbtool.sh

adb_all "shell mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system/"
adb_n 1 "pull /system/build.prop /tmp/build.prop"
sed -i "s/dalvik.vm.heapsize=.*/dalvik.vm.heapsize=300m/g" /tmp/build.prop
adb_all "push /tmp/build.prop /system/build.prop"
rm /tmp/build.prop
adb_all "push sysctl.conf /etc/sysctl.conf"
