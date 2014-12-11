#!/bin/bash

# Runs on Linux

. $ANDROID_TESTRUNNER/adbtool.sh

TEST_DIR=$1
DURATION=$2
NUM_DEVICES=$3
COOLDOWN=600

ANDROID_TEST_DIR=$4
OUTPUTFILE="$ANDROID_TEST_DIR/outputfile"

for i in $(seq 1 $NUM_DEVICES);
do
  NODE_NAME="n$i"
  USB="usb$((i-1))"
  DEVICE=$(getDevice $USB)

  runADB $USB "push $TEST_DIR/config.xml /data/data/org.haggle.kernel/files/config.xml" &> /dev/null
  # Attempt to push phone-specific configuration file
  runADB $USB "push $TEST_DIR/config.xml.$NODE_NAME /data/data/org.haggle.kernel/files/config.xml" &> /dev/null

  runADB $USB "push $TEST_DIR/start_haggle_onAndroid.sh $ANDROID_TEST_DIR" &> /dev/null
  # runADB $USB "push $TEST_DIR/start_neighbor_monitor.sh $ANDROID_TEST_DIR" &> /dev/null
  # runADB $USB "push $TEST_DIR/start_resource_monitor.sh $ANDROID_TEST_DIR" &> /dev/null
  runADB $USB "push $TEST_DIR/stop_haggle.sh $ANDROID_TEST_DIR" &> /dev/null
  runADB $USB "push $TEST_DIR/app.sh $ANDROID_TEST_DIR" &> /dev/null

  timeout $((DURATION+COOLDOWN)) $ADB -s $DEVICE shell "/system/xbin/bash $ANDROID_TEST_DIR/start_haggle_onAndroid.sh $ANDROID_TEST_DIR $NODE_NAME $DURATION $OUTPUTFILE" &
done

wait
