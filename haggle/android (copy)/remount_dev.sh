#!/bin/bash
# This script makes it easy to remount the /system partition of your Android
# device in read-write or read-only mode.
#
# Usage: ./remount_dev.sh [ ro | rw ]

if [ -z $(which adb) ]; then
    echo "The adb command is not in your path."
    echo "Please make sure that the Android SDK is installed on your host system."
    exit
fi

if [ "$1" = "ro" ] || [ "$1" = "rw" ]; then
    echo "Setting device's /system partition in $1 mode"
    adb shell su -c "mount -o remount,$1 -t yaffs2 /dev/block/mtdblock3 /system"
else
    echo "You must specify either 'ro' or 'rw' as argument."
    exit
fi
