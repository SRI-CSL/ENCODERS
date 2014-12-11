#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

# Runs on Android

TEST_DIR=$1
NODE_NAME=$2
DURATION=$3
OUTPUTFILE=$4

export PATH=/data:$PATH

# start haggle

am startservice -a android.intent.action.MAIN -n org.haggle.kernel/org.haggle.kernel.Haggle > /dev/null 2> /dev/null

# test if you can connect to haggle using haggletest nop

CONNECTED=0
for i in $(seq 1 6); do
    sleep ${i}
    /data/haggletest nop > /dev/null 2> /dev/null
    if [ "0" == "$?" ]; then
        CONNECTED=1
        break
    fi
done

if [ "0" == "${CONNECTED}" ]; then
    echo "Could not get connect to haggle." >> $TEST_DIR/fail.log
    exit 1
fi

logcat > $TEST_DIR/logcat &
LOGCAT_PID=$!

# run resource monitor
# /system/xbin/bash $TEST_DIR/start_resource_monitor.sh $TEST_DIR $DURATION &

# run neighbor monitor
# /system/xbin/bash $TEST_DIR/start_neighbor_monitor.sh $TEST_DIR $DURATION &

# run observer
HOST_NAME=`cat /proc/sys/kernel/hostname`
/system/xbin/bash $TEST_DIR/start_observer.sh $TEST_DIR $HOST_NAME &

# run app.sh
/system/xbin/bash $TEST_DIR/app.sh $NODE_NAME $OUTPUTFILE $TEST_DIR/fail.log

# run stop_haggle.sh
/system/xbin/bash $TEST_DIR/stop_haggle.sh $TEST_DIR $NODE_NAME $OUTPUTFILE

kill $LOGCAT_PID
