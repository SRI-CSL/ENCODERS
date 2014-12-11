#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

NUM_NODES=$1
ANDROID_TEST_DIR=$2
NETWORK_TYPE=$3

. $ANDROID_TESTRUNNER/adbtool.sh

reboot_all
sleep 7

if [ $NETWORK_TYPE == "CORE" ]; then
    $ANDROID_TESTRUNNER/setupPhoneNetwork.sh $NUM_NODES &> /dev/null
    sleep 8

    until [ $(ifconfig | grep usb | wc -l) -eq "${NUM_NODES}" ]; do
      echo "Failed to properly set up network interfaces. Retrying..."
      reboot_all
      sleep 7
      $ANDROID_TESTRUNNER/setupPhoneNetwork.sh $NUM_NODES &> /dev/null
      sleep 8
    done

    sudo bash -c "/etc/init.d/core-daemon stop &> /dev/null"

    if [ "$?" -ne "0" ]; then
        echo "ERROR: Could not stop core."
        exit 1
    fi

    corepids=$(ps aux | egrep "(core.tcl|pycore|python.*core)" | awk '{print $2;}')
    for corepid in ${corepids}; do
        sudo bash -c "kill -9 ${corepid} &> /dev/null"
    done

    sudo bash -c "/etc/init.d/core-daemon start &> /dev/null"

    if [ "$?" -ne "0" ]; then
        echo "ERROR: could not start core."
        exit 1
    fi

    sudo rm -rf /tmp/pycore* &> /dev/null
elif [ $NETWORK_TYPE == "OTA" ]; then
    $ANDROID_TESTRUNNER/setupOTANetwork.sh $NUM_NODES &> /dev/null
fi

adb_retry_n 15 $NUM_NODES "shell rm -f /data/data/org.haggle.kernel/files/haggle.db" &> /dev/null
adb_retry_n 15 $NUM_NODES "shell rm -f /data/data/org.haggle.kernel/files/haggle.log" &> /dev/null
adb_retry_n 15 $NUM_NODES "shell rm -f /data/data/org.haggle.kernel/files/haggle.pid" &> /dev/null
adb_retry_n 15 $NUM_NODES "shell rm -f /data/tombstones/*" &> /dev/null
adb_retry_n 300 $NUM_NODES "shell rm -rf /sdcard/haggle" &> /dev/null
adb_retry_n 120 $NUM_NODES "shell rm -rf $ANDROID_TEST_DIR" &> /dev/null
adb_retry_n 15 $NUM_NODES "shell mkdir -p $ANDROID_TEST_DIR" &> /dev/null
adb_retry_n 15 $NUM_NODES "shell mkdir -p /sdcard/haggle" &> /dev/null
# adb_retry_n 15 $NUM_NODES "shell mount -t tmpfs -o size=100M tmpf /sdcard/haggle"

CURRENT_DATE=$(date +"%Y%m%d.%H%M%S")
adb_retry_n 10 $NUM_NODES "shell date -s $CURRENT_DATE" &> /dev/null
