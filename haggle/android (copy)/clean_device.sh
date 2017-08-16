#!/bin/bash

adb root

DEVICES=$(adb devices | awk '{ if (match($2,"device")) print $1}')
NUM_DEVICES=$(echo $DEVICES | awk '{print split($0,a, " ")}')

for dev in $DEVICES; do
    echo "Cleaning device $dev"
    echo

    adb -s $dev shell mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system
    #adb -s $dev remount

    echo "Uninstalling PhotoShare"
    adb -s $dev uninstall org.haggle.PhotoShare

    echo "Removing binaries"
    adb -s $dev shell rm /system/bin/haggle

    echo "Removing libraries"
    adb -s $dev shell rm /system/lib/libhaggle*

    echo "Removing frameworks"
    adb -s $dev shell rm /system/framework/haggle.jar 

    echo "Cleaning up data files"
    adb -s $dev shell rm /data/haggle/*
    adb -s $dev shell rm -r /data/haggle
    
    # Reset to read-only
    adb -s \$dev shell mount -o remount,ro -t yaffs2 /dev/block/mtdblock3 /system
    
    echo
done


echo "done!"
