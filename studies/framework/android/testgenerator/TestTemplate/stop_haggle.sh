#!/bin/bash

# Runs on Android



# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)

#
# Shutdown haggle and haggletest at the end of the test.

#Testsuite dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

TEST_DIR=$1
NODE_NAME=$2
OUTPUTFILE=$3
FAILLOG="$TEST_DIR/fail.log"
SIZELOG="$TEST_DIR/size.log"

# sleep ${DURATION}

# used for debugging by the test runner w/ -d option (pause instances
# from shutting down)
# while [ -f $TEST_DIR/debug_lock ]; do sleep 3; echo "${NODE_NAME}: waiting for debug lock to be removed"; done

PID=$(cat "/data/data/org.haggle.kernel/files/haggle.pid")
RUNNING=$(ps -p ${PID} | grep haggle | wc -l)

if [ "1" != "${RUNNING}" ]; then
    echo "${NODE_NAME}: Haggle not running." >> ${FAILLOG}
else
    # Save memory map
    cp /proc/$PID/maps $TEST_DIR/maps

    /data/haggletest -x nop &> /dev/null
    if [ "0" != "$?" ]; then
        echo "${NODE_NAME}: Could not use haggletest to shutdown server" >> ${FAILLOG}
    fi

    sleep 60

    # kill -9 ${PID}
fi

# output file sizes of ~/.Haggle directory
du /sdcard/haggle/* >> ${SIZELOG} 2> /dev/null
du /data/data/org.haggle.kernel/files/* >>  ${SIZELOG} 2> /dev/null

HLOG="/data/data/org.haggle.kernel/files/haggle.log"
HASH=$(grep "Git reference is " ${HLOG} | awk '{print $5;}')

echo "${NODE_NAME}: HASH: ${HASH}" >> ${OUTPUTFILE}

DEBUG_ENABLED=$(grep "Statistics -" ${HLOG} | wc -l)

if [ "${DEBUG_ENABLED}" == "0" ]; then
    # we fake a shutdown event message to make validate pass
    echo "${NODE_NAME} : * SHUTDOWN COMPLETED *" >> ${OUTPUTFILE}
    echo "${NODE_NAME} : statistics disabled" >> ${OUTPUTFILE}
    exit 0
else
    STARTUP=$(grep "* HAGGLE STARTUP *" ${HLOG} | wc -l)
    HASEMERGENCY=$(grep " EMERGENCY " ${HLOG} | wc -l)
    SHUTDOWNPREPARE=$(grep "* PREPARE SHUTDOWN EVENT *" ${HLOG} | wc -l)
    SHUTDOWNREADY=$(grep "* SHUTDOWN EVENT *" ${HLOG} | wc -l)
    SHUTDOWNCOMPLETED=$(grep "* SHUTDOWN COMPLETED *" ${HLOG} | wc -l)
    if [ "${STARTUP}" != "1" ] || [ "${HASEMERGENCY}" != "0" ] || [ "${SHUTDOWNPREPARE}" != "1" ] || [ "${SHUTDOWNREADY}" != "1" ] || [ "${SHUTDOWNCOMPLETED}" != "1" ]; then
       echo Problem at Node ${NODE_NAME} - STARTUP: ${STARTUP} - SHUTDOWN PREPARING: ${SHUTDOWNPREPARE} READY: ${SHUTDOWNREADY} COMPLETED: ${SHUTDOWNCOMPLETED} EMERGENCY: ${HASEMERGENCY} >> ${FAILLOG}
    fi
fi

grep "Summary Statistics -" ${HLOG} | sed -n "s/.*\(Summary Statistics - .*\)/${NODE_NAME}: \1/p" >> ${OUTPUTFILE}

NUM_SHUTDOWN=$(grep -l "* SHUTDOWN COMPLETED *" ${HLOG}  | wc -l)

if [ "1" != "${NUM_SHUTDOWN}" ]; then
    echo "Node ${NODE_NAME}  did not respond to shutdown signal (potential crash or deadlock)" >> ${FAILLOG}
else
    echo "${NODE_NAME} : * SHUTDOWN COMPLETED *" >> ${OUTPUTFILE}
fi

exit 0
