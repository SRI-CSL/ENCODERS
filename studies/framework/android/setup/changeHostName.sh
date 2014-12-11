#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

# Load adb tool
. $ANDROID_TESTRUNNER/adbtool.sh

echo "Change Host Name ..."
# Execute script on Android
for device in $(cat /tmp/device_ids.txt); do
  host=$(echo "$device" | sed 's/[0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z][0-9A-Z]$//g')
  host=$(echo "SRI$host")
  echo "generated hostname = $host"
  $ADB -s $device shell "mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system/"
  sed "s/%%HOST%%/$host/g" 90hostname >> /tmp/90hostname
  $ADB -s $device push /tmp/90hostname /etc/init.d/90hostname
  $ADB -s $device shell "chmod 744 /etc/init.d/90hostname"
  rm /tmp/90hostname
  echo "changed hostname = $($ADB -s $device shell "cat /etc/init.d/90hostname | grep echo | awk '{print \$2}'")"
done
echo ""
echo "[SUCCESS]"

